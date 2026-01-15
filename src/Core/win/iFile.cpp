#include "iFile.h"

#include "iTRC.h"

#include "xFile.h"
#include "xDebug.h"
#include "xMath.h"
#include "xTRC.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

struct file_queue_entry
{
    tag_xFile* file;
    void* buf;
    U32 size;
    U32 offset;
    IFILE_READSECTOR_STATUS stat;
    void (*callback)(tag_xFile* file);
    U32 asynckey;
};

file_queue_entry file_queue[4];

static U32 tbuffer[1024 + 8];
static U32* buffer32;
volatile U32 iFileSyncAsyncReadActive;

void iFileInit()
{
    // TODO: Commented out code is whats in the decomp repo. Reimplement later
        
    xASSERT(sizeof(iFile::fd) == sizeof(FILE*));

    //buffer32 = (U32*)OSRoundUp32B(tbuffer);
}

void iFileExit()
{
}

// TODO: Every function that is commented out below, access's a gamecube specific offset or function. Return to this when viable

U32* iFileLoad(const char* name, U32* buffer, U32* size)
{
    char fullpath[IFILE_NAMELEN_MAX];
    iFileFullPath(name, fullpath);

    tag_xFile file;
    iFileOpen(name, IFILE_OPEN_ABSPATH, &file);

    U32 fsize = iFileGetSize(&file);
    if (!buffer) {
        buffer = (U32*)malloc((fsize + 31) & ~31);
    }

    iFileRead(&file, buffer, fsize);

    if (size) {
        *size = fsize;
    }

    iFileClose(&file);

    return buffer;
}

U32 iFileOpen(const char* name, S32 flags, tag_xFile* file) // TODO: investigate this function. Commented out code is the decomp counterpart to seils code
{
    iFile* ps = &file->ps;
    char openFlags[16];

    if (flags & IFILE_OPEN_ABSPATH) {
        strcpy(ps->path, name);
    }
    else {
        iFileFullPath(name, ps->path);
    }

    if (flags & IFILE_OPEN_WRITE) {
        strcpy(openFlags, "wb");
    }
    else if (flags & IFILE_OPEN_READ) {
        strcpy(openFlags, "rb");
    }
    else {
        strcpy(openFlags, "rb");
    }

    ps->fd = (S32)fopen(ps->path, openFlags);
    if (!ps->fd) return 1;

    ps->flags = IFILE_OPENED;
    iFileSeek(file, 0, IFILE_SEEK_SET);

    return 0;

    /*tag_iFile* ps = &file->ps;

    if (flags & IFILE_OPEN_ABSPATH)
    {
        strcpy(ps->path, name);
    }
    else
    {
        iFileFullPath(name, ps->path);
    }

    ps->entrynum = DVDConvertPathToEntrynum(ps->path);

    if (ps->entrynum == -1)
    {
        return 1;
    }

    if (!DVDFastOpen(ps->entrynum, &ps->fileInfo))
    {
        ps->entrynum = -1;
        return 1;
    }

    ps->unkC4 = 0;
    ps->flags = 0x1;

    iFileSeek(file, 0, IFILE_SEEK_SET);

    return 0;*/
}

S32 iFileSeek(tag_xFile* file, S32 offset, S32 whence) // TODO: investigate this. Using seils code here instead of decomp code
{
    iFile* ps = &file->ps;
    int mode;

    switch (whence) {
    case IFILE_SEEK_SET:
        mode = SEEK_SET;
        break;
    case IFILE_SEEK_CUR:
        mode = SEEK_CUR;
        break;
    case IFILE_SEEK_END:
        mode = SEEK_END;
        break;
    default:
        xASSERTFAILMSG("bad seek mode");
        mode = SEEK_SET;
        break;
    }

    return (S32)fseek((FILE*)ps->fd, offset, mode);

    /*tag_iFile* ps = &file->ps;
    S32 position, new_pos;

    switch (whence)
    {
    case IFILE_SEEK_SET:
    {
        new_pos = offset;
        break;
    }
    case IFILE_SEEK_CUR:
    {
        if (DVDGetCommandBlockStatus(&ps->fileInfo.cb) == DVD_STATE_BUSY)
        {
            return -1;
        }

        new_pos = offset + ps->offset;
        break;
    }
    case IFILE_SEEK_END:
    {
        new_pos = ps->fileInfo.length - offset;

        if (new_pos < 0)
        {
            new_pos = 0;
        }

        break;
    }
    default:
    {
        new_pos = offset;
        break;
    }
    }

    ps->offset = new_pos;

    return ps->offset;*/
}

static void ifilereadCB(tag_xFile* file)
{
    iFileSyncAsyncReadActive = 0;
}

U32 iFileRead(tag_xFile* file, void* buf, U32 size)
{
    iFile* ps = &file->ps;
    U32 numItemsRead = (U32)fread(buf, 1, size, (FILE*)ps->fd);
    return numItemsRead;
}

//static void async_cb(s32 result, DVDFileInfo* fileInfo)
//{
//    file_queue_entry* entry = &file_queue[(S32)fileInfo->cb.userData];
//    s32 r7 = DVD_RESULT_FATAL_ERROR;
//
//    switch (result)
//    {
//    case DVD_RESULT_FATAL_ERROR:
//    {
//        xTRCDisk(TRC_DiskFatal);
//        return;
//    }
//    case DVD_RESULT_GOOD:
//    case DVD_RESULT_IGNORED:
//    {
//        if (result >= DVD_RESULT_GOOD)
//        {
//            r7 = result;
//        }
//
//        break;
//    }
//    }
//
//    if (r7 < DVD_RESULT_GOOD)
//    {
//        entry->stat = IFILE_RDSTAT_FAIL;
//
//        if (entry->callback)
//        {
//            entry->callback(entry->file);
//        }
//    }
//    else if (r7 + entry->offset >= entry->size ||
//             r7 + entry->offset + entry->file->ps.offset >= entry->file->ps.fileInfo.length)
//    {
//        entry->stat = IFILE_RDSTAT_DONE;
//        entry->offset = entry->size;
//
//        if (entry->callback)
//        {
//            entry->callback(entry->file);
//        }
//
//        entry->file->ps.asynckey = -1;
//    }
//    else
//    {
//        entry->offset += r7;
//        entry->stat = IFILE_RDSTAT_INPROG;
//
//        s32 length;
//        if ((entry->size - entry->offset < 0x8000))
//        {
//            length = ALIGN(entry->size - entry->offset, 4);
//        }
//        else
//        {
//            length = 0x10000 - 0x8000;
//        }
//
//        if (length + entry->offset + entry->file->ps.offset > entry->file->ps.fileInfo.length)
//        {
//            // length = OSRoundUp32B(entry->file->ps.fileInfo.length - entry->file->ps.offset -
//            //     entry->offset);
//            length = entry->file->ps.fileInfo.length;
//            length -= entry->file->ps.offset;
//            length -= entry->offset;
//            length = length + 32 - 1;
//            length = length & ~(32 - 1);
//        }
//
//        void* addr = (void*)((U32)entry->buf + entry->offset);
//        DVDReadAsync(&entry->file->ps.fileInfo, addr, length,
//                     entry->file->ps.offset + entry->offset, async_cb);
//    }
//}

typedef void(*iFileReadAsyncDoneCallback)(tag_xFile*);

S32 iFileReadAsync(tag_xFile* file, void* buf, U32 asize, iFileReadAsyncDoneCallback dcb, S32 priority)
{
    iFile* ps = &file->ps;
    fread(buf, 1, asize, (FILE*)ps->fd);
    if (dcb) dcb(file);
    return 0;
}

IFILE_READSECTOR_STATUS iFileReadAsyncStatus(S32 key, S32* amtToFar)
{
    return IFILE_RDSTAT_DONE;
}

U32 iFileClose(tag_xFile* file)
{
    iFile* ps = &file->ps;
    fclose((FILE*)ps->fd);
    ps->flags = 0;
    return 0;
}

U32 iFileGetSize(tag_xFile* file)
{
    iFile* ps = &file->ps;
    long cur = ftell((FILE*)ps->fd);
    fseek((FILE*)ps->fd, 0, SEEK_END);
    long end = ftell((FILE*)ps->fd);
    fseek((FILE*)ps->fd, cur, SEEK_SET);
    return (U32)end;
}

void iFileReadStop()
{
}

void iFileFullPath(const char* relname, char* fullname)
{
    strcpy(fullname, relname);
}

void iFileSetPath(char* path)
{
}

U32 iFileFind(const char* name, tag_xFile* file)
{
    return iFileOpen(name, 0, file);
}

void iFileGetInfo(tag_xFile* file, U32* starting_sector, U32* size_in_bytes)
{
    iFile* ps = &file->ps;

    if (starting_sector) {
        *starting_sector = 0;
    }

    if (size_in_bytes) {
        *size_in_bytes = iFileGetSize(file);
    }
}
