#include "n3d.h"

#include <stdlib.h>

#include "zip/zip.h"

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

char *CWN3DLoadLanguageEnu(const char *path)
{
	char *buf = NULL;
	struct zip_t *zip = zip_open(path, 0, 'r');
	if (zip == NULL)
	{
		goto bail;
	}
	zip_entry_open(zip, "language.enu");
	const size_t bufsize = zip_entry_size(zip);
	buf = malloc(bufsize);
	zip_entry_noallocread(zip, (void *)buf, bufsize);

bail:
	zip_entry_close(zip);
	zip_close(zip);
	return buf;
}

static char *LoadLanguageEnuString(const char *buf, const char *key)
{
	const char *start = buf;
	char linebuf[1024];
	while (start)
	{
		long long len = MIN(1024, strlen(start));
		char *nl = strchr(start, '\n');
		if (nl != NULL)
		{
			len = MIN(len, nl - start);
		}
		strncpy(linebuf, start, 1024);
		linebuf[len] = '\0';

		// Find entry lines
		char *equals = strstr(linebuf, " = ");
		if (equals != NULL)
		{
			// Key is now the start of the line
			*equals = '\0';
			if (strcmp(linebuf, key) == 0)
			{
				char *value = equals + strlen(" = \"");
				char *endQuote = linebuf + len - strlen("\";");
				*endQuote = '\0';
				// Unescape newlines in the string
				char *dst = value;
				const char *src = value;
				while (*src)
				{
					if (*src == '\\' && *(src + 1) == 'n')
					{
						*(dst++) = '\n';
						src += 2;
					}
					else
					{
						*(dst++) = *(src++);
					}
				}
				*dst = '\0';
				return strdup(value);
			}
		}

		if (nl == NULL)
		{
			break;
		}
		start = nl + 1;
	}
	return NULL;
}

char *CWLevelN3DLoadDescription(const char *buf, const int level)
{
	switch (level)
	{
	case 0:
		return LoadLanguageEnuString(buf, "NOAH_BRIEF_01");
	case 3:
		return LoadLanguageEnuString(buf, "NOAH_BRIEF_02");
	case 7:
		return LoadLanguageEnuString(buf, "NOAH_BRIEF_03");
	case 12:
		return LoadLanguageEnuString(buf, "NOAH_BRIEF_04");
	case 17:
		return LoadLanguageEnuString(buf, "NOAH_BRIEF_05");
	case 23:
		return LoadLanguageEnuString(buf, "NOAH_BRIEF_06");
	}
	return NULL;
}
