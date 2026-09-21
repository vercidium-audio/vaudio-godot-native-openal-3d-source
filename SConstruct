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

platform = env["platform"]

if platform == "windows":
    vaudio_libdir = "thirdparty/vaudio/lib/win64/"
    vaudio_link_lib = "vaudionative"
    vaudio_runtime_lib = "vaudionative.dll"
    openal_libdir = "thirdparty/openal/lib/win64/"
    openal_runtime_lib = "soft_oal.dll"
elif platform == "linux":
    vaudio_libdir = "thirdparty/vaudio/lib/linux/"
    vaudio_link_lib = "vaudionative"
    vaudio_runtime_lib = "libvaudionative.so"
    openal_libdir = "thirdparty/openal/lib/linux/"
    openal_runtime_lib = "libopenal.so.1"
elif platform == "macos":
    vaudio_libdir = "thirdparty/vaudio/lib/mac/"
    vaudio_link_lib = "vaudionative"
    vaudio_runtime_lib = "libvaudionative.dylib"
    openal_libdir = "thirdparty/openal/lib/mac/"
    openal_runtime_lib = "libopenal.1.dylib"
else:
    raise SCons.Errors.UserError(
        "vaudio-godot-native-openal-3d only supports the 'windows', 'linux' and 'macos' platforms."
    )

env.Append(LIBPATH=[vaudio_libdir])
env.Append(LIBS=[vaudio_link_lib])

# godot-cpp already adds an $ORIGIN rpath on Linux; add the @loader_path equivalent on macOS so the libvaudionative / libopenal copies SConstruct drops beside the plugin are found at load time. build-unix.sh additionally rewrites libvaudionative's recorded path to @loader_path (it's linked with no -install_name).
if platform == "macos":
    env.Append(LINKFLAGS=["-Wl,-rpath,@loader_path"])

bin_dir = "addons/vaudio-godot-native-openal-3d/bin/"

library = env.SharedLibrary(
    bin_dir + "libvaudiogodotnativeopenal3d{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
    source=sources,
)

# Keep the vaudionative shared library alongside the plugin binary so the loader finds it at runtime (win64: next to the import lib; Linux/macOS: via the $ORIGIN / @loader_path rpath added above).
vaudio_dll_copy = env.Command(
    bin_dir + vaudio_runtime_lib,
    vaudio_libdir + vaudio_runtime_lib,
    Copy("$TARGET", "$SOURCE"),
)
env.Depends(library, vaudio_dll_copy)

# OpenAL Soft is loaded at runtime via LoadLibrary / dlopen, not linked against an import lib - see src/openal/al_manager.cpp. It still needs to be alongside the built extension so the load-from-beside-our-own-module path resolves.
openal_dll_copy = env.Command(
    bin_dir + openal_runtime_lib,
    openal_libdir + openal_runtime_lib,
    Copy("$TARGET", "$SOURCE"),
)
env.Depends(library, openal_dll_copy)

# vaudio-debug-window (the out-of-process debug renderer) only ships in vaudio's dev package, so it may not be vendored here - copy it into bin/ if present, skip silently otherwise.
debug_window_name = "vaudio-debug-window.exe" if platform == "windows" else "vaudio-debug-window"
debug_window_src = vaudio_libdir + debug_window_name
if os.path.isfile(debug_window_src):
    debug_window_copy = env.Command(bin_dir + debug_window_name, debug_window_src, Copy("$TARGET", "$SOURCE"))
    env.Depends(library, debug_window_copy)

# vaudio-debug-window on Linux/macOS is dynamically linked against GLFW via @loader_path/$ORIGIN - vendor it alongside when present (dev builds only, not shipped on Windows where GLFW is statically linked).
if platform in ("linux", "macos"):
    glfw_lib_name = "libglfw.so.3" if platform == "linux" else "libglfw.3.dylib"
    glfw_lib_src = vaudio_libdir + glfw_lib_name
    if os.path.isfile(glfw_lib_src):
        glfw_lib_copy = env.Command(bin_dir + glfw_lib_name, glfw_lib_src, Copy("$TARGET", "$SOURCE"))
        env.Depends(library, glfw_lib_copy)

env.NoCache(library)
Default(library)
