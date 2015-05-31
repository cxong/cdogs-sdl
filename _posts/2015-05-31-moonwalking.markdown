---
layout: post
title:  "Moonwalking"
date:   2015-05-31
categories: progress
---
Some modest but promising progress:

![](https://raw.githubusercontent.com/cxong/cdogs-sdl/gh-pages/_posts/moonwalking.gif)

What you see here is two instances of C-Dogs SDL, connected over localhost. One is sending move commands to the other. I haven't added direction in yet, hence the hovering.

The hard part about adding networked multiplayer to an older game like C-Dogs is its architecture: the whole thing must be partitioned into client and server portions, with events connecting the two. That's [well under way](https://github.com/cxong/cdogs-sdl/issues/225), but connecting everything up is fiddly and feels like transplant surgery. Fingers crossed, the next version of C-Dogs SDL might have networked multiplayer!