@echo off
setlocal enabledelayedexpansion

set EXTRACT_COMMAND=7z x -y

rem PLEASE NO SPACES IN SDL2_* VARIABLES

set SDL2_URL=http://www.libsdl.org/release/SDL2-2.0.5-win32-x86.zip
set SDL2_ARCHIVE=SDL2-2.0.5-win32-x86.zip

set SDL2_IMAGE_URL=http://www.libsdl.org/projects/SDL_image/release/SDL2_image-2.0.1-win32-x86.zip
set SDL2_IMAGE_ARCHIVE=SDL2_image-2.0.1-win32-x86.zip

set SDL2_MIXER_URL=https://www.libsdl.org/projects/SDL_mixer/release/SDL2_mixer-2.0.1-win32-x86.zip
set SDL2_MIXER_ARCHIVE=SDL2_mixer-2.0.1-win32-x86.zip

rem ========================================================


set DESTDIR=%1
set DOWNLOAD_COMMAND=%~2
if "!DESTDIR!" == "" (
	echo Usage %0 destination_dir [download_command]
	echo Assume you have 7z in your PATH
	exit /b 1
)

if not exist !DESTDIR!\* (
	echo Directory "!DESTDIR!" doesn't exist. Creating it...
	md "!DESTDIR!"
) else (
	echo Directory "!DESTDIR!" exists...
)

echo cd into "!DESTDIR!"
cd "!DESTDIR!"

call :downloadIfNeeded !SDL2_URL!
call :downloadIfNeeded !SDL2_IMAGE_URL!
call :downloadIfNeeded !SDL2_MIXER_URL!

%EXTRACT_COMMAND% !SDL2_ARCHIVE!
%EXTRACT_COMMAND% !SDL2_IMAGE_ARCHIVE!
%EXTRACT_COMMAND% !SDL2_MIXER_ARCHIVE!

exit /b
rem ========================================================


rem --------------------------------------------------------
rem Downloads file and places it as destination file IN CURRENT DIR
rem	%1 -- URL
rem 	%2 -- destination file
:download
	if "!DOWNLOAD_COMMAND!" == "" (
		rem TODO: bitsadmin is deprecated, but it is the only method to download file from pure cmd. 
		rem TODO: It will have to be fixed some day

		for /F %%i in ("%1") do bitsadmin.exe /transfer "Download  %%~ni%%~xi" %1  "%cd%/%%~ni%%~xi"
	) else (
		!DOWNLOAD_COMMAND! %1
	)
exit /b
rem --------------------------------------------------------


rem --------------------------------------------------------
rem Downloads file IF FILE IS MISSING IN CURRENT DIR
rem	%1 -- URL
:downloadIfNeeded
	for /F %%i in ("%1") do if exist "%%~ni%%~xi" (
			echo "%%~ni%%~xi" already exists. Skipping download...
		) else (
			echo Downloading "%%~ni%%~xi" ...
			call :download %1 
		)
exit /b
rem --------------------------------------------------------
