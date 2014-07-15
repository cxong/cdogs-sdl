local zlibfiles = {
	"zlib123/adler32.c",
    "zlib123/compress.c",
    "zlib123/crc32.c",
    "zlib123/deflate.c",
    "zlib123/gzio.c",
    "zlib123/infback.c",
    "zlib123/inffast.c",
    "zlib123/inflate.c",
    "zlib123/inftrees.c",
    "zlib123/trees.c",
    "zlib123/uncompr.c",
    "zlib123/zutil.c"}
	
local lzmafiles = {
	"lzma/C/7zCrc.c",
    "lzma/C/Archive/7z/7zBuffer.c",
    "lzma/C/Archive/7z/7zDecode.c",
    "lzma/C/Archive/7z/7zExtract.c",
    "lzma/C/Archive/7z/7zHeader.c",
    "lzma/C/Archive/7z/7zIn.c",
    "lzma/C/Archive/7z/7zItem.c",
    "lzma/C/Archive/7z/7zMethodID.c",
    "lzma/C/Compress/Branch/BranchX86.c",
    "lzma/C/Compress/Branch/BranchX86_2.c",
    "lzma/C/Compress/Lzma/LzmaDecode.c"}
	
local physfsfiles = {
	"physfs.c",
    "physfs_byteorder.c",
    "physfs_unicode.c",
    "platform/os2.c",
    "platform/pocketpc.c",
    "platform/posix.c",
    "platform/unix.c",
    "platform/macosx.c",
    "platform/windows.c",
    "archivers/dir.c",
    "archivers/grp.c",
    "archivers/hog.c",
    "archivers/lzma.c",
    "archivers/mvl.c",
    "archivers/qpak.c",
    "archivers/wad.c",
    "archivers/zip.c"}

project "physfs"
	version = "2.0.3"
	
	kind ("StaticLib")
	language "C"
	
	includedirs { "." }
	files ( physfsfiles )
	
	--build with zip support
	defines { "PHYSFS_SUPPORTS_ZIP=1" }
	
	--zlib dep
		includedirs { LDIR_THIRDPARTY .. "zlib/" }
		links { "zlib" }
	
	targetdir( LDIR_THIRDPARTY_LIB )
	location( LDIR_THIRDPARTY_BUILD )

	suffix_macro ( version, true )