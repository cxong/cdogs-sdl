#define SDL_MAIN_HANDLED
#include <cbehave/cbehave.h>

#include <pic.h>
#include <config.h>
#include <grafx.h>


FEATURE(PicLoad, "Pic load")
	SCENARIO("Load png")
		GIVEN("a graphics context")
			if (SDL_Init(SDL_INIT_VIDEO) != 0)
			{
				printf("Failed to init SDL: %s\n", SDL_GetError());
				ASSERT(false, 1);
			}
			gConfig = ConfigDefault();
			ConfigResetDefault(ConfigGet(&gConfig, "Graphics"));
			GraphicsInit(&gGraphicsDevice, &gConfig);
			GraphicsInitialize(&gGraphicsDevice);
			ASSERT(gGraphicsDevice.IsInitialized, 1);
		AND("a single pixel PNG")
			SDL_Surface *image = LoadImgToSurface("r64g128b192.png");
			SDL_LockSurface(image);

		WHEN("I load the pic")
			Pic p;
			p.size = svec2i(1, 1);
			p.offset = svec2i_zero();
			PicLoad(&p, p.size, svec2i_zero(), image, false);

		THEN("the loaded pic should have values that match");
			const color_t c = PIXEL2COLOR(p.Data[0]);
			SHOULD_INT_EQUAL(c.r, 64);
			SHOULD_INT_EQUAL(c.g, 128);
			SHOULD_INT_EQUAL(c.b, 192);
		SDL_UnlockSurface(image);
		SDL_FreeSurface(image);
	SCENARIO_END
FEATURE_END

CBEHAVE_RUN("Pic features are:", TEST_FEATURE(PicLoad))
