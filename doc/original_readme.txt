+----------------------------+
|        C-Dogs v1.06        |
+----------------------------+
|                            |
|    A FreeWare DOS-game     |
|                            |
| (c) 1997-2001 Ronny Wester |
|                            |
+----------------------------+
This game, the code and artwork are copyrighted by the author, Ronny Wester.


This file is best viewed with a monospace font.


+------------+
| Disclaimer |
+------------+

You use this game at your own risk. While I will try to help you out to the
best of my abilities should you experience any problems, you're essentially on
your own.


+----------+
| Contents |
+----------+

Overview
So is it shareware or freeware, and what took you so long anyway?
System requirements
Getting started
Controls
Menus
Playing the game
How to get music
A note on the sound FX
The weapons
Frequently Asked Questions
Background information


+----------+
| Overview |
+----------+

C-Dogs is a 1 or 2 player game for DOS.
The player character(s) are elite soldiers, taking on mission after mission
to defeat evil in any form. That is, you kill it or blow it up.
Sounds violent? Yep, that's right. Still, the emphasis is on gameplay rather
than gore. But the game will involve a lot of mowing down of enemies - and
the occasional unlucky civilian - so now you have been warned.


+-------------------------------------------------------------------+
| So is it shareware or freeware, and what took you so long anyway? |
+-------------------------------------------------------------------+

This is strictly a spare time project for me. I work fulltime as a software
designer and developer and sometimes programming is not how I want to spend
my spare-time. Aikido-practice takes up a lot of time too.

Reactions to the original Cyberdogs were very positive. So positive that I
changed my mind about doing more work on it in fact 8-). Part of those reactions
was offers from publishers and since getting a game published has been a dream
for me since I was a child these offers were quite attractive. Well, things are
not always easy and due to a number of reasons I have been undecided about the
shareware/freeware issue for some time. Lately the freeware approach seemed most
likely but then I got a solid offer from Magicomm - who did some nice shareware
compilations featuring the original Cyberdogs - that at the time of writing
looked like the most likely route.

In the end though, I opted for the less work route and thus the game is freeware.

The basic terms for redistribution are as follows:
a) All files must be distributed unmodified
b) There must be no charge for the game as such.
You may want to read the section on sound fx as well...


+---------------------+
| System requirements |
+---------------------+

Theoretical minimum configuration (to my knowledge at least):

80386
DOS 3.3+
2Mb RAM (with no music or small modules)


Minimum tested configuration:
80486 66MHz
DOS 6.2
8Mb RAM
Local bus graphics card


Recommended configuration:
80486+
DOS 6.2+
8+ Mb RAM
PCI graphics card
Stereo sound card
2 gamepads and a Y-cable

Running under Windows 3.1 or Windows 95 (in a DOS box) might work, but don't
complain to me if it doesn't. The recommended way is to boot your old DOS, or
run it in Windows 95 DOS mode.

Note: I run it in a DOS box under Windows 98 and it works fine except for the
occasional need to tap a key or move the mouse to make sure Windows keeps giving
it enough CPU time (otherwise framerate drops after a while).


+-----------------+
| Getting started |
+-----------------+

1. Ensure all the files of the archive are in the same directory.
Start by running DSETUP32.EXE to set sound. Select your sound card or
"No sound" if you do not have a soundcard. Speaker sound effects are not
supported. Ensure that any joysticks are attached, and - if you have the
ability to switch between different joysticks - that the correct ones are
activated.
Then run CDOGS.EXE. Make sure the current working directory is the one where
the files are located.


+----------+
| Controls |
+----------+

If joysticks are connected, they will be used as default, unless you select
other controls in the Options/Controls menu.
The default keyboard controls are:

Control:              Player 1:      Player 2:
-------------------------------------------------
Left                  Left arrow     Keypad 4
Right                 Right arrow    Keypad 6
Up                    Up arrow       Keypad 8
Down                  Down arrow     Keypad 2
Button 1              Left Ctrl      Keypad 0
Button 2              Enter          Keypad Enter
-------------------------------------------------

Common controls:
-------------------------------------------------
Automap               Tab
Pause                 Esc (Any OTHER key continues)
Abort game            Esc twice
-------------------------------------------------

Button 1 is used to select menu items and to fire weapons in the game.
Button 2 is sometimes used to cancel operations. In the game button 2
can be used to change weapons or, with directional controls, to slide.

The keyboard controls can be freely customized, see [The Menus].

In menus, player one's controls are used. In addition the arrow keys and Enter
can be used, unless they are configured as controls.

The following keys are reserved for special purposes:
-------------------------------------------------------
Esc   Exit out of most menus/pause or abort game
F10   Recalibrate joysticks (assumes centered position)
F9    Reset controls to keyboard only
-------------------------------------------------------


+-----------+
| The Menus |
+-----------+

The main menu:
------------------------------------------------------
1 player game     Starts a one player game
2 player game     Starts a two player cooperative game
Dogfight          Starts a two player death match
Game options...   Goes to the options menu
Controls...       Goes to the controls menu
Sound...          Goes to the sound menu
Quit              Quits the game
------------------------------------------------------

  Starting a game:

    1. Select your character:

      -------------------------------------------------------------
      Name              Allows you to name your character
      Face              Select a face
      Skin              Select skin color
      Hair              Select hair color (for those who have hair)
      Arms              Select arm color
      Body              Select body color
      Legs              Select leg color
      Use template      Select a premade character
      Save template     Save the character you have made
      Done              End character selection
      -------------------------------------------------------------

      Editing your name:
      Use directional controls to select letters. Button 1 will select letters,
      Button 2 will remove letters. The first letter, and any letter following
      a space will be capitalized, all others will be small.

      In Use/Save templates Button 1 confirms and Button 2 cancels.

   2. Select a campaign (or Dogfight arena)

      Pick one. Names preceded by a filename are campaign/arena files created
      by the campaign editor that have been found in the current directory.
      (full version only, shareware version cannot load campaign files).

   3. Start game or Enter code (1 or 2 player games only)

      If you have received a level code previously you can enter that code by
      selecting the [Enter code] menu item. Level codes are different for
      different campaigns and also for 1 and 2 player games.


Game options menu:
-------------------------------------------------------------------------------
Players shot hurt  [Yes/No]   If set, player bullets will hurt the other
                              player in cooperative mode. This setting has no
                              effect in a dogfight.

FPS monitor        [On/Off]   Displays a frame rate counter in the lower
                              right corner of the game display

Clock              [On/Off]   Displays the current time in the lower left
                              corner of the game display

Copy to video [rep movsd or   Determines the assembly instructions used to copy
               dec/jnz]       the RAM screen to the video memory. Use whichever
                              is fastest. On a Pentium I can't notice a
                              difference.

Brightness         <number>   Brightens/darkens the colors if need be.
                              Use left/right to set a value.
                              (-10...10)

Splitscreen always [Yes/No]   If No, two characters will share the screen if
                              they are close enough together. If Yes the screen
                              will always be split.

Random seed        <number>   This number "controls" the pseudo-random
                              generation of maps and placing of items.
                              By changing this you can replay any campaign with
                              new maps (oh, the level codes change too...).
                              Use left/right to change. Pressing one or both
                              buttons will increase the rate of change.
                              (0...very large)

Difficulty		  <5 levels>  Controls the behaviour of the non-players.

Slow motion        [On/Off]   If set to On, makes the game run half as fast.

Density           <25-200%>   Changes the number of characters in mission.

Non-player HP     <25-200%>   Modifies the maximum hitpoints of non-players.

Player HP         <25-200%>   Modifies the maximum hitpoints of players.

Done                          Return to main menu
-------------------------------------------------------------------------------


Controls menu:
-------------------------------------------------------------------------------
Player 1 [Joystick 1/Joystick 2/Keyboard]  Select controls of player 1
Player 2 [Joystick 1/Joystick 2/Keyboard]  Select controls of player 2
Swap buttons of joystick 1                 Swap button 1 and 2, these settings
Swap buttons of joystick 2                 do NOT affect keyboard controls.
Calibrate joystick                         Recalibrate joysticks
Done                                       Return to main menu
-------------------------------------------------------------------------------

Notes:

Control changes take effect immediately. This can be confusing since
player 1 controls can be used to navigate the menus.

Recalibrate joystick actually recalibrates both sticks. The sticks are assumed
to be in their center positions. An automatic recalibration is performed when
the game is started and can also be achieved by pressing F10 in any menu.

F9 resets player controls to Keyboard and is useful if you're having joystick
problems.


Sound menu:
---------------------------------------------------------------
Sound effects              <number>  Volume of effects    (0-8)
Music                      <number>  Volume of music      (0-8)
Sound effects channels     <number>  Simultaneous effects (2-8)
Disable interrupts in game [Yes/No]  See note below
Done                                 Return to main menu
---------------------------------------------------------------

Notes:
The sound system is usually run as an interrupt to ensure that it gets the
CPU time it needs in order to avoid delays and stutters in the sound.
However, interrupts can occasionally interfer with the logic that synchronizes
screen updates with video refresh cycles and cause occasional "stutter" in the
game. In practice you will not notice this unless you have a steady 70fps and
you should not activate this feature unless you get at least 40fps. If you are
running this game in a DOS box under eg Windows 95 you will get that kind of
problem anyway so you might as well leave it off (better yet, run the game in
DOS instead!).

Sound effects channels specify the number of simultaneous sound effects
possible. The total number of sound channels in use will depend on the number
of channels used by music. A higher number will sound better, particulary in
two player games, but will require more CPU power. In practice it is usually
the music modules which are the real CPU hogs, so the best action if there's a
performance problem is usually to select low channel-count modules for your
music.

The sound system used in this game is the DSMI (Digital Sound and Music
Interface) by Otto Chrons. It is a commercial library. Further inquiries are
best directed to Otto himself at oc@iki.fi.


+------------------+
| Playing the game |
+------------------+

Game controls:
Directional controls move your character about.
Button 1 fires the selected weapon.
Button 2 cycles through your weapons, if pressed and released WITHOUT any
movement taking place.
If you press and hold Button 2 and then move in some direction your character
will "slide" in that direction at greater than normal speed. It is not possible
to move at this speed constantly, one slide must come to a stop before another
may be initiated. It is a great way to get out of a tight spot - such as when
grenades come rolling towards you - in a hurry, or, as you get more experienced,
to rapidly close the distance to a baddie in order to deliver a knife thrust or
a shotgun burst at close quarters.


The actual gameplay takes the form of several missions making up a campaign.
There are several campaigns to choose from and more can be made with the
included campaign editor. Each mission has one or more objectives that need to
be fulfilled in order to progress to the next level.

In two player mode, only one player needs to complete the level. Both players
will then be able to go on to the next level. A player who fails a mission will
have his score reset to zero though with the current score being entered into
the highscore list if it is good enough.

Every mission takes place in a rectangular area containing walls, rooms and
assorted objects - and villains of course. Prior to a mission you will receive
a short briefing, a listing of all objectives and the option to choose your
weaponry. The weapons available to you may vary between missions.

You may pick any three from the available weapons. Ammo is unlimited, but a
scoring penalty applies. Each shot fired deducts from your score, more or less
depending on the weapon used. The knife foes not incur any penalty, but is also
the most dangerous to use as it involves close combat. The knife can, however,
be a very lethal weapon...particulary in a dogfight.

Completing a mission:
A mission is completed once all objectives have been accomplished, ie when all
objectives are labelled as "Done" at the bottom of the game screen. There is
still the problem of getting out in one piece, once the mission is complete a
pickup-zone will be highlighted, both on-screen and on the map. The player(s) -
along with any prisoner rescued - must remain in the pickup zone for 5 seconds
- a countdown is displayed while in the zone - to be picked up and the mission
safely concluded.

Civilians:
In some missions you'll come across innocent bystanders. You should try to keep
them from getting harmed. Any harm you do to them will be taken out of your
score. More importantly, the game always maintain a certain number of characters
in the mission area, and the more of those that are bystanders rather than
enemies the better, don't you think?

The automap:
Sometimes your objectives will be known ahead of time, giving you their location
even in unexplored territory. Other times objectives may not show up on the map
at all...typically the case when the objectives are live opponents, or with
bonus objectives. Most of the time objectives will show up on the map, but not
until you have come across them personally.
The automap is not available in Dogfights.


Bonuses & penalties:
You score for enemies killed and objectives fulfilled. You lose points for
using up ammo and for killing innocents. In addition there are some other
bonuses and penalties which may apply:


Access bonus
Possession of keys will grant you a bonus dependant on the number of keys
collected.

Perfect (500p)
You completed all tasks within an objective when only some were required.

Health bonus (10-500p)
You get 10p for each health point left from the top 50 at the end of a mission

Time bonus
For a quick completion of missions you score extra, 10p per second under the
given limit (60 seconds + 30 seconds per objective). Yes, some missions are far
easier to complete within the time limit than others. No, it is not fair.

Friendly bonus (500p)
Should you complete a mission without killing anyone, you are rewarded.

Ninja bonus
If you use ONLY the knife you get an additional 50p per enemy killed.

Butcher penalty
Kill too many civilians and it will cost you an extra 100p per victim.

Resurrection fee (-500p)
If you are killed in the very last moment, you complete the mission but
the resurrection will cost you. But that's a price one pays gladly, no?


Tips:

Focus on the objectives. It is generally better to avoid engaging hostiles
unless you are required to.

Keep moving. Quickly if there's enemy fire, cautiously if it's quiet.

Remember where things - primarily keys - are located. Characters change each
time you play a mission, but walls and items always remain the same.

Avoid fighting an enemy with an identical weapon. Don't take on a flamer with
a flamer. Gun the enemy down from greater range using a machine gun instead.
A machine gunner you might want to take on by rapidly closing and firing a
shotgun burst or by bullseye'ing them with the sniper gun.

Learn the weapons and choose the right one for the job. Sometimes your favorite
gun just won't be suited to the task at hand.


+------------------+
| How to get music |
+------------------+

There are no music files included with the game. There are two primary reasons
for this:
1. Keeping the size of this archive small.
2. I do not have any original music of my own, and I do not feel it is right
for me to distribute other people's work.

C-Dogs can play most MOD-style music pieces. You will need to get some suitable
music files, typical extensions are .MOD and .S3M, put them somewhere suitable,
they do not need to be in the same directory, and, lastly, tell C-Dogs to play
these files. The last step is performed by entering the filename, and it's
complete path if they're not in the same directory as the rest of the game
files, into one of the two music configuration files: MENUSONG.CFG and
GAMESONG.CFG. The first lists songs which should be played in menus, the second
songs that will be played during missions. The game will cycle through all
specified songs. It will not actually change songs during a mission, only when
a mission is begun or concluded.

For those of you familiar with the original Cyberdogs there is no longer a need
to convert music files into AMF-format. Indeed, the AMF files of the original
Cyberdogs tend to not work, or sound poorly. Sorry.

For the computer-savvy people, there is a way to specify a default directory in
which to look for specified module files. You will have to edit the OPTIONS.CNF
text file (it is created automatically by the game the first time you run it).
The second to last line should be empty (the last line should contain a number).
Here you can enter a path that should indicate a directory. This path will be
prepended to any module file that:
   a) could not be loaded as is,
   b) does not contain a backslash (\) character.

If a music file can not be loaded for some reason, a message to that effect
will be displayed at the start of a game. If a menu song can not be loaded no
message will appear. Note that some modules fail to load, even though the file
is there and plays fine in separate module players. I do not know why that is.


+------------------------+
| A note on the sound FX |
+------------------------+

Some of the sound fx may sound familiar to you. Well, they have been, ahem,
borrowed from some other games.
If you feel like trying your own sound fx you should create a text file named
SOUND_FX.CFG. It should contain a number of rows with a filename, filename ONLY
- no paths!, followed by the frequency with which it should be played. This is
the default configuration:

BOOOM.RAW    11025
LAUNCH.RAW   11025
MG.RAW       11025
FLAMER.RAW   11025
SHOTGUN.RAW  11025
FUSION.RAW   11025
SWITCH.RAW   11025
SCREAM.RAW   11025
AARGH1.RAW    8060
AARGH2.RAW    8060
AARGH3.RAW   11110
HAHAHA.RAW    8060
BANG.RAW     11025
PICKUP.RAW   11025
CLICK.RAW    11025
WHISTLE.RAW  11110
POWERGUN.RAW 11110
MG.RAW       11025

Please note that if you tinker with this, you're on your own. It s not a
supported feature so please don't ask me about any problems you my experience.


+-------------+
| The weapons |
+-------------+

These are the weapons available to players, some additonal weaponry, eg the
confusion bombs, are for the computer only. Tough luck!
Also, beware that some foes may be immune to the effects of fire!

Knife
  Just walk into who-/whatever you want to hurt.
  Can cause a lot of pain!
  The knife will NOT harm explosive or flammable items, eg powder kegs.
  This to avoid inadvertently blowing yourself up by mistake.
  No scoring penalty.

Machine gun
  Sprays a lot of bullets in the general direction you're firing.
  Each bullet does 10hp damage and incurs a 1p scoring penalty.

Power gun
  Longer range and more of a punch than the MG, this one does 20hp of damage
  and carries a 3p penalty per shot. It has a slower rate of fire.

Flamer
  Short range and fair damage.
  It is the easiest weapon to hit with, doing 12hp damage and incurring a 1p
  penalty per "flame".

Sniper gun
  "One shot, one kill" is the philosophy with this one. Same speed and range
  as the Power gun, this one packs 50p of damage with a 5p penalty. A very low
  rate of fire makes this the marksmans weapon.

Shotgun
  Arguably the weapon of choice, this one fires five bullets in a 45 degree arc.
  Each bullet causes 15p of damage for a whooping 75p damage when fired at very
  close range. Each burst will cost you 5p and the firing rate is rather low.

The remaining weapons can hurt the player(s) as well as the bad guys, so take
caution!

Grenades
  Throw'em and they'll bounce about and then go off with a big bang and lots of
  damage to those unfortunate enough to be close by. 20p penalty for each one.

Shrapnel bombs
  When these explode, they send eight pieces of shrapnel flying in all
  directions. Each of these cause 40p of damage. This one too incurs a 20p
  penalty.

Molotovs
  A very dangerous weapon, and quite a common cause of death amongst careless
  players. This one doesn't bounce, but creates a flaming inferno at impact.
  Aim well in a corridor! Another 20p penalty.

Dynamite
  Mostly used when the objective is to blow up hazardous things, it can also be
  used to good effect against tough adversaries. Once placed, it explodes after
  three seconds. A 5p penalty for each.

Proximity mine
  Place one, and after two seconds it'll arm and detonate when anyone - yes,
  that includes you! - wanders too close. Frequently used when dog-fighting,
  this baby has caused more player deaths than any other weapon. Of course, it
  wasn't always the intended target that was the victim! When not dog-fighting
  you would do well to place mines where you can see them clearly!
  A 10p penalty for each.


+----------------------------+
| Frequently Asked Questions |
+----------------------------+

Q. How can you have a FAQ for a new game?
A. These are common questions for the old Cyberdogs game, and some new ones
   I believe may pop up.

Q. Are there any cheat codes?
A. No. I don't need them so I haven't added them.
   If you find it too hard, you are able to modify both your own health and 
   that of the opponents in the Options menu.

Q. Can I have the source? Please?
A. No. [Ed. note: Ronny has released the source now... yay!]

Q. I want more powerful weaponry!
A. Now where's the fun in that? I have tried to create a balanced set of
   weapons, each with it's own strong and weak points.

Q. What about network/modem play?
A. There are no plans for either one. I do not have the know-how, nor do I have
   access to a network or modem on which to test it if I did.
   Currently, I have more interesting things do than learn these things...

Q. How do these joystick buttons work anyway?
A. Well, the standard PC joystick interface allows for two joysticks with two
   buttons each. Period. Joysticks which allow for more than that usually makes
   partial use of the second joystick for it's extra buttons, throttle wheels
   and other gadgets. Then there's the new breed of digital toys, such as the
   Gravis GRiP pads and the Sidewinder pad. They need special drivers to
   operate and are not supported by C-Dogs. To summarize: when playing C-Dogs
   with two joysticks/gamepads you have two buttons each, no more. Any
   additional buttons may act in various ways depending on your pad, eg the
   standard Gravis Gamepad has four buttons where buttons 3 and 4 are really
   buttons 1 and 2 of the other joystick.

Q. Why doesn't my Sidewinder pad work?
A. The Sidewinder pad needs special drivers to operate, and may only be used
   with Windows 95 games.

Q. How come my GRiP pads don't work?
A. You will need to put them in "normal" mode, not GRiP mode for them to work
   with games that only support standard joysticks, such as C-Dogs.

Q. How does this editor work anyway?
A. Read the FAQ on the editor at http://www.orcsoftware.com/~ronny

Q. I die instantly whenever I try to play!!
A. Yes, that's a bug that crops up sometimes when the game is first installed.
   Go into Options, change player HP and then change it back. 
   You only have to do this once.


+------------------------+
| Background information |
+------------------------+

Who am I?

My name is Ronny Wester. I am, at the time of writing, 31 years old (born 1969).
I live in Stockholm, the capital of Sweden where I was born and raised. I have
been involved with computers since I was 14 or thereabouts when I got my
first programmable calculator. I made all sorts of games for it and soon after
got myself a ZX Spectrum.

(I played Atic Atac a LOT...when I got my hands on a
Speccy emulator some time back I was able to escape the castle in 17 minutes
with the loss of only one life, using the keyboard. Pretty good considering I
hadn't played the game for, oh, six or seven years or so! The veterans among
you probably recognize the confusion bombs as a homage of sorts to Sabre Wulfe).

I studied science (physics, math, chemistry etc) as well as computers and went
on to study computer science and engineering at the Royal Institute of
Technology in Stockholm, getting a Masters in December 1990. For the ones
familiar with the Swedish school system who are puzzled by the years, they are
correct - I finished comprehensive school in seven years.

I wrote a small strategy/RPG game involving robots on my mothers Mac.

After moving out I got myself first an Atari ST and later an Atari TT - that
illfated beast of a machine. It was later sold to be used as scrap parts. On the
ST - and later the TT - I wrote some games, most notable Magus which was later
ported to the PC at the insistant request of some of my friends.

I started to work on the company where I still work today, Research & Trade.
I worked with PCs, in DOS, using Pascal or C. Later I did some work in OS/2,
but nothing came of it. Later we switched to Windows, now exclusively in Pascal.
When we finally decided that we could no longer put up with the environment we
made the decision to go to NextStep and we have yet to regret it. The language
used now is Objective C and what a language it is. How C++ got to be the bigger
of the two I will not understand.

I got myself a PC, a 486 with Localbus. As a benchmark of sorts, I decided to
try to quickly port an old abandoned 8-way scroller from the TT. I did a quick
shoddy job using Turbo Pascal and the SPX library (by Scott Ramsay). It turned
out pretty well. So well, in fact, that I kept adding to it and eventually it
turned into a pretty good freeware game, Cyberdogs. However, the basic design
flaws and the limitations of Turbo Pascal's 16-bit protected mode soon made
further development a nightmare. So the project was closed.

Tired of 16-bit limitations I got myself Watcom C. As I was playing around
with it, the idea of doing a GOOD job of Cyberdogs, with a proper design, bobbed
around in the back of my head and I deemed it a suitable introduction to the
new world of 32-bit programming. It got out of hand this time too and eventually
it turned into the game you now have before you. Hope you enjoy it!

When I'm not programming I practice aikido - Aikikai shodan for the initiates,
read books - SF&F mostly - and listen to music. Favorite artists include - but
are not limited to - Enya, Vangelis, Loreena McKennit, Eric Serra, Jean-Michel
Jarre and Vanessa Mae.

And, of course, I play computer games. Some of my favorite games are:
'Atic Atac' and 'Lords of Midnight' on the Speccy, Projectyle and Speedball
(the first one more than the second) on the ST. Some of the best games ever on
the PC are, IMHO of course, Tie Fighter, Dark Forces & Tomb Raider.

I would also like to thank everyone who has sent me email, postcards or letters
about Cyberdogs. I appreciate it. Unfortunately I am not able to reply to
all letters I receive, but I do read them all. I try to reply to all emails I
receive, but sometimes my replies bounce.

        Ronny Wester
        Stockholm, January 15th 2001

Email:            ronny@orcsoftware.com
www:              http://www.orcsoftware.com/~ronny
