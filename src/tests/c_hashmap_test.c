#include <cbehave/cbehave.h>

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

CBEHAVE_RUN(
	"c_hashmap features are:",
	TEST_FEATURE(hashmap_put),
	TEST_FEATURE(hashmap_get),
	TEST_FEATURE(hashmap_remove)
)
