/*++

Copyright (c) Microsoft Corporation

Module Name:
    BlockFileTest.c

Abstract:
    Tests KXM BlockFile functionality.

--*/

#define _GNU_SOURCE
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
// TODO fix https://bugzilla.redhat.com/show_bug.cgi?id=1476120
#if defined(LINUX_DISTRIB_REDHAT)
#include <linux/falloc.h>
#endif
#include <string.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <ctype.h>
#include <stdarg.h>
#include <malloc.h>
#include <errno.h>
#include <pthread.h>
#include <linux/fs.h>
#include <linux/fiemap.h>
#include "KUShared.h"
#include "KXMApi.h"
#include "KXMKApi.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include "strsafe.h"
#include "ktlpal.h"
#include "paldef.h"


#define BLOCK_FILE_NAME "/tmp/testblk"
#define KXM_BLOCK_FILE_NAME "/tmp/testblk"
#define KB(x)   ((x)<<10)
#define MB(x)   ((x)<<20)
#define BLOCK_FILE_SIZE  MB(64)
#define DEFAULT_TIMEOUT 10000

#define Message(a, b...) printf("[%s@%d] " a "\n", __FUNCTION__, __LINE__, ##b)

#define EXIT_ON_ERROR(condition, msg, args...)  if((condition)) { printf("[%s@%d]: GetLastError %#x, " msg "\n", __FUNCTION__, __LINE__, GetLastError(), ##args); exit(-1);}
#define LSB32(x)    ((x) & 0xffffffffU)
#define MSB32(x)    ((x>>32) & 0xffffffffU)


struct IOContext 
{
    OVERLAPPED Overlapped;
    void *rwBuffer;
    unsigned long long startTicks;
};


//Tests following failure scenarios,
// - Pass invalid KXM device ID.
// - Pass invalid file name.
// - Pass unaligned page address.
// - Pass invalid block file handle.
// - Create test file with root and drop root privilege. Try 
//   to open this file using KXM APi and it should fail.
static NTSTATUS BlockFileFailureTests(HANDLE DevFD, int FileFD)
{
    HANDLE blockHandle = NULL;
    char *buffer; 
    long alignment = sysconf(_SC_PAGESIZE);
    struct IOContext context;
    NTSTATUS status;
    HANDLE compHandle;
	ULONG flags;

    posix_memalign((void **)&buffer, alignment, alignment*2);
    EXIT_ON_ERROR(buffer == NULL, "Couldn't allocate memory.\n");

    //Pass invalid KXM device ID.
	flags = FileFD != -1 ? KXMFLAG_USE_FILESYSTEM_ONLY : 0;
    blockHandle = KXMCreateFile(INVALID_HANDLE_VALUE, FileFD, "/junk", NULL, FALSE, BLOCK_FLUSH, 0, flags);
    EXIT_ON_ERROR(blockHandle != INVALID_HANDLE_VALUE, "Passing Invalid KXM dev id succeeded!\n");

    //Pass invalid file name.
	flags = FileFD != -1 ? KXMFLAG_USE_FILESYSTEM_ONLY : 0;
    blockHandle = KXMCreateFile(DevFD, FileFD, "/junk", NULL, FALSE, BLOCK_FLUSH, 0, flags);
    EXIT_ON_ERROR(blockHandle != INVALID_HANDLE_VALUE, "Opening junk file succeeded!\n");
   
    //Create a BlockFile.
	flags = FileFD != -1 ? KXMFLAG_USE_FILESYSTEM_ONLY : 0;
    blockHandle = KXMCreateFile(DevFD, FileFD, KXM_BLOCK_FILE_NAME, NULL, FALSE, BLOCK_FLUSH, 0, flags);
    EXIT_ON_ERROR(blockHandle == INVALID_HANDLE_VALUE, "Couldn't open BlockFile.\n");

    //Submit request without associating completion port.
    memset(&context, 0, sizeof(struct IOContext));
    status = KXMReadFileAsync(DevFD, blockHandle, buffer, 1, (LPOVERLAPPED)&context);
    EXIT_ON_ERROR(status != STATUS_INVALID_DEVICE_REQUEST, "KXMReadFile accepted unaligned address!\n");

    status = KXMWriteFileAsync(DevFD, blockHandle, buffer, 1, (LPOVERLAPPED)&context);
    EXIT_ON_ERROR(status != STATUS_INVALID_DEVICE_REQUEST, "KXMWriteFile accepted unaligned address!\n");
   
    //Associate completion port.
    compHandle = KXMCreateIoCompletionPort(DevFD, blockHandle, NULL, 0ULL, 0);
    EXIT_ON_ERROR(compHandle == NULL, "Completion Port Creation failed!\n");

    //Pass unaligned user address..
    memset(&context, 0, sizeof(struct IOContext));
    status = KXMReadFileAsync(DevFD, blockHandle, buffer+1 /*pass unaligned address*/,
                            1, (LPOVERLAPPED)&context);
    EXIT_ON_ERROR(status != STATUS_INVALID_PARAMETER, "KXMReadFile accepted unaligned address!\n");

    status = KXMWriteFileAsync(DevFD, blockHandle, buffer+1 /*pass unaligned address*/,
                            1, (LPOVERLAPPED)&context);
    EXIT_ON_ERROR(status != STATUS_INVALID_PARAMETER, "KXMWriteFile accepted unaligned address!\n");

 
    KXMCloseFile(DevFD, blockHandle);
    KXMCloseIoCompletionPort(DevFD, compHandle);

    //Pass invalid block file handle for read/write.
    status = KXMReadFileAsyncK(DevFD, blockHandle, buffer, 1,
                                (LPOVERLAPPED)&context);
    EXIT_ON_ERROR(status != STATUS_INVALID_HANDLE, "KXMReadFileAsync didn't fail with invalid block handle.\n");

    //Drop root privileges temporary and try to open file.
    if (getuid() == 0) {
       //process is running as root, drop privileges temporary.
       //non-root user id starts with 1000.
       EXIT_ON_ERROR(setegid(1000) != 0, "Unable to drop group privileges");
       EXIT_ON_ERROR(seteuid(1000) != 0, "Unable to drop user privileges");

       //Create a BlockFile - This must fail since the blockfile was created by the root.
	   flags = FileFD != -1 ? KXMFLAG_USE_FILESYSTEM_ONLY : 0;
       blockHandle = KXMCreateFile(DevFD, FileFD, KXM_BLOCK_FILE_NAME, NULL, FALSE, BLOCK_FLUSH, 0, flags);
       EXIT_ON_ERROR(blockHandle != INVALID_HANDLE_VALUE, "Could open BlockFile after dropping root privilege!\n");

       //Regain root privileges.
       EXIT_ON_ERROR(setegid(0) != 0, "Unable to regain root group privileges");
       EXIT_ON_ERROR(seteuid(0) != 0, "Unable to regain root user privileges");
    }
    
    //Free buffer 
    free(buffer);
    return STATUS_SUCCESS;
}


static int FSFileRead(int FD, char *Buffer, long Size, long long Offset)
{
    struct iovec readIov;
    int bytesread;

    readIov.iov_base = Buffer;
    readIov.iov_len = Size;   
    bytesread = preadv(FD, &readIov, 1, Offset);
    EXIT_ON_ERROR(bytesread != Size, "preadv: failed reading data.\n");
    return 0;
}

static int FSFileWrite(int FD, char *Buffer, long Size, long long Offset)
{
    struct iovec writeIov;
    int byteswritten;

    writeIov.iov_base = Buffer;
    writeIov.iov_len = Size;   
    byteswritten = pwritev(FD, &writeIov, 1, Offset);
    EXIT_ON_ERROR(byteswritten != Size, "pwritev: failed writing data\n");
    return 0;
}

static int FillPatternBlock(char *Pattern, int BlockIndex, int PatternSize)
{
    int i;
    int j;

    for(i=0, j=0; i<PatternSize; i+=4, j++)
    {
        *(int *)(Pattern + i) = (j << 16) | (BlockIndex);
    }
    return 0;
}

static int FillFileWithPattern(char *Filename, long TotalSize, int PatternSize)
{
    char *buffer = NULL;
    long alignment = sysconf(_SC_PAGESIZE);
    int numblocks = TotalSize / PatternSize;
    int i;
    int openFlags = O_RDWR|O_DIRECT|O_SYNC;
    int fd;

    fd = open(Filename, openFlags, S_IRWXU);
    EXIT_ON_ERROR((fd < 0), "Testfile open failed\n");

    posix_memalign((void **)&buffer, alignment, PatternSize);
    EXIT_ON_ERROR(buffer == NULL, "Couldn't allocate memory for pattern.\n");

    for(i=0; i<numblocks; i++)
    {
        FillPatternBlock(buffer, i, PatternSize);
        FSFileWrite(fd, buffer, PatternSize, i*PatternSize);
    }
    close(fd);
    return 0;
}

static int CreateTestFile(char* Filename, unsigned long long FileSize, int PatternSize)
{
    int status;
    int fd;
    int openFlags = O_RDWR | O_CLOEXEC | O_CREAT | O_TRUNC;
    unsigned long long size = 0;
    char *buf;

    printf("Creating test file [%s]\n", Filename);
    fd = open(Filename, openFlags, S_IRWXU);
    EXIT_ON_ERROR((fd < 0), "Testfile open failed: %s\n", strerror(errno));

    status = ftruncate(fd, FileSize);
    EXIT_ON_ERROR((status != 0), "Testfile truncate failed: %s\n", strerror(errno));

    status = fallocate(fd, FALLOC_FL_KEEP_SIZE, 0, FileSize);
    EXIT_ON_ERROR((status != 0), "Testfile fallocate failed: %s\n", strerror(errno));

    //Fill entire file with dummy data so that newly allocated secotrs become valid for the filesystem read calls.
    buf = (char *)malloc(PatternSize);
    EXIT_ON_ERROR((buf == NULL), "Buffer allocation failed: %s\n", strerror(errno));
    memset(buf, 0xa5, PatternSize);
    while(size < FileSize)
    {
        write(fd, buf, PatternSize);
        size += PatternSize;
    }
    free(buf);

    fsync(fd);
    close(fd);
    return 0;
}

static int RemoveTestFile(char *Filename)
{
    unlink(Filename);
    return 0;
}

static int MergeExtents(struct fiemap_extent *Extents, int NumExtents)
{
    int prev = 0, curr = 1;
    int new_num_extents = 1;
    int i;

    NumExtents--;

    while(NumExtents){
        if(Extents[curr].fe_physical ==
                    (Extents[prev].fe_physical + Extents[prev].fe_length)){
            Extents[prev].fe_length += Extents[curr].fe_length;
            Extents[prev].fe_flags = Extents[curr].fe_flags;
        }else{
            //Fill the hole
            Extents[prev+1].fe_logical = Extents[curr].fe_logical;
            Extents[prev+1].fe_length = Extents[curr].fe_length;
            Extents[prev+1].fe_physical = Extents[curr].fe_physical;
            Extents[prev+1].fe_flags = Extents[curr].fe_flags;
            prev++;
        }
        curr++;
        NumExtents--;
    }
    printf("\n\n***MERGED EXTENTS***\n\n");
    for(i=0; i<(prev+1); i++) {
        printf("Extent%d, Offset %#llx, Length %#llx, Physical %#llx, Flags %#x\n", i,
                Extents[i].fe_logical,
                Extents[i].fe_length,
                Extents[i].fe_physical,
                Extents[i].fe_flags);
    }
    return prev+1;
}

static int LoadFiemap(char *FileName, size_t FileSize, struct fiemap_extent *Extents, int NumExtents)
{
    int fd=-1, i;
    struct fiemap *fiemap = NULL;
    int status;
    int mapped_extents = 0;
    int size = 0;
    int ret = 0;

    size = sizeof(struct fiemap) + (sizeof(struct fiemap_extent)*NumExtents);
    fiemap = (struct fiemap *)malloc(size);
    EXIT_ON_ERROR(fiemap == NULL, "Couldn't allocate fiemap memory\n");

    fd = open(FileName, O_RDWR, S_IRWXU);
    EXIT_ON_ERROR(fd < 0, "Couldn't open the file.");

    memset(fiemap, 0, sizeof(struct fiemap)+sizeof(struct fiemap_extent)*NumExtents);
    fiemap->fm_start = 0;
    fiemap->fm_length = FileSize;
    fiemap->fm_flags = FIEMAP_FLAG_SYNC;
    fiemap->fm_extent_count = NumExtents;

    status = ioctl(fd, FS_IOC_FIEMAP, fiemap);
    EXIT_ON_ERROR(status < 0, "IOC_FIEMAP ioctl failed");

    mapped_extents = fiemap->fm_mapped_extents;
    memcpy(Extents, fiemap->fm_extents, sizeof(struct fiemap_extent)*NumExtents);

    for(i=0; i<mapped_extents; i++){
        printf("Extent%d, Offset %#llx, Length %#llx, Physical %#llx, Flags %#x\n", i,
                Extents[i].fe_logical,
                Extents[i].fe_length,
                Extents[i].fe_physical,
                Extents[i].fe_flags);
    }

    free(fiemap);
    close(fd);
    return 0;
}


static int GetNumExtents(char *FileName, off_t FileSize)
{
    int fd;
    struct fiemap fiemap;
    int status;

    fd = open(FileName, O_RDWR, S_IRWXU);
    EXIT_ON_ERROR(fd < 0, "Failed opening file\n");

    printf("get_num_extents FD = %d\n", fd);
    fiemap.fm_start = 0;
    fiemap.fm_length = FileSize;
    fiemap.fm_flags = FIEMAP_FLAG_SYNC;
    fiemap.fm_extent_count = 0;

    status = ioctl(fd, FS_IOC_FIEMAP, &fiemap);
    EXIT_ON_ERROR(status < 0, "FIEMAP ioctl failed for file");

    close(fd);
    return fiemap.fm_mapped_extents;
}


static int MapLogicalToPhysical(unsigned long long Logical, 
                    size_t Length, unsigned long long *Physical, 
                    struct fiemap_extent* Extent, int ExtentCount)
{
    int i;

    for (int i = 0; i < ExtentCount; i++)
    {
        off_t LogicalStart = Extent[i].fe_logical;
        off_t LogicalEnd = LogicalStart + Extent[i].fe_length;
        if ((Logical >= LogicalStart) && (Logical < LogicalEnd))
        {
            off_t offset = Logical - LogicalStart;
            if ((Logical + Length) > LogicalEnd)
            {
                return -1;
            }
            *Physical = (Extent[i].fe_physical + offset);
            break;
        }
    }
    return 0;
}

//Write 64MB in file using KXM API and Read the data using
//FileSystem APIs and verify the result.
static NTSTATUS BlockFileWriteTest(HANDLE DevFD, int FileFD, int PatternSize)
{
    char *expected_pattern = NULL;
    char *buffer = NULL;
    HANDLE blockHandle, compHandle = NULL;
    int i;
    int blockIndex;
    long pagesize = sysconf(_SC_PAGESIZE);
    unsigned long completionKey;
    DWORD numbytes;
    long size;
    struct IOContext context, *retcontext;
    int numExtents; 
    struct fiemap_extent *extents;
    unsigned long long offset, physical;
    int ret;
    NTSTATUS status;
    int openFlags = O_RDWR | O_DIRECT | O_SYNC;
    int fd;
    bool result;
	ULONG flags;

    //Open file with KXM API.
	flags = FileFD != -1 ? KXMFLAG_USE_FILESYSTEM_ONLY : 0;	
    blockHandle = KXMCreateFile(DevFD, FileFD, KXM_BLOCK_FILE_NAME, NULL, FALSE, BLOCK_FLUSH, 0, flags);
    EXIT_ON_ERROR(blockHandle == INVALID_HANDLE_VALUE, "KXMCreateFile failed!\n");

    //Create CompletionPort and associate with the blockHandle.
    compHandle = KXMCreateIoCompletionPort(DevFD, blockHandle, NULL, 0ULL, 0);
    EXIT_ON_ERROR(compHandle == NULL, "KXMCreateIoCompletion Port failed!\n");

    //Get the fiemap.
    numExtents = GetNumExtents(BLOCK_FILE_NAME, BLOCK_FILE_SIZE);

    extents = (struct fiemap_extent *)malloc(sizeof(struct fiemap_extent)*numExtents);
    EXIT_ON_ERROR(extents == NULL, "Extent memory allocation failed.\n");

    LoadFiemap(BLOCK_FILE_NAME, BLOCK_FILE_SIZE, extents, numExtents);

    numExtents = MergeExtents(extents, numExtents);

    posix_memalign((void **)&buffer, pagesize, PatternSize);
    posix_memalign((void **)&expected_pattern, pagesize, PatternSize);
    EXIT_ON_ERROR((buffer == NULL || expected_pattern == NULL), "Buffer allocation failed.");
    
    //KXMRegister
    result = KXMRegisterCompletionThread(DevFD, compHandle);
    EXIT_ON_ERROR(result != true, "Registeration of Thread failed!\n");

    for(offset=0, blockIndex=0; offset<BLOCK_FILE_SIZE;     
                        offset += PatternSize, blockIndex++)
    {
        ret = MapLogicalToPhysical(offset, PatternSize, &physical, 
                        extents, numExtents);
        if(ret == -1)
            continue;
        context.Overlapped.Offset = LSB32(physical);
        context.Overlapped.OffsetHigh = MSB32(physical);

        FillPatternBlock(buffer, blockIndex, PatternSize);
        status = KXMWriteFileAsync(DevFD, blockHandle, buffer,
                PatternSize/pagesize, (LPOVERLAPPED)&context);
        EXIT_ON_ERROR(status != STATUS_SUCCESS, "KXMWriteFileAsync failed!");

        //Get the completed event.
        result = KXMGetQueuedCompletionStatus(DevFD, 
                                compHandle, &numbytes,
                                &completionKey,
                                (LPOVERLAPPED *)&retcontext, DEFAULT_TIMEOUT);
        EXIT_ON_ERROR(result != true, 
                        "KXMGetQueuedCompletion failed!");
    }
    //KXMUnregister
    result = KXMUnregisterCompletionThread(DevFD, compHandle);
    EXIT_ON_ERROR(result != true, "Unregisteration of Thread failed!\n");

    //Read file using preadv and compare the data.
    fd = open(BLOCK_FILE_NAME, openFlags, S_IRWXU);
    EXIT_ON_ERROR((fd < 0), "Testfile open failed\n");
    
    for(offset=0, blockIndex=0; offset<BLOCK_FILE_SIZE;     
                        offset += PatternSize, blockIndex++)
    {
        ret = MapLogicalToPhysical(offset, PatternSize, &physical, 
                        extents, numExtents);
        if(ret == -1)
            continue;

        FSFileRead(fd, buffer, PatternSize, offset);

        FillPatternBlock(expected_pattern, blockIndex, PatternSize);

        if(memcmp(buffer, expected_pattern, PatternSize) != 0)
        {
            printf("LogicalOffset %#llx, PhysicalOffset %#llx\n", offset, physical);
            for(i=0; i<PatternSize/sizeof(int); i++)
            {
                printf("Got [%#x], Expected [%#x]\n", 
                                ((int *)(buffer))[i],
                                ((int *)(expected_pattern))[i]);
            }
            printf("Buffer mismatch!!\n");
            exit(-1);
        }
        printf("LogicalOffset %#llx, PhysicalOffset %#llx Compared!\n",
                            offset, physical);
    }
    close(fd);
    free(extents);
    free(buffer);
    free(expected_pattern);
    KXMCloseFile(DevFD, blockHandle);
    KXMCloseIoCompletionPort(DevFD, compHandle);
    return STATUS_SUCCESS;
}

//Write 64MB in file using KXM API and Read the data using
//KXM SG APIs and verify the result.
static NTSTATUS BlockFileWriteReadSGTest(HANDLE DevFD, int FileFD, int PatternSize, int MaxFrags)
{
    char *expected_pattern = NULL;
    char *buffer = NULL;
    HANDLE blockHandle, compHandle = NULL;
    int i;
    int blockIndex;
    long pagesize = sysconf(_SC_PAGESIZE);
    unsigned long completionKey;
    DWORD numbytes;
    long size;
    struct IOContext context, *retcontext;
    int numExtents; 
    struct fiemap_extent *extents;
    unsigned long long offset, physical;
    int ret;
    NTSTATUS status;
    bool result;
    KXMFileOffset *frags;
    int fragsize;
	const ULONG workBufferSize = 16384;  // TODO: make this more precise
	UCHAR workBuffer[workBufferSize];
	ULONG flags;

    frags = (KXMFileOffset *)malloc(sizeof(KXMFileOffset) * MaxFrags);
    EXIT_ON_ERROR(frags == NULL, "Couldn't allocate frags memory.\n");

    //Open file with KXM API.
    flags = FileFD != -1 ? KXMFLAG_USE_FILESYSTEM_ONLY : 0;
    blockHandle = KXMCreateFile(DevFD, FileFD, KXM_BLOCK_FILE_NAME, NULL, FALSE, BLOCK_FLUSH, 0, flags);
    EXIT_ON_ERROR(blockHandle == INVALID_HANDLE_VALUE, "KXMCreateFile failed!\n");

    //Create CompletionPort and associate with the blockHandle.
    compHandle = KXMCreateIoCompletionPort(DevFD, blockHandle, NULL, 0ULL, 0);
    EXIT_ON_ERROR(compHandle == NULL, "CreateIoCompletionPort failed!\n");

    //Get the fiemap.
    numExtents = GetNumExtents(BLOCK_FILE_NAME, BLOCK_FILE_SIZE);

    extents = (struct fiemap_extent *)malloc(sizeof(struct fiemap_extent)*numExtents);
    EXIT_ON_ERROR(extents == NULL, "Extent memory allocation failed.\n");

    LoadFiemap(BLOCK_FILE_NAME, BLOCK_FILE_SIZE, extents, numExtents);

    numExtents = MergeExtents(extents, numExtents);

    posix_memalign((void **)&buffer, pagesize, PatternSize);
    posix_memalign((void **)&expected_pattern, pagesize, PatternSize);
    EXIT_ON_ERROR((buffer == NULL || expected_pattern == NULL), 
                        "Buffer allocation failed.");
    
    //Register
    result = KXMRegisterCompletionThread(DevFD, compHandle);
    EXIT_ON_ERROR(result != true, "Registeration of Thread failed!\n");
    
    for(offset=0, blockIndex=0; offset<BLOCK_FILE_SIZE;     
                        offset += PatternSize, blockIndex++)
    {
        ret = MapLogicalToPhysical(offset, PatternSize, &physical, 
                        extents, numExtents);
        if(ret == -1)
            continue;
        context.Overlapped.Offset = LSB32(physical);
        context.Overlapped.OffsetHigh = MSB32(physical);

        FillPatternBlock(buffer, blockIndex, PatternSize);

        //Setup multiple frags.
        fragsize = PatternSize / MaxFrags;
        printf("fragsize %d\n", fragsize);
        for(i=0; i<MaxFrags; i++)
        {
            frags[i].UserAddress = (unsigned long long)buffer + i*fragsize;
            frags[i].PhysicalAddress = physical + i*fragsize;
            frags[i].NumPages = fragsize/pagesize;
            printf("Frag%d, User %#llx, Phys %#llx, NumPages %#llx\n", i, frags[i].UserAddress, frags[i].PhysicalAddress, frags[i].NumPages);
        }
        status = KXMWriteFileGatherAsync(DevFD, blockHandle, frags,
                            MaxFrags, (LPOVERLAPPED)&context);
        EXIT_ON_ERROR(status != STATUS_PENDING, "KXMWriteFileAsyncGather failed!");        

        //Get the completed event.
        result = KXMGetQueuedCompletionStatus(DevFD, 
                                compHandle, &numbytes,
                                &completionKey,
                                (LPOVERLAPPED *)&retcontext, DEFAULT_TIMEOUT);
        EXIT_ON_ERROR(result != true, 
                        "GetQueuedCompletion failed!");
    }

    for(offset=0, blockIndex=0; offset<BLOCK_FILE_SIZE;
                        offset += PatternSize, blockIndex++)
    {
        ret = MapLogicalToPhysical(offset, PatternSize, &physical, 
                        extents, numExtents);
        if(ret == -1)
            continue;
        context.Overlapped.Offset = LSB32(physical);
        context.Overlapped.OffsetHigh = MSB32(physical);

        FillPatternBlock(expected_pattern, blockIndex, PatternSize);

        //Setup multiple frags.
        fragsize = PatternSize / MaxFrags;
        for(i=0; i<MaxFrags; i++)
        {
            frags[i].UserAddress = (unsigned long long)buffer + i*fragsize;
            frags[i].PhysicalAddress = physical + i*fragsize;
            frags[i].NumPages = fragsize/pagesize;
        }
        status = KXMReadFileScatterAsync(DevFD, blockHandle, frags, 
                            MaxFrags, (LPOVERLAPPED)&context);
        EXIT_ON_ERROR(status != STATUS_PENDING, "KXMReadFileScatterAsync failed!");

        //Get the completed event.
        result = KXMGetQueuedCompletionStatus(DevFD, 
                                compHandle, &numbytes,
                                &completionKey,
                                (LPOVERLAPPED *)&retcontext, DEFAULT_TIMEOUT);
        EXIT_ON_ERROR(result != true, 
                        "GetQueuedCompletion failed!");

        if(memcmp(buffer, expected_pattern, PatternSize) != 0)
        {
            printf("LogicalOffset %#llx, PhysicalOffset %#llx\n", offset, physical);
            for(i=0; i<PatternSize/sizeof(int); i++)
            {
                printf("Got [%#x], Expected [%#x]\n", 
                                ((int *)(buffer))[i],
                                ((int *)(expected_pattern))[i]);
            }
            printf("Buffer mismatch!!\n");
            exit(-1);
        }
        printf("LogicalOffset %#llx, PhysicalOffset %#llx Compared!\n",
                            offset, physical);
    }

    //Unregister
    result = KXMUnregisterCompletionThread(DevFD, compHandle);
    EXIT_ON_ERROR(result != true, "Unregisteration of Thread failed!\n");

    free(extents);
    free(buffer);
    free(expected_pattern);
    KXMCloseFile(DevFD, blockHandle);
    KXMCloseIoCompletionPort(DevFD, compHandle);
    return STATUS_SUCCESS;
}

//Write 64MB in file using KXM API and Read the data using
//KXM APIs and verify the result.
static NTSTATUS BlockFileWriteReadTest(HANDLE DevFD, int FileFD, int PatternSize)
{
    char *expected_pattern = NULL;
    char *buffer = NULL;
    HANDLE blockHandle, compHandle = NULL;
    int i;
    int blockIndex;
    long pagesize = sysconf(_SC_PAGESIZE);
    unsigned long completionKey;
    DWORD numbytes;
    long size;
    struct IOContext context, *retcontext;
    int numExtents; 
    struct fiemap_extent *extents;
    unsigned long long offset, physical;
    int ret;
    NTSTATUS status;
    bool result;
	ULONG flags;

    //Open file with KXM API.
    flags = FileFD != -1 ? KXMFLAG_USE_FILESYSTEM_ONLY : 0;	
    blockHandle = KXMCreateFile(DevFD, FileFD, KXM_BLOCK_FILE_NAME, NULL, FALSE, BLOCK_FLUSH, 0, flags);
    EXIT_ON_ERROR(blockHandle == INVALID_HANDLE_VALUE, "KXMCreateFile failed!\n");

    //Create CompletionPort and associate with the blockHandle.
    compHandle = KXMCreateIoCompletionPort(DevFD, blockHandle, NULL, 0ULL, 0);
    EXIT_ON_ERROR(compHandle == NULL, "KXMCreateIoCompletionPort failed!\n");

    //Get the fiemap.
    numExtents = GetNumExtents(BLOCK_FILE_NAME, BLOCK_FILE_SIZE);

    extents = (struct fiemap_extent *)malloc(sizeof(struct fiemap_extent)*numExtents);
    EXIT_ON_ERROR(extents == NULL, "Extent memory allocation failed.\n");

    LoadFiemap(BLOCK_FILE_NAME, BLOCK_FILE_SIZE, extents, numExtents);

    numExtents = MergeExtents(extents, numExtents);

    posix_memalign((void **)&buffer, pagesize, PatternSize);
    posix_memalign((void **)&expected_pattern, pagesize, PatternSize);
    EXIT_ON_ERROR((buffer == NULL || expected_pattern == NULL), "Buffer allocation failed.");
    
    //KXMRegister
    result = KXMRegisterCompletionThread(DevFD, compHandle);
    EXIT_ON_ERROR(result != true, "Registeration of Thread failed!\n");
    
    for(offset=0, blockIndex=0; offset<BLOCK_FILE_SIZE;     
                        offset += PatternSize, blockIndex++)
    {
        ret = MapLogicalToPhysical(offset, PatternSize, &physical, 
                        extents, numExtents);
        if(ret == -1)
            continue;
        context.Overlapped.Offset = LSB32(physical);
        context.Overlapped.OffsetHigh = MSB32(physical);

        FillPatternBlock(buffer, blockIndex, PatternSize);
        status = KXMWriteFileAsync(DevFD, blockHandle, buffer,
                PatternSize/pagesize, (LPOVERLAPPED)&context);
        EXIT_ON_ERROR(status != STATUS_SUCCESS, "KXMWriteFileAsync failed!");        

        //Get the completed event.
        result = KXMGetQueuedCompletionStatus(DevFD, 
                                compHandle, &numbytes,
                                &completionKey,
                                (LPOVERLAPPED *)&retcontext, DEFAULT_TIMEOUT);
        EXIT_ON_ERROR(result != true, 
                        "KXMGetQueuedCompletion failed!");
    }

    for(offset=0, blockIndex=0; offset<BLOCK_FILE_SIZE;     
                        offset += PatternSize, blockIndex++)
    {
        ret = MapLogicalToPhysical(offset, PatternSize, &physical, 
                        extents, numExtents);
        if(ret == -1)
            continue;
        context.Overlapped.Offset = LSB32(physical);
        context.Overlapped.OffsetHigh = MSB32(physical);

        FillPatternBlock(expected_pattern, blockIndex, PatternSize);
        status = KXMReadFileAsync(DevFD, blockHandle, buffer,
                PatternSize/pagesize, (LPOVERLAPPED)&context);
        EXIT_ON_ERROR(status != STATUS_SUCCESS, "KXMReadFileAsync failed!");

        //Get the completed event.
        result = KXMGetQueuedCompletionStatus(DevFD, 
                                compHandle, &numbytes,
                                &completionKey,
                                (LPOVERLAPPED *)&retcontext, DEFAULT_TIMEOUT);
        EXIT_ON_ERROR(result != true, 
                        "KXMGetQueuedCompletion failed!");

        if(memcmp(buffer, expected_pattern, PatternSize) != 0)
        {
            printf("LogicalOffset %#llx, PhysicalOffset %#llx\n", offset, physical);
            for(i=0; i<PatternSize/sizeof(int); i++)
            {
                printf("Got [%#x], Expected [%#x]\n", 
                                ((int *)(buffer))[i],
                                ((int *)(expected_pattern))[i]);
            }
            printf("Buffer mismatch!!\n");
            exit(-1);
        }
        printf("LogicalOffset %#llx, PhysicalOffset %#llx Compared!\n",
                            offset, physical);
    }

    //KXMUnregister
    result = KXMUnregisterCompletionThread(DevFD, compHandle);
    EXIT_ON_ERROR(result != true, "Unregisteration of Thread failed!\n");

    free(extents);
    free(buffer);
    free(expected_pattern);
    KXMCloseFile(DevFD, blockHandle);
    KXMCloseIoCompletionPort(DevFD, compHandle);
    return STATUS_SUCCESS;
}


//Write 64MB in file using FileSystem API and Read the data using
//KXM APIs and verify the result.
static NTSTATUS BlockFileReadTest(HANDLE DevFD, int FileFD, int PatternSize)
{
    char *expected_pattern = NULL;
    char *buffer = NULL;
    HANDLE blockHandle, compHandle = NULL;
    int i;
    int blockIndex;
    long pagesize = sysconf(_SC_PAGESIZE);
    unsigned long completionKey;
    DWORD numbytes;
    long size;
    struct IOContext context, *retcontext;
    int numExtents; 
    struct fiemap_extent *extents;
    unsigned long long offset, physical;
    int ret;
    NTSTATUS status;
    bool result;
	ULONG flags;

    //Fill file with a specific pattern using file system apis.
    FillFileWithPattern(BLOCK_FILE_NAME, BLOCK_FILE_SIZE, PatternSize);

    //Open file with KXM API.
    flags = FileFD != -1 ? KXMFLAG_USE_FILESYSTEM_ONLY : 0;
    blockHandle = KXMCreateFile(DevFD, FileFD, KXM_BLOCK_FILE_NAME, NULL, FALSE, BLOCK_FLUSH, 0, flags);
    EXIT_ON_ERROR(blockHandle == INVALID_HANDLE_VALUE, "KXMCreateFile failed!\n");

    //Create CompletionPort and associate with the blockHandle.
    compHandle = KXMCreateIoCompletionPort(DevFD, blockHandle, NULL, 0ULL, 0);
    EXIT_ON_ERROR(compHandle == NULL, "KXMCreateIoCompletionPort failed!\n");

    //Get the fiemap.
    numExtents = GetNumExtents(BLOCK_FILE_NAME, BLOCK_FILE_SIZE);

    extents = (struct fiemap_extent *)malloc(sizeof(struct fiemap_extent)*numExtents);
    EXIT_ON_ERROR(extents == NULL, "Extent memory allocation failed.\n");

    LoadFiemap(BLOCK_FILE_NAME, BLOCK_FILE_SIZE, extents, numExtents);

    numExtents = MergeExtents(extents, numExtents);

    posix_memalign((void **)&buffer, pagesize, PatternSize);
    posix_memalign((void **)&expected_pattern, pagesize, PatternSize);
    EXIT_ON_ERROR((buffer == NULL || expected_pattern == NULL), "Buffer allocation failed.");
    
    //KXMRegister
    result = KXMRegisterCompletionThread(DevFD, compHandle);
    EXIT_ON_ERROR(result != true, "Registeration of Thread failed!\n");

    for(offset=0, blockIndex=0; offset<BLOCK_FILE_SIZE;     
                        offset += PatternSize, blockIndex++)
    {
        ret = MapLogicalToPhysical(offset, PatternSize, &physical, 
                        extents, numExtents);
        if(ret == -1)
            continue;
        context.Overlapped.Offset = LSB32(physical);
        context.Overlapped.OffsetHigh = MSB32(physical);

        //Read _BLOCK_SIZE chunk and compare with the expected pattern. 
        status = KXMReadFileAsync(DevFD, blockHandle, buffer,
                PatternSize/pagesize, (LPOVERLAPPED)&context);
        printf("status %d\n", status);
        EXIT_ON_ERROR(status != STATUS_SUCCESS, "KXMReadFileAsync failed!");

        //Get the completed event.
        result = KXMGetQueuedCompletionStatus(DevFD, 
                                compHandle, &numbytes,
                                &completionKey,
                                (LPOVERLAPPED *)&retcontext, DEFAULT_TIMEOUT);
        EXIT_ON_ERROR(result != true, 
                        "KXMGetQueuedCompletion failed!");

        FillPatternBlock(expected_pattern, blockIndex, PatternSize);
        if(memcmp(buffer, expected_pattern, PatternSize) != 0)
        {
            printf("LogicalOffset %#llx, PhysicalOffset %#llx\n", offset, physical);
            for(i=0; i<PatternSize/sizeof(int); i++)
            {
                printf("Got [%#x], Expected [%#x]\n", 
                        ((int *)(buffer))[i],
                        ((int *)(expected_pattern))[i]);
            }
            printf("[%s @ %d] Buffer mismatch!!\n", __FUNCTION__, __LINE__);
            exit(-1);
        }
        printf("LogicalOffset %#llx, PhysicalOffset %#llx Compared!\n",
                            offset, physical);
        memset(buffer, 0, PatternSize);
        memset(expected_pattern, 0, PatternSize);
        if(offset >= BLOCK_FILE_SIZE)
            break; 
    }

    //Unegister
    result = KXMUnregisterCompletionThread(DevFD, compHandle);
    EXIT_ON_ERROR(result != true, "Unregisteration of Thread failed!\n");

    free(extents);
    free(buffer);
    free(expected_pattern);
    KXMCloseFile(DevFD, blockHandle);
    KXMCloseIoCompletionPort(DevFD, compHandle);
    return STATUS_SUCCESS;
}

int main(int argc, char *argv[])
{
    HANDLE devFD = NULL;
    int patternBlockSize[] = {KB(256), KB(512), KB(1024)};
    int i = 0;
    
    devFD = KXMOpenDev();
    printf("DevFD %p\n", devFD);
    EXIT_ON_ERROR((long)devFD <= (long)INVALID_HANDLE_VALUE, "KXM Device File open failed!");

    //Create TestFile.
    CreateTestFile(BLOCK_FILE_NAME, BLOCK_FILE_SIZE, KB(1024));

    printf("ULONG_PTR %d, PULONG_PTR %d, DWORD %d, LPDWORD %d, PVOID %d, HANDLE %d\n", sizeof(ULONG_PTR), sizeof(PULONG_PTR), sizeof(DWORD), sizeof(LPDWORD), sizeof(PVOID), sizeof(HANDLE));


	// TODO: Add tests for emulation mode
	
    //File Read/Write testing.
    for(i=0; i<sizeof(patternBlockSize)/sizeof(int); i++)
    {
        printf("Starting BlockFileReadTest with pattern block size %d\n", 
                        patternBlockSize[i]);
        BlockFileReadTest(devFD, -1, patternBlockSize[i]);

        printf("Starting BlockFileWriteTest with pattern block size %d\n", 
                        patternBlockSize[i]);
        BlockFileWriteTest(devFD, -1, patternBlockSize[i]);

        printf("Starting BlockFileWriteReadTest with pattern block size %d\n", 
                        patternBlockSize[i]);
        BlockFileWriteReadTest(devFD, -1, patternBlockSize[i]);

        printf("Starting BlockFileWriteReadSGTest with pattern block size %d with frags %d\n",
                        patternBlockSize[i], KXM_MAX_FILE_SG_ON_STACK);
        BlockFileWriteReadSGTest(devFD, -1, patternBlockSize[i], KXM_MAX_FILE_SG_ON_STACK);

        printf("Starting BlockFileWriteReadSGTest with pattern block size %d with frags %d\n",
                        patternBlockSize[i], 2*KXM_MAX_FILE_SG_ON_STACK);
        BlockFileWriteReadSGTest(devFD, -1, patternBlockSize[i], 2*KXM_MAX_FILE_SG_ON_STACK);
    }
    //Negative Testing.
    BlockFileFailureTests(devFD, -1);
    
    RemoveTestFile(BLOCK_FILE_NAME);
    printf("TEST COMPLETED SUCCESSFULLY\n");
    return 0;
}
