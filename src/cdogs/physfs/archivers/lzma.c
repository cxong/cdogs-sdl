/*
 * LZMA support routines for PhysicsFS.
 *
 * Please see the file lzma.txt in the lzma/ directory.
 *
 *  This file was written by Dennis Schridde, with some peeking at "7zMain.c"
 *   by Igor Pavlov.
 */

#if (defined PHYSFS_SUPPORTS_7Z)

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "physfs.h"

#define __PHYSICSFS_INTERNAL__
#include "physfs_internal.h"

#include "lzma/C/7zCrc.h"
#include "lzma/C/Archive/7z/7zIn.h"
#include "lzma/C/Archive/7z/7zExtract.h"


/* 7z internal from 7zIn.c */
extern int TestSignatureCandidate(Byte *testBytes);


#ifdef _LZMA_IN_CB
# define BUFFER_SIZE (1 << 12)
#endif /* _LZMA_IN_CB */


/*
 * Carries filestream metadata through 7z
 */
typedef struct _FileInputStream
{
    ISzAlloc allocImp; /* Allocation implementation, used by 7z */
    ISzAlloc allocTempImp; /* Temporary allocation implementation, used by 7z */
    ISzInStream inStream; /* Input stream with read callbacks, used by 7z */
    void *file; /* Filehandle, used by read implementation */
#ifdef _LZMA_IN_CB
    Byte buffer[BUFFER_SIZE]; /* Buffer, used by read implementation */
#endif /* _LZMA_IN_CB */
} FileInputStream;

/*
 * In the 7z format archives are splited into blocks, those are called folders
 * Set by LZMA_read()
*/
typedef struct _LZMAfolder
{
    PHYSFS_uint32 index; /* Index of folder in archive */
    PHYSFS_uint32 references; /* Number of files using this block */
    PHYSFS_uint8 *cache; /* Cached folder */
    size_t size; /* Size of folder */
} LZMAfolder;

/*
 * Set by LZMA_openArchive(), except folder which gets it's values
 *  in LZMA_read()
 */
typedef struct _LZMAarchive
{
    struct _LZMAfile *files; /* Array of files, size == archive->db.Database.NumFiles */
    LZMAfolder *folders; /* Array of folders, size == archive->db.Database.NumFolders */
    CArchiveDatabaseEx db; /* For 7z: Database */
    FileInputStream stream; /* For 7z: Input file incl. read and seek callbacks */
} LZMAarchive;

/* Set by LZMA_openArchive(), except offset which is set by LZMA_read() */
typedef struct _LZMAfile
{
    PHYSFS_uint32 index; /* Index of file in archive */
    LZMAarchive *archive; /* Link to corresponding archive */
    LZMAfolder *folder; /* Link to corresponding folder */
    CFileItem *item; /* For 7z: File info, eg. name, size */
    size_t offset; /* Offset in folder */
    size_t position; /* Current "virtual" position in file */
} LZMAfile;


/* Memory management implementations to be passed to 7z */

static void *SzAllocPhysicsFS(size_t size)
{
    return ((size == 0) ? NULL : allocator.Malloc(size));
} /* SzAllocPhysicsFS */


static void SzFreePhysicsFS(void *address)
{
    if (address != NULL)
        allocator.Free(address);
} /* SzFreePhysicsFS */


/* Filesystem implementations to be passed to 7z */

#ifdef _LZMA_IN_CB

/*
 * Read implementation, to be passed to 7z
 * WARNING: If the ISzInStream in 'object' is not contained in a valid FileInputStream this _will_ break horribly!
 */
SZ_RESULT SzFileReadImp(void *object, void **buffer, size_t maxReqSize,
                        size_t *processedSize)
{
    FileInputStream *s = (FileInputStream *)(object - offsetof(FileInputStream, inStream)); /* HACK! */
    PHYSFS_sint64 processedSizeLoc = 0;

    if (maxReqSize > BUFFER_SIZE)
        maxReqSize = BUFFER_SIZE;
    processedSizeLoc = __PHYSFS_platformRead(s->file, s->buffer, 1, maxReqSize);
    *buffer = s->buffer;
    if (processedSize != NULL)
        *processedSize = (size_t) processedSizeLoc;

    return SZ_OK;
} /* SzFileReadImp */

#else

/*
 * Read implementation, to be passed to 7z
 * WARNING: If the ISzInStream in 'object' is not contained in a valid FileInputStream this _will_ break horribly!
 */
SZ_RESULT SzFileReadImp(void *object, void *buffer, size_t size,
                        size_t *processedSize)
{
    FileInputStream *s = (FileInputStream *)((unsigned long)object - offsetof(FileInputStream, inStream)); /* HACK! */
    size_t processedSizeLoc = __PHYSFS_platformRead(s->file, buffer, 1, size);
    if (processedSize != 0)
        *processedSize = processedSizeLoc;
    return SZ_OK;
} /* SzFileReadImp */

#endif

/*
 * Seek implementation, to be passed to 7z
 * WARNING: If the ISzInStream in 'object' is not contained in a valid FileInputStream this _will_ break horribly!
 */
SZ_RESULT SzFileSeekImp(void *object, CFileSize pos)
{
    FileInputStream *s = (FileInputStream *)((unsigned long)object - offsetof(FileInputStream, inStream)); /* HACK! */
    if (__PHYSFS_platformSeek(s->file, (PHYSFS_uint64) pos))
        return SZ_OK;
    return SZE_FAIL;
} /* SzFileSeekImp */


/*
 * Translate Microsoft FILETIME (used by 7zip) into UNIX timestamp
 */
static PHYSFS_sint64 lzma_filetime_to_unix_timestamp(CArchiveFileTime *ft)
{
    /* MS counts in nanoseconds ... */
    const PHYSFS_uint64 FILETIME_NANOTICKS_PER_SECOND = __PHYSFS_UI64(10000000);
    /* MS likes to count seconds since 01.01.1601 ... */
    const PHYSFS_uint64 FILETIME_UNIX_DIFF = __PHYSFS_UI64(11644473600);

    PHYSFS_uint64 filetime = ft->Low | ((PHYSFS_uint64)ft->High << 32);
    return filetime/FILETIME_NANOTICKS_PER_SECOND - FILETIME_UNIX_DIFF;
} /* lzma_filetime_to_unix_timestamp */


/*
 * Compare a file with a given name, C89 stdlib variant
 * Used for sorting
 */
static int lzma_file_cmp_stdlib(const void *key, const void *object)
{
    const char *name = (const char *) key;
    LZMAfile *file = (LZMAfile *) object;
    return(strcmp(name, file->item->Name));
} /* lzma_file_cmp_posix */


/*
 * Compare two files with each other based on the name
 * Used for sorting
 */
static int lzma_file_cmp(void *_a, PHYSFS_uint32 one, PHYSFS_uint32 two)
{
    LZMAfile *files = (LZMAfile *) _a;
    return(strcmp(files[one].item->Name, files[two].item->Name));
} /* lzma_file_cmp */


/*
 * Swap two entries in the file array
 */
static void lzma_file_swap(void *_a, PHYSFS_uint32 one, PHYSFS_uint32 two)
{
    LZMAfile tmp;
    LZMAfile *first = &(((LZMAfile *) _a)[one]);
    LZMAfile *second = &(((LZMAfile *) _a)[two]);
    memcpy(&tmp, first, sizeof (LZMAfile));
    memcpy(first, second, sizeof (LZMAfile));
    memcpy(second, &tmp, sizeof (LZMAfile));
} /* lzma_file_swap */


/*
 * Find entry 'name' in 'archive'
 */
static LZMAfile * lzma_find_file(LZMAarchive *archive, const char *name)
{
    LZMAfile *file = bsearch(name, archive->files, archive->db.Database.NumFiles, sizeof(*archive->files), lzma_file_cmp_stdlib); /* FIXME: Should become __PHYSFS_search!!! */

    BAIL_IF_MACRO(file == NULL, ERR_NO_SUCH_FILE, NULL);

    return(file);
} /* lzma_find_file */


/*
 * Load metadata for the file at given index
 */
static int lzma_file_init(LZMAarchive *archive, PHYSFS_uint32 fileIndex)
{
    LZMAfile *file = &archive->files[fileIndex];
    PHYSFS_uint32 folderIndex = archive->db.FileIndexToFolderIndexMap[fileIndex];

    file->index = fileIndex; /* Store index into 7z array, since we sort our own. */
    file->archive = archive;
    file->folder = (folderIndex != (PHYSFS_uint32)-1 ? &archive->folders[folderIndex] : NULL); /* Directories don't have a folder (they contain no own data...) */
    file->item = &archive->db.Database.Files[fileIndex]; /* Holds crucial data and is often referenced -> Store link */
    file->position = 0;
    file->offset = 0; /* Offset will be set by LZMA_read() */

    return(1);
} /* lzma_load_file */


/*
 * Load metadata for all files
 */
static int lzma_files_init(LZMAarchive *archive)
{
    PHYSFS_uint32 fileIndex = 0, numFiles = archive->db.Database.NumFiles;

    for (fileIndex = 0; fileIndex < numFiles; fileIndex++ )
    {
        if (!lzma_file_init(archive, fileIndex))
        {
            return(0); /* FALSE on failure */
        }
    } /* for */

   __PHYSFS_sort(archive->files, numFiles, lzma_file_cmp, lzma_file_swap);

    return(1);
} /* lzma_load_files */


/*
 * Initialise specified archive
 */
static void lzma_archive_init(LZMAarchive *archive)
{
    memset(archive, 0, sizeof(*archive));

    /* Prepare callbacks for 7z */
    archive->stream.inStream.Read = SzFileReadImp;
    archive->stream.inStream.Seek = SzFileSeekImp;

    archive->stream.allocImp.Alloc = SzAllocPhysicsFS;
    archive->stream.allocImp.Free = SzFreePhysicsFS;

    archive->stream.allocTempImp.Alloc = SzAllocPhysicsFS;
    archive->stream.allocTempImp.Free = SzFreePhysicsFS;
}


/*
 * Deinitialise archive
 */
static void lzma_archive_exit(LZMAarchive *archive)
{
    /* Free arrays */
    allocator.Free(archive->folders);
    allocator.Free(archive->files);
    allocator.Free(archive);
}

/*
 * Wrap all 7z calls in this, so the physfs error state is set appropriately.
 */
static int lzma_err(SZ_RESULT rc)
{
    switch (rc)
    {
        case SZ_OK: /* Same as LZMA_RESULT_OK */
            break;
        case SZE_DATA_ERROR: /* Same as LZMA_RESULT_DATA_ERROR */
            __PHYSFS_setError(ERR_DATA_ERROR);
            break;
        case SZE_OUTOFMEMORY:
            __PHYSFS_setError(ERR_OUT_OF_MEMORY);
            break;
        case SZE_CRC_ERROR:
            __PHYSFS_setError(ERR_CORRUPTED);
            break;
        case SZE_NOTIMPL:
            __PHYSFS_setError(ERR_NOT_IMPLEMENTED);
            break;
        case SZE_FAIL:
            __PHYSFS_setError(ERR_UNKNOWN_ERROR);  /* !!! FIXME: right? */
            break;
        case SZE_ARCHIVE_ERROR:
            __PHYSFS_setError(ERR_CORRUPTED);  /* !!! FIXME: right? */
            break;
        default:
            __PHYSFS_setError(ERR_UNKNOWN_ERROR);
    } /* switch */

    return(rc);
} /* lzma_err */


static PHYSFS_sint64 LZMA_read(fvoid *opaque, void *outBuffer,
                               PHYSFS_uint32 objSize, PHYSFS_uint32 objCount)
{
    LZMAfile *file = (LZMAfile *) opaque;

    size_t wantedSize = objSize*objCount;
    size_t remainingSize = file->item->Size - file->position;
    size_t fileSize = 0;

    BAIL_IF_MACRO(wantedSize == 0, NULL, 0); /* quick rejection. */
    BAIL_IF_MACRO(remainingSize == 0, ERR_PAST_EOF, 0);

    if (remainingSize < wantedSize)
    {
        wantedSize = remainingSize - (remainingSize % objSize);
        objCount = (PHYSFS_uint32) (remainingSize / objSize);
        BAIL_IF_MACRO(objCount == 0, ERR_PAST_EOF, 0); /* quick rejection. */
        __PHYSFS_setError(ERR_PAST_EOF); /* this is always true here. */
    } /* if */

    /* Only decompress the folder if it is not allready cached */
    if (file->folder->cache == NULL)
    {
        int rc = lzma_err(SzExtract(
            &file->archive->stream.inStream, /* compressed data */
            &file->archive->db, /* 7z's database, containing everything */
            file->index, /* Index into database arrays */
            /* Index of cached folder, will be changed by SzExtract */
            &file->folder->index,
            /* Cache for decompressed folder, allocated/freed by SzExtract */
            &file->folder->cache,
            /* Size of cache, will be changed by SzExtract */
            &file->folder->size,
            /* Offset of this file inside the cache, set by SzExtract */
            &file->offset,
            &fileSize, /* Size of this file */
            &file->archive->stream.allocImp,
            &file->archive->stream.allocTempImp));

        if (rc != SZ_OK)
            return -1;
    } /* if */

    /* Copy wanted bytes over from cache to outBuffer */
    memcpy(outBuffer,
            (file->folder->cache +
                    file->offset + file->position),
            wantedSize);
    file->position += wantedSize; /* Increase virtual position */

    return objCount;
} /* LZMA_read */


static PHYSFS_sint64 LZMA_write(fvoid *opaque, const void *buf,
                               PHYSFS_uint32 objSize, PHYSFS_uint32 objCount)
{
    BAIL_MACRO(ERR_NOT_SUPPORTED, -1);
} /* LZMA_write */


static int LZMA_eof(fvoid *opaque)
{
    LZMAfile *file = (LZMAfile *) opaque;
    return (file->position >= file->item->Size);
} /* LZMA_eof */


static PHYSFS_sint64 LZMA_tell(fvoid *opaque)
{
    LZMAfile *file = (LZMAfile *) opaque;
    return (file->position);
} /* LZMA_tell */


static int LZMA_seek(fvoid *opaque, PHYSFS_uint64 offset)
{
    LZMAfile *file = (LZMAfile *) opaque;

    BAIL_IF_MACRO(offset < 0, ERR_SEEK_OUT_OF_RANGE, 0);
    BAIL_IF_MACRO(offset > file->item->Size, ERR_PAST_EOF, 0);

    file->position = offset; /* We only use a virtual position... */

    return 1;
} /* LZMA_seek */


static PHYSFS_sint64 LZMA_fileLength(fvoid *opaque)
{
    LZMAfile *file = (LZMAfile *) opaque;
    return (file->item->Size);
} /* LZMA_fileLength */


static int LZMA_fileClose(fvoid *opaque)
{
    LZMAfile *file = (LZMAfile *) opaque;

    BAIL_IF_MACRO(file->folder == NULL, ERR_NOT_A_FILE, 0);

	/* Only decrease refcount if someone actually requested this file... Prevents from overflows and close-on-open... */
    if (file->folder->references > 0)
        file->folder->references--;
    if (file->folder->references == 0)
    {
        /* Free the cache which might have been allocated by LZMA_read() */
        allocator.Free(file->folder->cache);
        file->folder->cache = NULL;
    }

    return(1);
} /* LZMA_fileClose */


static int LZMA_isArchive(const char *filename, int forWriting)
{
    PHYSFS_uint8 sig[k7zSignatureSize];
    void *in;

    BAIL_IF_MACRO(forWriting, ERR_ARC_IS_READ_ONLY, 0);

    in = __PHYSFS_platformOpenRead(filename);
    BAIL_IF_MACRO(in == NULL, NULL, 0);

    /* Read signature bytes */
    if (__PHYSFS_platformRead(in, sig, k7zSignatureSize, 1) != 1)
    {
        __PHYSFS_platformClose(in); /* Don't forget to close the file before returning... */
        BAIL_MACRO(NULL, 0);
    }

    __PHYSFS_platformClose(in);

    /* Test whether sig is the 7z signature */
    return(TestSignatureCandidate(sig));
} /* LZMA_isArchive */


static void *LZMA_openArchive(const char *name, int forWriting)
{
    size_t len = 0;
    LZMAarchive *archive = NULL;

    BAIL_IF_MACRO(forWriting, ERR_ARC_IS_READ_ONLY, NULL);
    BAIL_IF_MACRO(!LZMA_isArchive(name,forWriting), ERR_UNSUPPORTED_ARCHIVE, 0);

    archive = (LZMAarchive *) allocator.Malloc(sizeof (LZMAarchive));
    BAIL_IF_MACRO(archive == NULL, ERR_OUT_OF_MEMORY, NULL);

    lzma_archive_init(archive);

    if ( (archive->stream.file = __PHYSFS_platformOpenRead(name)) == NULL )
    {
        __PHYSFS_platformClose(archive->stream.file);
        lzma_archive_exit(archive);
        return(NULL); /* Error is set by platformOpenRead! */
    }

    CrcGenerateTable();
    SzArDbExInit(&archive->db);
    if (lzma_err(SzArchiveOpen(&archive->stream.inStream,
                               &archive->db,
                               &archive->stream.allocImp,
                               &archive->stream.allocTempImp)) != SZ_OK)
    {
        SzArDbExFree(&archive->db, SzFreePhysicsFS);
        __PHYSFS_platformClose(archive->stream.file);
        lzma_archive_exit(archive);
        return NULL; /* Error is set by lzma_err! */
    } /* if */

    len = archive->db.Database.NumFiles * sizeof (LZMAfile);
    archive->files = (LZMAfile *) allocator.Malloc(len);
    if (archive->files == NULL)
    {
        SzArDbExFree(&archive->db, SzFreePhysicsFS);
        __PHYSFS_platformClose(archive->stream.file);
        lzma_archive_exit(archive);
        BAIL_MACRO(ERR_OUT_OF_MEMORY, NULL);
    }

    /*
     * Init with 0 so we know when a folder is already cached
     * Values will be set by LZMA_openRead()
     */
    memset(archive->files, 0, len);

    len = archive->db.Database.NumFolders * sizeof (LZMAfolder);
    archive->folders = (LZMAfolder *) allocator.Malloc(len);
    if (archive->folders == NULL)
    {
        SzArDbExFree(&archive->db, SzFreePhysicsFS);
        __PHYSFS_platformClose(archive->stream.file);
        lzma_archive_exit(archive);
        BAIL_MACRO(ERR_OUT_OF_MEMORY, NULL);
    }

    /*
     * Init with 0 so we know when a folder is already cached
     * Values will be set by LZMA_read()
     */
    memset(archive->folders, 0, len);

    if(!lzma_files_init(archive))
    {
        SzArDbExFree(&archive->db, SzFreePhysicsFS);
        __PHYSFS_platformClose(archive->stream.file);
        lzma_archive_exit(archive);
        BAIL_MACRO(ERR_UNKNOWN_ERROR, NULL);
    }

    return(archive);
} /* LZMA_openArchive */


/*
 * Moved to seperate function so we can use alloca then immediately throw
 *  away the allocated stack space...
 */
static void doEnumCallback(PHYSFS_EnumFilesCallback cb, void *callbackdata,
                           const char *odir, const char *str, size_t flen)
{
    char *newstr = __PHYSFS_smallAlloc(flen + 1);
    if (newstr == NULL)
        return;

    memcpy(newstr, str, flen);
    newstr[flen] = '\0';
    cb(callbackdata, odir, newstr);
    __PHYSFS_smallFree(newstr);
} /* doEnumCallback */


static void LZMA_enumerateFiles(dvoid *opaque, const char *dname,
                                int omitSymLinks, PHYSFS_EnumFilesCallback cb,
                                const char *origdir, void *callbackdata)
{
    size_t dlen = strlen(dname),
           dlen_inc = dlen + ((dlen > 0) ? 1 : 0);
    LZMAarchive *archive = (LZMAarchive *) opaque;
    LZMAfile *file = NULL,
            *lastFile = &archive->files[archive->db.Database.NumFiles];
        if (dlen)
        {
            file = lzma_find_file(archive, dname);
            if (file != NULL) /* if 'file' is NULL it should stay so, otherwise errors will not be handled */
                file += 1;
        }
        else
        {
            file = archive->files;
        }

    BAIL_IF_MACRO(file == NULL, ERR_NO_SUCH_FILE, );

    while (file < lastFile)
    {
        const char * fname = file->item->Name;
        const char * dirNameEnd = fname + dlen_inc;

        if (strncmp(dname, fname, dlen) != 0) /* Stop after mismatch, archive->files is sorted */
            break;

        if (strchr(dirNameEnd, '/')) /* Skip subdirs */
        {
            file++;
            continue;
        }

        /* Do the actual callback... */
        doEnumCallback(cb, callbackdata, origdir, dirNameEnd, strlen(dirNameEnd));

        file++;
    }
} /* LZMA_enumerateFiles */


static int LZMA_exists(dvoid *opaque, const char *name)
{
    LZMAarchive *archive = (LZMAarchive *) opaque;
    return(lzma_find_file(archive, name) != NULL);
} /* LZMA_exists */


static PHYSFS_sint64 LZMA_getLastModTime(dvoid *opaque,
                                         const char *name,
                                         int *fileExists)
{
    LZMAarchive *archive = (LZMAarchive *) opaque;
    LZMAfile *file = lzma_find_file(archive, name);

    *fileExists = (file != NULL);

    BAIL_IF_MACRO(file == NULL, NULL, -1);
	BAIL_IF_MACRO(!file->item->IsLastWriteTimeDefined, NULL, -1); /* write-time may not be defined for every file */

    return(lzma_filetime_to_unix_timestamp(&file->item->LastWriteTime));
} /* LZMA_getLastModTime */


static int LZMA_isDirectory(dvoid *opaque, const char *name, int *fileExists)
{
    LZMAarchive *archive = (LZMAarchive *) opaque;
    LZMAfile *file = lzma_find_file(archive, name);

    *fileExists = (file != NULL);

    return(file == NULL ? 0 : file->item->IsDirectory);
} /* LZMA_isDirectory */


static int LZMA_isSymLink(dvoid *opaque, const char *name, int *fileExists)
{
    BAIL_MACRO(ERR_NOT_SUPPORTED, 0);
} /* LZMA_isSymLink */


static fvoid *LZMA_openRead(dvoid *opaque, const char *name, int *fileExists)
{
    LZMAarchive *archive = (LZMAarchive *) opaque;
    LZMAfile *file = lzma_find_file(archive, name);

    *fileExists = (file != NULL);
    BAIL_IF_MACRO(file == NULL, ERR_NO_SUCH_FILE, NULL);
    BAIL_IF_MACRO(file->folder == NULL, ERR_NOT_A_FILE, NULL);

    file->position = 0;
    file->folder->references++; /* Increase refcount for automatic cleanup... */

    return(file);
} /* LZMA_openRead */


static fvoid *LZMA_openWrite(dvoid *opaque, const char *filename)
{
    BAIL_MACRO(ERR_NOT_SUPPORTED, NULL);
} /* LZMA_openWrite */


static fvoid *LZMA_openAppend(dvoid *opaque, const char *filename)
{
    BAIL_MACRO(ERR_NOT_SUPPORTED, NULL);
} /* LZMA_openAppend */


static void LZMA_dirClose(dvoid *opaque)
{
    LZMAarchive *archive = (LZMAarchive *) opaque;
    PHYSFS_uint32 fileIndex = 0, numFiles = archive->db.Database.NumFiles;

    for (fileIndex = 0; fileIndex < numFiles; fileIndex++)
    {
        LZMA_fileClose(&archive->files[fileIndex]);
    } /* for */

    SzArDbExFree(&archive->db, SzFreePhysicsFS);
    __PHYSFS_platformClose(archive->stream.file);
    lzma_archive_exit(archive);
} /* LZMA_dirClose */


static int LZMA_remove(dvoid *opaque, const char *name)
{
    BAIL_MACRO(ERR_NOT_SUPPORTED, 0);
} /* LZMA_remove */


static int LZMA_mkdir(dvoid *opaque, const char *name)
{
    BAIL_MACRO(ERR_NOT_SUPPORTED, 0);
} /* LZMA_mkdir */


const PHYSFS_ArchiveInfo __PHYSFS_ArchiveInfo_LZMA =
{
    "7Z",
    LZMA_ARCHIVE_DESCRIPTION,
    "Dennis Schridde <devurandom@gmx.net>",
    "http://icculus.org/physfs/",
};


const PHYSFS_Archiver __PHYSFS_Archiver_LZMA =
{
    &__PHYSFS_ArchiveInfo_LZMA,
    LZMA_isArchive,          /* isArchive() method      */
    LZMA_openArchive,        /* openArchive() method    */
    LZMA_enumerateFiles,     /* enumerateFiles() method */
    LZMA_exists,             /* exists() method         */
    LZMA_isDirectory,        /* isDirectory() method    */
    LZMA_isSymLink,          /* isSymLink() method      */
    LZMA_getLastModTime,     /* getLastModTime() method */
    LZMA_openRead,           /* openRead() method       */
    LZMA_openWrite,          /* openWrite() method      */
    LZMA_openAppend,         /* openAppend() method     */
    LZMA_remove,             /* remove() method         */
    LZMA_mkdir,              /* mkdir() method          */
    LZMA_dirClose,           /* dirClose() method       */
    LZMA_read,               /* read() method           */
    LZMA_write,              /* write() method          */
    LZMA_eof,                /* eof() method            */
    LZMA_tell,               /* tell() method           */
    LZMA_seek,               /* seek() method           */
    LZMA_fileLength,         /* fileLength() method     */
    LZMA_fileClose           /* fileClose() method      */
};

#endif  /* defined PHYSFS_SUPPORTS_7Z */

/* end of lzma.c ... */

