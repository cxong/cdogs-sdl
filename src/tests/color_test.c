#include <cbehave/cbehave.h>

#include <color.h>

#include <float.h>


FEATURE(1, "Multiply")
	SCENARIO("Multiply two colors")
	{
		color_t c1, c2, result;
		GIVEN("two colors")
			c1.r = c1.g = c1.b = 123;
			c2.r = c2.g = c2.b = 234;
		GIVEN_END

		WHEN("I multiply them")
			result = ColorMult(c1, c2);
		WHEN_END

		THEN("the result should be a multiply blend of the two color's components");
			SHOULD_INT_EQUAL(result.r, c1.r * c2.r / 255);
			SHOULD_INT_EQUAL(result.g, c1.g * c2.g / 255);
			SHOULD_INT_EQUAL(result.b, c1.b * c2.b / 255);
		THEN_END
	}
	SCENARIO_END

	SCENARIO("Multiply by white")
	{
		color_t c, white, result;
		GIVEN("a color")
			c.r = 123;
			c.g = 234;
			c.b = 45;
		GIVEN_END

		WHEN("I multiply it by white")
			white.r = white.g = white.b = 255;
			result = ColorMult(c, white);
		WHEN_END

		THEN("the result should be the same as the color");
			SHOULD_INT_EQUAL(result.r, c.r);
			SHOULD_INT_EQUAL(result.g, c.g);
			SHOULD_INT_EQUAL(result.b, c.b);
		THEN_END
	}
	SCENARIO_END

	SCENARIO("Multiply by black")
	{
		color_t c, black, result;
		GIVEN("a color")
			c.r = 123;
			c.g = 234;
			c.b = 45;
		GIVEN_END

		WHEN("I multiply it by black")
			black.r = black.g = black.b = 0;
			result = ColorMult(c, black);
		WHEN_END

		THEN("the result should be black");
			SHOULD_INT_EQUAL(result.r, black.r);
			SHOULD_INT_EQUAL(result.g, black.g);
			SHOULD_INT_EQUAL(result.b, black.b);
		THEN_END
	}
	SCENARIO_END
FEATURE_END

FEATURE(2, "Alpha blend")
	SCENARIO("Alpha blend two colors")
	{
		color_t c1, c2, result;
		GIVEN("two colors")
			c1.r = c1.g = c1.b = c1.a = 123;
			c2.r = c2.g = c2.b = c2.a = 234;
		GIVEN_END

		WHEN("I alpha blend them")
			result = ColorAlphaBlend(c1, c2);
		WHEN_END

		THEN("the result should be an alpha blend of the two color's components");
			SHOULD_INT_EQUAL(result.r, ((int)c1.r*(255 - c2.a) + (int)c2.r*c2.a)/255);
			SHOULD_INT_EQUAL(result.g, ((int)c1.g*(255 - c2.a) + (int)c2.g*c2.a)/255);
			SHOULD_INT_EQUAL(result.b, ((int)c1.r*(255 - c2.a) + (int)c2.b*c2.a)/255);
			SHOULD_INT_EQUAL(result.a, 255);
		THEN_END
	}
	SCENARIO_END

	SCENARIO("Alpha blend with full transparent")
	{
		color_t c, transparent, result;
		GIVEN("a color")
			c.r = 123;
			c.g = 234;
			c.b = 45;
			c.a = 42;
		GIVEN_END

		WHEN("I alpha blend it by transparent")
			transparent.r = transparent.g = transparent.b = 255;
			transparent.a = 0;
			result = ColorAlphaBlend(c, transparent);
		WHEN_END

		THEN("the result should be the same as the color");
			SHOULD_INT_EQUAL(result.r, c.r);
			SHOULD_INT_EQUAL(result.g, c.g);
			SHOULD_INT_EQUAL(result.b, c.b);
			SHOULD_INT_EQUAL(result.a, 255);
		THEN_END
	}
	SCENARIO_END

	SCENARIO("Alpha blend with full opaque black")
	{
		color_t c, black, result;
		GIVEN("a color")
			c.r = 123;
			c.g = 234;
			c.b = 45;
			c.a = 194;
		GIVEN_END

		WHEN("I alpha blend it by opaque black")
			black.r = black.g = black.b = 0;
			black.a = 255;
			result = ColorAlphaBlend(c, black);
		WHEN_END

		THEN("the result should be black");
			SHOULD_INT_EQUAL(result.r, black.r);
			SHOULD_INT_EQUAL(result.g, black.g);
			SHOULD_INT_EQUAL(result.b, black.b);
			SHOULD_INT_EQUAL(result.a, black.a);
		THEN_END
	}
	SCENARIO_END
FEATURE_END

FEATURE(3, "Tint")
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

FEATURE(4, "String conversion")
	SCENARIO("Convert from hex")
		GIVEN("a hex string")
			const char *str = "6495ED";
		GIVEN_END
		WHEN("I convert it into a color")
			color_t result = StrColor(str);
		WHEN_END
		THEN("the result should be the expected color")
			color_t expected = { 100, 149, 237, 255 };
			SHOULD_BE_TRUE(ColorEquals(result, expected));
		THEN_END
	SCENARIO_END
	
	SCENARIO("Convert invalid string")
		GIVEN("an invalid string")
			const char *str = "unknown";
		GIVEN_END
		WHEN("I convert it into a color")
			color_t result = StrColor(str);
		WHEN_END
		THEN("the result should be black")
			SHOULD_BE_TRUE(ColorEquals(result, colorBlack));
		THEN_END
	SCENARIO_END

	SCENARIO("Convert to hex")
		GIVEN("a colour")
			color_t c = { 0x64, 0x95, 0xed, 0xff };
		GIVEN_END
		WHEN("I convert it to hex")
			char buf[8];
			ColorStr(buf, c);
		WHEN_END
		THEN("the result should be the expected hex string")
			const char *expected = "6495ed";
			SHOULD_STR_EQUAL(buf, expected);
		THEN_END
	SCENARIO_END
FEATURE_END

int main(void)
{
	cbehave_feature features[] =
	{
		{feature_idx(1)},
		{feature_idx(2)},
		{feature_idx(3)},
		{feature_idx(4)}
	};
	
	return cbehave_runner("Color features are:", features);
}
