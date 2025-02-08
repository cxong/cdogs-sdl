---
layout: post
title: "High Scores"
date: 2025-02-08
categories: progress, other
published: true
---

The high score system is due for a [major overhaul](https://github.com/cxong/cdogs-sdl/issues/151), and while we'll add cool new features in the next version like showing the character used, accuracy and favourite weapon used, unfortunately this will be a breaking change: old high scores will no longer be available. This does go against one of [C-Dogs SDL's principles](https://github.com/cxong/cdogs-sdl/wiki/Development-Plan#general-principles), but it's a necessary change.

To explain why, we need to go back in history. C-Dogs' predecessor, [Cyberdogs (1994)](https://www.mobygames.com/game/1055/cyberdogs/), was a fairly straightforward shooting game. Fight through 10 semi-randomly-generated levels shooting bad guys, collecting objectives while the difficulty ramps up, and if you manage to survive until the end, you enter your name into the high scores list. It's pretty typical of 90's games.

<iframe width="560" height="315" src="https://www.youtube.com/embed/hbcsTmcyq_w?si=a_szrdJegRL7Wtpl" title="YouTube video player" frameborder="0" allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture; web-share" referrerpolicy="strict-origin-when-cross-origin" allowfullscreen></iframe>

Although the game is "semi-randomly-generated", what this means is the objectives of each level stays the same, it's only the level layout, enemy and item placement that gets randomised, so playing with a different seed meant you couldn't memorise where things are, but the game itself is pretty similar overall. This means a single high score across the whole game made sense.

Fast forward a little to C-Dogs, which came with 5 campaigns of varying themes and lengths. This greatly improved the variety and replayability of the game, but as in many aspects C-Dogs was two-steps-forward-one-step-back compared to Cyberdogs, the high score system felt a little half-baked. It largely inherited the high score system from Cyberdogs, with a small enhancement - the game would show all-time high scores in addition to today's high scores - but it still used one single set of scores for the whole game, regardless of campaign. This made less sense, since some campaigns would naturally result in higher scores than others.

![harmful chrysalis](https://raw.githubusercontent.com/cxong/cdogs-sdl/gh-pages/_posts/harmful_crysalis.png)

What's worse is that fans created [lots of custom campaigns](https://cdogs.morezombies.net), of greatly varying lengths and quality. Some poorly-made campaigns filled with easy objectives could have a player easily rack up a huge high score. And now C-Dogs SDL comes with perhaps a dozen high quality campaigns, with even more incomparable scores. The high score system was truly broken.

![New Campaigns](https://raw.githubusercontent.com/cxong/cdogs-sdl/gh-pages/_posts/new_campaigns.png)

As I looked into the high score system, the need to rework it entirely was inevitable. We had to have **per-campaign high scores**! This means completely ditching the old high score system and rebuilding it from scratch, and old high scores will not be available in the next release. This decision wasn't made lightly, as high scores can often have a lot of sentimental value, but as we saw over its history:

- it never really made sense in C-Dogs
- it made even less sense in C-Dogs SDL
- it always seemed more like an afterthought (you could only see high scores after playing through a campaign!)

So once we rebuild an enhanced version of the high score system, we'll give it greater prominence - allow you to view the high scores without playing the game. Stick around to see the shiny new high score system in the next release!

![Victory screen](https://raw.githubusercontent.com/cxong/cdogs-sdl/gh-pages/_posts/victory.jpg)
