@echo off
setlocal enabledelayedexpansion

if "!BUTLER_API_KEY!" == "" (
  echo Unable to deploy - No BUTLER_API_KEY environment variable specified
  exit /b 1
)

set PROJECT="cxong/cdogs-sdl"

echo "Preparing butler..."
curl -L -o butler.zip https://broth.itch.ovh/butler/windows-amd64/LATEST/archive/default
7z x -y butler.zip
butler -V

butler .\C-Dogs*.exe !PROJECT!:win
butler .\C-Dogs*.zip !PROJECT!:win
