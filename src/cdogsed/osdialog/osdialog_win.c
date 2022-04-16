#include <string.h>
#include <stdio.h>
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#include "osdialog.h"


static char* wchar_to_utf8(const wchar_t* s) {
	if (!s)
		return NULL;
	int len = WideCharToMultiByte(CP_UTF8, 0, s, -1, NULL, 0, NULL, NULL);
	if (!len)
		return NULL;
	char* r = OSDIALOG_MALLOC(len);
	WideCharToMultiByte(CP_UTF8, 0, s, -1, r, len, NULL, NULL);
	return r;
}

static wchar_t* utf8_to_wchar(const char* s) {
	if (!s)
		return NULL;
	int len = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
	if (!len)
		return NULL;
	wchar_t* r = OSDIALOG_MALLOC(len * sizeof(wchar_t));
	MultiByteToWideChar(CP_UTF8, 0, s, -1, r, len);
	return r;
}


int osdialog_message(osdialog_message_level level, osdialog_message_buttons buttons, const char* message) {
	UINT type = MB_APPLMODAL;
	switch (level) {
		default:
		case OSDIALOG_INFO: type |= MB_ICONINFORMATION; break;
		case OSDIALOG_WARNING: type |= MB_ICONWARNING; break;
		case OSDIALOG_ERROR: type |= MB_ICONERROR; break;
	}

	switch (buttons) {
		default:
		case OSDIALOG_OK: type |= MB_OK; break;
		case OSDIALOG_OK_CANCEL: type |= MB_OKCANCEL; break;
		case OSDIALOG_YES_NO: type |= MB_YESNO; break;
	}

	HWND window = GetActiveWindow();
	wchar_t* messageW = utf8_to_wchar(message);
	int result = MessageBoxW(window, messageW, L"", type);
	OSDIALOG_FREE(messageW);

	switch (result) {
		case IDOK:
		case IDYES:
			return 1;
		default:
			return 0;
	}
}


/*
Helpful resources:
https://docs.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-dlgtemplate
https://stackoverflow.com/a/2270250/272642
*/
#define DLG_FONT L"MS Shell Dlg"
#define DLG_OK L"&OK"
#define DLG_CANCEL L"&Cancel"
#define LENGTHOF(a) (sizeof(a) / sizeof((a)[0]))

#pragma pack(push, 4)
static struct {
	DWORD style;
	DWORD dwExtendedStyle;
	WORD  cdit; // number of controls
	short x;
	short y;
	short cx;
	short cy;
	WORD menu;
	WORD windowClass;
	WCHAR title[1];
	short pointSize;
	WCHAR typeface[LENGTHOF(DLG_FONT)];

	struct {
		DWORD style;
		DWORD dwExtendedStyle;
		short x;
		short y;
		short cx;
		short cy;
		WORD  id;
		WORD sysClass; // 0xFFFF identifies a system window class
		WORD idClass; // ordinal of a system window class
		WCHAR title[1];
		WORD cbCreationData; // bytes of following creation data
	} edit;

	struct {
		DWORD style;
		DWORD dwExtendedStyle;
		short x;
		short y;
		short cx;
		short cy;
		WORD  id;
		WORD sysClass; // 0xFFFF identifies a system window class
		WORD idClass; // ordinal of a system window class
		WCHAR title[LENGTHOF(DLG_OK)];
		WORD cbCreationData; // bytes of following creation data
	} ok;

	struct {
		DWORD style;
		DWORD dwExtendedStyle;
		short x;
		short y;
		short cx;
		short cy;
		WORD  id;
		WORD sysClass; // 0xFFFF identifies a system window class
		WORD idClass; // ordinal of a system window class
		WCHAR title[LENGTHOF(DLG_CANCEL)];
		WORD cbCreationData; // bytes of following creation data
	} cancel;

} promptTemplate = {
	WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME | DS_CENTER | DS_SHELLFONT,
	0x0, // dwExtendedStyle
	3, // cdit
	0, 0, 5+200+10+50+5+50+5, 5+14+5,
	0, // menu
	0, // windowClass
	L"", // title
	8, // typeface
	DLG_FONT,

	{
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL,
		WS_EX_NOPARENTNOTIFY,
		5, 5, 200, 14,
		42,
		0xFFFF, 0x0081, // Edit
		L"", 0,
	},

	{
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
		WS_EX_NOPARENTNOTIFY,
		5+200+10, 5, 50, 14,
		IDOK,
		0xFFFF, 0x0080, // Button
		DLG_OK, 0,
	},

	{
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
		WS_EX_NOPARENTNOTIFY,
		5+200+10+50+5, 5, 50, 14,
		IDCANCEL,
		0xFFFF, 0x0080, // Button
		DLG_CANCEL, 0,
	},
};
#pragma pack(pop)

static wchar_t promptBuffer[1 << 13] = L"";

static INT_PTR CALLBACK promptProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	(void) lParam;

	switch (message) {
		case WM_INITDIALOG: {
			SendDlgItemMessageW(hDlg, 42, WM_SETTEXT, (WPARAM) 0, (LPARAM) promptBuffer);
			return TRUE;
		} break;

		case WM_DESTROY: {
			EndDialog(hDlg, 0);
			return TRUE;
		} break;

		case WM_COMMAND: {
			switch (wParam) {
				case IDOK: {
					int len = SendDlgItemMessageW(hDlg, 42, WM_GETTEXT, (WPARAM) LENGTHOF(promptBuffer), (LPARAM) promptBuffer);
					(void) len;
					EndDialog(hDlg, 1);
					return TRUE;
				} break;

				case IDCANCEL: {
					EndDialog(hDlg, 0);
					return TRUE;
				} break;
			}
		} break;
	}
	return FALSE;
}

char* osdialog_prompt(osdialog_message_level level, const char* message, const char* text) {
	(void) level;
	(void) message;

	promptBuffer[0] = 0;
	if (text) {
		MultiByteToWideChar(CP_UTF8, 0, text, -1, promptBuffer, LENGTHOF(promptBuffer));
	}

	HWND window = GetActiveWindow();
	int res = DialogBoxIndirectParamW(NULL, (LPCDLGTEMPLATEW) &promptTemplate, window, promptProc, (LPARAM) NULL);
	if (res) {
		return wchar_to_utf8(promptBuffer);
	}
	return NULL;
}


static INT CALLBACK browseCallbackProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	(void) wParam;

	if (message == BFFM_INITIALIZED) {
		SendMessageW(hWnd, BFFM_SETEXPANDED, 1, lParam);
	}
	return 0;
}

char* osdialog_file(osdialog_file_action action, const char* dir, const char* filename, osdialog_filters* filters) {
	if (action == OSDIALOG_OPEN_DIR) {
		// open directory dialog
		BROWSEINFOW bInfo;
		ZeroMemory(&bInfo, sizeof(bInfo));
		bInfo.hwndOwner = GetActiveWindow();
		bInfo.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;

		// dir
		wchar_t initialDir[MAX_PATH] = L"";
		if (dir) {
			// We need to convert the path to a canonical absolute path with GetFullPathNameW()
			wchar_t* dirW = utf8_to_wchar(dir);
			GetFullPathNameW(dirW, MAX_PATH, initialDir, NULL);
			OSDIALOG_FREE(dirW);
			bInfo.lpfn = (BFFCALLBACK) browseCallbackProc;
			bInfo.lParam = (LPARAM) initialDir;
		}

		PIDLIST_ABSOLUTE lpItem = SHBrowseForFolderW(&bInfo);
		if (!lpItem) {
			return NULL;
		}
		wchar_t szDir[MAX_PATH] = L"";
		SHGetPathFromIDListW(lpItem, szDir);
		return wchar_to_utf8(szDir);
	}
	else {
		// open or save file dialog
		OPENFILENAMEW ofn;
		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = GetActiveWindow();
		ofn.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

		// filename
		wchar_t strFile[MAX_PATH] = L"";
		if (filename) {
			wchar_t* filenameW = utf8_to_wchar(filename);
			_snwprintf(strFile, MAX_PATH, L"%S", filenameW);
			OSDIALOG_FREE(filenameW);
		}
		ofn.lpstrFile = strFile;
		ofn.nMaxFile = MAX_PATH;

		// dir
		wchar_t strInitialDir[MAX_PATH] = L"";
		if (dir) {
			// We need to convert the dir to a canonical absolute dir with GetFullPathNameW()
			wchar_t* dirW = utf8_to_wchar(dir);
			GetFullPathNameW(dirW, MAX_PATH, strInitialDir, NULL);
			OSDIALOG_FREE(dirW);
			ofn.lpstrInitialDir = strInitialDir;
		}

		// filters
		wchar_t* strFilter = NULL;
		if (filters) {
			char fBuf[4096];
			int fLen = 0;

			for (; filters; filters = filters->next) {
				fLen += snprintf(fBuf + fLen, sizeof(fBuf) - fLen, "%s", filters->name);
				fBuf[fLen++] = '\0';
				for (osdialog_filter_patterns* patterns = filters->patterns; patterns; patterns = patterns->next) {
					fLen += snprintf(fBuf + fLen, sizeof(fBuf) - fLen, "*.%s", patterns->pattern);
					if (patterns->next)
						fLen += snprintf(fBuf + fLen, sizeof(fBuf) - fLen, ";");
				}
				fBuf[fLen++] = '\0';
			}
			fBuf[fLen++] = '\0';

			// Don't use utf8_to_wchar() because this is not a NULL-terminated string.
			strFilter = OSDIALOG_MALLOC(fLen * sizeof(wchar_t));
			MultiByteToWideChar(CP_UTF8, 0, fBuf, fLen, strFilter, fLen);
			ofn.lpstrFilter = strFilter;
			ofn.nFilterIndex = 1;
		}

		BOOL success;
		if (action == OSDIALOG_OPEN) {
			success = GetOpenFileNameW(&ofn);
		}
		else {
			success = GetSaveFileNameW(&ofn);
		}

		// Clean up
		if (strFilter) {
			OSDIALOG_FREE(strFilter);
		}
		if (!success) {
			return NULL;
		}
		return wchar_to_utf8(strFile);
	}
}


int osdialog_color_picker(osdialog_color* color, int opacity) {
	(void) opacity;
	if (!color)
		return 0;

	CHOOSECOLOR cc;
	ZeroMemory(&cc, sizeof(cc));

	COLORREF c = RGB(color->r, color->g, color->b);
	static COLORREF acrCustClr[16];

	cc.lStructSize = sizeof(cc);
	cc.lpCustColors = (LPDWORD) acrCustClr;
	cc.rgbResult = c;
	cc.Flags = CC_FULLOPEN | CC_ANYCOLOR | CC_RGBINIT;

	if (ChooseColor(&cc)) {
		color->r = GetRValue(cc.rgbResult);
		color->g = GetGValue(cc.rgbResult);
		color->b = GetBValue(cc.rgbResult);
		color->a = 255;
		return 1;
	}

	return 0;
}
