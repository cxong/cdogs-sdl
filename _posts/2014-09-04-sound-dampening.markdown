---
layout: post
title:  "Sound dampening"
date:   2014-09-04
categories: progress
---
This video demonstrates some simple sound dampening added recently. Any sounds that occur out of the player's direct line of sight is attenuated and dampened. Listen for yourself:

[![](http://img.youtube.com/vi/vmRj6k1hhUA/0.jpg)](http://www.youtube.com/watch?v=vmRj6k1hhUA)

[The implementation](https://github.com/cxong/cdogs-sdl/commit/c8d66798a2f9aef50ed740cd278724382499f7e6) is quite simple. The line of sight routine is already available and used in a few places already - AI, even the editor for drawing lines - and [dampening the high frequencies](http://en.wikipedia.org/wiki/High-pass_filter) is very easy: just apply a moving average on all the audio samples. I encourage anyone using SDL_mixer to try this technique out!