#ifndef RLUTIL_H
#define RLUTIL_H
/**
 * File: rlutil.h
 *
 * About: Description
 * This file provides some useful utilities for console mode
 * roguelike game development with C and C++. It is aimed to
 * be cross-platform (at least Windows and Linux).
 *
 * About: Copyright
 * (C) 2010 Tapio Vierros
 *
 * About: Licensing
 * See <License>
 */


/// Define: RLUTIL_USE_ANSI
/// Define this to use ANSI escape sequences also on Windows
/// (defaults to using WinAPI instead).
#if 0
#define RLUTIL_USE_ANSI
#endif

/// Define: RLUTIL_STRING_T
/// Define/typedef this to your preference to override rlutil's string type.
///
/// Defaults to std::string with C++ and char* with C.
#if 0
#define RLUTIL_STRING_T char*
#endif

#ifndef RLUTIL_INLINE
	#ifdef _MSC_VER
		#define RLUTIL_INLINE __inline
	#else
		#define RLUTIL_INLINE __inline__
	#endif
#endif

#ifdef __cplusplus
	/// Common C++ headers
	#include <iostream>
	#include <string>
	#include <sstream>
	/// Namespace forward declarations
	namespace rlutil {
		void locate(int x, int y);
	}
#else
	RLUTIL_INLINE void locate(int x, int y); // Forward declare for C to avoid warnings
#endif // __cplusplus

#ifdef _WIN32
	#include <windows.h>  // for WinAPI and Sleep()
	#define _NO_OLDNAMES  // for MinGW compatibility
#else
	#include <unistd.h> // for getch(), kbhit() and (u)sleep()
#endif // _WIN32

#ifndef gotoxy
/// Function: gotoxy
/// Same as <rlutil.locate>.
RLUTIL_INLINE void gotoxy(int x, int y) {
	#ifdef __cplusplus
	rlutil::
	#endif
	locate(x,y);
}
#endif // gotoxy

#ifdef __cplusplus
/// Namespace: rlutil
/// In C++ all functions except <getch>, <kbhit> and <gotoxy> are arranged
/// under namespace rlutil. That is because some platforms have them defined
/// outside of rlutil.
namespace rlutil {
#endif

/**
 * Defs: Internal typedefs and macros
 * RLUTIL_STRING_T - String type depending on which one of C or C++ is used
 * RLUTIL_PRINT(str) - Printing macro independent of C/C++
 */

#ifdef __cplusplus
	#ifndef RLUTIL_STRING_T
		typedef std::string RLUTIL_STRING_T;
	#endif // RLUTIL_STRING_T

	inline void RLUTIL_PRINT(RLUTIL_STRING_T st) { std::cout << st; }

#else // __cplusplus
	#ifndef RLUTIL_STRING_T
		typedef char* RLUTIL_STRING_T;
	#endif // RLUTIL_STRING_T

	#include <stdio.h>
	#define RLUTIL_PRINT(st) printf("%s", st)
#endif // __cplusplus

/**
 * Enums: Color codes
 *
 * BLACK - Black
 * BLUE - Blue
 * GREEN - Green
 * CYAN - Cyan
 * RED - Red
 * MAGENTA - Magenta / purple
 * BROWN - Brown / dark yellow
 * GREY - Grey / dark white
 * DARKGREY - Dark grey / light black
 * LIGHTBLUE - Light blue
 * LIGHTGREEN - Light green
 * LIGHTCYAN - Light cyan
 * LIGHTRED - Light red
 * LIGHTMAGENTA - Light magenta / light purple
 * YELLOW - Yellow (bright)
 * WHITE - White (bright)
 */
enum {
	BLACK,
	BLUE,
	GREEN,
	CYAN,
	RED,
	MAGENTA,
	BROWN,
	GREY,
	DARKGREY,
	LIGHTBLUE,
	LIGHTGREEN,
	LIGHTCYAN,
	LIGHTRED,
	LIGHTMAGENTA,
	YELLOW,
	WHITE
};

/**
 * Consts: ANSI color strings
 *
 * ANSI_CLS - Clears screen
 * ANSI_BLACK - Black
 * ANSI_RED - Red
 * ANSI_GREEN - Green
 * ANSI_BROWN - Brown / dark yellow
 * ANSI_BLUE - Blue
 * ANSI_MAGENTA - Magenta / purple
 * ANSI_CYAN - Cyan
 * ANSI_GREY - Grey / dark white
 * ANSI_DARKGREY - Dark grey / light black
 * ANSI_LIGHTRED - Light red
 * ANSI_LIGHTGREEN - Light green
 * ANSI_YELLOW - Yellow (bright)
 * ANSI_LIGHTBLUE - Light blue
 * ANSI_LIGHTMAGENTA - Light magenta / light purple
 * ANSI_LIGHTCYAN - Light cyan
 * ANSI_WHITE - White (bright)
 */
static const RLUTIL_STRING_T ANSI_CLS = "\033[2J";
static const RLUTIL_STRING_T ANSI_BLACK = "\033[22;30m";
static const RLUTIL_STRING_T ANSI_RED = "\033[22;31m";
static const RLUTIL_STRING_T ANSI_GREEN = "\033[22;32m";
static const RLUTIL_STRING_T ANSI_BROWN = "\033[22;33m";
static const RLUTIL_STRING_T ANSI_BLUE = "\033[22;34m";
static const RLUTIL_STRING_T ANSI_MAGENTA = "\033[22;35m";
static const RLUTIL_STRING_T ANSI_CYAN = "\033[22;36m";
static const RLUTIL_STRING_T ANSI_GREY = "\033[22;37m";
static const RLUTIL_STRING_T ANSI_DARKGREY = "\033[01;30m";
static const RLUTIL_STRING_T ANSI_LIGHTRED = "\033[01;31m";
static const RLUTIL_STRING_T ANSI_LIGHTGREEN = "\033[01;32m";
static const RLUTIL_STRING_T ANSI_YELLOW = "\033[01;33m";
static const RLUTIL_STRING_T ANSI_LIGHTBLUE = "\033[01;34m";
static const RLUTIL_STRING_T ANSI_LIGHTMAGENTA = "\033[01;35m";
static const RLUTIL_STRING_T ANSI_LIGHTCYAN = "\033[01;36m";
static const RLUTIL_STRING_T ANSI_WHITE = "\033[01;37m";

/// Function: getANSIColor
/// Return ANSI color escape sequence for specified number 0-15.
///
/// See <Color Codes>
static RLUTIL_INLINE RLUTIL_STRING_T getANSIColor(const int c) {
	switch (c) {
		case 0 : return ANSI_BLACK;
		case 1 : return ANSI_BLUE; // non-ANSI
		case 2 : return ANSI_GREEN;
		case 3 : return ANSI_CYAN; // non-ANSI
		case 4 : return ANSI_RED; // non-ANSI
		case 5 : return ANSI_MAGENTA;
		case 6 : return ANSI_BROWN;
		case 7 : return ANSI_GREY;
		case 8 : return ANSI_DARKGREY;
		case 9 : return ANSI_LIGHTBLUE; // non-ANSI
		case 10: return ANSI_LIGHTGREEN;
		case 11: return ANSI_LIGHTCYAN; // non-ANSI;
		case 12: return ANSI_LIGHTRED; // non-ANSI;
		case 13: return ANSI_LIGHTMAGENTA;
		case 14: return ANSI_YELLOW; // non-ANSI
		case 15: return ANSI_WHITE;
		default: return "";
	}
}

/// Function: setColor
/// Change color specified by number (Windows / QBasic colors).
///
/// See <Color Codes>
static RLUTIL_INLINE void setColor(int c) {
#if defined(_WIN32) && !defined(RLUTIL_USE_ANSI)
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, (WORD)c);
#else
	RLUTIL_PRINT(getANSIColor(c));
#endif
}

/// Function: cls
/// Clears screen and moves cursor home.
RLUTIL_INLINE void cls(void) {
#if defined(_WIN32) && !defined(RLUTIL_USE_ANSI)
	// TODO: This is cheating...
	system("cls");
#else
	RLUTIL_PRINT("\033[2J\033[H");
#endif
}

/// Function: locate
/// Sets the cursor position to 1-based x,y.
RLUTIL_INLINE void locate(int x, int y) {
#if defined(_WIN32) && !defined(RLUTIL_USE_ANSI)
	COORD coord;
	coord.X = (SHORT)x-1;
	coord.Y = (SHORT)y-1; // Windows uses 0-based coordinates
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
#else // _WIN32 || USE_ANSI
	#ifdef __cplusplus
		std::ostringstream oss;
		oss << "\033[" << y << ";" << x << "H";
		RLUTIL_PRINT(oss.str());
	#else // __cplusplus
		char buf[32];
		sprintf(buf, "\033[%d;%df", y, x);
		RLUTIL_PRINT(buf);
	#endif // __cplusplus
#endif // _WIN32 || USE_ANSI
}

/// Function: hidecursor
/// Hides the cursor.
RLUTIL_INLINE void hidecursor(void) {
#if defined(_WIN32) && !defined(RLUTIL_USE_ANSI)
	HANDLE hConsoleOutput;
	CONSOLE_CURSOR_INFO structCursorInfo;
	hConsoleOutput = GetStdHandle( STD_OUTPUT_HANDLE );
	GetConsoleCursorInfo( hConsoleOutput, &structCursorInfo ); // Get current cursor size
	structCursorInfo.bVisible = FALSE;
	SetConsoleCursorInfo( hConsoleOutput, &structCursorInfo );
#else // _WIN32 || USE_ANSI
	RLUTIL_PRINT("\033[?25l");
#endif // _WIN32 || USE_ANSI
}

/// Function: showcursor
/// Shows the cursor.
RLUTIL_INLINE void showcursor(void) {
#if defined(_WIN32) && !defined(RLUTIL_USE_ANSI)
	HANDLE hConsoleOutput;
	CONSOLE_CURSOR_INFO structCursorInfo;
	hConsoleOutput = GetStdHandle( STD_OUTPUT_HANDLE );
	GetConsoleCursorInfo( hConsoleOutput, &structCursorInfo ); // Get current cursor size
	structCursorInfo.bVisible = TRUE;
	SetConsoleCursorInfo( hConsoleOutput, &structCursorInfo );
#else // _WIN32 || USE_ANSI
	RLUTIL_PRINT("\033[?25h");
#endif // _WIN32 || USE_ANSI
}

/// Function: trows
/// Get the number of rows in the terminal window or -1 on error.
RLUTIL_INLINE int trows(void) {
#ifdef _WIN32
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))
		return -1;
	else
		return csbi.srWindow.Bottom - csbi.srWindow.Top + 1; // Window height
		// return csbi.dwSize.Y; // Buffer height
#else
#ifdef TIOCGSIZE
	struct ttysize ts;
	ioctl(STDIN_FILENO, TIOCGSIZE, &ts);
	return ts.ts_lines;
#elif defined(TIOCGWINSZ)
	struct winsize ts;
	ioctl(STDIN_FILENO, TIOCGWINSZ, &ts);
	return ts.ws_row;
#else // TIOCGSIZE
	return -1;
#endif // TIOCGSIZE
#endif // _WIN32
}

/// Function: tcols
/// Get the number of columns in the terminal window or -1 on error.
RLUTIL_INLINE int tcols(void) {
#ifdef _WIN32
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))
		return -1;
	else
		return csbi.srWindow.Right - csbi.srWindow.Left + 1; // Window width
		// return csbi.dwSize.X; // Buffer width
#else
#ifdef TIOCGSIZE
	struct ttysize ts;
	ioctl(STDIN_FILENO, TIOCGSIZE, &ts);
	return ts.ts_cols;
#elif defined(TIOCGWINSZ)
	struct winsize ts;
	ioctl(STDIN_FILENO, TIOCGWINSZ, &ts);
	return ts.ws_col;
#else // TIOCGSIZE
	return -1;
#endif // TIOCGSIZE
#endif // _WIN32
}	
	
// TODO: Allow optional message for anykey()?

#ifndef min
/// Function: min
/// Returns the lesser of the two arguments.
#ifdef __cplusplus
template <class T> const T& min ( const T& a, const T& b ) { return (a<b)?a:b; }
#else
#define min(a,b) (((a)<(b))?(a):(b))
#endif // __cplusplus
#endif // min

#ifndef max
/// Function: max
/// Returns the greater of the two arguments.
#ifdef __cplusplus
template <class T> const T& max ( const T& a, const T& b ) { return (b<a)?a:b; }
#else
#define max(a,b) (((b)<(a))?(a):(b))
#endif // __cplusplus
#endif // max

// Classes are here at the end so that documentation is pretty.

#ifdef __cplusplus
/// Class: CursorHider
/// RAII OOP wrapper for <rlutil.hidecursor>.
/// Hides the cursor and shows it again
/// when the object goes out of scope.
struct CursorHider {
	CursorHider() { hidecursor(); }
	~CursorHider() { showcursor(); }
};

} // namespace rlutil
#endif

#endif
