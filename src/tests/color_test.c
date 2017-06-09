#include <cbehave/cbehave.h>

#include <color.h>

#include <float.h>


FEATURE(ColorMult, "Multiply")
	SCENARIO("Multiply two colors")
		GIVEN("two colors")
			color_t c1, c2;
			c1.r = c1.g = c1.b = 123;
			c2.r = c2.g = c2.b = 234;

		WHEN("I multiply them")
			color_t result = ColorMult(c1, c2);

		THEN("the result should be a multiply blend of the two color's components");
			SHOULD_INT_EQUAL(result.r, c1.r * c2.r / 255);
			SHOULD_INT_EQUAL(result.g, c1.g * c2.g / 255);
			SHOULD_INT_EQUAL(result.b, c1.b * c2.b / 255);
	SCENARIO_END

	SCENARIO("Multiply by white")
		GIVEN("a color")
			color_t c;
			c.r = 123;
			c.g = 234;
			c.b = 45;

		WHEN("I multiply it by white")
			color_t white;
			white.r = white.g = white.b = 255;
			color_t result = ColorMult(c, white);

		THEN("the result should be the same as the color")
			SHOULD_INT_EQUAL(result.r, c.r);
			SHOULD_INT_EQUAL(result.g, c.g);
			SHOULD_INT_EQUAL(result.b, c.b);
	SCENARIO_END

	SCENARIO("Multiply by black")
		GIVEN("a color")
			color_t c;
			c.r = 123;
			c.g = 234;
			c.b = 45;

		WHEN("I multiply it by black")
			color_t black;
			black.r = black.g = black.b = 0;
			color_t result = ColorMult(c, black);

		THEN("the result should be black")
			SHOULD_INT_EQUAL(result.r, black.r);
			SHOULD_INT_EQUAL(result.g, black.g);
			SHOULD_INT_EQUAL(result.b, black.b);
	SCENARIO_END
FEATURE_END

FEATURE(ColorAlphaBlend, "Alpha blend")
	SCENARIO("Alpha blend two colors")
		GIVEN("two colors")
			color_t c1, c2;
			c1.r = c1.g = c1.b = c1.a = 123;
			c2.r = c2.g = c2.b = c2.a = 234;

		WHEN("I alpha blend them")
			color_t result = ColorAlphaBlend(c1, c2);

		THEN("the result should be an alpha blend of the two color's components")
			SHOULD_INT_EQUAL(result.r, ((int)c1.r*(255 - c2.a) + (int)c2.r*c2.a)/255);
			SHOULD_INT_EQUAL(result.g, ((int)c1.g*(255 - c2.a) + (int)c2.g*c2.a)/255);
			SHOULD_INT_EQUAL(result.b, ((int)c1.r*(255 - c2.a) + (int)c2.b*c2.a)/255);
			SHOULD_INT_EQUAL(result.a, 255);
	SCENARIO_END

	SCENARIO("Alpha blend with full transparent")
		GIVEN("a color")
			color_t c;
			c.r = 123;
			c.g = 234;
			c.b = 45;
			c.a = 42;

		WHEN("I alpha blend it by transparent")
			color_t transparent;
			transparent.r = transparent.g = transparent.b = 255;
			transparent.a = 0;
			color_t result = ColorAlphaBlend(c, transparent);

		THEN("the result should be the same as the color")
			SHOULD_INT_EQUAL(result.r, c.r);
			SHOULD_INT_EQUAL(result.g, c.g);
			SHOULD_INT_EQUAL(result.b, c.b);
			SHOULD_INT_EQUAL(result.a, 255);
	SCENARIO_END

	SCENARIO("Alpha blend with full opaque black")
		GIVEN("a color")
			color_t c;
			c.r = 123;
			c.g = 234;
			c.b = 45;
			c.a = 194;

		WHEN("I alpha blend it by opaque black")
			color_t black;
			black.r = black.g = black.b = 0;
			black.a = 255;
			color_t result = ColorAlphaBlend(c, black);

		THEN("the result should be black")
			SHOULD_INT_EQUAL(result.r, black.r);
			SHOULD_INT_EQUAL(result.g, black.g);
			SHOULD_INT_EQUAL(result.b, black.b);
			SHOULD_INT_EQUAL(result.a, black.a);
	SCENARIO_END
FEATURE_END

FEATURE(ColorTint, "Tint")
	SCENARIO("Tint to gray (0 saturation)")
		GIVEN("a color")
			color_t c;
			c.r = 123;
			c.g = 234;
			c.b = 45;

		WHEN("I tint it with 0 saturation")
			HSV hsv;
			hsv.h = 0.0;
			hsv.s = 0.0;
			hsv.v = 1.0;
			c = ColorTint(c, hsv);

		THEN("the result should have equal RGB components")
			SHOULD_INT_EQUAL(c.r, c.g);
			SHOULD_INT_EQUAL(c.g, c.b);
	SCENARIO_END

	SCENARIO("Tint to white (max value)")
		GIVEN("a color")
			color_t c;
			c.r = 123;
			c.g = 234;
			c.b = 45;

		WHEN("I tint it with max value and no hue")
			HSV hsv;
			hsv.h = -1.0;
			hsv.s = 1.0;
			hsv.v = DBL_MAX;
			c = ColorTint(c, hsv);

		THEN("the result should be white")
			SHOULD_INT_EQUAL(c.r, 255);
			SHOULD_INT_EQUAL(c.g, 255);
			SHOULD_INT_EQUAL(c.b, 255);
	SCENARIO_END

	SCENARIO("Tint to red")
		GIVEN("a color")
			color_t c;
			c.r = 123;
			c.g = 234;
			c.b = 45;

		WHEN("I tint it with the red hue")
			HSV hsv;
			hsv.h = 0.0;
			hsv.s = 1.0;
			hsv.v = 1.0;
			c = ColorTint(c, hsv);

		THEN("the result should be red")
			SHOULD_INT_GT(c.r, 0);
			SHOULD_INT_EQUAL(c.g, 0);
			SHOULD_INT_EQUAL(c.b, 0);
	SCENARIO_END

	SCENARIO("Tint with nothing")
		GIVEN("a color")
			color_t c;
			c.r = 123;
			c.g = 234;
			c.b = 45;

		WHEN("I tint it with null values (-1 hue)")
			HSV hsv;
			hsv.h = -1.0;
			hsv.s = 1.0;
			hsv.v = 1.0;
			color_t result = ColorTint(c, hsv);

		THEN("the result should be the same as the original color")
			SHOULD_MEM_EQUAL(&result, &c, sizeof result);
	SCENARIO_END
FEATURE_END

FEATURE(StrColor, "String conversion")
	SCENARIO("Convert from hex")
		GIVEN("a hex string")
			const char *str = "6495ED";

		WHEN("I convert it into a color")
			color_t result = StrColor(str);

		THEN("the result should be the expected color")
			color_t expected = { 100, 149, 237, 255 };
			SHOULD_BE_TRUE(ColorEquals(result, expected));
	SCENARIO_END
	
	SCENARIO("Convert invalid string")
		GIVEN("an invalid string")
			const char *str = "unknown";

		WHEN("I convert it into a color")
			color_t result = StrColor(str);

		THEN("the result should be black")
			SHOULD_BE_TRUE(ColorEquals(result, colorBlack));
	SCENARIO_END

	SCENARIO("Convert to hex")
		GIVEN("a colour")
			color_t c = { 0x64, 0x95, 0xed, 0xff };

		WHEN("I convert it to hex")
			char buf[8];
			ColorStr(buf, c);

		THEN("the result should be the expected hex string")
			const char *expected = "6495ed";
			SHOULD_STR_EQUAL(buf, expected);
	SCENARIO_END
FEATURE_END

CBEHAVE_RUN(
	"Color features are:",
	TEST_FEATURE(ColorMult),
	TEST_FEATURE(ColorAlphaBlend),
	TEST_FEATURE(ColorTint),
	TEST_FEATURE(StrColor)
)
