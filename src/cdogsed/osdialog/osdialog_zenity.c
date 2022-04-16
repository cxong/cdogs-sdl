#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h> // for waitpid
#include "osdialog.h"


static const char zenityBin[] = "zenity";


static void string_list_clear(char** list) {
	while (*list) {
		OSDIALOG_FREE(*list);
		*list = NULL;
		list++;
	}
}


static int string_list_exec(const char* path, const char* const* args, char* outBuf, size_t outLen, char* errBuf, size_t errLen) {
	int outStream[2];
	if (outBuf)
		if (pipe(outStream) == -1)
			return -1;

	int errStream[2];
	if (errBuf)
		if (pipe(errStream) == -1)
			return -1;

	// The classic fork-and-exec routine
	pid_t pid = fork();
	if (pid < 0) {
		return -1;
	}
	else if (pid == 0) {
		// child process
		// Route stdout to outStream
		if (outBuf) {
			while ((dup2(outStream[1], STDOUT_FILENO) == -1) && (errno == EINTR)) {}
			close(outStream[0]);
			close(outStream[1]);
		}
		// Route stdout to outStream
		if (errBuf) {
			while ((dup2(errStream[1], STDERR_FILENO) == -1) && (errno == EINTR)) {}
			close(errStream[0]);
			close(errStream[1]);
		}
		// POSIX guarantees that execvp does not modify the args array, so it's safe to remove const with a cast.
		int err = execvp(path, (char* const*) args);
		if (err)
			exit(0);
		// Will never reach
		exit(0);
	}

	// parent process
	// Close the pipe inputs because the parent doesn't need them
	if (outBuf)
		close(outStream[1]);
	if (errBuf)
		close(errStream[1]);
	// Wait for child process to close
	int status = -1;
	int options = 0;
	waitpid(pid, &status, options);
	// Read streams
	if (outBuf) {
		ssize_t count = read(outStream[0], outBuf, outLen - 1);
		if (count < 0)
			count = 0;
		outBuf[count] = '\0';
		close(outStream[0]);
	}
	if (errBuf) {
		ssize_t count = read(errStream[0], errBuf, errLen - 1);
		if (count < 0)
			count = 0;
		errBuf[count] = '\0';
		close(errStream[0]);
	}
	return status;
}


int osdialog_message(osdialog_message_level level, osdialog_message_buttons buttons, const char* message) {
	char* args[32];
	int argIndex = 0;

	args[argIndex++] = osdialog_strdup(zenityBin);
	// The API doesn't provide a title, so just make it blank.
	args[argIndex++] = osdialog_strdup("--title");
	args[argIndex++] = osdialog_strdup("");

	args[argIndex++] = osdialog_strdup("--no-markup");

	args[argIndex++] = osdialog_strdup("--width");
	args[argIndex++] = osdialog_strdup("500");

	if (buttons == OSDIALOG_OK_CANCEL) {
		args[argIndex++] = osdialog_strdup("--question");
		args[argIndex++] = osdialog_strdup("--ok-label");
		args[argIndex++] = osdialog_strdup("OK");
		args[argIndex++] = osdialog_strdup("--cancel-label");
		args[argIndex++] = osdialog_strdup("Cancel");
	}
	else if (buttons == OSDIALOG_YES_NO) {
		args[argIndex++] = osdialog_strdup("--question");
		args[argIndex++] = osdialog_strdup("--ok-label");
		args[argIndex++] = osdialog_strdup("Yes");
		args[argIndex++] = osdialog_strdup("--cancel-label");
		args[argIndex++] = osdialog_strdup("No");
	}
	else if (level == OSDIALOG_INFO) {
		args[argIndex++] = osdialog_strdup("--info");
	}
	else if (level == OSDIALOG_WARNING) {
		args[argIndex++] = osdialog_strdup("--warning");
	}
	else if (level == OSDIALOG_ERROR) {
		args[argIndex++] = osdialog_strdup("--error");
	}

	args[argIndex++] = osdialog_strdup("--text");
	args[argIndex++] = osdialog_strdup(message);

	args[argIndex++] = NULL;
	int ret = string_list_exec(zenityBin, (const char* const*) args, NULL, 0, NULL, 0);
	string_list_clear(args);
	return ret == 0;
}


char* osdialog_prompt(osdialog_message_level level, const char* message, const char* text) {
	char* args[32];
	int argIndex = 0;

	args[argIndex++] = osdialog_strdup(zenityBin);
	args[argIndex++] = osdialog_strdup("--title");
	args[argIndex++] = osdialog_strdup("");
	args[argIndex++] = osdialog_strdup("--entry");
	args[argIndex++] = osdialog_strdup("--text");
	args[argIndex++] = osdialog_strdup(message ? message : "");
	args[argIndex++] = osdialog_strdup("--entry-text");
	args[argIndex++] = osdialog_strdup(text ? text : "");
	// Unfortunately the level is ignored

	args[argIndex++] = NULL;
	char outBuf[4096 + 1];
	int ret = string_list_exec(zenityBin, (const char* const*) args, outBuf, sizeof(outBuf), NULL, 0);
	string_list_clear(args);
	if (ret != 0)
		return NULL;

	// Remove trailing newline
	size_t outLen = strlen(outBuf);
	if (outLen > 0)
		outBuf[outLen - 1] = '\0';
	return osdialog_strdup(outBuf);
}


char* osdialog_file(osdialog_file_action action, const char* dir, const char* filename, osdialog_filters* filters) {
	char* args[32];
	int argIndex = 0;

	args[argIndex++] = osdialog_strdup(zenityBin);
	args[argIndex++] = osdialog_strdup("--title");
	args[argIndex++] = osdialog_strdup("");
	args[argIndex++] = osdialog_strdup("--file-selection");
	if (action == OSDIALOG_OPEN) {
		// This is the default
	}
	else if (action == OSDIALOG_OPEN_DIR) {
		args[argIndex++] = osdialog_strdup("--directory");
	}
	else if (action == OSDIALOG_SAVE) {
		args[argIndex++] = osdialog_strdup("--save");
		args[argIndex++] = osdialog_strdup("--confirm-overwrite");
	}

	if (dir || filename) {
		args[argIndex++] = osdialog_strdup("--filename");
		char buf[4096];
		if (dir) {
			// If we don't add a slash, the open dialog will open in the parent directory.
			// If a slash is already present, a second one will have no effect.
			snprintf(buf, sizeof(buf), "%s/%s", dir, filename ? filename : "");
		}
		else {
			snprintf(buf, sizeof(buf), "%s", filename);
		}
		args[argIndex++] = osdialog_strdup(buf);
	}

	for (osdialog_filters* filter = filters; filter; filter = filter->next) {
		args[argIndex++] = osdialog_strdup("--file-filter");
		char buf[1024];
		// TODO Add support for multiple patterns per filter
		snprintf(buf, sizeof(buf), "%s|*.%s", filters->name, filters->patterns->pattern);
		args[argIndex++] = osdialog_strdup(buf);
	}

	args[argIndex++] = NULL;
	char outBuf[4096 + 1];
	int ret = string_list_exec(zenityBin, (const char* const*) args, outBuf, sizeof(outBuf), NULL, 0);
	string_list_clear(args);
	if (ret != 0)
		return NULL;

	// Remove trailing newline
	size_t outLen = strlen(outBuf);
	if (outLen > 0)
		outBuf[outLen - 1] = '\0';
	return osdialog_strdup(outBuf);
}


int osdialog_color_picker(osdialog_color* color, int opacity) {
	char* args[32];
	int argIndex = 0;

	args[argIndex++] = osdialog_strdup(zenityBin);
	args[argIndex++] = osdialog_strdup("--title");
	args[argIndex++] = osdialog_strdup("");
	args[argIndex++] = osdialog_strdup("--color-selection");

	if (!opacity) {
		color->a = 255;
	}

	// Convert osdialog_color to string
	char buf[128];
	snprintf(buf, sizeof(buf), "rgba(%d,%d,%d,%f)", color->r, color->g, color->b, color->a / 255.f);
	args[argIndex++] = osdialog_strdup("--color");
	args[argIndex++] = osdialog_strdup(buf);

	args[argIndex++] = NULL;
	int ret = string_list_exec(zenityBin, (const char* const*) args, buf, sizeof(buf), NULL, 0);
	string_list_clear(args);
	if (ret != 0)
		return 0;

	// Convert string to osdialog_color
	int r = 0, g = 0, b = 0;
	float a = 1.f;
	if (buf[3] == 'a') {
		sscanf(buf, "rgba(%d,%d,%d,%f)", &r, &g, &b, &a);
	}
	else {
		sscanf(buf, "rgb(%d,%d,%d)", &r, &g, &b);
	}
	color->r = r;
	color->g = g;
	color->b = b;
	color->a = (int) (a * 255.f);

	if (!opacity) {
		color->a = 255;
	}

	return 1;
}
