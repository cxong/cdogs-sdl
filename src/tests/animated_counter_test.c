#define SDL_MAIN_HANDLED
#include <cbehave/cbehave.h>

#include <animated_counter.h>


FEATURE(AnimatedCounterUpdate, "Update animated counter")
	SCENARIO("Normal update")
		GIVEN("an animated counter")
			const int max = 100;
			AnimatedCounter c = AnimatedCounterNew("foo", max);

		WHEN("I update it")
			AnimatedCounterUpdate(&c, 1);

		THEN("the current counter should be greater than 0")
			SHOULD_INT_GT(c.current, 0);
		AND("less than the max")
			SHOULD_INT_LT(c.current, max);
	SCENARIO_END

	SCENARIO("Negative max")
		GIVEN("an animated counter with negative max")
			const int max = -100;
			AnimatedCounter c = AnimatedCounterNew("foo", max);

		WHEN("I update it")
			AnimatedCounterUpdate(&c, 1);

		THEN("the current counter should be less than 0")
			SHOULD_INT_LT(c.current, 0);
		AND("greater than than the max")
			SHOULD_INT_GT(c.current, max);
	SCENARIO_END
FEATURE_END

CBEHAVE_RUN(
	"Animated counter features are:",
	TEST_FEATURE(AnimatedCounterUpdate)
)
