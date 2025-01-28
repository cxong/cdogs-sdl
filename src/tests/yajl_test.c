#define SDL_MAIN_HANDLED
#include <cbehave/cbehave.h>

#include <yajl_utils.h>

FEATURE(string_pair, "String pair")
	SCENARIO("String pair")
		GIVEN("a string key/value pair")
			const char *key = "key";
			const char *value = "value";

		WHEN("I add the pair")
			yajl_gen g = yajl_gen_alloc(NULL);
			yajl_gen_map_open(g);
			YAJLAddStringPair(g, key, value);
			yajl_gen_map_close(g);
			const unsigned char *buf;
			size_t len;
			yajl_gen_get_buf(g, &buf, &len);
		AND("I load the document back")
			yajl_val node = yajl_tree_parse((const char *)buf, NULL, 0);

		THEN("the loaded value should be the same")
			char *loadedValue = YAJLGetStr(node, key);
			SHOULD_STR_EQUAL(loadedValue, value);
		CFREE(loadedValue);
		yajl_tree_free(node);
		yajl_gen_clear(g);
		yajl_gen_free(g);
	SCENARIO_END
FEATURE_END

CBEHAVE_RUN("YAJL features are:", TEST_FEATURE(string_pair))
