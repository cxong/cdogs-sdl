---
layout: post
title:  "Co-op AI confusion"
date:   2014-05-12
categories: progress
---
Hope you're enjoying C-Dogs SDL 0.5.5.1!

[0.5.6 development](https://github.com/cxong/cdogs-sdl/issues?milestone=16&state=open) is under way; the goal once again being more polishing and refactoring, so that more awesome features can be added.

This upcoming release will have some neat co-op AI enhancements. The co-op AI will form the basis for the new AI system, but it's in its infancy and has a lot of flaws. One of them is that the AI reacts badly to being confused - it would simply run in the opposite direction to where it's meant to, which often means away from the player!

Instead, the AI has been improved to be alternately confused. It would alternate between performing some random, nonsensical action, and doing the right thing. Here's a video demonstrating this:

<video src="https://raw2.github.com/cxong/cdogs-sdl/gh-pages/_posts/co-op-ai-confusion.webm" width="1024" height="768" controls preload></video>

The AI will still be handicapped by the confuse gas, but they will not simply run away.

Besides this, the AI has also had some fixes to its tendency to get stuck behind objects. This should make playing with co-op AI much more fun.

Stay tuned for more progress updates!