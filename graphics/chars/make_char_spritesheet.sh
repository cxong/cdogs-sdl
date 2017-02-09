#!/bin/sh
#
# Create character spritesheet by joining all the rendered images
#
if [ "$#" -ne 1 ]; then
    echo "Usage: make_char_spritesheet.sh infileprefix"
    exit 1
fi

DIMENSIONS=`identify -format "%wx%h" $100.png`
OUTFILE=$1_${DIMENSIONS}.png
rm $OUTFILE
command='montage -geometry +0+0 -background none -tile 8x $1*.png -channel A $OUTFILE'

eval $command

chmod 644 $OUTFILE

echo "Created $OUTFILE"
