#!/bin/sh
#
# Create character spritesheet and join all the rendered images
#
if [ "$#" -ne 2 ]; then
    echo "Usage: make_char_spritesheet.sh in.blend outfile"
    exit 1
fi

# TODO: linux path
BLENDER=/Applications/blender.app/Contents/MacOS/blender
INFILE=$1
$BLENDER -b $INFILE -P char_render.py
DIMENSIONS=`identify -format "%wx%h" out/char_0_00.png`
OUTFILE=$2_${DIMENSIONS}.png
rm $OUTFILE
command='montage -geometry +0+0 -background none -tile x8 out/char*.png -channel A $OUTFILE'

eval $command

chmod 644 $OUTFILE

rm -rf out/

echo "Created $OUTFILE"
