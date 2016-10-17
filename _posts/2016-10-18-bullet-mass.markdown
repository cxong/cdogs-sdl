---
layout: post
title:  "Bullet Mass"
date:   2016-10-18
categories: progress
---

[Recently](https://github.com/cxong/cdogs-sdl/issues/444) we've added *mass* as a parameter for bullets. This decouples the amount of pushback from the amount of damage the bullet deals. Previously these were the same.

This helps improve some guns like the flamer, which deals a lot of damage but doesn't necessarily push back a lot.

What's interesting is that mass can be *negative*, which means instead of pushing back, it pulls the target forward, which allows some interesting weapon ideas...

![scorpion spear weapon](https://raw.githubusercontent.com/cxong/cdogs-sdl/gh-pages/_posts/scorpion_spear.gif)

This weapon only appears in the custom weapon tech demo campaign, but one day we might see more cool weapon ideas in proper campaigns!