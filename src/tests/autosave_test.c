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
int ConfigGetInt(Config *c, const char *name)
{
	UNUSED(c); UNUSED(name); return 0;
}
int ConfigGetJSONVersion(FILE *f)
{
	UNUSED(f);
	return 0;
}
bool ConfigIsOld(FILE *f)
{
	UNUSED(f);
	return false;
}
Pic *PicManagerGetPic(const PicManager *pm, const char *name)
{
	UNUSED(pm);
	UNUSED(name);
	return NULL;
}
const WeaponClass *StrWeaponClass(const char *s)
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
		GIVEN("an autosave")
			Autosave autosave;

		WHEN("I initialise it")
			AutosaveInit(&autosave);

		THEN("the last campaign index should be -1")
			SHOULD_INT_EQUAL(autosave.LastCampaignIndex, -1);
		AND("the campaigns should be empty")
			SHOULD_INT_EQUAL(autosave.Campaigns.size, 0);
	SCENARIO_END
FEATURE_END

FEATURE(save_and_load, "Save and load")
	SCENARIO("Save and load")
		GIVEN("an autosave with some values")
			Autosave autosave1;
			AutosaveInit(&autosave1);
			CampaignSave cs1;
			CampaignSaveInit(&cs1);
			CSTRDUP(cs1.Campaign.Path, "campaign.cdogscpn");
			cs1.NextMission = 1;
			AutosaveAddCampaign(&autosave1, &cs1);
		AND("I save it to file")
			AutosaveSave(&autosave1, "tmp");

		WHEN("I initialise and load a second autosave from that file")
			Autosave autosave2;
			AutosaveInit(&autosave2);
			AutosaveLoad(&autosave2, "tmp");
			CampaignSave *cs2 = CArrayGet(&autosave2.Campaigns, 0);

		THEN("their mission paths should equal")
			SHOULD_STR_EQUAL(cs2->Campaign.Path, cs1.Campaign.Path);
		AND("their next missions should equal")
			SHOULD_INT_EQUAL(cs2->NextMission, cs1.NextMission);
	SCENARIO_END
FEATURE_END

FEATURE(campaign_autosaves, "Campaign autosaves")
	SCENARIO("Load non-existing campaign autosave")
		GIVEN("an empty autosave")
			Autosave autosave;
			AutosaveInit(&autosave);

		WHEN("I attempt to load a non-existing mission from it")
			const CampaignSave *cs = AutosaveGetCampaign(&autosave, "mission.cdogscpn");
		
		THEN("the result should be null")
			SHOULD_BE_TRUE(cs == NULL);
	SCENARIO_END

	SCENARIO("Add new campaign autosave")
		GIVEN("an autosave and a campaign")
			Autosave autosave;
			AutosaveInit(&autosave);
			CampaignSave cs1;
			CampaignSaveInit(&cs1);
			CSTRDUP(cs1.Campaign.Path, "campaign.cdogscpn");
			cs1.NextMission = 1;

		WHEN("I add a new campaign autosave to it")
			AutosaveAddCampaign(&autosave, &cs1);
		
		THEN("I should be able to find the mission in the autosave")
			const CampaignSave *cs2 = AutosaveGetCampaign(&autosave, cs1.Campaign.Path);
			SHOULD_STR_EQUAL(cs2->Campaign.Path, cs1.Campaign.Path);
			SHOULD_INT_EQUAL(cs2->NextMission, cs1.NextMission);
	SCENARIO_END

	SCENARIO("Add existing campaign autosave")
		GIVEN("an autosave and a campaign")
			Autosave autosave;
			AutosaveInit(&autosave);
			CampaignSave cs1;
			CampaignSaveInit(&cs1);
			CSTRDUP(cs1.Campaign.Path, "campaign.cdogscpn");
			cs1.NextMission = 1;
			int mission = 0;
			CArrayPushBack(&cs1.MissionsCompleted, &mission);
		AND("I add the campaign to the autosave")
			AutosaveAddCampaign(&autosave, &cs1);

		WHEN("I add the same campaign but with different next mission")
			CampaignSaveInit(&cs1);
			CSTRDUP(cs1.Campaign.Path, "campaign.cdogscpn");
			cs1.NextMission = 2;
		AND("and different missions completed")
			mission = 1;
			CArrayPushBack(&cs1.MissionsCompleted, &mission);
			AutosaveAddCampaign(&autosave, &cs1);

		THEN("I should be able to find the mission in the autosave")
			const CampaignSave *cs2 = AutosaveGetCampaign(&autosave, cs1.Campaign.Path);
		AND("with the new details")
			SHOULD_STR_EQUAL(cs2->Campaign.Path, cs1.Campaign.Path);
		BUT("the greatest next mission")
			SHOULD_INT_EQUAL(cs2->NextMission, 2);
		AND("the union of completed missions")
			SHOULD_INT_EQUAL(*(int *)CArrayGet(&cs2->MissionsCompleted, 0), 0);
			SHOULD_INT_EQUAL(*(int *)CArrayGet(&cs2->MissionsCompleted, 1), 1);
	SCENARIO_END

	SCENARIO("Adding autosave updates last campaign")
		GIVEN("an autosave and a campaign")
			Autosave autosave;
			AutosaveInit(&autosave);
			CampaignSave cs1;
			CampaignSaveInit(&cs1);
			CSTRDUP(cs1.Campaign.Path, "campaign.cdogscpn");
			cs1.NextMission = 1;

		WHEN("I add a new campaign autosave to it")
			AutosaveAddCampaign(&autosave, &cs1);
		
		THEN("the last campaign will be the same")
			const CampaignSave *cs2 = AutosaveGetLastCampaign(&autosave);
			SHOULD_STR_EQUAL(
				cs2->Campaign.Path, cs1.Campaign.Path);
			SHOULD_INT_EQUAL(cs2->NextMission, cs1.NextMission);
	SCENARIO_END
FEATURE_END

CBEHAVE_RUN(
	"Autosave features are:",
	TEST_FEATURE(AutosaveInit),
	TEST_FEATURE(save_and_load),
	TEST_FEATURE(campaign_autosaves)
)
