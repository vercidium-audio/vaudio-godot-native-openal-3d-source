#include "al_manager.h"

#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "../va_device_name.h"
#include "../va_engine_util.h"

#include <cstring>
#include <string>

using namespace godot;

static ALManager *singleton = nullptr;

ALManager *ALManager::get_singleton()
{
    return singleton;
}

// get_available_devices()/get_available_capture_devices() plus the runtime device-switching and mixer-property methods
void ALManager::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_available_devices"), &ALManager::get_available_devices);
    ClassDB::bind_method(D_METHOD("get_available_capture_devices"), &ALManager::get_available_capture_devices);
    ClassDB::bind_method(D_METHOD("set_output_device", "device_name"), &ALManager::set_output_device);

    ClassDB::bind_method(D_METHOD("set_master_volume", "value"), &ALManager::set_master_volume);

    ClassDB::bind_method(D_METHOD("set_distance_model", "value"), &ALManager::set_distance_model);
    ClassDB::bind_method(D_METHOD("get_distance_model"), &ALManager::get_distance_model);
    // Matches ALDistanceModel's enum order/values (AL/al.h) and VAWorld::distance_model's PROPERTY_HINT_ENUM.
    ADD_PROPERTY(PropertyInfo(Variant::INT, "distance_model", PROPERTY_HINT_ENUM, "None:0,InverseDistance:53249,InverseDistanceClamped:53250,LinearDistance:53251,LinearDistanceClamped:53252,ExponentDistance:53253,ExponentDistanceClamped:53254"), "set_distance_model", "get_distance_model");

    ClassDB::bind_method(D_METHOD("set_meters_per_unit", "value"), &ALManager::set_meters_per_unit);
    ClassDB::bind_method(D_METHOD("get_meters_per_unit"), &ALManager::get_meters_per_unit);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "meters_per_unit"), "set_meters_per_unit", "get_meters_per_unit");

    ClassDB::bind_method(D_METHOD("set_speed_of_sound", "value"), &ALManager::set_speed_of_sound);
    ClassDB::bind_method(D_METHOD("get_speed_of_sound"), &ALManager::get_speed_of_sound);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "speed_of_sound"), "set_speed_of_sound", "get_speed_of_sound");

    ClassDB::bind_method(D_METHOD("set_reverb_only", "value"), &ALManager::set_reverb_only);
    ClassDB::bind_method(D_METHOD("get_reverb_only"), &ALManager::get_reverb_only);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "reverb_only"), "set_reverb_only", "get_reverb_only");
}

ALManager::~ALManager()
{
    shutdown();
}

// A bare LoadLibraryW(L"soft_oal.dll") only searches PATH and the host executable's directory, not this GDExtension binary's own directory,
// so it misses the copy SConstruct places alongside it - resolve our own module's path and load soft_oal.dll from beside it instead.
static std::wstring get_own_module_directory()
{
    HMODULE self = nullptr;
    GetModuleHandleExW(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&get_own_module_directory),
        &self);

    wchar_t path[MAX_PATH];
    DWORD length = GetModuleFileNameW(self, path, MAX_PATH);

    if (length == 0 || length == MAX_PATH)
    {
        return L"";
    }

    std::wstring result(path, length);
    size_t last_slash = result.find_last_of(L"\\/");

    if (last_slash == std::wstring::npos)
    {
        return L"";
    }

    return result.substr(0, last_slash + 1);
}

bool ALManager::load_library()
{
    std::wstring directory = get_own_module_directory();
    std::wstring full_path = directory + L"soft_oal.dll";

    library = LoadLibraryW(full_path.c_str());

    if (!library)
    {
        VA_ERROR("Failed to load soft_oal.dll from ", String(full_path.c_str()));
        return false;
    }

    return true;
}

template <typename T>
static bool resolve(HMODULE library, const char *name, T &out_fn)
{
    out_fn = reinterpret_cast<T>(GetProcAddress(library, name));

    if (!out_fn)
    {
        VA_ERROR("soft_oal.dll is missing expected export: ", name);
        return false;
    }

    return true;
}

// EFX entry points aren't exports of soft_oal.dll (they're an AL extension) - must be looked up via alGetProcAddress instead of Win32 GetProcAddress.
template <typename T>
static bool resolve_ext(alGetProcAddressFn alGetProcAddress, const char *name, T &out_fn)
{
    out_fn = reinterpret_cast<T>(alGetProcAddress(name));

    if (!out_fn)
    {
        VA_ERROR("OpenAL Soft is missing expected EFX entry point: ", name);
        return false;
    }

    return true;
}

bool ALManager::resolve_functions()
{
    bool ok = true;

    ok &= resolve(library, "alcOpenDevice", alcOpenDevice_);
    ok &= resolve(library, "alcCloseDevice", alcCloseDevice_);
    ok &= resolve(library, "alcCreateContext", alcCreateContext_);
    ok &= resolve(library, "alcMakeContextCurrent", alcMakeContextCurrent_);
    ok &= resolve(library, "alcDestroyContext", alcDestroyContext_);
    ok &= resolve(library, "alcGetError", alcGetError_);
    ok &= resolve(library, "alcGetString", alcGetString_);
    ok &= resolve(library, "alcGetIntegerv", alcGetIntegerv_);
    ok &= resolve(library, "alcIsExtensionPresent", alcIsExtensionPresent_);
    ok &= resolve(library, "alcGetProcAddress", alcGetProcAddress_);

    ok &= resolve(library, "alcCaptureOpenDevice", alcCaptureOpenDevice_);
    ok &= resolve(library, "alcCaptureCloseDevice", alcCaptureCloseDevice_);
    ok &= resolve(library, "alcCaptureStart", alcCaptureStart_);
    ok &= resolve(library, "alcCaptureStop", alcCaptureStop_);
    ok &= resolve(library, "alcCaptureSamples", alcCaptureSamples_);

    ok &= resolve(library, "alGetError", alGetError_);
    ok &= resolve(library, "alGetString", alGetString_);
    ok &= resolve(library, "alDistanceModel", alDistanceModel_);
    ok &= resolve(library, "alSpeedOfSound", alSpeedOfSound_);
    ok &= resolve(library, "alListenerf", alListenerf_);
    ok &= resolve(library, "alListenerfv", alListenerfv_);

    ok &= resolve(library, "alGenBuffers", alGenBuffers_);
    ok &= resolve(library, "alDeleteBuffers", alDeleteBuffers_);
    ok &= resolve(library, "alIsBuffer", alIsBuffer_);
    ok &= resolve(library, "alBufferData", alBufferData_);

    ok &= resolve(library, "alGenSources", alGenSources_);
    ok &= resolve(library, "alDeleteSources", alDeleteSources_);
    ok &= resolve(library, "alIsSource", alIsSource_);
    ok &= resolve(library, "alSourcei", alSourcei_);
    ok &= resolve(library, "alSourcef", alSourcef_);
    ok &= resolve(library, "alSourcefv", alSourcefv_);
    ok &= resolve(library, "alGetSourcei", alGetSourcei_);
    ok &= resolve(library, "alSourcePlay", alSourcePlay_);
    ok &= resolve(library, "alSourceStop", alSourceStop_);
    ok &= resolve(library, "alSource3i", alSource3i_);

    ok &= resolve(library, "alGetProcAddress", alGetProcAddress_);

    return ok;
}

// EFX function pointers aren't core soft_oal.dll exports - resolved via alGetProcAddress only after a device/context exist. Missing EFX isn't a hard
// failure: has_efx() just stays false and callers (ALFilter) are expected to no-op.
bool ALManager::resolve_efx_functions()
{
    if (alcIsExtensionPresent_(device, ALC_EXT_EFX_NAME) == ALC_FALSE)
    {
        VA_WARN("ALC_EXT_EFX not present on this device - lowpass filter support disabled");
        return false;
    }

    bool ok = true;

    ok &= resolve_ext(alGetProcAddress_, "alGenFilters", alGenFilters_);
    ok &= resolve_ext(alGetProcAddress_, "alDeleteFilters", alDeleteFilters_);
    ok &= resolve_ext(alGetProcAddress_, "alFilteri", alFilteri_);
    ok &= resolve_ext(alGetProcAddress_, "alFilterf", alFilterf_);

    ok &= resolve_ext(alGetProcAddress_, "alGenEffects", alGenEffects_);
    ok &= resolve_ext(alGetProcAddress_, "alDeleteEffects", alDeleteEffects_);
    ok &= resolve_ext(alGetProcAddress_, "alEffecti", alEffecti_);
    ok &= resolve_ext(alGetProcAddress_, "alEffectf", alEffectf_);
    ok &= resolve_ext(alGetProcAddress_, "alEffectfv", alEffectfv_);

    ok &= resolve_ext(alGetProcAddress_, "alGenAuxiliaryEffectSlots", alGenAuxiliaryEffectSlots_);
    ok &= resolve_ext(alGetProcAddress_, "alDeleteAuxiliaryEffectSlots", alDeleteAuxiliaryEffectSlots_);
    ok &= resolve_ext(alGetProcAddress_, "alAuxiliaryEffectSloti", alAuxiliaryEffectSloti_);
    ok &= resolve_ext(alGetProcAddress_, "alAuxiliaryEffectSlotf", alAuxiliaryEffectSlotf_);

    // AL_SOFT_callback_buffer is a separate AL extension, resolved here too since both need a device/context. Not folded into `ok`: a driver
    // without this should still get EFX, it just won't get streaming buffers (ALStreamBuffer checks al_buffer_callback_soft() for null).
    if (!resolve_ext(alGetProcAddress_, "alBufferCallbackSOFT", alBufferCallbackSOFT_))
    {
        VA_WARN("AL_SOFT_callback_buffer not present on this device - streaming source support disabled");
    }

    return ok;
}

void ALManager::unload_library()
{
    if (library)
    {
        FreeLibrary(library);
        library = nullptr;
    }
}

bool ALManager::open_device_and_context()
{
    const ALCchar *requested_device = device_name.empty() ? nullptr : device_name.c_str();
    device = alcOpenDevice_(requested_device);

    if (!device)
    {
        VA_ERROR("alcOpenDevice failed for device '", String::utf8(device_name.c_str()), "'");
        return false;
    }

    ALCint attributes[11];
    int count = 0;

    if (max_auxiliary_sends > 0)
    {
        attributes[count++] = ALC_MAX_AUXILIARY_SENDS;
        attributes[count++] = max_auxiliary_sends;
    }

    if (max_mono_sources > 0)
    {
        attributes[count++] = ALC_MONO_SOURCES;
        attributes[count++] = max_mono_sources;
    }

    if (max_stereo_sources > 0)
    {
        attributes[count++] = ALC_STEREO_SOURCES;
        attributes[count++] = max_stereo_sources;
    }

    if (sample_rate > 0)
    {
        attributes[count++] = ALC_FREQUENCY;
        attributes[count++] = sample_rate;
    }

    attributes[count++] = ALC_HRTF_SOFT;
    attributes[count++] = hrtf_enabled ? ALC_TRUE : ALC_FALSE;

    attributes[count++] = 0;

    context = alcCreateContext_(device, attributes);

    if (!context)
    {
        VA_ERROR("alcCreateContext failed, ALC error ", (int)alcGetError_(device));
        alcCloseDevice_(device);
        device = nullptr;
        return false;
    }

    if (alcMakeContextCurrent_(context) == ALC_FALSE)
    {
        VA_ERROR("alcMakeContextCurrent failed, ALC error ", (int)alcGetError_(device));
        alcDestroyContext_(context);
        context = nullptr;
        alcCloseDevice_(device);
        device = nullptr;
        return false;
    }

    // Re-applied here since distance model is per-context state, lost whenever open_device_and_context() recreates the context.
    alDistanceModel_(distance_model);

    // meters_per_unit/speed_of_sound are also per-context listener state, lost across a context recreation - re-apply the last value set.
    alListenerf_(AL_METERS_PER_UNIT, meters_per_unit);
    alSpeedOfSound_(speed_of_sound);

    efx_present = resolve_efx_functions();

    // Per-device state like EFX - re-queried every (re)open since a fallback device reached after switching outputs might not support it.
    if (alcIsExtensionPresent_(device, "ALC_SOFT_reopen_device") == ALC_FALSE)
    {
        alcReopenDeviceSOFT_ = nullptr;
    }
    else
    {
        alcReopenDeviceSOFT_ = reinterpret_cast<alcReopenDeviceSOFTFn>(alcGetProcAddress_(device, "alcReopenDeviceSOFT"));
    }

    return true;
}

// Reads device_name/max_auxiliary_sends/max_mono_sources/max_stereo_sources/sample_rate/hrtf_enabled from the audio/vaudio/* Project Settings
// registered by register_project_settings() (register_types.cpp), which must already have run so every setting exists with its registered default.
void ALManager::read_settings_from_project_settings()
{
    ProjectSettings *settings = ProjectSettings::get_singleton();

    // PROPERTY_HINT_ENUM on a STRING property stores the literal entry text picked (see refresh_output_device_hint(), register_types.cpp), so the
    // UI writes DEFAULT_DEVICE_LABEL when a user picks "System Default" - translate that back to "" (alcOpenDevice's own "use the driver default").
    String device_name_setting = settings->get_setting("audio/vaudio/output_device");

    if (device_name_setting == String(DEFAULT_DEVICE_LABEL))
        device_name_setting = String();

    CharString device_name_utf8 = device_name_setting.utf8();
    device_name = device_name_utf8.get_data();

    max_auxiliary_sends = MAX(1, (int)settings->get_setting("audio/vaudio/max_reverb_sends"));
    max_mono_sources = settings->get_setting("audio/vaudio/max_mono_sources");
    max_stereo_sources = settings->get_setting("audio/vaudio/max_stereo_sources");
    sample_rate = settings->get_setting("audio/vaudio/sample_rate");
    hrtf_enabled = settings->get_setting("audio/vaudio/hrtf_enabled");
}

bool ALManager::initialize()
{
    if (is_initialized())
    {
        return true;
    }

    read_settings_from_project_settings();

    if (!load_library())
    {
        return false;
    }

    if (!resolve_functions())
    {
        unload_library();
        return false;
    }

    if (!open_device_and_context())
    {
        unload_library();
        return false;
    }

    VA_LOG("ALManager initialized - device '", device_name.empty() ? "(default)" : device_name.c_str(),
        "', max_reverb_sends ", max_auxiliary_sends, ", max_mono_sources ", max_mono_sources,
        ", max_stereo_sources ", max_stereo_sources, ", sample_rate ", sample_rate, ", hrtf_enabled ", hrtf_enabled);

    singleton = this;
    return true;
}

void ALManager::close_device()
{
    if (singleton == this)
    {
        singleton = nullptr;
    }

    if (context)
    {
        alcMakeContextCurrent_(nullptr);
        alcDestroyContext_(context);
        context = nullptr;
    }

    if (device)
    {
        alcCloseDevice_(device);
        device = nullptr;
    }
}

bool ALManager::reinitialize(const String &new_device_name, int new_max_auxiliary_sends, int new_sample_rate, bool new_hrtf_enabled)
{
    CharString new_device_name_utf8 = new_device_name.utf8();
    const char *new_device_name_cstr = new_device_name_utf8.get_data();

    if (device_name == new_device_name_cstr && max_auxiliary_sends == new_max_auxiliary_sends
        && sample_rate == new_sample_rate && hrtf_enabled == new_hrtf_enabled && is_initialized())
    {
        return true;
    }

    device_name = new_device_name_cstr;
    max_auxiliary_sends = new_max_auxiliary_sends;
    sample_rate = new_sample_rate;
    hrtf_enabled = new_hrtf_enabled;

    if (!library)
    {
        return initialize();
    }

    // Prefer alcReopenDeviceSOFT when supported: it redirects the existing ALC device/context to the new output device in place, keeping every
    // existing AL object valid - unlike the close_device()+open_device_and_context() fallback below, a full teardown/recreate that invalidates them.
    if (alcReopenDeviceSOFT_ && is_initialized())
    {
        const ALCchar *requested_device = device_name.empty() ? nullptr : device_name.c_str();

        ALCint attributes[11];
        int count = 0;

        if (max_auxiliary_sends > 0)
        {
            attributes[count++] = ALC_MAX_AUXILIARY_SENDS;
            attributes[count++] = max_auxiliary_sends;
        }

        if (max_mono_sources > 0)
        {
            attributes[count++] = ALC_MONO_SOURCES;
            attributes[count++] = max_mono_sources;
        }

        if (max_stereo_sources > 0)
        {
            attributes[count++] = ALC_STEREO_SOURCES;
            attributes[count++] = max_stereo_sources;
        }

        if (sample_rate > 0)
        {
            attributes[count++] = ALC_FREQUENCY;
            attributes[count++] = sample_rate;
        }

        attributes[count++] = ALC_HRTF_SOFT;
        attributes[count++] = hrtf_enabled ? ALC_TRUE : ALC_FALSE;

        attributes[count++] = 0;

        if (alcReopenDeviceSOFT_(device, requested_device, attributes) == ALC_TRUE)
        {
            singleton = this;
            return true;
        }

        VA_WARN("alcReopenDeviceSOFT failed for device '", String::utf8(device_name.c_str()),
            "', ALC error ", (int)alcGetError_(device), " - falling back to full device recreation");
    }

    close_device();

    bool ok = open_device_and_context();

    if (ok)
    {
        singleton = this;
    }

    return ok;
}

PackedStringArray ALManager::get_available_devices()
{
    PackedStringArray devices;

    if (!alcGetString_)
    {
        return devices;
    }

    ALCenum specifier = alcIsExtensionPresent_ && alcIsExtensionPresent_(nullptr, "ALC_ENUMERATE_ALL_EXT")
        ? ALC_ALL_DEVICES_SPECIFIER
        : ALC_DEVICE_SPECIFIER;

    // Standard ALC enumeration convention: a single buffer of null-terminated device names, itself terminated by an extra empty string.
    const ALCchar *list = alcGetString_(nullptr, specifier);

    if (!list)
    {
        return devices;
    }

    while (*list)
    {
        devices.push_back(String::utf8(list));
        list += strlen(list) + 1;
    }

    return devices;
}

PackedStringArray ALManager::get_available_capture_devices()
{
    PackedStringArray devices;

    if (!alcGetString_)
    {
        return devices;
    }

    // Same convention as get_available_devices() above, just for ALC_CAPTURE_DEVICE_SPECIFIER.
    const ALCchar *list = alcGetString_(nullptr, ALC_CAPTURE_DEVICE_SPECIFIER);

    if (!list)
    {
        return devices;
    }

    while (*list)
    {
        devices.push_back(String::utf8(list));
        list += strlen(list) + 1;
    }

    return devices;
}

void ALManager::shutdown()
{
    close_device();
    unload_library();
}
