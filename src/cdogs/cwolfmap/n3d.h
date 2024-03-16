#pragma once

// Load the noah3d.pak/language.enu file to char buffer, containing briefs and
// quizzes
char *CWN3DLoadLanguageEnu(const char *path);

char *CWLevelN3DLoadDescription(const char *buf, const int level);
