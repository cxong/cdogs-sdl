---
layout: post
title:  "SDL2 ported, drag and drop"
date:   2015-10-25
categories: progress
---
After quite a few commits, C-Dogs SDL is now ported to SDL2!

Initially, this will only be noticeable as some under-the-hood improvements, for example since the game now renders to a hardware-accelerated texture, it can be scaled with virtually no slowdown. In fact, the window is resizeable, so you can do this:

![](https://raw.githubusercontent.com/cxong/cdogs-sdl/gh-pages/_posts/resize.gif)

One of the first features to take advantage of SDL2 is drag and drop support in the editor: instead of fiddly typing in paths, you can simply drag and drop campaign files into the window:

![](https://raw.githubusercontent.com/cxong/cdogs-sdl/gh-pages/_posts/drag-and-drop-open.gif)

Look out for these features, and more, in the next release!
