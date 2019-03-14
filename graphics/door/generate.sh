#!/bin/bash
#
# Generate game-ready door sprites
# TODO: generate spritesheet?
#
if [ "$#" -ne 1 ]; then
    echo "Usage: generate.sh <doorname>"
    exit 1
fi

# Copy simple sprites
# TODO: generate open
for s in open_h open_v wall; do
  cp $1/base/${s}.png $1/${s}.png
done

# Generate door bases
# normal_h = open_h + h + left + right
convert $1/base/open_h.png -background none -gravity South -extent 16x21 \
  $1/base/h.png -geometry +0+0 -composite \
  $1/base/left.png -geometry +0+0 -composite \
  $1/base/right.png -geometry +0+0 -composite \
  $1/normal_h.png
#convert $1/base/open_h.png -background none -gravity South -extent 16x21 tmp.png
#convert -composite tmp.png $1/base/h.png $1/base/left.png $1/base/right.png $1/normal_h.png
#rm tmp.png
# normal_v = v + top + bottom
convert -composite $1/base/v.png $1/base/top.png $1/base/bottom.png $1/normal_v.png

# Generate keys
for key in blue green red yellow; do
  # normal_h + key
  convert -composite $1/normal_h.png $1/base/${key}_h.png $1/${key}_h.png
  # normal_v + key
  convert -composite $1/normal_v.png $1/base/${key}_v.png $1/${key}_v.png
done
