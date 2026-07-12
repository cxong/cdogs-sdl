#!/bin/bash
set -euo pipefail

cd $(dirname $0)"/../.."

# The version is assembled from three parts in CMakeLists.txt
# (set(VERSION_MAJOR "2"), etc.), so read each one and join them.
VERSION_MAJOR=$(grep -oP '^set\(VERSION_MAJOR "\K[^"]+' CMakeLists.txt)
VERSION_MINOR=$(grep -oP '^set\(VERSION_MINOR "\K[^"]+' CMakeLists.txt)
VERSION_PATCH=$(grep -oP '^set\(VERSION_PATCH "\K[^"]+' CMakeLists.txt)
VERSION="${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}"
PACKAGE_NAME="cdogs-sdl-${VERSION}-linux-bundle"
PACKAGE_DIR="distrib/"$PACKAGE_NAME

rm -Rf $PACKAGE_DIR
mkdir -p $PACKAGE_DIR

# Game binaries
cp build-output/src/cdogs-sdl $PACKAGE_DIR
cp build-output/src/cdogs-sdl-editor $PACKAGE_DIR

# Game data. These are all resolved relative to the working directory at
# runtime, so they sit next to the binary and the launcher scripts chdir here.
cp -av data $PACKAGE_DIR/data
cp -av missions $PACKAGE_DIR/missions
cp -av dogfights $PACKAGE_DIR/dogfights
cp -av graphics $PACKAGE_DIR/graphics
cp -av music $PACKAGE_DIR/music
cp -av sounds $PACKAGE_DIR/sounds
cp -av doc $PACKAGE_DIR/doc
cp -av README.md $PACKAGE_DIR/README.md

# Launcher scripts. The game locates its data relative to the current working
# directory, so chdir into the bundle root before exec'ing the binary. This
# lets the bundle be launched from anywhere (file manager, PATH, ...).
cp -av .github/workflows/cdogs-sdl.sh $PACKAGE_DIR/cdogs-sdl.sh
cp -av .github/workflows/cdogs-sdl-editor.sh $PACKAGE_DIR/cdogs-sdl-editor.sh

# Copy all .so files these binaries were built for, to make a self contained
# bundle.
#
# We must NOT bundle any part of glibc. glibc's sub-libraries (libpthread,
# libm, libdl, librt, libresolv, ...) share internal GLIBC_PRIVATE symbols
# with libc.so.6 and are only ABI-compatible with the exact libc.so.6 they
# shipped with. The final binary always loads the *host's* libc.so.6 and
# dynamic loader (their path is baked into the ELF interpreter / NEEDED and
# can't be overridden by rpath), so bundling e.g. Debian's libpthread.so.0
# next to a host libc.so.6 breaks with
#   undefined symbol: __libc_pthread_init, version GLIBC_PRIVATE
# Because we build against an older glibc than the deploy targets, letting the
# whole glibc family resolve from the host is both correct and forward-safe.
#
# The exclusion list below is the glibc-provided set only; genuinely external
# libraries with confusingly similar names (libSDL, libmikmod, ...) are still
# bundled.
GLIBC_EXCLUDE='/(ld-linux[^/]*|libc|libpthread|libm|libmvec|libdl|librt|libresolv|libutil|libanl|libBrokenLocale|libnss_[^/]*|libcrypt)\.so'
mkdir -p $PACKAGE_DIR/lib
ldd $PACKAGE_DIR/cdogs-sdl $PACKAGE_DIR/cdogs-sdl-editor \
	| grep "=> /" | awk '{print $3}' | sort -u \
	| grep -vE "$GLIBC_EXCLUDE" | xargs -I '{}' cp -v '{}' $PACKAGE_DIR/lib/

cd $PACKAGE_DIR

# Patch binaries to use local lib/ folder
patchelf --set-rpath '$ORIGIN/lib' --force-rpath cdogs-sdl
patchelf --set-rpath '$ORIGIN/lib' --force-rpath cdogs-sdl-editor

# Patch libraries to use local lib/ folder for transitive dependencies
patchelf --set-rpath '$ORIGIN' --force-rpath ./lib/*.so*
