#define SDL_MAIN_HANDLED
#include <cbehave/cbehave.h>

#include <cdogs/config.h>
#include <command_line.h>

FEATURE(boolean_config, "Parse boolean command-line config")
	SCENARIO("Boolean config options allow later options to be parsed")
		GIVEN("default config and boolean options with both values")
			gConfig = ConfigDefault();
			ENetAddress connectAddr = {0};
			const char *loadCampaign = NULL;
			int demoQuitTimer = 0;
			char executable[] = "cdogs-sdl";
			char friendlyFire[] = "--config=Game.FriendlyFire,true";
			char healthPickups[] = "--config=Game.HealthPickups,false";
			char scale[] = "--scale=2";
			char *argv[] = {
				executable, friendlyFire, healthPickups, scale, NULL};

		WHEN("the command line is parsed")
			const bool shouldRun = ParseArgs(
				4, argv, &connectAddr, &loadCampaign, &demoQuitTimer);

		THEN("both boolean values should be applied")
			SHOULD_BE_TRUE(shouldRun);
			SHOULD_BE_TRUE(ConfigGetBool(&gConfig, "Game.FriendlyFire"));
			SHOULD_BE_FALSE(ConfigGetBool(&gConfig, "Game.HealthPickups"));
		AND("the later option should also be parsed")
			SHOULD_INT_EQUAL(ConfigGetInt(&gConfig, "Graphics.ScaleFactor"), 2);
	SCENARIO_END
FEATURE_END

CBEHAVE_RUN("Command-line boolean config features are:", TEST_FEATURE(boolean_config))
