#include <cbehave/cbehave.h>

#include <pic.h>

#include <SDL_image.h>
#include <config.h>


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
void hqxInit(void)
{
	return;
}


FEATURE(1, "Pic load")
	SCENARIO("Load png")
		ASSERT(SDL_Init(SDL_INIT_VIDEO) == 0, 1);
		gConfig = ConfigDefault();
		ConfigResetDefault(ConfigGet(&gConfig, "Graphics"));
		GraphicsInit(&gGraphicsDevice, &gConfig);
		GraphicsInitialize(&gGraphicsDevice, false);
		ASSERT(gGraphicsDevice.IsInitialized, 1);
		GIVEN("a single pixel PNG")
			SDL_RWops *rwops = SDL_RWFromFile("r64g128b192.png", "rb");
			ASSERT(IMG_isPNG(rwops), 1);
			SDL_Surface *image = IMG_Load_RW(rwops, 0);
			SDL_LockSurface(image);
		GIVEN_END

		WHEN("I load the pic")
			Pic p;
			p.size = Vec2iNew(1, 1);
			p.offset = Vec2iZero();
			PicLoad(&p, p.size, Vec2iZero(), image);
		WHEN_END

		THEN("the loaded pic should have values that match");
			const color_t c = PIXEL2COLOR(p.Data[0]);
			SHOULD_INT_EQUAL(c.r, 64);
			SHOULD_INT_EQUAL(c.g, 128);
			SHOULD_INT_EQUAL(c.b, 192);
		THEN_END
		SDL_UnlockSurface(image);
		SDL_FreeSurface(image);
		rwops->close(rwops);
	SCENARIO_END
FEATURE_END

int main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	cbehave_feature features[] =
	{
		{feature_idx(1)}
	};
	
	return cbehave_runner("Pic features are:", features);
}
