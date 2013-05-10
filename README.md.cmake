
C-Dogs SDL Port v@VERSION@
========================

(based on C-Dogs 1.06)

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
     `---------------'               v@VERSION@

1. Introduction
2. Tested Environments
3. Installation
4. Command line options
5. Contact


1. Introduction
---------------

C-Dogs SDL is a classic overhead run-and-gun game, supporting up to two players
in co-op and 1v1 modes. Customize your player, choose from up to 11 weapons,
and try the dozens of user-created campaigns. Have fun!

For the latest release notes please see https://github.com/cxong/cdogs-sdl/wiki/Release-Notes

For more information about the original C-Dogs read 'original\_readme.txt'.


2.Tested Environments
---------------------

C-Dogs SDL has been ported with a Unix development environment.
In theory, C-Dogs SDL could run on any system that is supported by SDL.
See the SDL website - <http://www.libsdl.org> for more about SDL itself.

C-Dogs SDL runs on Linux, \*BSD, Windows and Mac OS X.

There are also unofficial ports to GP2X, BeOS/ZETA/Haiku, MorphOS,
Nintendo DS, and AmigaOS 4 to name a few.

And of course, should you succeed in running C-Dogs SDL on a different
operating system, platform or toaster please contact us! :-)


3. Installation
---------------

Please use the installers for your platform for installing.

To compile C-Dogs SDL you need to have the SDL libraries previously installed,
the SDL\_mixer libraries also installed if you want better sound (and music),
and a compiler (gcc recommended) and the appropriate header files.

See the wiki for getting started instructions.

4. Command line options
-----------------------

The C-Dogs binary is called "cdogs-sdl". Below are some command line arguments:

### Video Options

    -fullscreen Try and use a fullscreen video mode.
    -scale=n    Scale the window resolution up by a factor of n
                  Factors: 2, 3, 4
    -screen=WxH Set virtual screen width to W x H
                  Modes: 320x200, 320x240, 400x300, 640x480, 800x600
    -forcemode  Don't check video mode sanity

### Sound Options

    -nosound    Disable sound

### Control Options

    -nojoystick     Disable joystick(s)
    -js1threshold=n Joystick 1 threshold.
    -js2threshold=n Joystick 2 threshold.

### Game Options

    -savecampaigns  Save builtin campaigns as in the current directory.
    -wait           Wait for a key hit before initialising video.
    -shakemult=n    Screen shaking multiplier (0 = disable).

    -help           Display command line options and version information.

These can be used in any order/combination:

```bash
$ cdogs-sdl -fullscreen -screen=400x300
```

Which will make the game try to run fullscreen at 400x300 resolution.

5. Contact
----------

If you have an questions, comments, bug reports (yes please), patches (even
better) or anything else related to C-Dogs SDL email:

> Cong <congusbongus@gmail.com>

