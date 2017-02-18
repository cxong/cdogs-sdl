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
# Render separate actions
actions=(idle run)
action_frames=(1 80)
len_i=${#parts[*]}
for (( i=0; i<len_i; i++ ))
do
    part=${parts[$i]}
    len_j=${#actions[*]}
    for (( j=0; j<len_j; j++ ))
    do
      action=${actions[$j]}
      frames=${action_frames[$j]}
      $BLENDER -b $INFILE -P char_render.py -- $i $action $frames $part

      DIMENSIONS=`identify -format "%wx%h" out/${part}_${action}_0_00.png`
      OUTFILE=${part}_${action}_${DIMENSIONS}.png
      rm $OUTFILE
      montage -geometry +0+0 -background none -tile x8 out/${part}_${action}_*.png -channel A $OUTFILE
      chmod 644 $OUTFILE
      echo "Created $OUTFILE"
    done
done

rm -rf out/
