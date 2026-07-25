#!/bin/sh
# Launcher for the campaign editor in the self-contained Linux bundle. See
# cdogs-sdl.sh for why this chdirs into the bundle root first.
cd "$(dirname "$0")"
exec ./cdogs-sdl-editor "$@"
