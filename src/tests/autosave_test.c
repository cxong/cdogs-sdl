#include <cbehave/cbehave.h>

#include <autosave.h>

#include <string.h>


FEATURE(1, "Initialise autosave")
	SCENARIO("Initialise autosave")
	{
		Autosave autosave1, autosave2;
		GIVEN("two autosaves")
		GIVEN_END

		WHEN("I initialise both")
			AutosaveInit(&autosave1);
			AutosaveInit(&autosave2);
		WHEN_END

		THEN("they should equal each other");
			SHOULD_MEM_EQUAL(&autosave1.LastMission.Campaign, &autosave2.LastMission.Campaign, sizeof autosave1.LastMission.Campaign);
			SHOULD_STR_EQUAL(autosave1.LastMission.Password, autosave2.LastMission.Password);
		THEN_END
	}
	SCENARIO_END
FEATURE_END

FEATURE(2, "Save and load")
	SCENARIO("Save and load")
	{
		Autosave autosave1, autosave2;
		MissionSave mission1, mission2;
		GIVEN("an autosave with some values, and I save it to file")
			AutosaveInit(&autosave1);
			memset(&mission1, 0, sizeof mission1);
			strcpy(mission1.Campaign.path, "path/to/mission");
			strcpy(mission1.Password, "password");
			AutosaveAddMission(&autosave1, &mission1, 0);
			AutosaveSave(&autosave1, "tmp");
		GIVEN_END
		
		WHEN("I initialise and load a second autosave from that file")
			AutosaveInit(&autosave2);
			AutosaveLoad(&autosave2, "tmp");
		WHEN_END
		
		THEN("they should equal each other");
			SHOULD_STR_EQUAL(autosave2.LastMission.Campaign.path, autosave1.LastMission.Campaign.path);
			SHOULD_STR_EQUAL(autosave2.LastMission.Password, autosave1.LastMission.Password);
			AutosaveLoadMission(&autosave2, &mission2, mission1.Campaign.path, 0);
			SHOULD_STR_EQUAL(mission2.Campaign.path, mission1.Campaign.path);
			SHOULD_STR_EQUAL(mission2.Password, mission1.Password);
		THEN_END
	}
	SCENARIO_END
FEATURE_END

FEATURE(3, "Mission autosaves")
	SCENARIO("Load non-existing mission autosave")
	{
		Autosave autosave;
		MissionSave mission;
		GIVEN("an empty autosave")
			AutosaveInit(&autosave);
		GIVEN_END

		WHEN("I attempt to load a mission from it")
			AutosaveLoadMission(&autosave, &mission, "path/to/mission", 0);
		WHEN_END
		
		THEN("the mission should be empty");
			SHOULD_STR_EQUAL(mission.Campaign.path, "");
			SHOULD_STR_EQUAL(mission.Password, "");
		THEN_END
	}
	SCENARIO_END

	SCENARIO("Add new mission autosave")
	{
		Autosave autosave;
		MissionSave mission1, mission2;
		GIVEN("an autosave and a mission")
			AutosaveInit(&autosave);
			memset(&mission1, 0, sizeof mission1);
			strcpy(mission1.Campaign.path, "path/to/mission");
			strcpy(mission1.Password, "password");
		GIVEN_END

		WHEN("I add a new mission autosave to it")
			AutosaveAddMission(&autosave, &mission1, 0);
		WHEN_END
		
		THEN("I should be able to find the mission in the autosave");
			AutosaveLoadMission(&autosave, &mission2, mission1.Campaign.path, 0);
			SHOULD_STR_EQUAL(mission2.Campaign.path, mission1.Campaign.path);
			SHOULD_STR_EQUAL(mission2.Password, mission1.Password);
		THEN_END
	}
	SCENARIO_END

	SCENARIO("Add builtin mission autosave")
	{
		Autosave autosave;
		MissionSave mission1, mission2;
		int builtinIndex = 3;
		GIVEN("an autosave and a builtin mission")
			AutosaveInit(&autosave);
			memset(&mission1, 0, sizeof mission1);
			strcpy(mission1.Campaign.path, "");
			mission1.Campaign.isBuiltin = 1;
			mission1.Campaign.builtinIndex = builtinIndex;
			strcpy(mission1.Password, "password");
		GIVEN_END

		WHEN("I add a new mission autosave to it")
			AutosaveAddMission(&autosave, &mission1, 0);
		WHEN_END

		THEN("I should be able to find the mission in the autosave");
			AutosaveLoadMission(
				&autosave, &mission2, mission1.Campaign.path, builtinIndex);
			SHOULD_STR_EQUAL(mission2.Campaign.path, mission1.Campaign.path);
			SHOULD_INT_EQUAL(mission2.Campaign.builtinIndex, builtinIndex);
			SHOULD_STR_EQUAL(mission2.Password, mission1.Password);
		THEN_END
	}
	SCENARIO_END

	SCENARIO("Add existing mission autosave")
	{
		Autosave autosave;
		MissionSave mission1, mission2;
		GIVEN("an autosave and a mission, and I add the mission to the autosave")
			AutosaveInit(&autosave);
			memset(&mission1, 0, sizeof mission1);
			strcpy(mission1.Campaign.path, "path/to/mission");
			strcpy(mission1.Password, "password");
			AutosaveAddMission(&autosave, &mission1, 0);
		GIVEN_END

		WHEN("I add the same mission but with new details")
			strcpy(mission1.Password, "new password");
			AutosaveAddMission(&autosave, &mission1, 0);
		WHEN_END
		
		THEN("I should be able to find the mission in the autosave, with the new details");
			AutosaveLoadMission(&autosave, &mission2, mission1.Campaign.path, 0);
			SHOULD_STR_EQUAL(mission2.Campaign.path, mission1.Campaign.path);
			SHOULD_STR_EQUAL(mission2.Password, mission1.Password);
		THEN_END
	}
	SCENARIO_END

	SCENARIO("Adding autosave updates last mission")
	{
		Autosave autosave;
		MissionSave mission;
		GIVEN("an autosave and a mission")
			AutosaveInit(&autosave);
			memset(&mission, 0, sizeof mission);
			strcpy(mission.Campaign.path, "path/to/mission");
			strcpy(mission.Password, "password");
		GIVEN_END

		WHEN("I add a new mission autosave to it")
			AutosaveAddMission(&autosave, &mission, 0);
		WHEN_END
		
		THEN("the last mission will be the same as the new mission");
			SHOULD_STR_EQUAL(autosave.LastMission.Campaign.path, mission.Campaign.path);
			SHOULD_STR_EQUAL(autosave.LastMission.Password, mission.Password);
		THEN_END
	}
	SCENARIO_END
FEATURE_END

int main(void)
{
	cbehave_feature features[] =
	{
		{feature_idx(1)},
		{feature_idx(2)},
		{feature_idx(3)}
	};
	
	return cbehave_runner("Autosave features are:", features);
}
