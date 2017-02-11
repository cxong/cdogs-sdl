"""
Render animation in 8 rotations
adapted from http://clintbellanger.net/articles/isometric_tiles/
"""
import bpy
import sys
from math import radians

angle = -45
axis = 2 # z-axis
platform = bpy.data.objects["armature"]
bpy.data.scenes[0].render.image_settings.file_format = 'PNG'
bpy.data.scenes[0].render.image_settings.color_mode ='RGBA'
bpy.data.scenes[0].render.alpha_mode = 'TRANSPARENT'
original_path = 'out/char##'
path = 'out/char_{}_##'
for i in range(0, 8):
    # rotate
    temp_rot = platform.rotation_euler
    temp_rot[axis] += radians(angle)
    platform.rotation_euler = temp_rot

    # set filename so that up is first
    bpy.data.scenes[0].render.filepath = path.format((i + 6) % 8)

    # render animation
    bpy.ops.render.render(animation=True)
