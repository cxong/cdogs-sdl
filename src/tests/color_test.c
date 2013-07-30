#include <cbehave/cbehave.h>

#include <color.h>


FEATURE(1, "Multiply")
	SCENARIO("Multiply two colors")
	{
		color_t c1, c2, result;
		GIVEN("two colors")
			c1.red = c1.green = c1.blue = 123;
			c2.red = c2.green = c2.blue = 234;
		GIVEN_END

		WHEN("I multiply them")
			result = ColorMult(c1, c2);
		WHEN_END

		THEN("the result should be a multiply blend of the two color's components");
			SHOULD_INT_EQUAL(result.red, c1.red * c2.red / 255);
			SHOULD_INT_EQUAL(result.green, c1.green * c2.green / 255);
			SHOULD_INT_EQUAL(result.blue, c1.blue * c2.blue / 255);
		THEN_END
	}
	SCENARIO_END

	SCENARIO("Multiply by white")
	{
		color_t c, white, result;
		GIVEN("a color")
			c.red = 123;
			c.green = 234;
			c.blue = 45;
		GIVEN_END

		WHEN("I multiply it by white")
			white.red = white.green = white.blue = 255;
			result = ColorMult(c, white);
		WHEN_END

		THEN("the result should be the same as the color");
			SHOULD_INT_EQUAL(result.red, c.red);
			SHOULD_INT_EQUAL(result.green, c.green);
			SHOULD_INT_EQUAL(result.blue, c.blue);
		THEN_END
	}
	SCENARIO_END

	SCENARIO("Multiply by black")
	{
		color_t c, black, result;
		GIVEN("a color")
			c.red = 123;
			c.green = 234;
			c.blue = 45;
		GIVEN_END

		WHEN("I multiply it by black")
			black.red = black.green = black.blue = 0;
			result = ColorMult(c, black);
		WHEN_END

		THEN("the result should be black");
			SHOULD_INT_EQUAL(result.red, black.red);
			SHOULD_INT_EQUAL(result.green, black.green);
			SHOULD_INT_EQUAL(result.blue, black.blue);
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
