# C-Dogs SDL Makefile

### Variables ###

# Make sure only one variable in the provided set is uncommented!

# Can be: linux, macosx, xmingw or mingw32
#SYSTEM		:= macosx
#SYSTEM		:= linux
#SYSTEM		:= xmingw
#SYSTEM		:= mingw32

# Can be either: sdlmixer, nomix or oldmix
# sdlmixer is recommended, as you get proper music and sounds =)
SOUND_CODE	:= sdlmixer
#SOUND_CODE	:= nomix
#SOUND_CODE	:= oldmix

# Destdir is where the base dir where everything is installed
DESTDIR		:= ../dist
#DESTDIR 	:= /

DATA		:= ../data
#DATA		:= /path/to/where/data/is/unpacked/

# Prefix is the *actual* base directory
# Bindir is where the cdogs-sdl binary is placed
# Datadir is where the data is
PREFIX		:= /usr/local
BINDIR		:= $(PREFIX)/games/bin
DATADIR		:= $(PREFIX)/share/games/cdogs-sdl
DOCDIR		:= $(PREFIX)/share/doc/cdogs-sdl

# The following are for a self-contained install
#PREFIX		:= .
#BINDIR		:= $(PREFIX)/
#DATADIR	:= $(PREFIX)/data
#CFGDIR		:= $(PREFIX)/config
#DOCDIR		:= $(PREFIX)/doc

# intel, powerpc... uncomment only one (if at all)
#CF_ARCH 	+= -march=pentium2 -mcpu=pentium2	
#CF_ARCH  	+= -mcpu=G3 -mtune=G3
#CF_ARCH 	+= -mcpu=G4 -mtune=G4 -maltivec	

CF_OPTIMISE	+= -O2

CC		:= gcc
STRIP		:= strip

# Set to 1 for debugging
DEBUG		:= 0
DEBUG_PROFILE	:= 0

### Change this to yes, once you are ready
I_AM_CONFIGURED	:= no

# Name of the cdogs-sdl binary (probably doesn't need to be changed)
CDOGS		:= cdogs-sdl
CDOGSED		:= cdogs-sdl-editor

# Suffix of the executable
PROG_SUFFIX	:=

# Use pkg-config to get library information
# currently only used when SYSTEM is linux
USE_PKGCONFIG	:= no


### Some logic to work out things

### Sound code
ifeq ($(SOUND_CODE), sdlmixer)
	DEFS	+= -DSND_SDLMIXER
endif

ifeq ($(SOUND_CODE), nomix)
	DEFS	+= -DSND_NOMIX
endif

# Just assume it's Linux, by default
ifeq ($(SYSTEM),)
	SYSTEM	:= linux
endif

# detect MacOS X
ifeq ($(shell uname -s), Darwin)
       SYSTEM := macosx
endif

### System specific parts

ifeq ($(SYSTEM), macosx)
	DEFS	+= -DSYS_MACOSX
	LDFLAGS	+= -framework SDL -framework SDL_mixer -framework AppKit -framework Foundation

	FRAMEWORK_DIR := /Library/Frameworks
	SDL_FRAMEWORK := $(FRAMEWORK_DIR)/SDL.framework
	SDLMIXER_FRAMEWORK := $(FRAMEWORK_DIR)/SDL_mixer.framework

	INCLUDES += -I$(SDL_FRAMEWORK)/Headers

	ifeq ($(SOUND_CODE), sdlmixer)
		INCLUDES += -I$(SDLMIXER_FRAMEWORK)/Headers
	endif

	EXTRA_OBJS := SDLmain.o
endif

ifeq ($(SYSTEM), linux)
	ifeq ($(USE_PKGCONFIG), yes)
		LDFLAGS	 += $(shell pkg-config --libs sdl)
		INCLUDES += $(shell pkg-config --cflags sdl)		
	else
		LDFLAGS	 += $(shell sdl-config --libs)	
		INCLUDES += $(shell sdl-config --cflags)
	endif

	ifeq ($(SOUND_CODE), sdlmixer)
		LDFLAGS	+= -lSDL_mixer
	endif
endif

ifeq ($(SYSTEM), xmingw)
	CC	:= i386-mingw32msvc-gcc
	STRIP	:= i386-mingw32msvc-strip
	WINDRES := i386-mingw32msvc-windres

	SDLCONFIG := i386-mingw32msvc-sdl-config

	LDFLAGS	 += $(shell $(SDLCONFIG) --libs)
	INCLUDES += $(shell $(SDLCONFIG) --cflags)

	DEFS	 += -DSYS_WIN

	ifeq ($(SOUND_CODE), sdlmixer)
		LDFLAGS += -lSDL_mixer
	endif

	PROG_SUFFIX	:= .exe

	EXTRA_OBJS := rc.o
endif

ifeq ($(SYSTEM), mingw32)
	CC	:= mingw32-gcc
	STRIP	:= mingw32-strip
	WINDRES := mingw32-windres

	SDLCONFIG := sdl-config

	LDFLAGS	 += $(shell $(SDLCONFIG) --libs)
	INCLUDES += $(shell $(SDLCONFIG) --cflags)

	DEFS	+= -DSYS_WIN

	ifeq ($(SOUND_CODE), sdlmixer)
		LDFLAGS += -lSDL_mixer
	endif

	PROG_SUFFIX	:= .exe

	EXTRA_OBJS := rc.o
endif

### Debugging

ifeq ($(DEBUG), 1)
	DEFS		+= -DCDOGS_DEBUG
	CF_DEBUG 	+= -ggdb -Wall

	ifeq ($(DEBUG_PROFILE), 1)
		CF_DEBUG += -pg	
		LDFLAGS  += -pg	
	endif
else
	# suppress warnings
	CF_DEBUG	+= -w
endif



### No need to edit below here ###

CFLAGS		+= $(CF_OPTIMISE) $(CF_ARCH) $(CF_DEBUG)

INCLUDES	+= -I./include -I./missions
LDFLAGS		+= 
DEFS		+= -DCDOGS_DATA_DIR=\"$(DATADIR)\"

CDOGS_OBJS = \
	cdogs.o draw.o pics.o actors.o map.o sounds.o defs.o objs.o \
	gamedata.o ai.o triggers.o input.o prep.o hiscores.o automap.o \
	mission.o game.o mainmenu.o password.o files.o menu.o joystick.o \
	sprcomp.o grafx.o blit.o text.o keyboard.o events.o utils.o \
	drawtools.o

CDOGSED_OBJS = \
	cdogsed.o draw.o pics.o actors.o map.o sounds.o defs.o objs.o \
	gamedata.o triggers.o input.o hiscores.o automap.o mission.o game.o \
	ai.o charsed.o events.o joystick.o sprcomp.o grafx.o blit.o text.o \
	keyboard.o drawtools.o utils.o

### Targets ###

.PHONY: clean install info tidy help

help:
	@echo "C-Dogs SDL Build System..."
	@echo
	@echo "REMEMBER TO EDIT THE MAKEFILE!"
	@echo
	@echo "(and change I_AM_CONFIGURED to yes)"
	@echo
	@echo "Targets:"
	@echo " * $(CDOGS) - build cdogs binary"
	@echo " * $(CDOGSED) - build cdogs editor binary"
	@echo " * install - install cdogs binary (and data)"
	@echo " * info - show configuration"
	@echo " * clean - clean tree for building"
	@echo " * help - this help"
	@echo
	@echo "Usage: make <target>"

$(CDOGS): $(CDOGS_OBJS) $(EXTRA_OBJS)
	@echo "Linking... ($(CDOGS))"
	$(CC) $(CDOGS_OBJS) $(EXTRA_OBJS) -o $(CDOGS)$(PROG_SUFFIX) $(LDFLAGS)
	@echo "Now type 'make install' as root!"

$(CDOGSED): $(CDOGSED_OBJS) $(EXTRA_OBJS)
	@echo "Linking... ($(CDOGSED))"
	@$(CC) $(CDOGSED_OBJS) $(EXTRA_OBJS) -o $(CDOGSED)$(PROG_SUFFIX) $(LDFLAGS)

.c.o:	include/*.h
	@if [ "$(I_AM_CONFIGURED)" != "yes" ] ; then \
	echo "Have you really configured the Makefile?" ; \
	echo "Change I_AM_CONFIGURED to yes" ; \
	exit 1 ; \
	fi 

	@echo "Compiling $<... (debug=$(DEBUG),profile=$(DEBUG_PROFILE))"

	$(CC) \
	 $(CFLAGS) \
	 $(INCLUDES) \
	 $(DEFS) \
	 -c $<

# This is for Mac OS X
SDLmain.o: ../build/macosx/SDLmain.m ../build/macosx/SDLmain.h
	@echo "Compiling SDLmain.m (MacOSX) (debug=$(DEBUG))"
	@$(CC) \
		$(CFLAGS) \
		$(INCLUDES) \
		-c ../build/macosx/SDLmain.m

# Windows resource
rc.o: ../build/windows/cdogs.rc ../build/windows/cdogs-icon.ico
	@echo "Compiling Windows resourcs (cdogs.rc) (debug=$(DEBUG))"
	$(WINDRES) -o rc.o -I ../build/windows/ ../build/windows/cdogs.rc

info:
	@echo "=[ Program ]="
	@echo "C-Dogs:   $(CDOGS)$(PROG_SUFFIX)"
	@echo "Editor:   $(CDOGSED)$(PROG_SUFFIX)"
	@echo ""
	@echo "=[ System ]="
	@echo "System:   $(SYSTEM)"
	@echo "Sound:    $(SOUND_CODE)"
	@echo ""
	@echo "=[ Paths ]="
	@echo "Prefix:   $(PREFIX)"
	@echo "Bin dir:  $(BINDIR)"
	@echo "Data dir: $(DATADIR)"
	@echo "Doc dir:  $(DOCDIR)"
	@echo "Dest dir: $(DESTDIR)"
	@echo ""
	@echo "=[ Compilation ]="
	@echo "CC:       $(CC)"
	@echo "CFLAGS:   $(CFLAGS)"
	@echo "INCLUDES: $(INCLUDES)"
	@echo "DEFS:     $(DEFS)"
	@echo "LDFLAGS:  $(LDFLAGS)"

install:
	DESTDIR=$(DESTDIR) \
	DOCDIR=$(DOCDIR) \
	DATADIR=$(DATADIR) \
	BINDIR=$(BINDIR) \
	PREFIX=$(PREFIX) \
	PROG=$(CDOGS)$(PROG_SUFFIX) \
	LOCALDATA=$(DATA) \
	LOCALDOCS="../doc/" \
	./install.sh

clean:
	rm -f *.o $(CDOGS)$(PROG_SUFFIX) $(CDOGSED)$(PROG_SUFFIX)
