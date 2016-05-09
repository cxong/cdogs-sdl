#include <cbehave/cbehave.h>

#include <c_hashmap/hashmap.h>


// All tests in this file adapted from example code in the original c_hashmap
// package's main.c


FEATURE(1, "Hashmap put")
	SCENARIO("Put one value")
		map_t map;
		GIVEN("a new hashmap")
			map = hashmap_new();

		int value = 42;
		int error;
		WHEN("I put a new value in")
			error = hashmap_put(map, "somekey", &value);

		THEN("the operation should be successful");
			SHOULD_INT_EQUAL(error, (int)MAP_OK);

		hashmap_free(map);
	SCENARIO_END
FEATURE_END

FEATURE(2, "Hashmap get")
	SCENARIO("Get an existing value")
		map_t map;
		int value = 42;
		GIVEN("a hashmap with a value")
			map = hashmap_new();
			hashmap_put(map, "somekey", &value);

		int error;
		int *valueOut;
		WHEN("I get it via its key")
			error = hashmap_get(map, "somekey", (void **)&valueOut);

		THEN("the operation should be successful")
			SHOULD_INT_EQUAL(error, (int)MAP_OK);
		AND("the value should match");
			SHOULD_INT_EQUAL(value, *valueOut);

		hashmap_free(map);
	SCENARIO_END

	SCENARIO("Get non-existing value")
		map_t map;
		int value = 42;
		GIVEN("a hashmap with a value")
			map = hashmap_new();
			hashmap_put(map, "somekey", &value);

		int error;
		int *valueOut;
		WHEN("I get a non-existing key")
			error = hashmap_get(map, "notkey", (void **)&valueOut);

		THEN("the operation should be unsuccessful")
			SHOULD_INT_EQUAL(error, (int)MAP_MISSING);

		hashmap_free(map);
	SCENARIO_END
FEATURE_END

FEATURE(3, "Hashmap remove")
	SCENARIO("Remove an existing value")
		map_t map;
		int value = 42;
		GIVEN("a hashmap with a value")
			map = hashmap_new();
			hashmap_put(map, "somekey", &value);

		int error;
		WHEN("I remove it via its key")
			error = hashmap_remove(map, "somekey");

		THEN("the operation should be successful")
			SHOULD_INT_EQUAL(error, (int)MAP_OK);

		hashmap_free(map);
	SCENARIO_END
FEATURE_END

int main(void)
{
	cbehave_feature features[] =
	{
		{feature_idx(1)},
		{feature_idx(2)},
		{feature_idx(3)}
	};
	
	return cbehave_runner("c_hashmap features are:", features);
}
