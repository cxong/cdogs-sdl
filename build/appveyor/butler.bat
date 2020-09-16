@echo off
setlocal enabledelayedexpansion

if "!BUTLER_API_KEY!" == "" (
  echo Unable to deploy - No BUTLER_API_KEY environment variable specified
  exit /b 1
)

set PROJECT="congusbongus/cdogs-sdl"

echo "Preparing butler..."
curl -L -o butler.zip https://broth.itch.ovh/butler/windows-amd64/LATEST/archive/default
7z x -y butler.zip
butler -V

for %%f in (C-Dogs*) do (
  butler push --userversion !VERSION! %%f !PROJECT!:win
)
