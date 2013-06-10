#include <cbehave/cbehave.h>

#include <config.h>


FEATURE(1, "Load default config")
	SCENARIO("Load a default config")
	{
		Config config1, config2;
		GIVEN("two configs")
		GIVEN_END

		WHEN("I load them both with defaults")
			ConfigLoadDefault(&config1);
			ConfigLoadDefault(&config2);
		WHEN_END

		THEN("they should equal each other")
			SHOULD_MEM_EQUAL(&config1, &config2, sizeof(Config));
		THEN_END
	}
	SCENARIO_END
FEATURE_END

FEATURE(2, "Save and load")
	SCENARIO("Save and load a config file")
	{
		Config config1, config2;
		GIVEN("a config file with some values, and I save the config to file")
			ConfigLoadDefault(&config1);
			config1.Game.FriendlyFire = 1;
			config1.Graphics.Brightness = 5;
			ConfigSave(&config1, "tmp");
		GIVEN_END

		WHEN("I load a second config from that file")
			ConfigLoad(&config2, "tmp");
		WHEN_END

		THEN("the two configs should be equal")
			SHOULD_MEM_EQUAL(&config1, &config2, sizeof(Config));
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

	return cbehave_runner("Config features are:", features);
}
