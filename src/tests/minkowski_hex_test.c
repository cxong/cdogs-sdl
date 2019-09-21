#include <cbehave/cbehave.h>

#include <collision/minkowski_hex.h>

// Stubs
const char *JoyName(const int deviceIndex)
{
	UNUSED(deviceIndex);
	return NULL;
}


FEATURE(minkowski_hex, "Minkowski Hex")
	SCENARIO("Same position")
		GIVEN("two rectangles with the same position")
			const struct vec2 rectPos1 = svec2(3, 4);
			const struct vec2 rectVel1 = svec2(-4, 5);
			const struct vec2i rectSize1 = svec2i(1, 3);
			const struct vec2 rectPos2 = rectPos1;
			const struct vec2 rectVel2 = svec2(7, -8);
			const struct vec2i rectSize2 = svec2i(2, 1);

		WHEN("I check for their collision")
			struct vec2 collide1, collide2, normal;
			const bool result = MinkowskiHexCollide(
				rectPos1, rectVel1, rectSize1,
				rectPos2, rectVel2, rectSize2,
				&collide1, &collide2, &normal);

		THEN("the result should be true");
			SHOULD_BE_TRUE(result);
		AND("the collision points should be at the start")
			SHOULD_INT_EQUAL((int)collide1.x, (int)rectPos1.x);
			SHOULD_INT_EQUAL((int)collide1.y, (int)rectPos1.y);
			SHOULD_INT_EQUAL((int)collide2.x, (int)rectPos2.x);
			SHOULD_INT_EQUAL((int)collide2.y, (int)rectPos2.y);
		AND("the normal should be (0, 0)")
			SHOULD_INT_EQUAL((int)normal.x, 0);
			SHOULD_INT_EQUAL((int)normal.y, 0);
	SCENARIO_END

	SCENARIO("No overlap, no movement")
		GIVEN("two non-overlapping rectangles with no movement")
			const struct vec2 rectPos1 = svec2(3, 4);
			const struct vec2 rectVel1 = svec2_zero();
			const struct vec2i rectSize1 = svec2i(1, 3);
			const struct vec2 rectPos2 = svec2(8, 9);
			const struct vec2 rectVel2 = svec2_zero();
			const struct vec2i rectSize2 = svec2i(2, 1);

		WHEN("I check for their collision")
			struct vec2 collide1, collide2, normal;
			const bool result = MinkowskiHexCollide(
				rectPos1, rectVel1, rectSize1,
				rectPos2, rectVel2, rectSize2,
				&collide1, &collide2, &normal);

		THEN("the result should be false");
			SHOULD_BE_FALSE(result);
	SCENARIO_END

	SCENARIO("Single axis single movement")
		GIVEN("two rectangles, one moving into the other")
			const struct vec2 rectPos1 = svec2_zero();
			const struct vec2 rectVel1 = svec2(10, 0);
			const struct vec2i rectSize1 = svec2i(2, 2);
			const struct vec2 rectPos2 = svec2(5, 0);
			const struct vec2 rectVel2 = svec2_zero();
			const struct vec2i rectSize2 = svec2i(2, 2);

		WHEN("I check for their collision")
			struct vec2 collide1, collide2, normal;
			const bool result = MinkowskiHexCollide(
				rectPos1, rectVel1, rectSize1,
				rectPos2, rectVel2, rectSize2,
				&collide1, &collide2, &normal);

		THEN("the result should be true");
			SHOULD_BE_TRUE(result);
		AND("the collision point for the moving rectangle should not overlap "
			"with the stationary rectangle")
			SHOULD_INT_EQUAL((int)collide1.x, 3);
			SHOULD_INT_EQUAL((int)collide1.y, 0);
		AND("the collision point for the stationary rectangle should be the "
			"same as its position")
			SHOULD_INT_EQUAL((int)collide2.x, (int)rectPos2.x);
			SHOULD_INT_EQUAL((int)collide2.y, (int)rectPos2.y);
		AND("the normal should be (-1, 0)")
			SHOULD_INT_EQUAL((int)normal.x, -1);
			SHOULD_INT_EQUAL((int)normal.y, 0);
	SCENARIO_END
FEATURE_END

CBEHAVE_RUN(
	"minkowski_hex features are:",
	TEST_FEATURE(minkowski_hex)
)
