---
layout: post
title: "Making a Wolf3D Mod"
date: 2024-10-07
categories: other
published: true
---

3 years ago, to mark the big [version 1.0 of C-Dogs SDL](https://cxong.github.io/cdogs-sdl/release/2021/08/21/c-dogs-1.0.0.html), we released Wolfenstein 3D support: play the classic Wolfenstein 3D, but in top-down 2D, inside C-Dogs SDL. It was pretty cool and a well-received release. Since then, we've [added Spear of Destiny mission pack support](https://cxong.github.io/cdogs-sdl/release/2021/12/03/c-dogs-1.2.0.html) and are working towards [Super 3D Noah's Ark](https://github.com/cxong/cdogs-sdl/issues/712). We've also received requests to add support for other Wolf3D games like [Blake Stone](https://github.com/cxong/cdogs-sdl/issues/714). [More are in the roadmap](https://cxong.github.io/cdogs-sdl/other/2022/04/16/future-plans.html).

<iframe width="560" height="315" src="https://www.youtube.com/embed/gnZX0IJV4oo" title="YouTube video player" frameborder="0" allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture" allowfullscreen></iframe>

While it's cool that the gameplay experience is very authentic, due to directly using the same levels, sounds and music, this also hides the fact that a lot of work was required to get to this point. Many elements, most notably the graphics, could not be reused, so they were meticulously recreated for C-Dogs SDL, not to mention many engine enhancements had to be made to support Wolf3D-style gameplay. The original release with Wolf3D + Spear of Destiny support took about 2 years, but even the [Mission Packs took 4 months of active development](https://cxong.github.io/cdogs-sdl/release/2021/12/03/c-dogs-1.2.0.html), and that was mostly a content-only update. I think we did a good job making an end product that plays both like Wolf3D and C-Dogs, and is still fun.

> ![Wolf3D wall textures](https://beta.wolf3d.net/sites/default/files/styles/128x128_hi_res_sprite_/public/users/Cabo_Fiambre/previews/prev.png?itok=giCEDucP) ![Wolf3D in C-Dogs SDL](https://raw.githubusercontent.com/cxong/cdogs-sdl/gh-pages/_posts/wolf3d_sprites.png)
>
> Wolf3D textures are 64x64, but C-Dogs tiles are 16x12. There is no way to automate this conversion process, so they had to be recreated by hand, with a lot of creative license added

And so I'd like to document exactly how much work goes into adding a Wolf3D mod like Super 3D Noah's Ark or Blake Stone. Perhaps you will, as I have, gain a better appreciation for people who make mods and the hard work required!

# CyberWolfenstein

I've had the idea of adding Wolf3D support into C-Dogs SDL for a long time. There were other versions of Wolf3D-in-2D [1](https://wl6.fandom.com/wiki/Vogelstein_2D) [2](https://gamebanana.com/mods/67588) [3](https://kevinlong.itch.io/wolf2d) [4](https://www.newgrounds.com/art/view/devilsgarage/wolfenstein-2-d) [5](https://pixeljoint.com/pixelart/28499.htm) but none of them were both high quality and authentic. I wanted to create the definitive version of Wolfenstein 2D. Both are 2D tile-based shooting games, and Wolf3D's data is well-documented and the source code is open, so with a bit of work this should be possible right?

> ![Wolf3D levels in a tile editor](https://steamuserimages-a.akamaihd.net/ugc/1839180655231070065/3F4F352553DDED7354F9BF3FF245DFC802D0A9D4/)
>
> Wolf3D is a raycasted pseudo-3D game over a 2D tile-based game underneath. Its levels were created with a tile map editor.

It turns out there were three big challenges in making this happen. Extracting the data out of Wolf3D was doable but not trivial; the game's data was not made to be moddable so a substantial amount of work was required just to get to the data. Fortunately there are plenty of resources like the [DOS Game Modding Wiki](https://moddingwiki.shikadi.net/wiki/Wolfenstein_3-D), and in a pinch I could rip code out of places like [ECWolf](https://maniacsvault.net/ecwolf/). Then there's C-Dogs SDL itself, which was made as a game with a specific kind of gameplay instead of a general purpose 2D shooter, which meant that even support for arbitrary amounts of tile types per map had to be developed, leading to difficult, months-long refactors. Finally some content simply could not be used and had to be recreated, such as the textures, sprites and characters.

| Wolf3D Feature | C-Dogs SDL Implementation |
| -------------- | ------------------------- |
| Levels         | ✅ Used as-is             |
| Sounds         | ✅ Used as-is             |
| Music          | ✅ Used as-is             |
| Text Strings   | ➖ Extracted              |
| NPC Behaviour  | ❌ Recreated              |
| Sprites        | ❌ Recreated              |
| Textures       | ❌ Recreated              |
| Weapons        | ❌ Recreated              |

## Data Extraction

For extracting Wolf3D data I specifically created the [CWolfMap C library](https://github.com/cxong/cwolfmap), which in theory could be used by all kinds of programs and games that want to use Wolf3D data. While there are a lot of programs that can do the same thing (my favourites include [SLADE3](https://slade.mancubus.net) and [HWE](https://hwolf3d.dugtrio17.com/index.php?section=hwe)), none of them are open source libraries that can be easily incorporated into games. The library also has to deal with complications like multiple audio formats (Wolf3D has both PCM and Adlib sounds; Super 3D Noah's Ark uses OGG-in-WAD music).

Another annoyance is that while the level data is pretty straight forward - they're just planes of tile IDs - the semantics of the tile IDs differs between different Wolf3D games. That is, an ID that represents "wooden table" in Wolf3D might be a totally different entity type in S3DNA, like a barrel, an enemy, or ammo pickup! So for every Wolf3D mod, we need to go through all the tile IDs and meticulously build a mapping to the right entity type. It is not a simple re-skinning job at all.

> ![Havoc's Wolf3D Editor, showing the map symbol editor](https://hwolf3d.dugtrio17.com/projects/hwe6.gif)
>
> Wolf3D maps consist of planes, each filled with integer tile IDs. What the IDs actually represent is completely up to the game/mod: the same ID could mean a wall, a door, map object, trigger, pickup, enemy, and more, depending on the game and plane.

## Data Recreation

Then there's recreating the parts of Wolf3D that cannot be easily extracted. We need to redraw the hundreds of textures and sprites in C-Dogs style, which represents dozens of hours of pixel art work. Some special characters also need some [3D modelling and animation](https://cxong.github.io/2017/03/3d-rendered-pixel-sprites) - notably to make the dog character, which C-Dogs did not have; ironic huh? 🐕

We took some liberties porting across enemy behaviour. Wolf3D actually has pretty complex NPC AI considering its age, most likely due to its aborted initial design of being an action-stealth game: enemies have patrol routes, rooms have defined noise triggers such that a gunshot in the same room alerts everyone in it, and enemies have vision cones so you can sneak up behind them. C-Dogs SDL also had an enemy awareness mechanic but it was much more rudimentary, so mechanics like vision cones and noise alerting had to be built in specifically for Wolf3D. However mechanics like patrolling were entirely left out; I don't think players would even notice!

Wolf3D's gunplay was also quite unique; apart from the knife, the 3 guns are all variations of hitscan weapons. C-Dogs SDL already had [hitscan weapons](https://cxong.github.io/cdogs-sdl/release/2017/04/23/c-dogs-0.6.5.html) thanks to earlier work with a Doom mod, but if you look closer, [Wolf3D doesn't actually use traditional hitscan weapons](https://wolfenstein.fandom.com/wiki/Hitscan)! Instead it uses damage formulas, which aim to simulate the experience of realistic gunplay: more damage is dealt/received if the target is close, there's a random chance of missing, depending on factors like whether you are running, but also weirdly, [enemies deal different damage to you if you can't see them](https://wolfenstein.fandom.com/wiki/Damage)!

Rather than try to recreate this tangle of a damage system, we were conservative and just went for simple hitscan bullets that deal constant, predictable damage. It's not the same and makes the game more C-Dogs-like than Wolf3D-like, but is still quite fun.

## Added Features

In order to fully recreate Wolf3D, some features had to be built from scratch because there were simply no equivalents in C-Dogs SDL, no matter how many creative liberties we took. These represented the bulk of the development time in order to support Wolf3D and Spear of Destiny:

- Rudimentary vehicles, to support Mecha Hitler
- Persisting guns, ammo and lives across levels
- Pickups with multiple effects (the 1up pickup adds score, ammo and health)
- Non-file music, to support decoding and streaming adlib music via a chip emulator
- Additional music types, to be played during the campaign briefing, mission briefing, and debrief
- Multiple tile types per level
- Multiple exits per level, and being able to exit back to a previous level, to support secret levels
- Enemies to drop specific items, like ammo and keys (for bosses)
- Dual-wielded guns, for the bosses
- Guns that shoot multiple types of bullets, for the bosses that can shoot both machine guns and rockets
- Custom corpses
- Custom footstep sounds, for dogs and Mecha Hitler
- Objects that can't be shot by bullets - Wolf3D objects don't interact with bullets at all; making them destructible would have broken some levels!
- Fake walls that can be walked through (as an expedient way of representing pushwalls)
- Different in-game gun models
- Enemy alert system - peripheral vision, wake on hearing gunfire, wake on seeing an ally being attacked
- New player models and parts, like helmets and peaked caps, dog head
- Enemy pain state, simulated using a short petrify effect
- And lots of miscellaneous fixes and tweaks

<video muted autoplay loop>
    <source src="https://raw.githubusercontent.com/cxong/cdogs-sdl/gh-pages/_posts/mech.webm" type="video/webm">
</video>

The list doesn't end there if we want to add other Wolf3D games; Super 3D Noah's Ark has melon launchers and the bible quiz pickup item, Blake Stone has NPC conversations and more.

# The Roadmap

To cap off, every Wolf3D mod will require, at minimum, a load of content to be recreated (mostly the textures and sprites), some gameplay features to be added, and some miscellaneous fixes and tweaks too. This is a significant amount of work.

Despite this I still plan to add as many Wolf3D mods as possible, albeit gradually. I think [these old shooters are pretty neat](https://cxong.github.io/2024/03/in-the-shadow-of-doom), and deserve some extra love by being ported to a modern game. Supporting these games has the side effect of making C-Dogs SDL a better game. And porting 3D shooters into a 2D topdown shooter game is honestly pretty cool, and something that we should have more of.
