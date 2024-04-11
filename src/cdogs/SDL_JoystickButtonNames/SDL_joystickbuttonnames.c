/*
    Copyright (c) 2015, Cong Xu

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
#include "SDL_joystickbuttonnames.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL_pixels.h>


typedef struct
{
	SDL_JoystickGUID guid;
	char joystickName[256];
	char buttonNames[SDL_CONTROLLER_BUTTON_MAX][256];
	SDL_Color buttonColors[SDL_CONTROLLER_BUTTON_MAX];
	char axisNames[SDL_CONTROLLER_AXIS_MAX][256];
	SDL_Color axisColors[SDL_CONTROLLER_AXIS_MAX];
} JoystickButtonNames;

const char *err = NULL;
JoystickButtonNames *jbn = NULL;
int nJBN = 0;
JoystickButtonNames jDefault;
#include "db.h"


static JoystickButtonNames DefaultJoystickButtonNames(void);
static int ReadMappingsString(const char *s);

int SDLJBN_Init(void)
{
	// Don't reinitialise
	if (jbn != NULL) return 0;

	jDefault = DefaultJoystickButtonNames();

	ReadMappingsString(db);

	return 0;
}

void SDLJBN_Quit(void)
{
	SDL_free(jbn);
	jbn = NULL;
}

int SDLJBN_AddMappingsFromFile(const char *file)
{
	SDL_RWops *rwops = SDL_RWFromFile(file, "r");
	if (rwops == NULL)
	{
		err = "Cannot open file";
		return -1;
	}
	return SDLJBN_AddMappingsFromRW(rwops, 1);
}

int SDLJBN_AddMappingsFromRW(SDL_RWops *rwops, int freerw)
{
	int ret = 0;
	char *s = NULL;

	SDLJBN_Init();
	if (rwops == NULL)
	{
		ret = -1;
		goto bail;
	}

	// Read file into memory
	const Sint64 fsize = rwops->size(rwops);
	if (fsize == -1)
	{
		err = "Cannot find file size";
		ret = -1;
		goto bail;
	}
	s = SDL_malloc((size_t)fsize + 1);
	if (s == NULL)
	{
		err = "Out of memory";
		ret = -1;
		goto bail;
	}
	if (SDL_RWread(rwops, s, (size_t)fsize, 1) == 0)
	{
		err = "Cannot read file";
		ret = -1;
		goto bail;
	}
	s[fsize] = '\0';

	ret = ReadMappingsString(s);

bail:
	if (rwops != NULL && freerw)
	{
		SDL_RWclose(rwops);
	}
	SDL_free(s);
	return ret;
}

static int ReadMappingsString(const char *s)
{
#define READ_TOKEN(buf, p, end)\
	if (end == NULL)\
	{\
		strcpy(buf, p);\
		p = NULL;\
	}\
	else\
	{\
		strncpy(buf, p, end - p);\
		buf[end - p] = '\0';\
		p = end + 1;\
	}

	// Read compiled string button names into memory
	// Search for a matching GUID + joystickName in the db
	int read = 0;
	for (const char *cur = s; cur;)
	{
		const char *nl = strchr(cur, '\n');
		char line[2048];
		READ_TOKEN(line, cur, nl);

		char buf[256];

		// Check for the platform string
		sprintf(buf, "platform:%s", SDL_GetPlatform());
		if (strstr(line, buf) == NULL) continue;

#define STR_NOT_EQ(expected, actualP, actualEnd)\
	strlen(expected) != (actualEnd) - (actualP) || \
	strncmp(expected, actualP, (actualEnd) - (actualP)) != 0
		const char *curL = line;
		// Ignore hash comments
		if (*curL == '#') continue;

		const char *nextComma;
		JoystickButtonNames j = jDefault;

		// Read GUID
		nextComma = strchr(curL, ',');
		if (nextComma == NULL || cur == nextComma) continue;
		READ_TOKEN(buf, curL, nextComma);
		j.guid = SDL_JoystickGetGUIDFromString(buf);

		// Read joystick name
		nextComma = strchr(curL, ',');
		if (nextComma == NULL || curL == nextComma) continue;
		READ_TOKEN(j.joystickName, curL, nextComma);

		// Check if GUID+joystick name already exists
		bool exists = false;
		for (int i = 0; i < nJBN; i++)
		{
			const JoystickButtonNames *jp = jbn + i;
			if (memcmp(&jp->guid, &j.guid, sizeof j.guid) == 0 &&
				strcmp(jp->joystickName, j.joystickName) == 0)
			{
				exists = true;
				break;
			}
		}
		if (exists) continue;

		// Read name and colors
		for (;; curL = nextComma + 1)
		{
			nextComma = strchr(curL, ',');
			if (nextComma == NULL) break;

			const char *nextColon;

			nextColon = strchr(curL, ':');
			if (nextColon == NULL || curL == nextColon) continue;
			READ_TOKEN(buf, curL, nextColon);
			const SDL_GameControllerButton button =
				SDL_GameControllerGetButtonFromString(buf);
			const SDL_GameControllerAxis axis =
				SDL_GameControllerGetAxisFromString(buf);
			char *name;
			SDL_Color *color;
			if (button != SDL_CONTROLLER_BUTTON_INVALID)
			{
				name = j.buttonNames[(int)button];
				color = &j.buttonColors[(int)button];
			}
			else if (axis != SDL_CONTROLLER_AXIS_INVALID)
			{
				name = j.axisNames[(int)axis];
				color = &j.axisColors[(int)axis];
			}
			else
			{
				continue;
			}
			// Read the real button/axis name
			nextColon = strchr(curL, ':');
			if (nextColon == NULL) continue;
			READ_TOKEN(name, curL, nextColon);
			// R
			nextColon = strchr(curL, ':');
			if (nextColon == NULL) continue;
			READ_TOKEN(buf, curL, nextColon);
			color->r = (Uint8)atoi(buf);
			// G
			nextColon = strchr(curL, ':');
			if (nextColon == NULL) continue;
			READ_TOKEN(buf, curL, nextColon);
			color->g = (Uint8)atoi(buf);
			// B
			READ_TOKEN(buf, curL, nextComma);
			color->b = (Uint8)atoi(buf);

			color->a = 255;
		}
		nJBN++;
		read++;
		jbn = SDL_realloc(jbn, nJBN * sizeof *jbn);
		memcpy(jbn + nJBN - 1, &j, sizeof j);
	}
	return read;
}

static const char *DefaultButtonName(SDL_GameControllerButton button);
static const char *DefaultAxisName(SDL_GameControllerAxis axis);
static SDL_Color DefaultButtonColor(SDL_GameControllerButton button);
static SDL_Color DefaultAxisColor(SDL_GameControllerAxis axis);

static JoystickButtonNames DefaultJoystickButtonNames(void)
{
	JoystickButtonNames j;
	memset(&j, 0, sizeof j);
	for (SDL_GameControllerButton button = SDL_CONTROLLER_BUTTON_A;
		button < SDL_CONTROLLER_BUTTON_MAX;
		button++)
	{
		strcpy(j.buttonNames[(int)button], DefaultButtonName(button));
		j.buttonColors[(int)button] = DefaultButtonColor(button);
	}
	for (SDL_GameControllerAxis axis = SDL_CONTROLLER_AXIS_LEFTX;
		axis < SDL_CONTROLLER_AXIS_MAX;
		axis++)
	{
		strcpy(j.axisNames[(int)axis], DefaultAxisName(axis));
		j.axisColors[(int)axis] = DefaultAxisColor(axis);
	}
	return j;
}

static const char *DefaultButtonName(SDL_GameControllerButton button)
{
	switch (button)
	{
	case SDL_CONTROLLER_BUTTON_A: return "A";
	case SDL_CONTROLLER_BUTTON_B: return "B";
	case SDL_CONTROLLER_BUTTON_X: return "X";
	case SDL_CONTROLLER_BUTTON_Y: return "Y";
	case SDL_CONTROLLER_BUTTON_BACK: return "Back";
	case SDL_CONTROLLER_BUTTON_GUIDE: return "Guide";
	case SDL_CONTROLLER_BUTTON_START: return "Start";
	case SDL_CONTROLLER_BUTTON_LEFTSTICK: return "Left Stick";
	case SDL_CONTROLLER_BUTTON_RIGHTSTICK: return "Right Stick";
	case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return "LB";
	case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return "RB";
	case SDL_CONTROLLER_BUTTON_DPAD_UP: return "D-pad Up";
	case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return "D-pad Down";
	case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return "D-pad Left";
	case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return "D-pad Right";
	default: return "";
	}
}

static const char *DefaultAxisName(SDL_GameControllerAxis axis)
{
	switch (axis)
	{
	case SDL_CONTROLLER_AXIS_LEFTX: return "Left Stick X";
	case SDL_CONTROLLER_AXIS_LEFTY: return "Left Stick Y";
	case SDL_CONTROLLER_AXIS_RIGHTX: return "Right Stick X";
	case SDL_CONTROLLER_AXIS_RIGHTY: return "Right Stick Y";
	case SDL_CONTROLLER_AXIS_TRIGGERLEFT: return "LT";
	case SDL_CONTROLLER_AXIS_TRIGGERRIGHT: return "RT";
	default: return "";
	}
}

static SDL_Color NewColor(Uint8 r, Uint8 g, Uint8 b);

static SDL_Color DefaultButtonColor(SDL_GameControllerButton button)
{
	// Default colors for Xbox 360 controller
	switch (button)
	{
	case SDL_CONTROLLER_BUTTON_A: return NewColor(96, 160, 0);
	case SDL_CONTROLLER_BUTTON_B: return NewColor(240, 0, 0);
	case SDL_CONTROLLER_BUTTON_X: return NewColor(0, 96, 208);
	case SDL_CONTROLLER_BUTTON_Y: return NewColor(255, 160, 0);
	case SDL_CONTROLLER_BUTTON_BACK: return NewColor(224, 224, 224);
	case SDL_CONTROLLER_BUTTON_GUIDE: return NewColor(128, 176, 0);
	case SDL_CONTROLLER_BUTTON_START: return NewColor(224, 224, 224);
	case SDL_CONTROLLER_BUTTON_LEFTSTICK: return NewColor(96, 128, 128);
	case SDL_CONTROLLER_BUTTON_RIGHTSTICK: return NewColor(96, 128, 128);
	case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return NewColor(224, 224, 224);
	case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return NewColor(224, 224, 224);
	case SDL_CONTROLLER_BUTTON_DPAD_UP: return NewColor(96, 128, 128);
	case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return NewColor(96, 128, 128);
	case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return NewColor(96, 128, 128);
	case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return NewColor(96, 128, 128);
	default: return NewColor(0, 0, 0);
	}
}

static SDL_Color DefaultAxisColor(SDL_GameControllerAxis axis)
{
	// Default colors for Xbox 360 controller
	switch (axis)
	{
	case SDL_CONTROLLER_AXIS_LEFTX: return NewColor(96, 128, 128);
	case SDL_CONTROLLER_AXIS_LEFTY: return NewColor(96, 128, 128);
	case SDL_CONTROLLER_AXIS_RIGHTX: return NewColor(96, 128, 128);
	case SDL_CONTROLLER_AXIS_RIGHTY: return NewColor(96, 128, 128);
	case SDL_CONTROLLER_AXIS_TRIGGERLEFT: return NewColor(224, 224, 224);
	case SDL_CONTROLLER_AXIS_TRIGGERRIGHT: return NewColor(224, 224, 224);
	default: return NewColor(0, 0, 0);
	}
}

int SDLJBN_GetButtonNameAndColor(SDL_Joystick *joystick,
                                 SDL_GameControllerButton button,
                                 char *s, Uint8 *r, Uint8 *g, Uint8 *b)
{
	SDLJBN_Init();

	if (joystick == NULL)
	{
		err = "joystick is NULL";
		return -1;
	}
	if (button < SDL_CONTROLLER_BUTTON_A ||
		button >= SDL_CONTROLLER_BUTTON_MAX)
	{
		err = "button is invalid";
		return -1;
	}
	// Use defaults first
	if (s) strcpy(s, jDefault.buttonNames[(int)button]);
	if (r) *r = jDefault.buttonColors[(int)button].r;
	if (g) *g = jDefault.buttonColors[(int)button].g;
	if (b) *b = jDefault.buttonColors[(int)button].b;

	const SDL_JoystickGUID guid = SDL_JoystickGetGUID(joystick);
	const char *joystickName = SDL_JoystickName(joystick);
	// Search for a matching GUID + joystickName in the db
	for (int i = 0; i < nJBN; i++)
	{
		const JoystickButtonNames *j = jbn + i;
		if (memcmp(&j->guid, &guid, sizeof guid) == 0 &&
			strcmp(j->joystickName, joystickName) == 0)
		{
			// GUID and joystick name match; read name and colors
			if (s && strlen(j->buttonNames[(int)button]) > 0)
				strcpy(s, j->buttonNames[(int)button]);
			if (r) *r = j->buttonColors[(int)button].r;
			if (g) *g = j->buttonColors[(int)button].g;
			if (b) *b = j->buttonColors[(int)button].b;
			break;
		}
	}

	return 0;
}


int SDLJBN_GetAxisNameAndColor(SDL_Joystick *joystick,
                               SDL_GameControllerAxis axis,
                               char *s, Uint8 *r, Uint8 *g, Uint8 *b)
{
	SDLJBN_Init();

	if (joystick == NULL)
	{
		err = "joystick is NULL";
		return -1;
	}
	if (axis < SDL_CONTROLLER_AXIS_LEFTX || axis >= SDL_CONTROLLER_AXIS_MAX)
	{
		err = "axis is invalid";
		return -1;
	}
	// Use defaults first
	if (s) strcpy(s, jDefault.axisNames[(int)axis]);
	if (r) *r = jDefault.axisColors[(int)axis].r;
	if (g) *g = jDefault.axisColors[(int)axis].g;
	if (b) *b = jDefault.axisColors[(int)axis].b;

	const SDL_JoystickGUID guid = SDL_JoystickGetGUID(joystick);
	const char *joystickName = SDL_JoystickName(joystick);
	// Search for a matching GUID + joystickName in the db
	for (int i = 0; i < nJBN; i++)
	{
		const JoystickButtonNames *j = jbn + i;
		if (memcmp(&j->guid, &guid, sizeof guid) == 0 &&
			strcmp(j->joystickName, joystickName) == 0)
		{
			// GUID and joystick name match; read name and colors
			if (s && strlen(j->axisNames[(int)axis]) > 0)
				strcpy(s, j->axisNames[(int)axis]);
			if (r) *r = j->axisColors[(int)axis].r;
			if (g) *g = j->axisColors[(int)axis].g;
			if (b) *b = j->axisColors[(int)axis].b;
			break;
		}
	}

	return 0;
}

static SDL_Color NewColor(Uint8 r, Uint8 g, Uint8 b)
{
	SDL_Color c;
	c.r = r;
	c.g = g;
	c.b = b;
	c.a = 255;
	return c;
}

const char *SDLJBN_GetError(void)
{
	return err;
}
