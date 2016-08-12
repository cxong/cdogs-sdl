#define SDL_MAIN_HANDLED
#include <cbehave/cbehave.h>

#include <autosave.h>

#include <string.h>


// Stubs
int MapNewScan(
	const char *filename, const bool isArchive, char **title, int *numMissions)
{
	UNUSED(filename);
	UNUSED(isArchive);
	UNUSED(title);
	UNUSED(numMissions);
	return 0;
}
Mix_Chunk *StrSound(const char *s)
{
	UNUSED(s);
	return NULL;
}
bool ConfigGetBool(Config *c, const char *name)
{
	UNUSED(c); UNUSED(name); return false;
}
Pic *PicManagerGetPic(const PicManager *pm, const char *name)
{
	UNUSED(pm);
	UNUSED(name);
	return NULL;
}
const GunDescription *StrGunDescription(const char *s)
{
	UNUSED(s);
	return NULL;
}
const char *JoyName(const int deviceIndex)
{
	UNUSED(deviceIndex);
	return NULL;
}
Config gConfig;
PicManager gPicManager;


FEATURE(AutosaveInit, "Initialise autosave")
	SCENARIO("Initialise autosave")
		GIVEN("two autosaves")
			Autosave autosave1, autosave2;

		WHEN("I initialise both")
			AutosaveInit(&autosave1);
			AutosaveInit(&autosave2);

		THEN("their campaign entries should equal each other")
			SHOULD_MEM_EQUAL(
				&autosave1.LastMission.Campaign,
				&autosave2.LastMission.Campaign,
				sizeof autosave1.LastMission.Campaign);
		AND("their passwords should equal each other")
			SHOULD_STR_EQUAL(
				autosave1.LastMission.Password,
				autosave2.LastMission.Password);
	SCENARIO_END
FEATURE_END

FEATURE(save_and_load, "Save and load")
	SCENARIO("Save and load")
		GIVEN("an autosave with some values")
			Autosave autosave1;
			AutosaveInit(&autosave1);
			MissionSave mission1;
			memset(&mission1, 0, sizeof mission1);
			CSTRDUP(mission1.Campaign.Path, "mission.cdogscpn");
			strcpy(mission1.Password, "password");
			AutosaveAddMission(&autosave1, &mission1);
		AND("I save it to file")
			AutosaveSave(&autosave1, "tmp");
		
		WHEN("I initialise and load a second autosave from that file")
			Autosave autosave2;
			AutosaveInit(&autosave2);
			AutosaveLoad(&autosave2, "tmp");
		
		THEN("their last mission paths should equal")
			SHOULD_STR_EQUAL(
				autosave2.LastMission.Campaign.Path,
				autosave1.LastMission.Campaign.Path);
		AND("their last mission passwords should equal")
			SHOULD_STR_EQUAL(
				autosave2.LastMission.Password,
				autosave1.LastMission.Password);
		AND("their mission paths should equal")
			MissionSave mission2;
			AutosaveLoadMission(&autosave2, &mission2, mission1.Campaign.Path);
			SHOULD_STR_EQUAL(
				mission2.Campaign.Path, mission1.Campaign.Path);
		AND("their mission passwords should equal")
			SHOULD_STR_EQUAL(mission2.Password, mission1.Password);
	SCENARIO_END
FEATURE_END

FEATURE(mission_autosaves, "Mission autosaves")
	SCENARIO("Load non-existing mission autosave")
		GIVEN("an empty autosave")
			Autosave autosave;
			AutosaveInit(&autosave);

		WHEN("I attempt to load a non-existing mission from it")
			MissionSave mission;
			AutosaveLoadMission(&autosave, &mission, "mission.cdogscpn");
		
		THEN("the mission should be empty")
			SHOULD_BE_TRUE(mission.Campaign.Path == NULL);
		AND("the password should be empty")
			SHOULD_STR_EQUAL(mission.Password, "");
	SCENARIO_END

	SCENARIO("Add new mission autosave")
		GIVEN("an autosave and a mission")
			Autosave autosave;
			AutosaveInit(&autosave);
			MissionSave mission1;
			memset(&mission1, 0, sizeof mission1);
			CSTRDUP(mission1.Campaign.Path, "mission.cdogscpn");
			strcpy(mission1.Password, "password");

		WHEN("I add a new mission autosave to it")
			AutosaveAddMission(&autosave, &mission1);
		
		THEN("I should be able to find the mission in the autosave")
			MissionSave mission2;
			AutosaveLoadMission(&autosave, &mission2, mission1.Campaign.Path);
			SHOULD_STR_EQUAL(mission2.Campaign.Path, mission1.Campaign.Path);
			SHOULD_STR_EQUAL(mission2.Password, mission1.Password);
	SCENARIO_END

	SCENARIO("Add existing mission autosave")
		GIVEN("an autosave and a mission")
			Autosave autosave;
			AutosaveInit(&autosave);
			MissionSave mission1;
			memset(&mission1, 0, sizeof mission1);
			CSTRDUP(mission1.Campaign.Path, "mission.cdogscpn");
			strcpy(mission1.Password, "password");
			mission1.MissionsCompleted = 3;
		AND("I add the mission to the autosave")
			AutosaveAddMission(&autosave, &mission1);

		WHEN("I add the same mission but with new password")
			strcpy(mission1.Password, "new password");
		AND("and less missions completed")
			mission1.MissionsCompleted = 2;
			AutosaveAddMission(&autosave, &mission1);
		
		THEN("I should be able to find the mission in the autosave")
			MissionSave mission2;
			AutosaveLoadMission(&autosave, &mission2, mission1.Campaign.Path);
		AND("with the new details")
			SHOULD_STR_EQUAL(mission2.Campaign.Path, mission1.Campaign.Path);
			SHOULD_STR_EQUAL(mission2.Password, mission1.Password);
		BUT("the greatest missions completed")
			SHOULD_INT_EQUAL(mission2.MissionsCompleted, 3);
	SCENARIO_END

	SCENARIO("Adding autosave updates last mission")
		GIVEN("an autosave and a mission")
			Autosave autosave;
			AutosaveInit(&autosave);
			MissionSave mission;
			memset(&mission, 0, sizeof mission);
			CSTRDUP(mission.Campaign.Path, "mission.cdogscpn");
			strcpy(mission.Password, "password");

		WHEN("I add a new mission autosave to it")
			AutosaveAddMission(&autosave, &mission);
		
		THEN("the last mission will be the same as the new mission")
			SHOULD_STR_EQUAL(
				autosave.LastMission.Campaign.Path, mission.Campaign.Path);
			SHOULD_STR_EQUAL(autosave.LastMission.Password, mission.Password);
	SCENARIO_END
FEATURE_END

CBEHAVE_RUN(
	"Autosave features are:",
	TEST_FEATURE(AutosaveInit),
	TEST_FEATURE(save_and_load),
	TEST_FEATURE(mission_autosaves)
)
