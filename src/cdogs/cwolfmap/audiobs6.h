/*
BStone: A Source port of
Blake Stone: Aliens of Gold and Blake Stone: Planet Strike

Copyright (c) 1992-2013 Apogee Entertainment, LLC
Copyright (c) 2013-2015 Boris I. Bendovsky (bibendovsky@hotmail.com)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the
Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef BSTONE_AUDIO_INCLUDED
#define BSTONE_AUDIO_INCLUDED

//
// Base offsets
//
#define STARTPCSOUNDS 0
#define STARTADLIBSOUNDS 100
#define STARTDIGISOUNDS 200
#define STARTMUSIC 300

const int16_t NUMSOUNDS = 100;

//
// Sound names & indexes
//
const int16_t HITWALLSND = 0;
const int16_t TERM_TYPESND = 1;
const int16_t GETPISTOLSND = 2;
const int16_t LIQUIDDIESND = 3;
const int16_t MOVEGUN2SND = 4;
const int16_t MOVEGUN1SND = 5;
const int16_t NOWAYSND = 6;
const int16_t SCOUT_ALERTSND = 7;
const int16_t GURNEYSND = 8;
const int16_t PLAYERDEATHSND = 9;
const int16_t CONCESSIONSSND = 10;
const int16_t ATKIONCANNONSND = 11;
const int16_t GETKEYSND = 12;
const int16_t WARPOUTSND = 13;
const int16_t WARPINSND = 14;
const int16_t ROBOT_SERVOSND = 15;
const int16_t INFORMANTDEATHSND = 16;
const int16_t GOLDSTERNHALTSND = 17;
const int16_t OPENDOORSND = 18;
const int16_t CLOSEDOORSND = 19;
const int16_t GETDETONATORSND = 20;
const int16_t HALTSND = 21;
const int16_t RENTDEATH2SND = 22;
const int16_t ATKAUTOCHARGESND = 23;
const int16_t ATKCHARGEDSND = 24;
const int16_t RADAR_POWERUPSND = 25;
const int16_t ATKBURSTRIFLESND = 26;
const int16_t VITAL_GONESND = 27;
const int16_t SHOOTDOORSND = 28;
const int16_t RENTDEATH1SND = 29;
const int16_t GETBURSTRIFLESND = 30;
const int16_t GETAMMOSND = 31;
const int16_t SHOOTSND = 32;
const int16_t HEALTH1SND = 33;
const int16_t HEALTH2SND = 34;
const int16_t BONUS1SND = 35;
const int16_t BONUS2SND = 36;
const int16_t BONUS3SND = 37;
const int16_t GETIONCANNONSND = 38;
const int16_t ESCPRESSEDSND = 39;
const int16_t ELECAPPEARSND = 40;
const int16_t EXTRA_MANSND = 41;
const int16_t ELEV_BUTTONSND = 42;
const int16_t INTERROGATESND = 43;
const int16_t BONUS5SND = 44;
const int16_t BONUS4SND = 45;
const int16_t PUSHWALLSND = 46;
const int16_t TERM_BEEPSND = 47;
const int16_t ROLL_SCORESND = 48;
const int16_t TURRETSND = 49;
const int16_t EXPLODE1SND = 50;
const int16_t __UNUSED_51__SND = 51;
const int16_t SWATDIESND = 52;
const int16_t GGUARDHALTSND = 53;
const int16_t EXPLODE2SND = 54;
const int16_t BLUEBOYHALTSND = 55;
const int16_t PROGUARDDEATHSND = 56;
const int16_t DOGBOYHALTSND = 57;
const int16_t ENGINE_THRUSTSND = 58;
const int16_t SCANHALTSND = 59;
const int16_t GETCANNONSND = 60;
const int16_t LCANHALTSND = 61;
const int16_t PROHALTSND = 62;
const int16_t GGUARDDEATHSND = 63;
const int16_t BLUEBOYDEATHSND = 64;
const int16_t GOLDSTERNLAUGHSND = 65;
const int16_t SCIENTISTHALTSND = 66;
const int16_t SCIENTISTDEATHSND = 67;
const int16_t DOGBOYDEATHSND = 68;
const int16_t H_BEATSND = 69;
const int16_t SWATHALTSND = 70;
const int16_t SCANDEATHSND = 71;
const int16_t LCANDEATHSND = 72;
const int16_t INFORMDEATH2SND = 73;
const int16_t INFORMDEATH3SND = 74;
const int16_t GURNEYDEATHSND = 75;
const int16_t PRODEATH2SND = 76;
const int16_t PRODEATH3SND = 77; // AOG
const int16_t SWATDEATH2SND = 78;
const int16_t LCANBREAKSND = 79;
const int16_t SCANBREAKSND = 80;
const int16_t HTECHDOOROPENSND = 81;
const int16_t HTECHDOORCLOSESND = 82;
const int16_t ELECARCDAMAGESND = 83;
const int16_t PODHATCHSND = 84;
const int16_t ELECTSHOTSND = 85;
const int16_t ELECDIESND = 86;
const int16_t ATKGRENADESND = 87;
const int16_t CLAWATTACKSND = 88;
const int16_t PUNCHATTACKSND = 89;
const int16_t SPITATTACKSND = 90;
const int16_t PODDEATHSND = 91;
const int16_t PODHALTSND = 92;
const int16_t SWATDEATH3SND = 93; // AOG
const int16_t SCIDEATH2SND = 94;
const int16_t SCIDEATH3SND = 95;
const int16_t GOTTOKENSND = 96;
const int16_t SWITCHSND = 97;
const int16_t STATS1SND = 98;
const int16_t STATS2SND = 99;
#define LASTSOUND 100

//
// Music names & indexes (AOG)
//
#define S2100A_MUS 0
#define GOLDA_MUS 1
#define APOGFNFM_MUS 2
#define DRKHALLA_MUS 3
#define FREEDOMA_MUS 4
#define GENEFUNK_MUS 5
#define TIMEA_MUS 6
#define HIDINGA_MUS 7
#define INCNRATN_MUS 8
#define JUNGLEA_MUS 9
#define LEVELA_MUS 10
#define MEETINGA_MUS 11
#define STRUTA_MUS 12
#define RACSHUFL_MUS 13
#define RUMBAA_MUS 14
#define SEARCHNA_MUS 15
#define THEME_MUS 16
#define THEWAYA_MUS 17
#define INTRIGEA_MUS 18
#define LASTMUSIC 19

//
// Music names & indexes (PS)
//
#define CATACOMB_MUS 0
#define STICKS_MUS 1
#define PLOT_MUS 3
#define CIRCLES_MUS 4
#define LASTLAFF_MUS 5
#define TOHELL_MUS 6
#define FORTRESS_MUS 7
#define GIVING_MUS 8
#define HARTBEAT_MUS 9
#define LURKING_MUS 10
#define MAJMIN_MUS 11
#define VACCINAP_MUS 12
#define DARKNESS_MUS 13
#define MONASTRY_MUS 14
#define TOMBP_MUS 15
#define TIME_MUS 16
#define MOURNING_MUS 17
#define SERPENT_MUS 18
#define HISCORE_MUS 19

#endif // BSTONE_AUDIO_INCLUDED
