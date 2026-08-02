#include "al_manager.h"

#include <godot_cpp/variant/utility_functions.hpp>

#include <string>

using namespace godot;

static ALManager *singleton = nullptr;

ALManager *ALManager::get_singleton()
{
    return singleton;
}

ALManager::~ALManager()
{
    shutdown();
}

// A bare LoadLibraryW(L"soft_oal.dll") only searches the host executable's
// directory and PATH - not this GDExtension binary's own directory - so it
// fails to find the copy SConstruct places next to
// libvaudiogodotnativeopenal.*.dll. Resolve our own module's path first
// (GetModuleHandleExW against an address inside this translation unit) and
// load soft_oal.dll from beside it instead.
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
        UtilityFunctions::push_error("[vaudio-godot-native-openal] Failed to load soft_oal.dll from ", String(full_path.c_str()));
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
        UtilityFunctions::push_error("[vaudio-godot-native-openal] soft_oal.dll is missing expected export: ", name);
        return false;
    }

    return true;
}

// EFX entry points aren't exports of soft_oal.dll at all (they're an AL
// extension) - they must be looked up via alGetProcAddress instead of
// Win32 GetProcAddress.
template <typename T>
static bool resolve_ext(alGetProcAddressFn alGetProcAddress, const char *name, T &out_fn)
{
    out_fn = reinterpret_cast<T>(alGetProcAddress(name));

    if (!out_fn)
    {
        UtilityFunctions::push_error("[vaudio-godot-native-openal] OpenAL Soft is missing expected EFX entry point: ", name);
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

// EFX (AL_EXT_EFX) function pointers aren't core soft_oal.dll exports - they
// must be resolved via alGetProcAddress, and only after a device/context
// exist (the extension is queried per-device via alcIsExtensionPresent).
// Missing EFX isn't a hard failure: has_efx() just stays false and callers
// (ALFilter) are expected to no-op rather than crash.
bool ALManager::resolve_efx_functions()
{
    if (alcIsExtensionPresent_(device, ALC_EXT_EFX_NAME) == ALC_FALSE)
    {
        UtilityFunctions::push_warning("[vaudio-godot-native-openal] ALC_EXT_EFX not present on this device - lowpass filter support disabled");
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

bool ALManager::initialize()
{
    if (is_initialized())
    {
        return true;
    }

    if (!load_library())
    {
        return false;
    }

    if (!resolve_functions())
    {
        unload_library();
        return false;
    }

    // nullptr requests the default playback device, matching
    // vaudio-godot-openal's ALManagerDevice.cs auto-selecting the first
    // entry in the output device list.
    device = alcOpenDevice_(nullptr);

    if (!device)
    {
        UtilityFunctions::push_error("[vaudio-godot-native-openal] alcOpenDevice failed - no playback device available");
        unload_library();
        return false;
    }

    context = alcCreateContext_(device, nullptr);

    if (!context)
    {
        UtilityFunctions::push_error("[vaudio-godot-native-openal] alcCreateContext failed, ALC error ", (int)alcGetError_(device));
        alcCloseDevice_(device);
        device = nullptr;
        unload_library();
        return false;
    }

    if (alcMakeContextCurrent_(context) == ALC_FALSE)
    {
        UtilityFunctions::push_error("[vaudio-godot-native-openal] alcMakeContextCurrent failed, ALC error ", (int)alcGetError_(device));
        alcDestroyContext_(context);
        context = nullptr;
        alcCloseDevice_(device);
        device = nullptr;
        unload_library();
        return false;
    }

    UtilityFunctions::print(
        "[vaudio-godot-native-openal] OpenAL device created: ", alcGetString_(device, ALC_DEVICE_SPECIFIER),
        " (", alGetString_(AL_VERSION), ")");

    // AL_INVERSE_DISTANCE_CLAMPED is already OpenAL's spec-default distance
    // model, but set it explicitly rather than relying on that - it's the
    // physically-correct real-world falloff curve (gain ~ referenceDistance /
    // (referenceDistance + rolloff * (distance - referenceDistance)), clamped
    // to each source's reference/max distance), as opposed to
    // AL_LINEAR_DISTANCE's straight-line falloff.
    alDistanceModel_(AL_INVERSE_DISTANCE_CLAMPED);

    efx_present = resolve_efx_functions();

    singleton = this;
    return true;
}

void ALManager::shutdown()
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

    unload_library();
}
