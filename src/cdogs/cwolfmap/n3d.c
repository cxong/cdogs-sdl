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
	const size_t bufsize = (size_t)zip_entry_size(zip);
	buf = malloc(bufsize);
	zip_entry_noallocread(zip, (void *)buf, bufsize);

bail:
	zip_entry_close(zip);
	zip_close(zip);
	return buf;
}

static char *LoadLanguageEnuString(
	const char *buf, const char *key, char **comment)
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
				char *endQuote = strstr(value, "\";");
				if (comment)
				{
					const char *commentStart = strstr(endQuote, "// ");
					if (commentStart)
					{
						*comment = strdup(commentStart + strlen("// "));
					}
				}
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
		return LoadLanguageEnuString(buf, "NOAH_BRIEF_01", NULL);
	case 3:
		return LoadLanguageEnuString(buf, "NOAH_BRIEF_02", NULL);
	case 7:
		return LoadLanguageEnuString(buf, "NOAH_BRIEF_03", NULL);
	case 12:
		return LoadLanguageEnuString(buf, "NOAH_BRIEF_04", NULL);
	case 17:
		return LoadLanguageEnuString(buf, "NOAH_BRIEF_05", NULL);
	case 23:
		return LoadLanguageEnuString(buf, "NOAH_BRIEF_06", NULL);
	}
	return NULL;
}

char *CWLevelN3DLoadQuizQuestion(const char *buf, const int quiz)
{
	char key[256];
	sprintf(key, "NOAH_QUIZ_Q%02d", quiz);
	return LoadLanguageEnuString(buf, key, NULL);
}
char *CWLevelN3DLoadQuizAnswer(
	const char *buf, const int quiz, const char answer, bool *correct)
{
	char key[256];
	sprintf(key, "NOAH_QUIZ_A%02d%c", quiz, answer);
	char *comment = NULL;
	char *result = LoadLanguageEnuString(buf, key, &comment);
	*correct = comment && strcmp(comment, "Correct") == 0;
	free(comment);
	return result;
}

void CWN3DQuizFree(CWN3DQuiz *quiz)
{
	free(quiz->question);
	free(quiz->answers);
}
