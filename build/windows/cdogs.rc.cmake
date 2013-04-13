#include <windows.h>

1 ICON DISCARDABLE "cdogs-icon.ico"

1 VERSIONINFO
FILEOS		VOS_NT_WINDOWS32
FILETYPE	VFT_APP
PRODUCTVERSION	@VERSION_RC@
FILEVERSION	@VERSION_RC@

BEGIN
	BLOCK "StringFileInfo"
	BEGIN
		BLOCK "040904E4"
		BEGIN
			VALUE "FileDescription", "C-Dogs SDL Action/Arcade Game\0"
			VALUE "FileVersion", "@VERSION@\0"
			VALUE "Comments", "@WEBSITE@\0"
			VALUE "InternalName", "cdogs\0"
			VALUE "LegalCopyright", "Copyright (C) 1997-2013 Various\0"
			VALUE "OriginalFilename", "cdogs-sdl.exe\0"
			VALUE "ProductName", "C-Dogs SDL\0"
			VALUE "ProductVersion", "@VERSION@\0"
		END
	END
	
	BLOCK "VarFileInfo"
	BEGIN
		VALUE "Translation", 0x409, 0
	END
END
