#!/bin/sh
#
# Create a bullet spritesheet by rotating a source image
#
if [ "$#" -ne 1 ]; then
    echo "Usage: make_bullet_spritesheet.sh infile"
    exit 1
fi
command='convert $1 -background none -virtual-pixel background'

for i in `seq 45 45 315`; do
  command="$command \\( -clone 0 -filter Point -distort SRT $i \\)"
done

BASENAME=`basename "$1" | cut -d. -f1`
DIMENSIONS=`identify -format "%wx%h" $1`
OUTFILE=${BASENAME}_${DIMENSIONS}.png

command="$command +append -channel A -threshold 60% $OUTFILE"

eval $command

chmod 644 $OUTFILE

echo "Created $OUTFILE"
