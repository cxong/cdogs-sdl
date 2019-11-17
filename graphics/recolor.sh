#!/bin/sh
#
# Recolor sprites based on C-Dogs palette
#
for image in *.png; do
  convert $image -colorspace rgb +depth +dither -remap palette.png ${image}
done
