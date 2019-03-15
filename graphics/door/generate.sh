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
# normal_v = v + top + bottom
convert $1/base/v.png \
  $1/base/top.png -geometry +0+0 -composite \
  $1/base/bottom.png -geometry +0+0 -composite \
  $1/normal_v.png

# Generate keys
for key in blue green red yellow; do
  # key_h = normal_h + key_h + key_left + key_right
  convert $1/normal_h.png \
    $1/base/key/${key}_h.png -geometry +0+0 -composite \
    $1/base/key/${key}_left.png -geometry +0+0 -composite \
    $1/base/key/${key}_right.png -geometry +0+0 -composite \
    $1/${key}_h.png
  # key_v = normal_v + key_v + key_top + key_bottom
  convert $1/normal_v.png \
    $1/base/key/${key}_v.png -geometry +0+0 -composite \
    $1/base/key/${key}_top.png -geometry +0+0 -composite \
    $1/base/key/${key}_bottom.png -geometry +0+0 -composite \
    $1/${key}_v.png
done
