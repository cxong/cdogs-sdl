#!/bin/sh
# Launcher for the self-contained Linux bundle. The game resolves its data
# files relative to the current working directory, so chdir into the bundle
# root before starting so it can be launched from anywhere.
cd "$(dirname "$0")"
exec ./cdogs-sdl "$@"
