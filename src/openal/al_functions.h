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

using alGetErrorFn = ALenum(AL_APIENTRY *)(void);
using alGetStringFn = const ALchar *(AL_APIENTRY *)(ALenum);
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
