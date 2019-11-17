#!/bin/sh
set -e

set -o pipefail

if [[ -z "${BUTLER_API_KEY}" ]]; then
  echo "Unable to deploy! No BUTLER_API_KEY environment variable specified!"
  exit 1
fi

PROJECT="cxong/cdogs-sdl"

echo "Preparing butler..."
curl -L -o butler.zip https://broth.itch.ovh/butler/linux-amd64/LATEST/archive/default
unzip butler.zip
chmod +x butler
./butler -V

prepare_and_push() {
    echo "Push $3 build to itch.io..."
    ./butler push $2 $1:$3
}

prepare_and_push $PROJECT $TRAVIS_BUILD_DIR/C-Dogs*SDL-*-Linux.tar.gz linux
prepare_and_push $PROJECT $TRAVIS_BUILD_DIR/C-Dogs*SDL-*-OSX.dmg mac

echo "Done."
exit 0
