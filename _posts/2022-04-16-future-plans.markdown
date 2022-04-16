---
layout: post
title:  "Future Plans"
date:   2022-04-16
categories: other
published: true
---

As [mentioned previously](https://cxong.github.io/cdogs-sdl/release/2021/08/21/c-dogs-1.0.0.html), C-Dogs SDL has well and truly gone into sporadic development mode. That doesn't mean it has stopped; work continues albeit at a slower pace, and there is another release in the works...

On that note, here are some future plans for C-Dogs SDL, although it's unlikely they will be achieved. Still, they might be interesting for you to think about:

# Cyberdogs

Yes, [the 1994 freeware DOS classic](https://www.mobygames.com/game/cyberdogs), Cyberdogs, by the same original author as C-Dogs! Cyberdogs was released a number of years earlier, and is still fondly remembered by many. Although it looks very similar and plays similarly to C-Dogs, and if you compare the feature list it has many things missing (like different types of maps, story-based campaigns, fewer weapons), crucially it has something that C-Dogs doesn't: **buying and selling weapons and ammo**. This feature is quite significant, and [creates a cool meta-game](https://cxong.github.io/2015/05/ammo-is-a-resource) on top of the base run-and-gun shooting game, which is why many players prefer Cyberdogs over C-Dogs.

Well what if we could have Cyberdogs and C-Dogs in the same game???

![Cyberdogs](https://www.mobygames.com/images/shots/l/218123-cyberdogs-dos-screenshot-getting-shot-at.png)

This is the plan: [add a Cyberdogs custom campaign to C-Dogs SDL](https://github.com/cxong/cdogs-sdl/issues/71). It will have the classic Cyberdogs guns, creepy zombie/cyborg baddies, the buy/sell phase, and all in C-Dogs SDL livery with its bells and whistles. Cyberdogs SDL if you will.

And it won't be terribly hard to do so. Virtually all the features are implemented, apart from the buy/sell mechanic.

# More Wolf3D-based Mods

The big version 1.0 release was all about being able to play Wolfenstein 3D and Spear of Destiny. It was well-received, and rightly so: both C-Dogs and Wolfenstein 3D are fun games, and Wolf3D-in-C-Dogs is the best rendition of Wolfenstein 3D in 2D (if I say so myself 🧐).

As you may know, there are a bunch of games that also use the Wolfenstein 3D engine. These include some cool but perhaps less well known games, like **Rise of the Triad**, [**Super 3D Noah's Ark**](https://github.com/cxong/cdogs-sdl/issues/712), [**Blake Stone**](https://github.com/cxong/cdogs-sdl/issues/714), and more <sub>*stinkers like Corridor 7...*</sub>

![Blake Stone](https://upload.wikimedia.org/wikipedia/en/e/ec/Blake_Stone_screenshot.png)

What if we could play these games, but in 2D?

This is possible but will require a bit of work. Because they use the same Wolfenstein 3D engine, their level and audio data is easily supportable, but graphics will need to be reworked, since the transition between first-person and overhead, C-Dogs graphics is not easy. Plus new gameplay elements will need to be added where necessary, e.g. talking to people like in Blake Stone. While a lot of work, it won't be out of the question...

# Good Internet Multiplayer

[LAN multiplayer was added back in version 0.6](https://cxong.github.io/cdogs-sdl/release/2016/03/10/c-dogs-0.6.0-released.html), and internet multiplayer is also supported although well hidden - you have to access it via a command-line parameter. It was hidden for good reason - [lack of game lobbies, and anti-lag.](https://github.com/cxong/cdogs-sdl/issues/384)

<a
    href="https://raw.githubusercontent.com/cxong/cdogs-sdl/gh-pages/_posts/portable_lan.jpg"
    data-fancybox="gallery">
![](https://raw.githubusercontent.com/cxong/cdogs-sdl/gh-pages/_posts/portable_lan_thumb.jpg)
</a>

Multiplayer is perennially one of the most requested features in C-Dogs SDL, so how come it isn't one of the top dev priorities? Well, because it's quite difficult, and requires a lot of stars to align to be worthwhile, so to speak:

- A dedicated online service to host game lobbies, and/or matchmaking
- A community of C-Dogs SDL players, which so far has not materialised
- Good multiplayer content
- Good multiplayer modes (deathmatch is ok, but what about persistent co-op campaigns, [CTF and so on](https://github.com/cxong/cdogs-sdl/issues/187)?)
- [A good multiplayer bot](https://github.com/cxong/cdogs-sdl/issues/521) for when human players aren't available

So it's a bit of a chicken-and-egg problem; without good multiplayer support it's hard for a community to form, but without a community there is little incentive to work on multiplayer. But who knows, maybe with enough effort chipping away at the edges...

# A Proper Story and World

One thing that constantly baffles new C-Dogs players is that they don't understand what the game is *about*. In the original game, players get 5 unrelated campaigns, where you battle an army of ogres, or fight a zombie infestation, or stop bank robbers... you name it, it was a random mix of action game tropes.

What if these campaigns were all interconnected, inside a larger story, or even game world?

[![Cyberpunk world](https://pixeljoint.com/files/icons/full/story3.gif)](https://pixeljoint.com/pixelart/120891.htm)

Perhaps there is a real, "main" campaign telling the story of intrepid cyborg-dogfighters, or "cyberdogs", taking on contracts in a gritty post-cyberpunk world. Players could walk around in a persistent hub-world, accessing side missions by talking to specific quest-givers like an RPG. Or maybe the existing campaigns are reworked so they fit within the same world. What would this world even look like? Does it take place in an interstellar empire, or a grimy Blade Runner-esque metropolis? And what's with the bug-eyed monsters anyway, are they AI run amok, or the physical manifestations of an insane genius?

These questions await answers, but perhaps only by a passionate writer willing to undertake such a crazy task.

# ...And More!

There are many more C-Dogs SDL ideas:

- Implementing Ronny Wester's roguelike, [Magus](https://obscuritory.com/rpg/magus/), as an action RPG
- Reading maps from an RTS, like *Command and Conquer*, so you can play as a MOBA
- Fleshing out melee combat, adding [beat-em-up mechanics](https://github.com/cxong/cdogs-sdl/issues/541)
- Adding more vehicles, aircraft, making even more exciting multiplayer content

Which ones would you most like to see? Would you be interested in contributing or even maintaining C-Dogs SDL? [Check out the project today!](https://github.com/cxong/cdogs-sdl)
