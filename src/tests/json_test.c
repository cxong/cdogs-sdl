#define SDL_MAIN_HANDLED
#include <cbehave/cbehave.h>

#include <json_utils.h>

#include <config.h>


// Stubs
Mix_Chunk *StrSound(const char *s)
{
	UNUSED(s);
	return NULL;
}
const char *JoyName(const int deviceIndex)
{
	UNUSED(deviceIndex);
	return NULL;
}
bool ConfigGetBool(Config *c, const char *name)
{
	UNUSED(c);
	UNUSED(name);
	return false;
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
int PicManagerGetPic(void) { return 0; }
int StrWeaponClass(void) { return 0; }
Config gConfig;
int gPicManager;


FEATURE(json_format_string, "String format")
	SCENARIO("Quote handling")
		GIVEN("a JSON structure containing escaped quotes")
			json_t *root = json_new_object();
			AddStringPair(root, "Foo", "bar \"baz\"");
			AddStringPair(root, "Spam", "I like \"spam\" and \"eggs\"");

		WHEN("I format the structure into a string")
			char *text;
			json_tree_to_string(root, &text);
			char *ftext = json_format_string(text);

		THEN("the result should be valid JSON")
			json_t *parsed = NULL;
			SHOULD_INT_EQUAL(
				(int)json_parse_document(&parsed, ftext), (int)JSON_OK);
		AND("contain the same strings")
			SHOULD_BE_TRUE(strstr(ftext, "I like \\\"spam\\\" and \\\"eggs\\\"") != NULL);
		CFREE(text);
		CFREE(ftext);
		json_free_value(&root);
		json_free_value(&parsed);
	SCENARIO_END
FEATURE_END

CBEHAVE_RUN("JSON features are:", TEST_FEATURE(json_format_string))
