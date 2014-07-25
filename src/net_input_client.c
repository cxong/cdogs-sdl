/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.

    Copyright (c) 2014, Cong Xu
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
    Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.
*/
#include <cdogs/config.h>
#include <cdogs/events.h>
#include <cdogs/files.h>
#include <cdogs/net_input_client.h>

int main(int argc, char *argv[])
{
	UNUSED(argc);
	UNUSED(argv);

	if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) != 0)
	{
		fprintf(stderr, "Could not initialise SDL: %s\n", SDL_GetError());
		return -1;
	}
	if (SDLNet_Init() == -1)
	{
		fprintf(stderr, "SDLNet_Init: %s\n", SDLNet_GetError());
		exit(EXIT_FAILURE);
	}
	if (!SDL_SetVideoMode(320, 200, 0, 0))
	{
		fprintf(stderr, "Could not set video mode: %s\n", SDL_GetError());
		SDL_Quit();
		exit(-1);
	}

	ConfigLoadDefault(&gConfig);
	ConfigLoad(&gConfig, GetConfigFilePath(CONFIG_FILE));
	gLastConfig = gConfig;
	EventInit(&gEventHandlers, NULL, false);

	NetInputClient client;
	NetInputClientInit(&client);
	NetInputClientConnect(&client, 0x7F000001);	// localhost

	printf("Press esc to exit\n");

	Uint32 ticksNow = SDL_GetTicks();
	Uint32 ticksElapsed = 0;
	for (;;)
	{
		Uint32 ticksThen = ticksNow;
		ticksNow = SDL_GetTicks();
		ticksElapsed += ticksNow - ticksThen;
		if (ticksElapsed < 1000 / FPS_FRAMELIMIT)
		{
			SDL_Delay(1);
			debug(D_VERBOSE, "Delaying 1 ticksNow %u elapsed %u\n", ticksNow, ticksElapsed);
			continue;
		}
		EventPoll(&gEventHandlers, SDL_GetTicks());
		int cmd = GetOnePlayerCmd(
			&gEventHandlers,
			&gConfig.Input.PlayerKeys[0],
			false,
			INPUT_DEVICE_KEYBOARD,
			0);
		if (cmd)
		{
			printf("Sending %s + %s\n",
				CmdStr(cmd & (CMD_LEFT | CMD_RIGHT | CMD_UP | CMD_DOWN)),
				CmdStr(cmd & (CMD_BUTTON1 | CMD_BUTTON2 | CMD_BUTTON3 | CMD_BUTTON4 | CMD_ESC)));
			NetInputClientSend(&client, cmd);
		}

		// Check keyboard escape
		if (KeyIsPressed(&gEventHandlers.keyboard, SDLK_ESCAPE))
		{
			break;
		}

		ticksElapsed -= 1000 / FPS_FRAMELIMIT;
	}

	NetInputClientTerminate(&client);
	EventTerminate(&gEventHandlers);
	SDLNet_Quit();
	SDL_Quit();
	return 0;
}
