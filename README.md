C-Dogs SDL Port
===============
[![Build Status](https://travis-ci.org/cxong/cdogs-sdl.svg?branch=master)](https://travis-ci.org/cxong/cdogs-sdl)[![Release](http://img.shields.io/github/release/cxong/cdogs-sdl.svg)](https://github.com/cxong/cdogs-sdl/releases/latest)[![Project Stats](https://www.openhub.net/p/cdogs-sdl-fork/widgets/project_thin_badge.gif)](https://www.openhub.net/p/cdogs-sdl-fork)

            .--------.
           |  _______|
          /  /  __
         |  |  |  '-.   .---.  .---.  .---.
        /  /  /  /\  \ |  _  ||  ___||  __|
       |  |  |  |_/   || (_) || | __  \ \
      /  /  /        / |     || '-' |.-' `.
     |  |   '-------'   '---'  '----''----'
     /  '.___________
    |                |                  SDL
     `---------------'

1. Introduction
2. Tested Environments
3. Installation
4. Command line options
5. Contact


1. Introduction
---------------

C-Dogs SDL is a classic overhead run-and-gun game, supporting up to 4 players
in co-op and deathmatch modes. Customize your player, choose from many weapons,
and blast, slide and slash your way through over 100 user-created campaigns.
Have fun!

[Releases and release notes](https://github.com/cxong/cdogs-sdl/releases)

For more information about the original C-Dogs read [`original\_readme.txt`](https://raw.githubusercontent.com/cxong/cdogs-sdl/master/doc/original_readme.txt).

[C-Dogs SDL mailing list](https://groups.google.com/forum/#!forum/c-dogs-sdl)


2.Tested Environments
---------------------

C-Dogs SDL is built using the SDL 1.2 API.
See the SDL website - <http://www.libsdl.org> for more about SDL itself.

C-Dogs SDL runs on Linux, \*BSD, Windows and Mac OS X.

C-Dogs SDL is also monitored by the continuous integration service TravisCI

3. Installation
---------------

Please use the installers for your platform for installing.

Follow the [getting started wiki](https://github.com/cxong/cdogs-sdl/wiki#getting-started) to build on your platform. You will need the SDL development libraries installed.

4. Command line options
-----------------------

The C-Dogs binary is called "cdogs-sdl". Below are some command line arguments:

### Video Options

    --fullscreen Try and use a fullscreen video mode.
    --scale=n    Scale the window resolution up by a factor of n
                   Factors: 2, 3, 4
    --screen=WxH Set virtual screen width to W x H
                   Modes: 320x200, 320x240, 400x300, 640x480, 800x600
    --forcemode  Don't check video mode sanity

### Sound Options

    --nosound    Disable sound

### Control Options

    --nojoystick     Disable joystick(s)

### Game Options

    --wait           Wait for a key hit before initialising video.
    --shakemult=n    Screen shaking multiplier (0 = disable).

    --help           Display command line options and version information.

These can be used in any order/combination:

```bash
$ cdogs-sdl --fullscreen --screen=400x300
```

Which will make the game try to run fullscreen at 400x300 resolution.

5. Contact
----------

If you have an questions, comments, bug reports (yes please), patches (even
better) or anything else related to C-Dogs SDL, check out the mailing list:

> https://groups.google.com/forum/#!forum/c-dogs-sdl

Or email:

> Cong <congusbongus@gmail.com>
