#include <cbehave/cbehave.h>

#include <color.h>

#include <float.h>


FEATURE(1, "Tint")
	SCENARIO("Tint to gray (0 saturation)")
	{
		color_t c;
		HSV hsv;
		GIVEN("a color")
			c.r = 123;
			c.g = 234;
			c.b = 45;
		GIVEN_END

		WHEN("I tint it with 0 saturation")
			hsv.h = 0.0;
			hsv.s = 0.0;
			hsv.v = 1.0;
			c = ColorTint(c, hsv);
		WHEN_END

		THEN("the result should have equal RGB components");
			SHOULD_INT_EQUAL(c.r, c.g);
			SHOULD_INT_EQUAL(c.g, c.b);
		THEN_END
	}
	SCENARIO_END

	SCENARIO("Tint to white (max value)")
	{
		color_t c;
		HSV hsv;
		GIVEN("a color")
			c.r = 123;
			c.g = 234;
			c.b = 45;
		GIVEN_END

		WHEN("I tint it with max value and no hue")
			hsv.h = -1.0;
			hsv.s = 1.0;
			hsv.v = DBL_MAX;
			c = ColorTint(c, hsv);
		WHEN_END

		THEN("the result should be white");
			SHOULD_INT_EQUAL(c.r, 255);
			SHOULD_INT_EQUAL(c.g, 255);
			SHOULD_INT_EQUAL(c.b, 255);
		THEN_END
	}
	SCENARIO_END

	SCENARIO("Tint to red")
	{
		color_t c;
		HSV hsv;
		GIVEN("a color")
			c.r = 123;
			c.g = 234;
			c.b = 45;
		GIVEN_END

		WHEN("I tint it with the red hue")
			hsv.h = 0.0;
			hsv.s = 1.0;
			hsv.v = 1.0;
			c = ColorTint(c, hsv);
		WHEN_END

		THEN("the result should be red");
			SHOULD_INT_GT(c.r, 0);
			SHOULD_INT_EQUAL(c.g, 0);
			SHOULD_INT_EQUAL(c.b, 0);
		THEN_END
	}
	SCENARIO_END

	SCENARIO("Tint with nothing")
	{
		color_t c, result;
		HSV hsv;
		GIVEN("a color")
			c.r = 123;
			c.g = 234;
			c.b = 45;
		GIVEN_END

		WHEN("I tint it with null values (-1 hue)")
			hsv.h = -1.0;
			hsv.s = 1.0;
			hsv.v = 1.0;
			result = ColorTint(c, hsv);
		WHEN_END

		THEN("the result should be the same as the original color");
			SHOULD_MEM_EQUAL(&result, &c, sizeof result);
		THEN_END
	}
	SCENARIO_END
FEATURE_END

int main(void)
{
	cbehave_feature features[] =
	{
		{feature_idx(1)}
	};
	
	return cbehave_runner("Color features are:", features);
}
