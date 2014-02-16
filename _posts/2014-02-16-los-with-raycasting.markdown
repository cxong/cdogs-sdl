---
layout: post
title:  "LOS with Raycasting"
date:   2014-02-16
categories: progress
---
![](https://raw2.github.com/cxong/cdogs-sdl/gh-pages/_posts/los.gif)

The line-of-sight algorithm got an upgrade in 0.5.4. This new algorithm uses raycasting; rays are cast from the player's tile outwards to the edges, setting tiles as lit until a wall is encountered.

[https://github.com/cxong/cdogs-sdl/issues/94](https://github.com/cxong/cdogs-sdl/issues/94)