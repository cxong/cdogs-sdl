#define SDL_MAIN_HANDLED 1
#include <cbehave/cbehave.h>

#include <utils.h>

#include <SDL_joystick.h>

#include <sys_config.h>
#include <sys_specifics.h>

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

#ifdef _MSC_VER
	SCENARIO("Relative path to a different Windows drive")
		GIVEN("a path to a drive")
			char to[CDOGS_PATH_MAX];
			strcpy(to, "D:\\foo");

		WHEN("I get the relative path from a different drive")
			char from[CDOGS_PATH_MAX];
			strcpy(from, "C:\\foo");

		THEN("the result should be an absolute path")
			char rel[CDOGS_PATH_MAX];
			RelPath(rel, to, from);
			SHOULD_STR_EQUAL(rel, "D:\\foo");
	SCENARIO_END
#endif
FEATURE_END

CBEHAVE_RUN("Pic features are:", TEST_FEATURE(path_funcs))
