#!/usr/bin/env bash
# Linux/macOS counterpart of build.bat. Linux builds an x86_64 .so, macOS builds an arm64 .dylib.
set -euo pipefail
cd "$(dirname "$0")"

if [ -f .env ]; then
    set -a
    . ./.env
    set +a
fi

# Bootstrap extern/godot-cpp on a fresh clone; a no-op once initialised.
git submodule update --init --recursive

case "$(uname -s)" in
    Linux) PLATFORM=linux; ARCH=x86_64; LIBEXT=".so" ;;
    Darwin)
        PLATFORM=macos; ARCH=arm64; LIBEXT=".dylib"
        # Non-interactive SSH gets a bare PATH without Homebrew, where scons lives.
        for brew_bin in /opt/homebrew/bin /usr/local/bin; do
            [ -x "$brew_bin/scons" ] && case ":$PATH:" in *":$brew_bin:"*) ;; *) PATH="$brew_bin:$PATH" ;; esac
        done
        ;;
    *) echo "Unsupported host: $(uname -s)" >&2; exit 1 ;;
esac

scons platform="$PLATFORM" arch="$ARCH" target=template_debug
scons platform="$PLATFORM" arch="$ARCH" target=template_release

BIN_DIR="addons/vaudio-godot-native-openal-3d/bin"
DEBUG_LIB="libvaudiogodotnativeopenal3d.${PLATFORM}.template_debug.${ARCH}${LIBEXT}"
RELEASE_LIB="libvaudiogodotnativeopenal3d.${PLATFORM}.template_release.${ARCH}${LIBEXT}"

# vaudionative and OpenAL Soft shared libs to carry alongside the plugin, per platform (see SConstruct).
if [ "$PLATFORM" = "linux" ]; then
    RUNTIME_LIBS=(libvaudionative.so libopenal.so.1)
else
    RUNTIME_LIBS=(libvaudionative.dylib libopenal.1.dylib)

    # libvaudionative.dylib is linked with no -install_name, so it records its absolute build-machine path -
    # rewrite the plugin's reference to it (and its own id) to @loader_path so the copy shipped in bin/ resolves.
    for plugin in "$BIN_DIR/$DEBUG_LIB" "$BIN_DIR/$RELEASE_LIB"; do
        dep=$(otool -L "$plugin" | awk '/libvaudionative\.dylib/ {print $1; exit}')
        [ -n "$dep" ] && install_name_tool -change "$dep" "@loader_path/libvaudionative.dylib" "$plugin"
    done
    install_name_tool -id "@loader_path/libvaudionative.dylib" "$BIN_DIR/libvaudionative.dylib" 2>/dev/null || true
fi

copy_to() {
    local dest="$1/bin"
    [ -d "$dest" ] || { echo "$dest missing - skipping"; return; }
    echo "Copying build output to $dest"
    cp -f "$BIN_DIR/$DEBUG_LIB" "$BIN_DIR/$RELEASE_LIB" "$dest/"
    for lib in "${RUNTIME_LIBS[@]}"; do
        [ -f "$BIN_DIR/$lib" ] && cp -f "$BIN_DIR/$lib" "$dest/"
    done
}

if [ -n "${VAUDIO_RELEASE_DIR:-}" ]; then copy_to "$VAUDIO_RELEASE_DIR"; else echo "VAUDIO_RELEASE_DIR not set - skipping copy to release repo"; fi
if [ -n "${VAUDIO_DEMO_ADDON_DIR:-}" ]; then copy_to "$VAUDIO_DEMO_ADDON_DIR"; else echo "VAUDIO_DEMO_ADDON_DIR not set - skipping copy to demo project addon"; fi
