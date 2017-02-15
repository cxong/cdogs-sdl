#!/bin/bash
#
# Create character spritesheet and join all the rendered images
#
if [ "$#" -ne 1 ]; then
    echo "Usage: make_char_spritesheet.sh in.blend"
    exit 1
fi

# TODO: linux path
BLENDER=/Applications/blender.app/Contents/MacOS/blender
INFILE=$1
# Render separate body parts (in layers)
parts=(legs upper)
len=${#parts[*]}
for (( i=0; i<len; i++ ))
do
    part=${parts[$i]}
    $BLENDER -b $INFILE -P char_render.py -- $i $part

    DIMENSIONS=`identify -format "%wx%h" out/${part}_0_00.png`
    OUTFILE=${part}_${DIMENSIONS}.png
    rm $OUTFILE
    montage -geometry +0+0 -background none -tile x8 out/$part*.png -channel A $OUTFILE
    chmod 644 $OUTFILE
    echo "Created $OUTFILE"
done

rm -rf out/
