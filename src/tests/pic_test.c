#define SDL_MAIN_HANDLED
#include <cbehave/cbehave.h>

#include <pic.h>

#include <SDL_image.h>
#include <config.h>
#include <grafx.h>


// Stubs
void BlitBackground(
	GraphicsDevice *device,
	const Pic *pic, Vec2i pos, const HSV *tint, const bool isTransparent)
{
	UNUSED(device);
	UNUSED(pic);
	UNUSED(pos);
	UNUSED(tint);
	UNUSED(isTransparent);
}
void BlitMasked(
	GraphicsDevice *device,
	const Pic *pic,
	Vec2i pos,
	color_t mask,
	int isTransparent)
{
	UNUSED(device);
	UNUSED(pic);
	UNUSED(pos);
	UNUSED(mask);
	UNUSED(isTransparent);
}
const char *JoyName(const int deviceIndex)
{
	UNUSED(deviceIndex);
	return NULL;
}
color_t PaletteToColor(unsigned char idx)
{
	UNUSED(idx);
	return colorBlack;
}
int ConfigGetJSONVersion(FILE *f)
{
	UNUSED(f);
	return 0;
}
bool ConfigIsOld(FILE *f)
{
	UNUSED(f);
	return false;
}
void DrawRectangle(
	GraphicsDevice *device, Vec2i pos, Vec2i size, color_t color, int flags)
{
	UNUSED(device);
	UNUSED(pos);
	UNUSED(size);
	UNUSED(color);
	UNUSED(flags);
}


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
			SDL_RWops *rwops = SDL_RWFromFile("r64g128b192.png", "rb");
			ASSERT(IMG_isPNG(rwops), 1);
			SDL_Surface *image = IMG_Load_RW(rwops, 0);
			SDL_LockSurface(image);

		WHEN("I load the pic")
			Pic p;
			p.size = Vec2iNew(1, 1);
			p.offset = Vec2iZero();
			PicLoad(&p, p.size, Vec2iZero(), image);

		THEN("the loaded pic should have values that match");
			const color_t c = PIXEL2COLOR(p.Data[0]);
			SHOULD_INT_EQUAL(c.r, 64);
			SHOULD_INT_EQUAL(c.g, 128);
			SHOULD_INT_EQUAL(c.b, 192);
		SDL_UnlockSurface(image);
		SDL_FreeSurface(image);
		rwops->close(rwops);
	SCENARIO_END
FEATURE_END

CBEHAVE_RUN("Pic features are:", TEST_FEATURE(PicLoad))
