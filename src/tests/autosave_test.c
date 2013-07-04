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
			SHOULD_STR_EQUAL(autosave1.LastMission.CampaignPath, autosave2.LastMission.CampaignPath);
			SHOULD_STR_EQUAL(autosave1.LastMission.Password, autosave2.LastMission.Password);
		THEN_END
	}
	SCENARIO_END
FEATURE_END

FEATURE(2, "Save and load")
	SCENARIO("Save and load")
	{
		Autosave autosave1, autosave2;
		GIVEN("an autosave with some values, and I save it to file")
			AutosaveInit(&autosave1);
			strcpy(autosave1.LastMission.CampaignPath, "path/to/file");
			strcpy(autosave1.LastMission.Password, "password");
			AutosaveSave(&autosave1, "tmp");
		GIVEN_END
		
		WHEN("I load a second autosave from that file")
			AutosaveLoad(&autosave2, "tmp");
		WHEN_END
		
		THEN("they should equal each other");
			SHOULD_STR_EQUAL(autosave1.LastMission.CampaignPath, autosave2.LastMission.CampaignPath);
			SHOULD_STR_EQUAL(autosave1.LastMission.Password, autosave2.LastMission.Password);
		THEN_END
	}
	SCENARIO_END
FEATURE_END

int main(void)
{
	cbehave_feature features[] =
	{
		{feature_idx(1)},
		{feature_idx(2)}
	};
	
	return cbehave_runner("Autosave features are:", features);
}
