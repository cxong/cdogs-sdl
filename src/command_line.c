/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2013-2016, 2019-2021, 2024 Cong Xu
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
#include "command_line.h"

#include <stdio.h>

#include <SDL_mixer.h>
#include <SDL_version.h>

#include <cdogs/XGetopt.h>
#include <cdogs/config.h>
#include <cdogs/log.h>
#include <cdogs/sys_config.h>
#include <cdogs/utils.h>

void PrintTitle(void)
{
	printf("C-Dogs SDL %s\n", CDOGS_SDL_VERSION);
	printf(
		"SDL version %d.%d.%d\n", SDL_MAJOR_VERSION, SDL_MINOR_VERSION,
		SDL_PATCHLEVEL);
	printf(
		"SDL_mixer version %d.%d.%d\n", SDL_MIXER_MAJOR_VERSION,
		SDL_MIXER_MINOR_VERSION, SDL_MIXER_PATCHLEVEL);
}

void PrintHelp(void)
{
	printf(
		"%s\n", "Video Options:\n"
				"    --fullscreen     Try and use a fullscreen video mode.\n"
				"    --scale=n        Set pixel size to a factor of n\n"
				"                       Factors: 1 - 16\n"
				"    --screen=WxH     Set window size to W x H\n");

	printf(
		"%s\n",
		"Config:\n"
		"    --config=K,V     Set arbitrary config option\n"
		"                     Example: --config=Game.FriendlyFire=true\n"
		"    --config         List all config options\n");

	printf(
		"Logging: logging is enabled per module and set at certain levels.\n"
		"Log modules are: ");
	for (int i = 0; i < (int)LM_COUNT; i++)
	{
		printf("%s", LogModuleName((LogModule)i));
		if (i < (int)LM_COUNT - 1)
			printf(", ");
		else
			printf("\n");
	}
	printf("Log levels are: ");
	for (int i = 0; i < (int)LL_COUNT; i++)
	{
		printf("%s", LogLevelName((LogLevel)i));
		if (i < (int)LL_COUNT - 1)
			printf(", ");
		else
			printf("\n");
	}
	printf(
		"    --log=M,L        Enable logging for module M at level L.\n\n"
		"    --log=L          Enable logging for all modules at level L.\n\n"
		"    --logfile=F      Log to file by filename\n\n");

	printf(
		"%s\n",
		"Other:\n"
		"    --connect=host   (Experimental) connect to a game server\n"
		"    --demo           (Experimental) run game for 30 seconds\n");
}

void ProcessCommandLine(char *buf, const int argc, char *argv[])
{
	buf[0] = '\0';
	for (int i = 0; i < argc; i++)
	{
		strcat(buf, " ");
		// HACK: for OS X, blank out the -psn_XXXX argument so that it doesn't
		// break arg parsing
#ifdef __APPLE__
		if (strncmp(argv[i], "-psn", strlen("-psn")) == 0)
		{
			argv[i] = "";
		}
#endif
		strcat(buf, argv[i]);
	}
}

static void PrintConfig(const Config *c, const int indent);
bool ParseArgs(
	const int argc, char *argv[], ENetAddress *connectAddr,
	const char **loadCampaign, int *demoQuitTimer)
{
	struct option longopts[] = {
		{"fullscreen", no_argument, NULL, 'f'},
		{"scale", required_argument, NULL, 's'},
		{"screen", required_argument, NULL, 'c'},
		{"connect", required_argument, NULL, 'x'},
		{"listen_port", required_argument, NULL, 'p'},
		{"config", optional_argument, NULL, 'C'},
		{"log", required_argument, NULL, 1000},
		{"logfile", required_argument, NULL, 1001},
		{"demo", no_argument, NULL, 1002},
		{"help", no_argument, NULL, 'h'},
		{0, 0, NULL, 0}};
	int opt = 0;
	int idx = 0;
	while ((opt = getopt_long(
				argc, argv, "fs:c:x:C::\0:\0:h", longopts, &idx)) != -1)
	{
		switch (opt)
		{
		case 'f':
			ConfigGet(&gConfig, "Graphics.Fullscreen")->u.Bool.Value = true;
			break;
		case 's':
			ConfigSetInt(&gConfig, "Graphics.ScaleFactor", atoi(optarg));
			break;
		case 'c':
			sscanf(
				optarg, "%dx%d",
				&ConfigGet(&gConfig, "Graphics.WindowWidth")->u.Int.Value,
				&ConfigGet(&gConfig, "Graphics.WindowHeight")->u.Int.Value);
			LOG(LM_MAIN, LL_DEBUG, "Window size %dx%d set...",
				ConfigGetInt(&gConfig, "Graphics.WindowWidth"),
				ConfigGetInt(&gConfig, "Graphics.WindowHeight"));
			break;
		case 'm': {
			ConfigGet(&gConfig, "Graphics.ShakeMultiplier")->u.Int.Value =
				MAX(atoi(optarg), 0);
			printf(
				"Shake multiplier: %d\n",
				ConfigGetInt(&gConfig, "Graphics.ShakeMultiplier"));
		}
		break;
		case 'h':
			PrintHelp();
			return false;
		case 'p':
			ConfigGet(&gConfig, "ListenPort")->u.Int.Value =
				MAX(atoi(optarg), 0);
			printf("Listen port: %d\n", ConfigGetInt(&gConfig, "ListenPort"));
			break;
		case 1000: {
			char *comma = strchr(optarg, ',');
			if (comma)
			{
				// Set logging level for a single module
				// The module and level are comma separated
				*comma = '\0';
				const LogLevel ll = StrLogLevel(comma + 1);
				LogModuleSetLevel(StrLogModule(optarg), ll);
				printf("Logging %s at %s\n", optarg, LogLevelName(ll));
			}
			else
			{
				// Set logging level for all modules
				const LogLevel ll = StrLogLevel(optarg);
				for (int i = 0; i < (int)LM_COUNT; i++)
				{
					LogModuleSetLevel((LogModule)i, ll);
				}
				printf("Logging everything at %s\n", LogLevelName(ll));
			}
		}
		break;
		case 1001:
			LogOpenFile(optarg);
			break;
		case 1002:
			*demoQuitTimer = 30 * 1000;
			printf("Entering demo mode; will auto-quit in 30 seconds\n");
			break;
		case 'x':
			if (enet_address_set_host(connectAddr, optarg) != 0)
			{
				printf("Error: unknown host %s\n", optarg);
			}
			break;
		case 'C':
			if (optarg == NULL)
			{
				PrintConfig(&gConfig, 0);
				return false;
			}
			else
			{
				char *comma = strchr(optarg, ',');
				if (comma == NULL)
				{
					const Config *c = ConfigGet(&gConfig, optarg);
					if (c == NULL)
					{
						PrintHelp();
					}
					else
					{
						PrintConfig(c, 0);
					}
					return false;
				}
				*comma = '\0';
				const char *value = comma + 1;
				if (!ConfigTrySetFromString(&gConfig, optarg, value))
				{
					PrintHelp();
					return false;
				}
			}
			break;
		default:
			PrintHelp();
			// Ignore unknown arguments
			break;
		}
	}
	if (optind < argc)
	{
		// non-option ARGV-elements
		for (; optind < argc; optind++)
		{
			// Load campaign
			*loadCampaign = argv[optind];
		}
	}

	return true;
}
static void PrintConfig(const Config *c, const int indent)
{
	// Print this config
	for (int i = 0; i < indent; i++)
	{
		printf("-");
	}
	if (c->Name != NULL)
	{
		printf("%s\n", c->Name);
	}
	if (c->Type == CONFIG_TYPE_GROUP)
	{
		CA_FOREACH(const Config, child, c->u.Group)
		PrintConfig(child, indent + 1);
		CA_FOREACH_END()
	}
}
