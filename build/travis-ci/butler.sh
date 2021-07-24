#!/bin/sh
set -e

set -o pipefail

if [[ -z "${BUTLER_API_KEY}" ]]; then
  echo "Unable to deploy! No BUTLER_API_KEY environment variable specified!"
  exit 1
fi

PROJECT="congusbongus/cdogs-sdl"
if [[ "$OSTYPE" == "darwin"* ]]; then
  BUTLER_URL=https://broth.itch.ovh/butler/darwin-amd64/LATEST/archive/default
  BUTLER_CHANNEL=mac
  FILE_SUFFIX=OSX.dmg
else
  BUTLER_URL=https://broth.itch.ovh/butler/linux-amd64/LATEST/archive/default
  BUTLER_CHANNEL=linux
  FILE_SUFFIX=Linux.tar.gz
fi

echo "Preparing butler..."
if ! command -v butler &> /dev/null
then
  curl -L -o butler.zip "$BUTLER_URL"
  unzip butler.zip
  chmod +x butler
fi
butler -V

prepare_and_push() {
  echo "./butler push \"$1\" $PROJECT:$2 --userversion $VERSION"
  butler push "$1" $PROJECT:$2 --userversion $VERSION
}

prepare_and_push $TRAVIS_BUILD_DIR/C-Dogs*SDL-*-"$FILE_SUFFIX" "$BUTLER_CHANNEL"

echo "Done."
exit 0
