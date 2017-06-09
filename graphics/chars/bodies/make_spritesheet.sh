#!/bin/bash
#
# Create character spritesheet and join all the rendered images
#
if [ "$#" -ne 1 ]; then
    echo "Usage: make_spritesheet.sh body"
    exit 1
fi

# TODO: linux path
BLENDER=/Applications/blender.app/Contents/MacOS/blender
INFILE=$1/src.blend
# Render separate body parts (in layers)
parts=(legs upper)
# Render separate actions
actions_legs=(idle run)
actions_upper=(idle idle_handgun run run_handgun)
idle_frames=1
run_frames=80
len_i=${#parts[*]}
for (( i=0; i<len_i; i++ ))
do
    part=${parts[$i]}
    if [[ $part == *"legs"* ]]
    then
      actions=${actions_legs[@]}
    else
      actions=${actions_upper[@]}
    fi
    for action in $actions
    do
      if [[ $action == *"run"* ]]
      then
        frames=$run_frames
      else
        frames=$idle_frames
      fi
      # Include right hand layer if the part is "upper" and action doesn't have
      # "handgun"
      layers=$i
      if [[ $part == *"upper"* ]] && [[ $action != *"handgun"* ]]
      then
        layers=$i,2
      fi
      $BLENDER -b $INFILE -P render.py -- $layers $action $frames $part

      DIMENSIONS=`identify -format "%wx%h" out/${part}_${action}_0_00.png`
      OUTFILE=$1/${part}_${action}_${DIMENSIONS}.png
      rm $OUTFILE
      montage -geometry +0+0 -background none -tile x8 out/${part}_${action}_*.png -channel A tmpimage
      convert tmpimage -dither None -colors 32 -level 25%,60% $OUTFILE
      rm tmpimage
      chmod 644 $OUTFILE
      echo "Created $OUTFILE"
    done
done

rm -rf out/
