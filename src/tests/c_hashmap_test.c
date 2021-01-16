#include <cbehave/cbehave.h>

#include <stdlib.h>
#include <string.h>
#include <c_hashmap/hashmap.h>


// All tests in this file adapted from example code in the original c_hashmap
// package's main.c


FEATURE(hashmap_put, "Hashmap put")
	SCENARIO("Put one value")
		GIVEN("a new hashmap")
			map_t map = hashmap_new();

		WHEN("I put a new value in")
			int value = 42;
			int error = hashmap_put(map, "somekey", &value);

		THEN("the operation should be successful");
			SHOULD_INT_EQUAL(error, (int)MAP_OK);

		hashmap_free(map);
	SCENARIO_END
FEATURE_END

FEATURE(hashmap_get, "Hashmap get")
	SCENARIO("Get an existing value")
		GIVEN("a hashmap with a value")
			map_t map = hashmap_new();
			int value = 42;
			hashmap_put(map, "somekey", &value);

		WHEN("I get it via its key")
			int *valueOut;
			int error = hashmap_get(map, "somekey", (void **)&valueOut);

		THEN("the operation should be successful")
			SHOULD_INT_EQUAL(error, (int)MAP_OK);
		AND("the value should match");
			SHOULD_INT_EQUAL(value, *valueOut);

		hashmap_free(map);
	SCENARIO_END

	SCENARIO("Get non-existing value")
		GIVEN("a hashmap with a value")
			map_t map = hashmap_new();
			int value = 42;
			hashmap_put(map, "somekey", &value);

		WHEN("I get a non-existing key")
			int *valueOut;
			int error = hashmap_get(map, "notkey", (void **)&valueOut);

		THEN("the operation should be unsuccessful")
			SHOULD_INT_EQUAL(error, (int)MAP_MISSING);

		hashmap_free(map);
	SCENARIO_END
FEATURE_END

FEATURE(hashmap_remove, "Hashmap remove")
	SCENARIO("Remove an existing value")
		GIVEN("a hashmap with a value")
			map_t map = hashmap_new();
			int value = 42;
			hashmap_put(map, "somekey", &value);

		WHEN("I remove it via its key")
			int error = hashmap_remove(map, "somekey");

		THEN("the operation should be successful")
			SHOULD_INT_EQUAL(error, (int)MAP_OK);

		hashmap_free(map);
	SCENARIO_END
FEATURE_END

static int copy_key(any_t data, any_t key)
{
    char **keys = (char **)data;
    while (*keys != NULL) keys++;
    *keys = strdup(key);
    return MAP_OK;
}
FEATURE(hashmap_iterate_keys_sorted, "Hashmap iterate keys sorted")
    SCENARIO("Iterate a hashmap sorted by keys")
        GIVEN("a hashmap with keys a, b, c")
            map_t map = hashmap_new();
            int value3 = 3;
            hashmap_put(map, "c", &value3);
            int value2 = 2;
            hashmap_put(map, "b", &value2);
            int value1 = 1;
            hashmap_put(map, "a", &value1);

        WHEN("I iterate it by keys sorted")
            char *keys[3];
            memset(keys, 0, sizeof keys);
            const int error = hashmap_iterate_keys_sorted(map, copy_key, keys);

        THEN("the keys should be in order")
            SHOULD_INT_EQUAL(error, MAP_OK);
            SHOULD_STR_EQUAL(keys[0], "a");
            SHOULD_STR_EQUAL(keys[1], "b");
            SHOULD_STR_EQUAL(keys[2], "c");

        for (int i = 0; i < 3; i++) free(keys[i]);
        hashmap_free(map);
    SCENARIO_END
FEATURE_END

CBEHAVE_RUN(
	"c_hashmap features are:",
	TEST_FEATURE(hashmap_put),
	TEST_FEATURE(hashmap_get),
	TEST_FEATURE(hashmap_remove),
    TEST_FEATURE(hashmap_sort)
)
