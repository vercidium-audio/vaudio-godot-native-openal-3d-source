#pragma once

// AL_LIBTYPE_STATIC collapses AL_API/ALC_API to no-op (rather than
// __declspec(dllimport)) since we have no vendored import .lib for
// soft_oal.dll - every entry point below is instead resolved manually via
// GetProcAddress in ALManager::initialize (see al_manager.cpp).
#define AL_LIBTYPE_STATIC
#define ALC_LIBTYPE_STATIC

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#include <AL/efx.h>

// Function pointer types for every OpenAL entry point this plugin currently
// uses. Named to match the real function with a "Fn" suffix, mirroring the
// alGetProcAddress/alcGetProcAddress convention OpenAL headers themselves
// suggest for runtime-resolved bindings.
using alcOpenDeviceFn = ALCdevice *(ALC_APIENTRY *)(const ALCchar *);
using alcCloseDeviceFn = ALCboolean(ALC_APIENTRY *)(ALCdevice *);
using alcCreateContextFn = ALCcontext *(ALC_APIENTRY *)(ALCdevice *, const ALCint *);
using alcMakeContextCurrentFn = ALCboolean(ALC_APIENTRY *)(ALCcontext *);
using alcDestroyContextFn = void(ALC_APIENTRY *)(ALCcontext *);
using alcGetErrorFn = ALCenum(ALC_APIENTRY *)(ALCdevice *);
using alcGetStringFn = const ALCchar *(ALC_APIENTRY *)(ALCdevice *, ALCenum);
using alcGetIntegervFn = void(ALC_APIENTRY *)(ALCdevice *, ALCenum, ALCsizei, ALCint *);
using alcIsExtensionPresentFn = ALCboolean(ALC_APIENTRY *)(ALCdevice *, const ALCchar *);

// ALC_EXT_capture (core in the ALC API since OpenAL 1.1, not looked up via
// alGetProcAddress) - resolved the same way as the other alc* entry points
// above, for ALCaptureDevice (src/openal/al_capture_device.h).
using alcCaptureOpenDeviceFn = ALCdevice *(ALC_APIENTRY *)(const ALCchar *, ALCuint, ALCenum, ALCsizei);
using alcCaptureCloseDeviceFn = ALCboolean(ALC_APIENTRY *)(ALCdevice *);
using alcCaptureStartFn = void(ALC_APIENTRY *)(ALCdevice *);
using alcCaptureStopFn = void(ALC_APIENTRY *)(ALCdevice *);
using alcCaptureSamplesFn = void(ALC_APIENTRY *)(ALCdevice *, ALCvoid *, ALCsizei);

using alGetErrorFn = ALenum(AL_APIENTRY *)(void);
using alGetStringFn = const ALchar *(AL_APIENTRY *)(ALenum);

// Convert an AL error code to its human-readable name, for logging (e.g. VA_ERROR("alBufferData failed: ", ALErrorToString(error))).
inline const char *ALErrorToString(ALenum error)
{
    switch (error)
    {
        case AL_NO_ERROR: return "no error";
        case AL_INVALID_NAME: return "invalid name (a bad handle/ID was passed to an AL call)";
        case AL_INVALID_ENUM: return "invalid enum (an unsupported enum value was passed to an AL call)";
        case AL_INVALID_VALUE: return "invalid value (an out-of-range or illegal value was passed to an AL call)";
        case AL_INVALID_OPERATION: return "invalid operation (the requested operation isn't valid in the current state)";
        case AL_OUT_OF_MEMORY: return "out of memory";
        default: return "unknown AL error";
    }
}
using alDistanceModelFn = void(AL_APIENTRY *)(ALenum);
using alSpeedOfSoundFn = void(AL_APIENTRY *)(ALfloat);
using alListenerfFn = void(AL_APIENTRY *)(ALenum, ALfloat);
using alListenerfvFn = void(AL_APIENTRY *)(ALenum, const ALfloat *);

using alGenBuffersFn = void(AL_APIENTRY *)(ALsizei, ALuint *);
using alDeleteBuffersFn = void(AL_APIENTRY *)(ALsizei, const ALuint *);
using alIsBufferFn = ALboolean(AL_APIENTRY *)(ALuint);
using alBufferDataFn = void(AL_APIENTRY *)(ALuint, ALenum, const ALvoid *, ALsizei, ALsizei);

using alGenSourcesFn = void(AL_APIENTRY *)(ALsizei, ALuint *);
using alDeleteSourcesFn = void(AL_APIENTRY *)(ALsizei, const ALuint *);
using alIsSourceFn = ALboolean(AL_APIENTRY *)(ALuint);
using alSourceiFn = void(AL_APIENTRY *)(ALuint, ALenum, ALint);
using alSourcefFn = void(AL_APIENTRY *)(ALuint, ALenum, ALfloat);
using alSourcefvFn = void(AL_APIENTRY *)(ALuint, ALenum, const ALfloat *);
using alGetSourceiFn = void(AL_APIENTRY *)(ALuint, ALenum, ALint *);
using alSourcePlayFn = void(AL_APIENTRY *)(ALuint);
using alSourceStopFn = void(AL_APIENTRY *)(ALuint);
using alSource3iFn = void(AL_APIENTRY *)(ALuint, ALenum, ALint, ALint, ALint);

// EFX (extension) entry points - resolved via alGetProcAddress rather than
// GetProcAddress against soft_oal.dll's core exports, since EFX is an AL
// extension, not a core export. alGetProcAddress itself IS a core export, so
// it's resolved the normal way in ALManager::resolve_functions.
using alGetProcAddressFn = void *(AL_APIENTRY *)(const ALchar *);

using alGenFiltersFn = LPALGENFILTERS;
using alDeleteFiltersFn = LPALDELETEFILTERS;
using alFilteriFn = LPALFILTERI;
using alFilterfFn = LPALFILTERF;

using alGenEffectsFn = LPALGENEFFECTS;
using alDeleteEffectsFn = LPALDELETEEFFECTS;
using alEffectiFn = LPALEFFECTI;
using alEffectfFn = LPALEFFECTF;
using alEffectfvFn = LPALEFFECTFV;

using alGenAuxiliaryEffectSlotsFn = LPALGENAUXILIARYEFFECTSLOTS;
using alDeleteAuxiliaryEffectSlotsFn = LPALDELETEAUXILIARYEFFECTSLOTS;
using alAuxiliaryEffectSlotiFn = LPALAUXILIARYEFFECTSLOTI;
using alAuxiliaryEffectSlotfFn = LPALAUXILIARYEFFECTSLOTF;

// AL_SOFT_callback_buffer extension - lets a buffer pull its samples from a
// callback instead of a fixed alBufferData upload, the basis for
// ALStreamBuffer's streaming playback (godot_stream_plan.md section 5.1).
using alBufferCallbackSOFTFn = LPALBUFFERCALLBACKSOFT;
