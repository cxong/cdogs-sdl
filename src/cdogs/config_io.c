/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.

    Copyright (c) 2013-2014, 2016, Cong Xu
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
#include "config_io.h"

#include "config_json.h"
#include "log.h"


Config ConfigLoad(const char *filename)
{
	// Load default values first
	Config c = ConfigDefault();
	if (filename == NULL)
	{
		// This is equivalent to loading nothing; just exit
		return c;
	}
	FILE *f = fopen(filename, "r");
	if (f == NULL)
	{
		printf("Error loading config '%s'\n", filename);
		return c;
	}
	const int configVersion = ConfigGetVersion(f);
	fclose(f);
	switch (configVersion)
	{
	case 0:
		printf("Classic config is no longer supported\n");
		break;
	default:
		ConfigLoadJSON(&c, filename);
		break;
	}
	ConfigSetChanged(&c);
	return c;
}

void ConfigSave(const Config *config, const char *filename)
{
	ConfigSaveJSON(config, filename);
}
