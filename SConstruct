#!/usr/bin/env python
import os

env = SConscript("extern/godot-cpp/SConstruct")

# For reference:
# - CCFLAGS are compilation flags shared between C and C++
# - CFLAGS are for C-specific compilation flags
# - CXXFLAGS are for C++-specific compilation flags
# - CPPFLAGS are for pre-processor flags
# - CPPDEFINES are for pre-processor defines
# - LINKFLAGS are for linking flags

env.Append(CPPPATH=["src/", "thirdparty/vaudio/include/", "thirdparty/openal/include/"])
sources = Glob("src/*.cpp") + Glob("src/openal/*.cpp")

if env["target"] in ["editor", "template_debug"]:
    doc_data = env.GodotCPPDocData("src/gen/doc_data.gen.cpp", source=Glob("doc_classes/*.xml"))
    sources.append(doc_data)

# Windows-only for v1 (see native_godot_plan.md architectural decision #8).
if env["platform"] != "windows":
    raise SCons.Errors.UserError(
        "vaudio-godot-native-openal only supports the 'windows' platform for v1."
    )

env.Append(LIBPATH=["thirdparty/vaudio/lib/win64/"])
env.Append(LIBS=["vaudionative"])

library = env.SharedLibrary(
    "addons/vaudio-godot-native-openal/bin/libvaudiogodotnativeopenal{}{}".format(
        env["suffix"], env["SHLIBSUFFIX"]
    ),
    source=sources,
)

# Keep the DLL alongside the import lib so the GDExtension can find it at runtime.
dll_copy = env.Command(
    "addons/vaudio-godot-native-openal/bin/vaudionative.dll",
    "thirdparty/vaudio/lib/win64/vaudionative.dll",
    Copy("$TARGET", "$SOURCE"),
)
env.Depends(library, dll_copy)

# soft_oal.dll (OpenAL Soft) is loaded at runtime via LoadLibrary, not linked
# against an import lib - see src/openal/al_manager.cpp. It still needs to be
# alongside the built extension so that LoadLibrary(L"soft_oal.dll") resolves.
openal_dll_copy = env.Command(
    "addons/vaudio-godot-native-openal/bin/soft_oal.dll",
    "thirdparty/openal/lib/win64/soft_oal.dll",
    Copy("$TARGET", "$SOURCE"),
)
env.Depends(library, openal_dll_copy)

env.NoCache(library)
Default(library)
