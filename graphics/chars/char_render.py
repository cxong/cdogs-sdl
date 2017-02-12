"""
Render animation in 8 rotations
adapted from http://clintbellanger.net/articles/isometric_tiles/
"""
import bpy
import sys
from math import radians

RESOLUTION = 32
FRAME_SKIP = 10

angle = -45
axis = 2 # z-axis
platform = bpy.data.objects["armature"]
scene = bpy.data.scenes[0]
render = scene.render
render.image_settings.file_format = 'PNG'
render.image_settings.color_mode ='RGBA'
render.alpha_mode = 'TRANSPARENT'
render.resolution_x = RESOLUTION
render.resolution_y = RESOLUTION
render.frame_map_new = 100 / FRAME_SKIP
scene.frame_end = 79 / FRAME_SKIP
original_path = 'out/char##'
path = 'out/char_{}_##'
for i in range(0, 8):
    # rotate
    temp_rot = platform.rotation_euler
    temp_rot[axis] += radians(angle)
    platform.rotation_euler = temp_rot

    # set filename so that up is first
    render.filepath = path.format((i + 6) % 8)

    # render animation
    bpy.ops.render.render(animation=True)
