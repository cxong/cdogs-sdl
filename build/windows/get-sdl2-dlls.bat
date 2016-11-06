@echo off
setlocal enabledelayedexpansion

set EXTRACT_COMMAND=7z x -y

rem PLEASE NO SPACES IN SDL2_* VARIABLES

set SDL2_URL="http://www.libsdl.org/release/SDL2-2.0.5-win32-x86.zip"
set SDL2_ARCHIVE=SDL2-2.0.5-win32-x86.zip

set SDL2_IMAGE_URL="http://www.libsdl.org/projects/SDL_image/release/SDL2_image-2.0.1-win32-x86.zip"
set SDL2_IMAGE_ARCHIVE=SDL2_image-2.0.1-win32-x86.zip

set SDL2_MIXER_URL="https://www.libsdl.org/projects/SDL_mixer/release/SDL2_mixer-2.0.1-win32-x86.zip"
set SDL2_MIXER_ARCHIVE=SDL2_mixer-2.0.1-win32-x86.zip

rem ========================================================


set DESTDIR=%1
if "!DESTDIR!" == "" (
	echo Usage %0 destination_dir
	echo Assume you have 7z in your PATH
	exit /b 1
)

if not exist !DESTDIR!\* (
	echo Directory "!DESTDIR!" doesn't exist. Creating it...
	md "!DESTDIR!"
) else (
	echo Directory "!DESTDIR!" exists...
)

echo "cd into "!DESTDIR!"
cd "!DESTDIR!"

call :downloadIfNeeded "!SDL2_URL!" "%cd%/!SDL2_ARCHIVE!"
call :downloadIfNeeded "!SDL2_IMAGE_URL!" "%cd%/!SDL2_IMAGE_ARCHIVE!"
call :downloadIfNeeded "!SDL2_MIXER_URL!" "%cd%/!SDL2_MIXER_ARCHIVE!"

%EXTRACT_COMMAND% !SDL2_ARCHIVE!
%EXTRACT_COMMAND% !SDL2_IMAGE_ARCHIVE!
%EXTRACT_COMMAND% !SDL2_MIXER_ARCHIVE!

exit /b
rem ========================================================


rem --------------------------------------------------------
rem Downloads file and places it as destination file
rem	%1 -- URL
rem 	%2 -- destination file
:download
	rem TODO: bitsadmin is deprecated, but it is the only method to download file from pure cmd. 
	rem TODO: It will have to be fixed some day

	bitsadmin.exe /transfer "Download %2" %1 %2
exit /b
rem --------------------------------------------------------


rem --------------------------------------------------------
rem Downloads file IF DESTINATION FILE IS MISSING
rem	%1 -- URL
rem 	%2 -- destination file
:downloadIfNeeded
	if exist "%2" (
		echo "%2" already exists. Skipping download...
	) else (
		echo Downloading "%2" ...
		call :download %1 %2
	)
exit /b
rem --------------------------------------------------------
