/*
	ogre.inc - Ogre Missions
	(C)opyright Ronny Wester.

	NOT covered by the GPL... ie: this is freeware
 
	Author: $Author$
	Rev:    $Revision$
	URL:    $HeadURL$
	ID:     $Id$
	
*/

TBadGuy OGRE_badguys[42] = {
	{1, 0, 2, 256, 50, 25, 2, 15, 1, 3, 9, 9, 9, 0, 40, 0x400}
	,
	{1, 0, 2, 256, 70, 35, 0, 10, 0, 6, 9, 9, 9, 1, 40, 0x400}
	,
	{1, 0, 2, 256, 55, 30, 20, 15, 1, 0, 9, 9, 9, 2, 40, 0x400}
	,
	{1, 0, 2, 256, 65, 40, 40, 10, 3, 6, 9, 9, 9, 3, 40, 0x400}
	,
	{1, 0, 2, 256, 70, 45, 50, 10, 4, 4, 9, 9, 9, 4, 40, 0x400}
	,
	{1, 0, 2, 256, 50, 25, 20, 10, 15, 3, 9, 9, 9, 5, 40, 0x400}
	,
	{1, 0, 2, 256, 70, 50, 15, 10, 11, 5, 9, 9, 9, 6, 40, 0x400}
	,
	{1, 0, 2, 256, 70, 50, 10, 10, 2, 8, 9, 9, 9, 7, 40, 0x400}
	,
	{1, 0, 2, 128, 95, 75, 30, 10, 4, 2, 9, 9, 9, 8, 80, 0x400}
	,
	{1, 0, 2, 384, 95, 75, 0, 10, 0, 9, 9, 9, 9, 9, 40, 0x400}
	,
	{1, 0, 2, 128, 95, 75, 50, 10, 1, 2, 9, 9, 9, 10, 80, 0x400}
	,
	{1, 0, 7, 256, 95, 50, 10, 5, 1, 3, 9, 5, 9, 11, 120, 0x400}
	,
	{1, 0, 7, 256, 95, 50, 20, 5, 15, 0, 9, 0, 9, 12, 120, 0x400}
	,
	{1, 0, 7, 256, 95, 75, 20, 5, 4, 8, 9, 2, 9, 13, 120, 0x400}
	,
	{1, 0, 7, 256, 50, 25, 30, 10, 8, 4, 9, 5, 9, 6, 120, 0x400}
	,
	{1, 0, 6, 256, 100, 0, 0, 10, 14, 10, 7, 7, 9, 12, 20, 0x600000}
	,
	{1, 0, 6, 256, 100, 0, 0, 10, 14, 1, 7, 8, 11, 10, 20, 0x600000}
	,
	{1, 0, 6, 256, 100, 0, 0, 10, 14, 11, 3, 3, 0, 12, 20, 0x600000}
	,
	{1, 0, 12, 256, 100, 0, 0, 10, 14, 1, 8, 10, 2, 8, 20, 0x600000}
	,
	{1, 0, 4, 256, 50, 0, 5, 10, 5, 11, 0, 0, 0, 8, 40, 0x20000}
	,
	{1, 0, 4, 256, 50, 0, 15, 10, 3, 1, 0, 0, 0, 8, 40, 0x20000}
	,
	{1, 0, 4, 256, 50, 0, 5, 10, 1, 10, 0, 0, 0, 8, 40, 0x20000}
	,
	{1, 0, 2, 256, 100, 0, 0, 10, 14, 10, 1, 5, 2, 12, 20, 0x600000}
	,
	{1, 0, 2, 256, 75, 0, 0, 15, 14, 3, 1, 0, 4, 12, 20, 0x600000}
	,
	{1, 0, 2, 256, 75, 0, 0, 15, 14, 6, 4, 3, 8, 12, 20, 0x600000}
	,
	{1, 0, 2, 256, 75, 0, 0, 15, 14, 9, 8, 7, 11, 12, 20, 0x600000}
	,
	{1, 0, 2, 256, 75, 0, 0, 15, 14, 11, 6, 8, 3, 11, 20, 0x600000}
	,
	{1, 0, 7, 256, 50, 25, 30, 5, 4, 5, 9, 1, 9, 1, 120, 0x400}
	,
	{1, 0, 16, 256, 100, 0, 0, 15, 14, 1, 1, 6, 4, 13, 20, 0x600000}
	,
	{1, 0, 7, 256, 100, 50, 0, 5, 14, 6, 9, 4, 9, 3, 400, 0x140400}
	,
	{1, 0, 2, 256, 75, 25, 52, 5, 4, 8, 12, 8, 9, 12, 80, 0x800400}
	,
	{1, 0, 2, 256, 75, 25, 52, 5, 15, 8, 12, 8, 9, 12, 80, 0x800400}
	,
	{1, 0, 2, 256, 75, 25, 52, 5, 3, 8, 12, 8, 9, 12, 80, 0x800400}
	,
	{1, 0, 2, 256, 70, 5, 10, 15, 4, 5, 6, 6, 6, 12, 80, 0x30400}
	,
	{1, 0, 2, 256, 50, 5, 15, 15, 5, 0, 6, 6, 6, 12, 80, 0x30400}
	,
	{1, 0, 2, 256, 50, 5, 32, 15, 8, 8, 6, 6, 6, 12, 80, 0x30400}
	,
	{1, 0, 7, 256, 80, 5, 0, 10, 0, 9, 6, 6, 6, 6, 120, 0x30400}
	,
	{1, 0, 7, 256, 75, 50, 10, 5, 1, 9, 9, 3, 9, 0, 200, 0x800600}
	,
	{1, 0, 7, 256, 75, 50, 10, 5, 5, 9, 9, 3, 9, 5, 200, 0x800600}
	,
	{1, 0, 7, 256, 75, 50, 10, 5, 8, 9, 9, 3, 9, 7, 200, 0x800600}
	,
	{1, 0, 7, 256, 75, 50, 10, 5, 3, 9, 9, 3, 9, 6, 200, 0x800600}
	,
	{1, 0, 7, 256, 75, 50, 10, 5, 4, 9, 9, 3, 9, 13, 200, 0x800600}
};

struct Mission OGRE_missions[16] = {
	{
	 "Maintain integrity",
	 "We were taken completely by surprise and Ogre troops have overrun severa"
	 "l installations. We are falling back from this one right now. We will ne"
	 "ed to destroy several computer terminals lest they, and the information "
	 "they contain, fall into enemy hands.",
	 0, 1, 10, 0, 0, 0, 64, 32, 50, 20, 10, 2,
	 0, 0, 0, 0,
	 1,
	 {
	  {
	   "Destroy all terminals",
	   2, 26, 5, 5, 0x6},
	  {
	   "",
	   0, 0, 0, 0, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0}
	  },
	 9,
	 {0, 1, 2, 21, 20, 19, 0, 1, 2, 0, 0, 0},
	 0,
	 {0, 0, 0, 0, 0, 0},
	 8,
	 {0, 1, 2, 3, 4, 5, 6, 7, 0, 0, 0, 0, 0, 0, 0, 0},
	 {30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 0, 0, 0, 0, 0, 0},
	 20, 0x53b,
	 "",
	 "",
	 13, 16, 24, 25},
	{
	 "Retrieve the stolen data",
	 "Intelligence reports that the Ogres managed to backup some of the data b"
	 "efore you could get to the terminals. However, troops have intercepted a"
	 "nd are now containing the Ogre forces in possession of the data. It's a "
	 "stalemate at the moment. You will have to resolve the situation...",
	 0, 1, 8, 0, 0, 0, 64, 32, 50, 20, 10, 0,
	 0, 0, 0, 0,
	 1,
	 {
	  {
	   "Get the disks back",
	   1, 1, 5, 5, 0x4},
	  {
	   "",
	   0, 0, 0, 0, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0}
	  },
	 5,
	 {0, 1, 2, 3, 21, 0, 0, 0, 0, 0, 0, 0},
	 0,
	 {0, 0, 0, 0, 0, 0},
	 8,
	 {0, 1, 2, 3, 4, 5, 6, 7, 0, 0, 0, 0, 0, 0, 0, 0},
	 {30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 0, 0, 0, 0, 0, 0},
	 20, 0x13b,
	 "",
	 "",
	 13, 4, 7, 0},
	{
	 "Intelligence gathering",
	 "This whole situation is decidedly odd. The Ogres have never been hostile"
	 " towards the Confed before. Besides, these forces are far from the best "
	 "of what the Ogre Empire can bring forth. We really need to find out what"
	 "'s going on. See what you can dig up.",
	 0, 8, 3, 0, 0, 0, 64, 32, 30, 25, 15, 0,
	 0, 0, 0, 0,
	 1,
	 {
	  {
	   "Find some documents",
	   1, 11, 2, 2, 0x4},
	  {
	   "",
	   0, 0, 0, 0, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0}
	  },
	 6,
	 {0, 1, 2, 3, 4, 19, 0, 0, 0, 0, 0, 0},
	 0,
	 {0, 0, 0, 0, 0, 0},
	 8,
	 {0, 1, 2, 3, 4, 5, 6, 7, 0, 0, 0, 0, 0, 0, 0, 0},
	 {30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 0, 0, 0, 0, 0, 0},
	 20, 0x13b,
	 "",
	 "",
	 6, 14, 4, 0},
	{
	 "Search'n'destroy",
	 "Intelligence are going through the data you brought back. Now I need you"
	 " for something different. A particulary aggressive Ogre commander are gi"
	 "ving us a lot of trouble in this area. We have established his current w"
	 "hereabouts, a nearby Ogre outpost. We need you to take him out.",
	 4, 6, 7, 1, 1, 1, 48, 48, 50, 20, 5, 3,
	 0, 0, 0, 0,
	 1,
	 {
	  {
	   "Kill Miihn-Bha'Stadh",
	   0, 0, 1, 1, 0x1},
	  {
	   "",
	   0, 0, 0, 0, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0}
	  },
	 7,
	 {0, 1, 2, 3, 4, 5, 6, 0, 0, 0, 0, 0},
	 1,
	 {12, 0, 0, 0, 0, 0},
	 8,
	 {8, 9, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0},
	 {30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 0, 0, 0, 0, 0, 0},
	 23, 0x0,
	 "",
	 "",
	 7, 14, 22, 0},
	{
	 "Gather more information",
	 "Intelligence has been all over the data you brought back. Something's no"
	 "t right here and we want to know what's up. Unfortunately, your data - w"
	 "hile most valuable - leaves a lot of blanks to be filled. We need more d"
	 "ata and you will have to get it for us.",
	 0, 8, 1, 0, 0, 0, 64, 64, 50, 50, 15, 2,
	 0, 0, 0, 0,
	 2,
	 {
	  {
	   "Steal Ogre documents",
	   1, 0, 2, 2, 0x4},
	  {
	   "...and any other data you can find",
	   1, 1, 7, 3, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0}
	  },
	 7,
	 {1, 2, 3, 4, 5, 21, 20, 20, 0, 0, 0, 0},
	 0,
	 {0, 0, 0, 0, 0, 0},
	 8,
	 {0, 1, 2, 3, 4, 5, 6, 7, 0, 0, 0, 0, 0, 0, 0, 0},
	 {30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 0, 0, 0, 0, 0, 0},
	 21, 0x13b,
	 "",
	 "",
	 4, 13, 6, 0},
	{
	 "Locate the Ogre chemical weapons facility",
	 "The bio-weapons they are using are causing severe causalties amongst our"
	 " troops. Our scientists are unable to come up with an antidote. They do "
	 "tell us the stuff is rather unstable and cannot be stored for long. They"
	 " must be manufacturing it in the vicinity. Try and find out where it is."
	 "",
	 1, 2, 10, 1, 1, 1, 64, 64, 40, 10, 15, 2,
	 0, 0, 0, 0,
	 1,
	 {
	  {
	   "Find the starmaps",
	   1, 4, 3, 3, 0x4},
	  {
	   "",
	   0, 0, 0, 0, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0}
	  },
	 6,
	 {0, 1, 2, 3, 5, 6, 6, 0, 0, 0, 0, 0},
	 0,
	 {0, 0, 0, 0, 0, 0},
	 8,
	 {8, 9, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0},
	 {30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 0, 0, 0, 0, 0, 0},
	 25, 0x0,
	 "",
	 "",
	 2, 4, 6, 7},
	{
	 "Wreck the Ogre laboratories",
	 "The data you brought back has provided us with the location of the Ogres"
	 " bio-chemical weapons manufacturing plant. If you can disable this facil"
	 "ity you will be saving many lives and help our efforts to restore piece "
	 "to this region again.",
	 4, 2, 7, 1, 1, 1, 64, 64, 25, 25, 20, 2,
	 0, 0, 0, 0,
	 1,
	 {
	  {
	   "Destroy all laboratory equipment",
	   2, 25, 7, 7, 0x4},
	  {
	   "",
	   0, 0, 0, 0, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0}
	  },
	 7,
	 {0, 1, 2, 3, 5, 6, 7, 7, 0, 0, 0, 0},
	 0,
	 {0, 0, 0, 0, 0, 0},
	 8,
	 {8, 9, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0},
	 {30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 0, 0, 0, 0, 0, 0},
	 20, 0x0,
	 "",
	 "",
	 14, 20, 23, 0},
	{
	 "Destroy their ammunition",
	 "The destruction of the bio-weapons lab was a substantial setback for the"
	 " Ogre forces. Now, as they are forced to retreat, a major ammunitions de"
	 "pot has had most of its garrison transferred elsewhere. It is still heav"
	 "ily guarded, but it is too good a target to pass up.",
	 1, 6, 10, 1, 1, 1, 64, 64, 50, 25, 10, 3,
	 0, 0, 0, 0,
	 2,
	 {
	  {
	   "Destroy ammo containers",
	   2, 22, 12, 7, 0x0},
	  {
	   "Destroy fuel barrels",
	   2, 27, 15, 10, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0}
	  },
	 5,
	 {0, 2, 3, 4, 1, 10, 0, 0, 0, 0, 0, 0},
	 0,
	 {0, 0, 0, 0, 0, 0},
	 8,
	 {8, 9, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0},
	 {30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 0, 0, 0, 0, 0, 0},
	 25, 0x0,
	 "",
	 "",
	 7, 13, 12, 0},
	{
	 "Retake installation",
	 "Your efforts have helped weaken the Ogre forces in the region and we are"
	 " now recapturing our lost installations. At this very moment our troops "
	 "are launching an assault. You are assigned the task of seeking out and e"
	 "liminating the Ogre commanders. With them out of the picture, the rest s"
	 "hould be an easy task.",
	 0, 5, 3, 0, 0, 0, 64, 64, 50, 50, 10, 0,
	 0, 0, 0, 0,
	 1,
	 {
	  {
	   "Take out as many squad leaders as possible",
	   0, 0, 5, 3, 0x1},
	  {
	   "",
	   0, 0, 0, 0, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0}
	  },
	 10,
	 {7, 9, 10, 0, 2, 3, 4, 21, 19, 20, 0, 0},
	 2,
	 {11, 13, 0, 0, 0, 0},
	 8,
	 {0, 1, 2, 3, 4, 5, 6, 7, 0, 0, 0, 0, 0, 0, 0, 0},
	 {30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 0, 0, 0, 0, 0, 0},
	 30, 0x0,
	 "",
	 "",
	 16, 26, 19, 0},
	{
	 "Counter strike",
	 "We have successfully recaptured all installations previously held by Ogr"
	 "e invaders. We are now on the offensive and our goal is now primarily to"
	 " cripple the Ogre war machinery in this area. We have data on an Ogre de"
	 "pot that has been stripped of most of its defences...",
	 1, 2, 7, 1, 1, 1, 48, 48, 25, 10, 15, 2,
	 0, 0, 0, 0,
	 4,
	 {
	  {
	   "Eliminate enemy officers",
	   0, 0, 5, 1, 0x1},
	  {
	   "Blow up their gunpowder",
	   2, 8, 10, 5, 0x0},
	  {
	   "Burn their fuel",
	   2, 27, 8, 4, 0x0},
	  {
	   "Destroy their ammunition",
	   2, 22, 12, 6, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0}
	  },
	 5,
	 {0, 1, 2, 3, 9, 0, 0, 0, 0, 0, 0, 0},
	 3,
	 {13, 12, 11, 0, 0, 0},
	 8,
	 {9, 10, 11, 12, 13, 14, 15, 17, 31, 0, 0, 0, 0, 0, 0, 0},
	 {15, 15, 15, 5, 20, 30, 10, 20, 0, 0, 0, 0, 0, 0, 0, 0},
	 25, 0x0,
	 "",
	 "",
	 2, 22, 21, 0},
	{
	 "Wet work",
	 "We now have the opportunity to strike a devastating blow to the Ogre war"
	 " machinery! Intelligence has uncovered the current location of Ogre high"
	 " command of this sector. The area has a high civilian presence, I will r"
	 "emind you that we do not wage war on civilians!",
	 4, 6, 10, 1, 1, 1, 54, 54, 25, 25, 20, 1,
	 0, 0, 0, 0,
	 1,
	 {
	  {
	   "Assassinate Ogre high command",
	   0, 0, 4, 4, 0x1},
	  {
	   "",
	   0, 0, 0, 0, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0}
	  },
	 9,
	 {1, 2, 3, 26, 25, 24, 23, 22, 0, 9, 0, 0},
	 2,
	 {27, 14, 0, 0, 0, 0},
	 9,
	 {9, 10, 12, 13, 14, 15, 17, 18, 20, 0, 0, 0, 0, 0, 0, 0},
	 {20, 15, 10, 10, 20, 10, 10, 15, 10, 0, 0, 0, 0, 0, 0, 0},
	 30, 0x0,
	 "",
	 "",
	 16, 12, 5, 4},
	{
	 "Terror attack!",
	 "In retaliation for our attack against Ogre high command, Ogre forces are"
	 " attacking a housing complex for the civilian workers who are repairing "
	 "damages caused by this conflict. This is a breach of all protocols and t"
	 "horoughly dishonorable and detestful act! Repel the attack!",
	 5, 5, 1, 0, 0, 0, 64, 32, 0, 0, 20, 0,
	 0, 0, 0, 0,
	 1,
	 {
	  {
	   "Eliminate all attackers",
	   0, 0, 40, 40, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0}
	  },
	 5,
	 {18, 17, 16, 15, 28, 28, 19, 0, 0, 0, 0, 0},
	 5,
	 {4, 3, 2, 7, 9, 9},
	 11,
	 {0, 1, 2, 4, 3, 5, 6, 16, 19, 23, 26, 0, 0, 0, 0, 0},
	 {15, 15, 15, 15, 15, 15, 10, 10, 10, 5, 20, 0, 0, 0, 0, 0},
	 20, 0x13b,
	 "",
	 "",
	 13, 25, 10, 0},
	{
	 "The defector",
	 "We have learned that a senior Ogre officer wishes to defect. Faced with "
	 "the same questions we have been contemplating all along he did some rese"
	 "arch and believes this campaign to be an unauthorized action by a renega"
	 "de faction within the Ogre fleet! This is a matter of crucial importance"
	 "!",
	 4, 2, 7, 1, 1, 1, 48, 48, 15, 15, 15, 1,
	 0, 0, 0, 0,
	 1,
	 {
	  {
	   "Get the officer here safely",
	   3, 29, 1, 1, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0}
	  },
	 5,
	 {2, 3, 4, 10, 0, 0, 0, 0, 0, 0, 0, 0},
	 0,
	 {0, 0, 0, 0, 0, 0},
	 11,
	 {2, 1, 8, 9, 11, 15, 12, 10, 14, 13, 24, 0, 0, 0, 0, 0},
	 {15, 15, 5, 15, 10, 20, 5, 15, 15, 5, 5, 0, 0, 0, 0, 0},
	 15, 0x0,
	 "",
	 "",
	 14, 12, 20, 0},
	{
	 "Kidnapping attempt!",
	 "A party of Ogre Elite Warriors from the renegade faction is attempting t"
	 "o recapture the defecting officer. Apparently the information he holds c"
	 "an give them very big problems with their masters back home. You must fo"
	 "il their plans!",
	 0, 1, 3, 0, 0, 0, 48, 32, 15, 14, 10, 1,
	 0, 0, 0, 0,
	 1,
	 {
	  {
	   "Defeat the Ogre assault team",
	   0, 0, 12, 12, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0}
	  },
	 3,
	 {21, 20, 19, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	 3,
	 {32, 31, 30, 13, 11, 0},
	 12,
	 {0, 3, 4, 5, 6, 16, 17, 19, 26, 23, 1, 2, 0, 0, 0, 0},
	 {20, 20, 10, 10, 5, 10, 25, 10, 25, 5, 10, 10, 0, 0, 0, 0},
	 10, 0x0,
	 "",
	 "",
	 16, 10, 19, 0},
	{
	 "The Enemy Stronghold",
	 "Officer Naijz-Gaij has been in contact with his masters, who were more t"
	 "han slightly upset. As you've noticed Ogre Imperial forces have been arr"
	 "iving all day and are engaging the hostiles as we speak. We are now moun"
	 "ting a joint attack against the enemy headquarters. Ogre Imperials, wear"
	 "ing red body armor, are your allies in this venture.",
	 4, 6, 10, 1, 2, 2, 48, 48, 15, 30, 12, 0,
	 0, 0, 0, 0,
	 2,
	 {
	  {
	   "Eliminate all resistance",
	   0, 0, 30, 30, 0x0},
	  {
	   "Find the controls to the inner defenses",
	   1, 9, 1, 1, 0x4},
	  {
	   "",
	   0, 0, 0, 0, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0}
	  },
	 11,
	 {36, 35, 34, 33, 21, 20, 19, 23, 25, 26, 24, 0},
	 6,
	 {4, 32, 31, 9, 10, 6},
	 14,
	 {8, 9, 10, 12, 13, 11, 14, 17, 22, 24, 25, 31, 23, 27, 0, 0},
	 {10, 20, 10, 5, 15, 10, 15, 10, 5, 5, 5, 5, 5, 10, 0, 0},
	 10, 0x0,
	 "",
	 "",
	 14, 2, 0, 1},
	{
	 "The last stand of the renegades",
	 "Good work, you are truly an honor to this outfit! Now ready yourselves f"
	 "or action once more as we face the remains of the renegade forces. The r"
	 "enegade commanders are cornered and will not yield. Our Ogre liasons tel"
	 "l us these are some mighty heavy dudes. They must be crushed like the ve"
	 "rmin they are, or they will surely rise again...",
	 5, 7, 6, 1, 2, 2, 48, 48, 40, 15, 14, 0,
	 0, 0, 0, 0,
	 1,
	 {
	  {
	   "Annihilate the renegade commanders",
	   0, 0, 15, 15, 0x1},
	  {
	   "",
	   0, 0, 0, 0, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0},
	  {
	   "",
	   0, 0, 0, 0, 0x0}
	  },
	 7,
	 {36, 35, 34, 33, 21, 19, 20, 0, 0, 0, 0, 0},
	 5,
	 {41, 37, 38, 39, 40, 14},
	 12,
	 {8, 9, 10, 11, 17, 22, 27, 25, 24, 32, 30, 31, 0, 0, 0, 0},
	 {15, 10, 10, 15, 10, 5, 15, 15, 10, 5, 5, 5, 0, 0, 0, 0},
	 25, 0x777,
	 "",
	 "",
	 14, 13, 5, 0},
};

struct CampaignSetting OGRE_campaign = {
	"Ogre Rampage",
	"Ronny Wester",
	"Suddenly, Confederate installations come under sudden, and most unexpected"
	    ", attack from Ogre troops. There has been no animosity between the Ogre Em"
	    "pire and the Confederation for ages. What's up?",
	16, OGRE_missions, 42, OGRE_badguys
};
