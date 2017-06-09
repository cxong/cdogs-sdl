#include <cbehave/cbehave.h>

#include <utils.h>

#include <SDL_joystick.h>

#include <sys_config.h>
#include <sys_specifics.h>

// Stubs
const char *JoyName(const int deviceIndex)
{
	UNUSED(deviceIndex);
	return NULL;
}

FEATURE(path_funcs, "Path functions")
	mkdir("/tmp/path", MKDIR_MODE);
	SCENARIO("Relative path")
		GIVEN("a relative path")
			char to[CDOGS_PATH_MAX];
			strcpy(to, "/tmp/path/to");

		WHEN("I get the relative path from another path")
			char from[CDOGS_PATH_MAX];
			strcpy(from, "/tmp/path/from");

		THEN("the result should be a relative path from one to the other")
			char rel[CDOGS_PATH_MAX];
			RelPath(rel, to, from);
			SHOULD_STR_EQUAL(rel, "../to");
	SCENARIO_END

	SCENARIO("Relative path from different path separators")
		GIVEN("a relative path with forward slashes")
			char to[CDOGS_PATH_MAX];
			strcpy(to, "/tmp/path/to");

		WHEN("I get the relative path from another path with back slashes")
			char from[CDOGS_PATH_MAX];
			strcpy(from, "\\tmp\\path\\from");

		THEN("the result should be a relative path from one to the other")
			char rel[CDOGS_PATH_MAX];
			RelPath(rel, to, from);
			SHOULD_STR_EQUAL(rel, "../to");
	SCENARIO_END
FEATURE_END

CBEHAVE_RUN("Pic features are:", TEST_FEATURE(path_funcs))
