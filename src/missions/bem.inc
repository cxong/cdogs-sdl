/* 
	bem.inc - B.E.M mission.
	(C)opyright Ronny Wester.

	NOT covered by the GPL... ie: this is freeware
 
	Author: $Author$
	Rev:    $Revision$
	URL:    $HeadURL$
	ID:     $Id$
*/

TBadGuy BEM_badguys[29] = {
	{1, 0, 4, 256, 50, 0, 15, 15, 5, 1, 3, 3, 3, 13, 40, 0x2020000}
	,
	{1, 0, 4, 256, 50, 0, 15, 15, 1, 11, 3, 3, 3, 13, 40, 0x2020000}
	,
	{1, 0, 4, 256, 50, 0, 15, 15, 3, 10, 3, 3, 3, 13, 40, 0x2020000}
	,
	{1, 0, 6, 256, 100, 50, 0, 15, 14, 1, 7, 7, 9, 13, 20, 0x610000}
	,
	{1, 0, 6, 256, 100, 50, 0, 15, 14, 11, 7, 7, 9, 12, 20, 0x610000}
	,
	{1, 0, 3, 256, 100, 50, 0, 15, 14, 10, 7, 8, 9, 8, 20, 0x610000}
	,
	{1, 0, 12, 256, 100, 50, 0, 15, 14, 1, 7, 10, 9, 9, 20, 0x610000}
	,
	{1, 0, 16, 256, 100, 50, 0, 15, 14, 1, 9, 6, 4, 6, 20, 0x610000}
	,
	{1, 0, 6, 256, 100, 50, 0, 15, 14, 1, 7, 10, 9, 6, 20, 0x610000}
	,
	{1, 0, 6, 256, 100, 50, 0, 15, 14, 1, 7, 0, 9, 10, 20, 0x610000}
	,
	{1, 0, 5, 256, 35, 25, 12, 15, 13, 5, 5, 0, 5, 12, 40, 0x400}
	,
	{1, 0, 5, 256, 50, 40, 27, 15, 13, 6, 6, 0, 6, 12, 40, 0x0}
	,
	{1, 0, 5, 256, 85, 70, 27, 15, 13, 3, 3, 0, 3, 12, 40, 0x0}
	,
	{1, 0, 5, 256, 20, 55, 32, 15, 12, 5, 5, 0, 5, 12, 40, 0x400}
	,
	{1, 0, 5, 256, 35, 25, 12, 15, 15, 4, 4, 0, 4, 12, 40, 0x400}
	,
	{1, 0, 13, 256, 50, 50, 12, 10, 5, 8, 8, 6, 8, 12, 80, 0x800400}
	,
	{1, 0, 13, 384, 100, 80, 0, 10, 0, 10, 10, 2, 10, 12, 40, 0x400}
	,
	{1, 0, 5, 256, 35, 25, 5, 15, 14, 5, 5, 0, 5, 12, 40, 0x400}
	,
	{1, 0, 5, 256, 35, 25, 10, 15, 11, 4, 4, 0, 4, 12, 40, 0x400}
	,
	{1, 0, 5, 256, 55, 25, 0, 15, 0, 7, 7, 0, 7, 12, 40, 0x0}
	,
	{1, 0, 5, 256, 50, 40, 27, 15, 3, 6, 6, 0, 6, 12, 40, 0x0}
	,
	{1, 0, 5, 256, 35, 25, 12, 15, 13, 8, 8, 8, 8, 12, 40, 0x800}
	,
	{1, 0, 5, 256, 60, 25, 0, 15, 0, 8, 8, 8, 8, 12, 40, 0x800}
	,
	{1, 0, 9, 256, 100, 5, 0, 20, 14, 11, 7, 7, 9, 12, 20, 0x610000}
	,
	{1, 0, 9, 256, 100, 5, 0, 20, 14, 10, 7, 7, 9, 8, 20, 0x610000}
	,
	{1, 0, 11, 256, 100, 5, 0, 20, 14, 1, 7, 7, 9, 7, 20, 0x610000}
	,
	{1, 0, 12, 256, 100, 5, 0, 20, 14, 1, 7, 7, 9, 6, 20, 0x610000}
	,
	{1, 0, 16, 256, 100, 5, 0, 20, 14, 10, 7, 7, 9, 9, 20, 0x610000}
	,
	{1, 0, 13, 384, 80, 80, 10, 10, 7, 13, 13, 13, 13, 12, 500,
	 0x2800600}
};

struct Mission BEM_missions[9] = {
	{
	 "They are destroying our base!",
	 "All of a sudden aliens started popping out of nowhere all over the place"
	 ". Fortunately they seem quite disorganized or we would surely have lost "
	 "the base by now. We must eliminate this attacking force, though...",
	 5, 1, 3, 0, 0, 0, 48, 48, 20, 20, 10, 3,
	 0, 0, 0, 0,
	 1,
	 {
	  {
	   "Defeat the attack",
	   0, 0, 20, 15, 0x0},
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
	 {0, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	 3,
	 {10, 13, 17, 17, 17, 0},
	 9,
	 {3, 6, 7, 16, 14, 23, 26, 27, 28, 0, 0, 0, 0, 0, 0, 0},
	 {40, 10, 5, 20, 30, 5, 20, 10, 5, 0, 0, 0, 0, 0, 0, 0},
	 10, 0x13b,
	 "",
	 "",
	 15, 12, 10, 0},
	{
	 "What are they up to now?",
	 "Now the goggle-eyes are rampaging through a suburb! They are killing inn"
	 "ocent civilians for no reason...we have got to stop them!",
	 0, 2, 10, 0, 0, 0, 48, 48, 0, 0, 25, 2,
	 0, 0, 0, 0,
	 1,
	 {
	  {
	   "Eliminate the monsters",
	   0, 0, 20, 20, 0x0},
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
	 {3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0},
	 6,
	 {10, 11, 12, 13, 17, 19},
	 9,
	 {21, 18, 16, 17, 14, 13, 9, 4, 6, 10, 0, 0, 0, 0, 0, 0},
	 {50, 30, 40, 5, 20, 20, 10, 50, 10, 0, 0, 0, 0, 0, 0, 0},
	 30, 0x13b,
	 "",
	 "",
	 24, 4, 18, 0},
	{
	 "Rescue threatened scientists",
	 "There may be some fiendish plot behind this madness after all. Our foes "
	 "are attacking a little known laboratory doing research on our big-eyed o"
	 "pponents. The other attacks may have been diversions. Still, you never k"
	 "now with these critters. Enough already, we must save the staff...",
	 5, 5, 9, 0, 0, 0, 64, 32, 0, 0, 25, 0,
	 0, 0, 0, 0,
	 1,
	 {
	  {
	   "Eradicate the attacking force",
	   0, 0, 25, 25, 0x0},
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
	 {27, 26, 25, 24, 23, 0, 0, 0, 0, 0, 0, 0},
	 6,
	 {20, 12, 18, 17, 14, 13},
	 9,
	 {0, 1, 2, 3, 6, 16, 19, 23, 25, 0, 0, 0, 0, 0, 0, 0},
	 {10, 10, 10, 50, 10, 50, 25, 5, 20, 0, 0, 0, 0, 0, 0, 0},
	 20, 0x13b,
	 "",
	 "",
	 24, 13, 12, 0},
	{
	 "Enough is enough!",
	 "We are now in a position where we may repay the B.E.M.s for their action"
	 "s. We have uncovered a B.E.M. ship carrying an arsenal of rockets which "
	 "they are probably planning to unleash on us at the earliest moment possi"
	 "ble. Now, wouldn't it be nice if they were to detonate onboard instead?",
	 3, 4, 4, 0, 3, 3, 30, 64, 50, 10, 15, 0,
	 0, 0, 0, 0,
	 1,
	 {
	  {
	   "Blow up the rockets",
	   2, 28, 20, 15, 0x0},
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
	 12,
	 {21, 20, 19, 18, 17, 15, 16, 14, 13, 10, 11, 12},
	 0,
	 {0, 0, 0, 0, 0, 0},
	 8,
	 {7, 11, 22, 5, 10, 24, 38, 39, 31, 0, 0, 0, 0, 0, 0, 0},
	 {20, 5, 5, 10, 5, 5, 5, 5, 0, 0, 0, 0, 0, 0, 0, 0},
	 25, 0x0,
	 "",
	 "",
	 9, 19, 25, 3},
	{
	 "They are robbing a BANK???",
	 "They never cease to amaze me. Now they are robbing banks too...the shrin"
	 "k department guys think it may be something they have picked up from tel"
	 "evision broadcasts...you know, the archetypical bad guy so to speak. Any"
	 "way, we can't have bubble-eyes stealing citizens' savings, can we?",
	 0, 7, 10, 0, 0, 0, 48, 32, 0, 0, 20, 0,
	 0, 0, 0, 0,
	 2,
	 {
	  {
	   "Fend off the \"bank-robbers from space\"",
	   0, 0, 25, 25, 0x0},
	  {
	   "Safeguard any loot you come across",
	   1, 6, 6, 0, 0x1},
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
	 {3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0},
	 6,
	 {16, 15, 21, 22, 13, 17},
	 7,
	 {3, 4, 6, 14, 16, 23, 26, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	 {50, 50, 10, 20, 10, 30, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	 20, 0x13b,
	 "",
	 "",
	 16, 19, 6, 13},
	{
	 "Retrieve the stolen money",
	 "Despite your valiant, and ultimately successful, efforts, the buggers to"
	 "ok off with a considerable amount of dough. Now, the city can do without"
	 " that kind of strain on the treasury...soooo...we will just have to retr"
	 "ieve the funds won't we?",
	 3, 4, 4, 0, 3, 3, 32, 64, 50, 10, 12, 0,
	 0, 0, 0, 0,
	 1,
	 {
	  {
	   "Find as much money as possible",
	   1, 6, 32, 25, 0x0},
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
	 12,
	 {10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21},
	 0,
	 {0, 0, 0, 0, 0, 0},
	 8,
	 {39, 38, 24, 22, 11, 10, 7, 5, 0, 0, 0, 0, 0, 0, 0, 0},
	 {5, 5, 5, 5, 5, 5, 20, 10, 0, 0, 0, 0, 0, 0, 0, 0},
	 25, 0x0,
	 "",
	 "",
	 16, 24, 3, 18},
	{
	 "On the offensive...",
	 "Okeeey...our top minds have come up with a plan to put an end to this no"
	 "nsens. These B.E.M. fellows, they're highly hierarchical you see...so we"
	 " take out the head and the body crumbles, see? You don't? Well, shut up "
	 "and follow orders then, will'ya?",
	 3, 4, 4, 0, 3, 3, 32, 64, 50, 10, 12, 0,
	 0, 0, 0, 0,
	 4,
	 {
	  {
	   "Steal B.E.M. technology",
	   1, 10, 5, 3, 0x4},
	  {
	   "Any gadget may prove helpful...",
	   1, 7, 2, 0, 0x5},
	  {
	   "Data files are good, too",
	   1, 2, 5, 0, 0x5},
	  {
	   "Blueprints are a great help!",
	   1, 4, 2, 0, 0x5},
	  {
	   "",
	   0, 0, 0, 0, 0x0}
	  },
	 12,
	 {22, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21},
	 0,
	 {0, 0, 0, 0, 0, 0},
	 8,
	 {39, 38, 24, 22, 11, 10, 7, 5, 0, 0, 0, 0, 0, 0, 0, 0},
	 {5, 5, 5, 5, 5, 5, 20, 10, 0, 0, 0, 0, 0, 0, 0, 0},
	 30, 0x0,
	 "",
	 "",
	 18, 3, 0, 24},
	{
	 "Board and seize an enemy vessel",
	 "The B.E.M. scout vessel 'Har-Har-Har' - they must be doing this just to "
	 "taunt us I'm sure - is vulnerable to a quick assault by a small elite fo"
	 "rce...hey, come back here...no, you may go to the bathroom afterwards!",
	 3, 4, 4, 0, 3, 3, 32, 64, 50, 10, 12, 0,
	 0, 0, 0, 0,
	 2,
	 {
	  {
	   "Eliminate the crew members",
	   0, 0, 30, 30, 0x0},
	  {
	   "Seize the remote to the bridge controls",
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
	 0,
	 {21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21},
	 6,
	 {22, 21, 18, 17, 16, 15},
	 8,
	 {39, 38, 24, 22, 11, 10, 7, 5, 0, 0, 0, 0, 0, 0, 0, 0},
	 {5, 5, 5, 5, 5, 5, 20, 10, 0, 0, 0, 0, 0, 0, 0, 0},
	 0, 0x0,
	 "",
	 "",
	 24, 16, 6, 7},
	{
	 "Mister Congeniality he/she/it ain't...",
	 "Actually, that remote was for the self-destruction mechanism, but you'd "
	 "only have been nervous had you known. What's with the sour faces? It end"
	 "ed well, didn't it? Anyway, I've got a treat for you. You have been chos"
	 "en to put an end to this conflict by nailing the head S.O.B. itself!",
	 3, 4, 4, 0, 3, 3, 24, 24, 10, 4, 0, 0,
	 0, 0, 0, 0,
	 1,
	 {
	  {
	   "Kill the Big Bug-Eye!",
	   0, 0, 1, 1, 0x0},
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
	 0,
	 {21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21},
	 1,
	 {28, 0, 0, 0, 0, 0},
	 7,
	 {39, 38, 24, 22, 11, 10, 7, 6, 0, 0, 0, 0, 0, 0, 0, 0},
	 {5, 5, 5, 5, 5, 5, 20, 10, 0, 0, 0, 0, 0, 0, 0, 0},
	 0, 0x777,
	 "",
	 "",
	 18, 3, 0, 24},
};

struct CampaignSetting BEM_campaign = {
	"Bug-Eyed Monsters",
	"Ronny Wester",
	"These irrational foes pop up now and again. Unpredictable and vicious, the"
	    "ir primary goal seems to be to make a nuisance of themselves rather than a"
	    "ctually attaining any specific goals.",
	9, BEM_missions, 29, BEM_badguys
};
