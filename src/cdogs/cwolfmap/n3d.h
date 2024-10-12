#pragma once

#include <stdbool.h>

#include "common.h"

// Load the noah3d.pak/language.enu file to char buffer, containing briefs and
// quizzes
char *CWN3DLoadLanguageEnu(const char *path);

char *CWLevelN3DLoadDescription(const char *buf, const int level);

// Load a quiz question (quiz starts from #1)
char *CWLevelN3DLoadQuizQuestion(const char *buf, const int quiz);
// Load a quiz answer (quiz starts from #1, answer starts from 'A')
char *CWLevelN3DLoadQuizAnswer(
	const char *buf, const int quiz, const char answer, bool *correct);

void CWN3DQuizFree(CWN3DQuiz *quiz);