/*
Copyright (c) 2021 Cong

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef _MSC_VER
#define _FSG_FUNC static __inline
#elif !defined __STDC_VERSION__ || __STDC_VERSION__ < 199901L
#define _FSG_FUNC static __inline__
#else
#define _FSG_FUNC static inline
#endif

#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winerror.h>
#include <winreg.h>

	static void _query_reg_key(
		char *out, const HKEY key, const char *keypath, const char *valname)
	{
		HKEY pathkey;
		DWORD pathtype;
		DWORD pathlen;
		LONG res;

		if (ERROR_SUCCESS ==
			RegOpenKeyEx(key, keypath, 0, KEY_QUERY_VALUE, &pathkey))
		{
			if (ERROR_SUCCESS ==
					RegQueryValueEx(
						pathkey, valname, 0, &pathtype, NULL, &pathlen) &&
				pathtype == REG_SZ && pathlen != 0)
			{
				res = RegQueryValueEx(
					pathkey, valname, 0, NULL, (LPBYTE)out, &pathlen);
				if (res != ERROR_SUCCESS)
				{
					out[0] = '\0';
				}
			}
			RegCloseKey(pathkey);
		}
	}
#endif

	_FSG_FUNC
	void fsg_get_steam_game_path(char *out, const char *name)
	{
		out[0] = '\0';
#ifdef _MSC_VER
		_query_reg_key(
			out, HKEY_CURRENT_USER, "Software\\Valve\\Steam", "SteamPath");
		if (strlen(out) == 0)
		{
			_query_reg_key(
				out, HKEY_LOCAL_MACHINE, "Software\\Valve\\Steam",
				"InstallPath");
			if (strlen(out) == 0)
				return;
		}
		strcat(out, "/SteamApps/common/");
		strcat(out, name);

		struct stat info;
		if (stat(out, &info) != 0 || !(info.st_mode & S_IFDIR))
		{
			out[0] = '\0';
			return;
		}
#endif
	}

	_FSG_FUNC
	void fsg_get_gog_game_path(char *out, const char *app_id)
	{
		out[0] = '\0';
#ifdef _MSC_VER
		char buf[4096];
		sprintf(buf, "Software\\Wow6432Node\\GOG.com\\Games\\%s", app_id);
		_query_reg_key(out, HKEY_LOCAL_MACHINE, buf, "Path");
#endif
	}

#ifdef __cplusplus
}
#endif
