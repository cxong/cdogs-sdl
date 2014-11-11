---
layout: post
title:  "Corner sliding"
date:   2014-09-04
categories: progress
---
Here's something that should make the game easier to pick up: corner sliding. If the player is walking into a corner of a wall, they will slide around the corner instead of being stopped completely. This makes walking through those tight corridors much eaiser.

![](https://raw.githubusercontent.com/cxong/cdogs-sdl/gh-pages/_posts/corner_sliding.gif)

[The feature was implemented](https://github.com/cxong/cdogs-sdl/issues/261) using a [hybrid diamond/AABB collision shape](https://github.com/cxong/cdogs-sdl/commit/86ba8c46ef4e19e35ca71807970d3b0e4140208b) for players. The diamond shape is used when moving in cardinal directions (up/down/left/right). This creates the corner sliding, as the diamond "slides" around the corners. AABB (axis-aligned bounding box) is used when moving diagonally, so that wall sliding works, which has been in C-Dogs all along.