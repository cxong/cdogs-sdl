/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (C) 1995 Ronny Webster
    Copyright (C) 2003 Jeremy Chin 
    Copyright (C) 2003 Lucas Martin-King 

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

-------------------------------------------------------------------------------

 sounds.h - <description here>
 
 Author: $Author$
 Rev:    $Revision$
 URL:    $HeadURL$
 ID:     $Id$
 
*/

#define SND_EXPLOSION   0
#define SND_LAUNCH      1
#define SND_MACHINEGUN  2
#define SND_FLAMER      3
#define SND_SHOTGUN     4
#define SND_POWERGUN    5
#define SND_SWITCH      6
#define SND_KILL        7
#define SND_KILL2       8
#define SND_KILL3       9
#define SND_KILL4      10
#define SND_HAHAHA     11
#define SND_BANG       12
#define SND_PICKUP     13
#define SND_DOOR       14
#define SND_DONE       15
#define SND_LASER      16
#define SND_MINIGUN    17
#define SND_COUNT      18

#define FX_MAXCHANNELS  8

#define SND_QUALITYMODE 1
#define SND_USE486      2

#define MODULE_OK       0
#define MODULE_NOLOAD   1
#define	MODULE_PLAYING	2
#define MODULE_PAUSED	3
#define MODULE_STOPPED	MODULE_OK

int InitializeSound(void);
void ShutDownSound(void);
int PlaySong(char *name);
void PlaySound(int sound, int panning, int volume);
void DoSounds(void);
void SetFXVolume(int volume);
int FXVolume(void);
void SetMusicVolume(int volume);
int MusicVolume(void);
void SetLeftEar(int x, int y);
void SetRightEar(int x, int y);
void PlaySoundAt(int x, int y, int sound);
void SetFXChannels(int channels);
int FXChannels(void);
void SetMinMusicChannels(int channels);
int MinMusicChannels(void);
void ToggleTrack(int track);
int ModuleStatus(void);
const char *ModuleMessage(void);
void SetModuleDirectory(const char *dir);
const char *ModuleDirectory(void);

#define SoundTick		( #error "Bad!" )
#define InterruptOn		( #error "Bad!" )
#define InterruptOff		( #error "Bad!" )
#define SetDynamicInterrupts	( #error "Bad!" )
#define DynamicInterrupts	( #error "Bad!" )
//void SoundTick(void); 
//void InterruptOn(void);
//void InterruptOff(void);
//void SetDynamicInterrupts(int flag);
//int DynamicInterrupts(void);
