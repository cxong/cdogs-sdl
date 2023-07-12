#define SDL_MAIN_HANDLED
#include <cbehave/cbehave.h>

#include <actors.h>

#include <character.h>
#include <game_events.h>
#include <handle_game_events.h>


FEATURE(pickup_empty, "Pickup into empty slot")

	CharacterStoreInit(&gCampaign.Setting.characters);
	Character *c = CharacterStoreAddOther(&gCampaign.Setting.characters);
	CharacterClass cc;
	memset(&cc, 0, sizeof cc);
	c->Class = &cc;
	WeaponClassesInitialize(&gWeaponClasses);
	WeaponClass wc;
	memset(&wc, 0, sizeof wc);
	CSTRDUP(wc.name, "thegun");
	CArrayPushBack(&gWeaponClasses.Guns, &wc);
	GameEventsInit(&gGameEvents);
	ActorsInit();
	gConfig = ConfigDefault();
	int slotStart, slotEnd;
	char buf[256];

	for (int slot = 0; slot < MAX_WEAPONS; slot++)
	{
		for (wc.Type = GUNTYPE_NORMAL; wc.Type < GUNTYPE_COUNT; wc.Type++)
		{
			TActor *a;
			GunTypeGetSlotStartEnd(wc.Type, &slotStart, &slotEnd);
			sprintf(
				buf, "Pickup %s weapon while having no weapons and in slot %d",
				GunTypeStr(wc.Type), slot);
			SCENARIO(buf)
				GIVEN("an actor with no weapons")
					GameEvent e = GameEventNewActorAdd(svec2(1, 1), c, NULL);
					e.u.ActorAdd.CharId = 0;
					a = ActorAdd(e.u.ActorAdd);
					sprintf(buf, "holding slot %d", slot);
				AND(buf)
					a->gunIndex = slot;
					sprintf(buf, "I pickup a %s weapon", GunTypeStr(wc.Type));
				WHEN(buf)
					ActorPickupGun(a, &wc);
					HandleGameEvents(&gGameEvents, NULL, NULL, NULL, NULL);
				THEN("the weapon should be the picked up weapon")
					SHOULD_STR_EQUAL(a->guns[slotStart].Gun->name, wc.name);
				if (ActorCanSwitchToWeapon(a, slotStart))
				{
					AND("the actor should be holding the new weapon")
						SHOULD_INT_EQUAL(a->gunIndex, slotStart);
				}
				else
				{
					AND("the actor should be holding the same weapon")
						SHOULD_INT_EQUAL(a->gunIndex, slot);
				}
			SCENARIO_END
			ActorDestroy(a);
		}
	}

	GunTypeGetSlotStartEnd(GUNTYPE_NORMAL, &slotStart, &slotEnd);
	WeaponClass wc2;
	memset(&wc2, 0, sizeof wc2);
	CSTRDUP(wc2.name, "othergun");
	CArrayPushBack(&gWeaponClasses.CustomGuns, &wc2);
	for (int slot = slotStart; slot <= slotEnd; slot++)
	{
		TActor *a;
		sprintf(buf, "Pickup gun while having an empty slot %d", slot);
		SCENARIO(buf)
			sprintf(buf, "an actor with guns in all slots except %d", slot);
			GIVEN(buf)
				GameEvent e = GameEventNewActorAdd(svec2(1, 1), c, NULL);
				e.u.ActorAdd.CharId = 0;
				a = ActorAdd(e.u.ActorAdd);
				for (int otherSlot = slotStart; otherSlot <= slotEnd; otherSlot++)
				{
					if (otherSlot != slot)
					{
						Weapon w = WeaponCreate(&wc2);
						memcpy(&a->guns[otherSlot], &w, sizeof w);
					}
				}
			WHEN("I pickup a gun")
				ActorPickupGun(a, &wc);
				HandleGameEvents(&gGameEvents, NULL, NULL, NULL, NULL);
			THEN("the actor should be holding the new weapon in the empty slot")
				SHOULD_INT_EQUAL(a->gunIndex, slot);
			AND("the weapon should be the picked up weapon")
				SHOULD_STR_EQUAL(a->guns[slot].Gun->name, wc.name);
		SCENARIO_END
		ActorDestroy(a);
	}
	WeaponClassesClear(&gWeaponClasses.CustomGuns);

	WeaponClassesTerminate(&gWeaponClasses);
	ActorsTerminate();
	GameEventsTerminate(&gGameEvents);
	CharacterStoreTerminate(&gCampaign.Setting.characters);
FEATURE_END

FEATURE(pickup_existing, "Pickup an existing gun")

	CharacterStoreInit(&gCampaign.Setting.characters);
	Character *c = CharacterStoreAddOther(&gCampaign.Setting.characters);
	CharacterClass cc;
	memset(&cc, 0, sizeof cc);
	c->Class = &cc;
	WeaponClassesInitialize(&gWeaponClasses);
	WeaponClass wc;
	memset(&wc, 0, sizeof wc);
	CSTRDUP(wc.name, "thegun");
	CArrayPushBack(&gWeaponClasses.Guns, &wc);
	GameEventsInit(&gGameEvents);
	ActorsInit();
	gConfig = ConfigDefault();
	int slotStart, slotEnd;
	char buf[256];

	for (int slot = 0; slot < MAX_WEAPONS; slot++)
	{
		for (wc.Type = GUNTYPE_NORMAL; wc.Type < GUNTYPE_COUNT; wc.Type++)
		{
			TActor *a;
			GunTypeGetSlotStartEnd(wc.Type, &slotStart, &slotEnd);
			sprintf(
				buf, "Pickup %s weapon while already having the weapon in slot %d and holding slot %d",
				GunTypeStr(wc.Type), slotStart, slot);
			SCENARIO(buf)
				sprintf(
					buf, "an actor with a %s weapon in slot %d", GunTypeStr(wc.Type), slot);
				GIVEN(buf)
					GameEvent e = GameEventNewActorAdd(svec2(1, 1), c, NULL);
					e.u.ActorAdd.CharId = 0;
					a = ActorAdd(e.u.ActorAdd);
					Weapon w = WeaponCreate(&wc);
					memcpy(&a->guns[slotStart], &w, sizeof w);
					sprintf(buf, "holding slot %d", slot);
				AND(buf)
					a->gunIndex = slot;
				WHEN("I pickup that same weapon")
					ActorPickupGun(a, &wc);
					HandleGameEvents(&gGameEvents, NULL, NULL, NULL, NULL);
				THEN("the weapon should be the picked up weapon")
					SHOULD_STR_EQUAL(a->guns[slotStart].Gun->name, wc.name);
				// TODO: test for pickup creation
				if (ActorCanSwitchToWeapon(a, slotStart))
				{
					AND("the actor should be holding the weapon")
						SHOULD_INT_EQUAL(a->gunIndex, slotStart);
				}
				else
				{
					AND("the actor should be holding the same weapon")
						SHOULD_INT_EQUAL(a->gunIndex, slot);
				}
			SCENARIO_END
			ActorDestroy(a);
		}
	}

	WeaponClassesTerminate(&gWeaponClasses);
	ActorsTerminate();
	GameEventsTerminate(&gGameEvents);
	CharacterStoreTerminate(&gCampaign.Setting.characters);
FEATURE_END

FEATURE(pickup_new, "Pickup into existing slot")

	CharacterStoreInit(&gCampaign.Setting.characters);
	Character *c = CharacterStoreAddOther(&gCampaign.Setting.characters);
	CharacterClass cc;
	memset(&cc, 0, sizeof cc);
	c->Class = &cc;
	WeaponClassesInitialize(&gWeaponClasses);
	WeaponClass wc;
	memset(&wc, 0, sizeof wc);
	CSTRDUP(wc.name, "thegun");
	CArrayPushBack(&gWeaponClasses.Guns, &wc);
	WeaponClass wc2;
	memset(&wc2, 0, sizeof wc2);
	CSTRDUP(wc2.name, "othergun");
	CArrayPushBack(&gWeaponClasses.Guns, &wc2);
	GameEventsInit(&gGameEvents);
	ActorsInit();
	gConfig = ConfigDefault();
	int slotStart, slotEnd;
	char buf[256];

	for (int slot = 0; slot < MAX_WEAPONS; slot++)
	{
		for (wc.Type = GUNTYPE_NORMAL; wc.Type < GUNTYPE_COUNT; wc.Type++)
		{
			TActor *a;
			GunTypeGetSlotStartEnd(wc.Type, &slotStart, &slotEnd);
			const int pickupSlot =
				(slot >= slotStart && slot <= slotEnd) ? slot : slotStart;
			sprintf(buf, "Pickup %s weapon and in slot %d", GunTypeStr(wc.Type), slot);
			SCENARIO(buf)
				GIVEN("an actor with weapons")
					GameEvent e = GameEventNewActorAdd(svec2(1, 1), c, NULL);
					e.u.ActorAdd.CharId = 0;
					a = ActorAdd(e.u.ActorAdd);
					for (int ws = 0; ws < MAX_WEAPONS; ws++)
					{
						Weapon w = WeaponCreate(&wc2);
						memcpy(&a->guns[ws], &w, sizeof w);
					}
					sprintf(buf, "holding slot %d", slot);
				AND(buf)
					a->gunIndex = slot;
					sprintf(buf, "I pickup a %s weapon", GunTypeStr(wc.Type));
				WHEN(buf)
					ActorPickupGun(a, &wc);
					HandleGameEvents(&gGameEvents, NULL, NULL, NULL, NULL);
				THEN("the weapon should be the picked up weapon")
					SHOULD_STR_EQUAL(a->guns[pickupSlot].Gun->name, wc.name);
				// TODO: test for pickup creation
				if (ActorCanSwitchToWeapon(a, pickupSlot))
				{
					AND("the actor should be holding the new weapon")
						SHOULD_INT_EQUAL(a->gunIndex, pickupSlot);
				}
				else
				{
					AND("the actor should be holding the same weapon")
						SHOULD_INT_EQUAL(a->gunIndex, slot);
				}
			SCENARIO_END
			ActorDestroy(a);
		}
	}

	WeaponClassesTerminate(&gWeaponClasses);
	ActorsTerminate();
	GameEventsTerminate(&gGameEvents);
	CharacterStoreTerminate(&gCampaign.Setting.characters);
FEATURE_END

CBEHAVE_RUN(
	"Actor features are:",
	TEST_FEATURE(pickup_empty),
	TEST_FEATURE(pickup_existing),
	TEST_FEATURE(pickup_new)
)
