#include <cbehave/cbehave.h>

#include <json_utils.h>


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
int PicManagerGetPic(void) { return 0; }
int PicManagerGetFromOld(void) { return 0; }
int StrGunDescription(void) { return 0; }
Config gConfig;
int gPicManager;


FEATURE(1, "String format")
	SCENARIO("Quote handling")
	{
		GIVEN("a JSON structure containing escaped quotes")
			json_t *root = json_new_object();
			AddStringPair(root, "Foo", "bar \"baz\"");
			AddStringPair(root, "Spam", "I like \"spam\" and \"eggs\"");
		GIVEN_END

		WHEN("I format the structure into a string")
			char *text;
			json_tree_to_string(root, &text);
			char *ftext = json_format_string(text);
		WHEN_END

		THEN("the result should be valid JSON and contain the same strings");
			json_t *parsed = NULL;
			SHOULD_INT_EQUAL(
				(int)json_parse_document(&parsed, ftext), (int)JSON_OK);
			SHOULD_BE_TRUE(strstr(ftext, "I like \\\"spam\\\" and \\\"eggs\\\"") != NULL);
		THEN_END
		CFREE(text);
		CFREE(ftext);
		json_free_value(&root);
		json_free_value(&parsed);
	}
	SCENARIO_END
FEATURE_END

int main(void)
{
	cbehave_feature features[] =
	{
		{feature_idx(1)}
	};
	
	return cbehave_runner("JSON features are:", features);
}
