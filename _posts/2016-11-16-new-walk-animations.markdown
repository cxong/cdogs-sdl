---
layout: post
title:  "New Walk Animations"
date:   2016-11-16
categories: other
---
For the next version of C-Dogs SDL, we're looking at [improving the walk animations](https://github.com/cxong/cdogs-sdl/issues/18). This would be a good thing to do before other character enhancements, like [character "textures"](https://github.com/cxong/cdogs-sdl/issues/13) - think clothes, uniforms, badges and so on.

I don't know about you, but I've always felt that the character animations, dating back to good-old Cyberdogs (1994), could do with an enhancement:

![](https://raw.githubusercontent.com/cxong/cdogs-sdl/gh-pages/_posts/cdogs_walk.gif)

Early on in C-Dogs SDL history, the animation speed was tweaked up so that the feet more or less track the ground. Before that it looked very floaty - just check out some videos of C-Dogs (the DOS game) or Cyberdogs online. Unfortunately, because of those short stubby legs, the animation speed is very fast, which makes them look like those cartoon Minions.

Instead, let's go with a more expressive run cycle, with big leg and arm movements, even some up-and-down movement for the body. Here's what that might look like:

![](https://raw.githubusercontent.com/cxong/cdogs-sdl/gh-pages/_posts/run.gif)

What do you think? Already going from a 4-frame animation to a 6-frame animation makes it look much more expressive.

Let's also take this chance to improve the gun-hand. The characters in C-Dogs either hold:

- a generic blaster,
- a knife,
- or nothing

in their gun-hand. Let's split out the gun from the hand, and have multiple poses, like this one for one-handed weapons:

![](https://raw.githubusercontent.com/cxong/cdogs-sdl/gh-pages/_posts/run_gun.gif)

This one works for grenades and knives too, the hand position is roughly the same.

For two-handed guns, something like this would look better:

![](https://raw.githubusercontent.com/cxong/cdogs-sdl/gh-pages/_posts/run_gun2.gif)

Works nicely even though the head and leg animations are identical.

Making these changes will require splitting the sprites into different layers:

- Head
- Torso
- Left arm
- Right arm
- Weapon
- Legs

If we also have 8 directions (up from 4 currently), that means a lot of extra sprite work. To make this easier, perhaps the animation can be done in 3D and rendered. We'll have to see how this looks, having nice sprites from exported 3D isn't easy.
