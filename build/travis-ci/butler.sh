#!/bin/sh
set -e

set -o pipefail

if [[ -z "${BUTLER_API_KEY}" ]]; then
  echo "Unable to deploy! No BUTLER_API_KEY environment variable specified!"
  exit 1
fi

PROJECT="congusbongus/cdogs-sdl"

echo "Preparing butler..."
curl -L -o butler.zip https://broth.itch.ovh/butler/linux-amd64/LATEST/archive/default
unzip butler.zip
chmod +x butler
./butler -V

prepare_and_push() {
    echo "./butler push $1 $PROJECT:$2 --userversion $VERSION"
    ./butler push $1 $PROJECT:$2 --userversion $VERSION
}

prepare_and_push $TRAVIS_BUILD_DIR/C-Dogs*SDL-*-Linux.tar.gz linux
prepare_and_push $TRAVIS_BUILD_DIR/C-Dogs*SDL-*-OSX.dmg mac

echo "Done."
exit 0
