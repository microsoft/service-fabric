/*++

Copyright (c) Microsoft Corporation

Module Name:

    LinuxIoPerf.cpp

Abstract:

    KTL based disk performance test

--*/
#define _CRT_RAND_S
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "KUShared.h"
#include "KXMApi.h"
#include "strsafe.h"
#include "ktlpal.h"
#include "paldef.h"

/*
#include <uuid/uuid.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <sys/times.h>
#include <wchar.h>
#include <execinfo.h>
#include <cxxabi.h>
*/

#include <stdarg.h>
#include <string.h>
#include <malloc.h>
#include <ctype.h>

#include <errno.h>
#include <time.h>
#include <sys/uio.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
// TODO fix https://bugzilla.redhat.com/show_bug.cgi?id=1476120
#if defined(LINUX_DISTRIB_REDHAT)
#include <linux/falloc.h>
#endif
#include <unistd.h>
#include <linux/fs.h>
#include <linux/fiemap.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <mntent.h>
#include <linux/aio_abi.h>
#include <sys/syscall.h>    /* for __NR_* definitions */
#include <sys/mman.h>

#define PERF_TEST_CONFIG _IOWR(0, 1, struct test_config)
#define KXM_DEVICE_FILENAME "/dev/kxm_perf"
#define AIO_DEVICE_FILENAME "/dev/aio_perf"

#define LSB_32(val) (val & 0xffffffffU)
#define MSB_32(val) (val>>32)

struct test_config
{
    unsigned int max_outstanding_ops;
    unsigned int blocksize;
    char filename[128];
};

#define TRUE 1
#define FALSE 0
typedef unsigned char       BOOLEAN, *PBOOLEAN;


#if !__has_builtin(__int2c)
#ifdef __aarch64__
extern "C" void __int2c();
#else
void
__int2c(void)
{
    __asm__ __volatile__("int $0x2c");
}
#endif

#endif

void FailOnError(BOOLEAN Condition, const char * Message)
{
    if (! Condition)
    {
        printf("******Error*********\n");
        printf("%s; errno= %d\n", Message, errno);
        printf("********************\n");
        __int2c();
        exit(-1);
    }
}


typedef enum _TimeConversionConstants
{
    tccSecondsToMillieSeconds       = 1000,         // 10^3
    tccSecondsToMicroSeconds        = 1000000,      // 10^6
    tccSecondsToNanoSeconds         = 1000000000,   // 10^9
    tccMillieSecondsToMicroSeconds  = 1000,         // 10^3
    tccMillieSecondsToNanoSeconds   = 1000000,      // 10^6
    tccMicroSecondsToNanoSeconds    = 1000,         // 10^3
    tccSecondsTo100NanoSeconds      = 10000000,     // 10^7
    tccMicroSecondsTo100NanoSeconds = 10            // 10^1
} TimeConversionConstants;

static unsigned long long _GetTickCount64()
{
    unsigned long long retval = 0;

    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
    {
        FailOnError(FALSE, "clock_gettime failed");
    }
    retval = ((unsigned long long)ts.tv_sec * tccSecondsToMillieSeconds) + (ts.tv_nsec / tccMillieSecondsToNanoSeconds);

    return retval;
}

static ssize_t PReadFileV(int fd, struct iovec* iov, int iovcnt, off64_t offset)
{
    //
    // This routine assumes that a full buffer is read or not read,
    // that is, a partial buffer is never returned
    //
    ssize_t numRead = 0;
    while (iovcnt > 0) {
        ssize_t num = preadv(fd, iov, iovcnt, offset);
        if (num == 0) {
            return numRead;
        }

        if ((num < 0) && (errno != EINTR)) {
            return numRead;
        }

        numRead += num;
        offset += num;

        while ((num > 0) && (iovcnt > 0))
        {
            num -= iov->iov_len;
            iov++;
            iovcnt--;
        }

    }
    return numRead;
}

static ssize_t PWriteFileV(int fd, struct iovec* iov, int iovcnt, off64_t offset)
{
    //
    // This routine assumes that a full buffer is written or not written,
    // that is, a partial buffer is never returned
    //
    ssize_t numRead = 0;
    while (iovcnt > 0) {
        ssize_t num = pwritev(fd, iov, iovcnt, offset);
        if (num == 0) {
            return numRead;
        }

        if ((num < 0) && (errno != EINTR)) {
            return numRead;
        }

        numRead += num;
        offset += num;

        while ((num > 0) && (iovcnt > 0))
        {
            num -= iov->iov_len;
            iov++;
            iovcnt--;
        }

    }
    return numRead;
}


static const unsigned int _MaximumQueueDepth = 65536;

void
Usage()
{
    printf("LinuxIoPerf - disk performance tool using KTL KBlockFile\n\n");
    printf("LinuxIoPerf -t:<read or write> -i:<sequential or random> -l:<Total # IOs> -z:<WriteThrough - TRUE or FALSE>\n");
    printf("             -f:<Path>");
    printf(" -b:<block size in 4KB> -n:<# blocks>\n");
    printf("             -q:<# foreground outstanding I/O>\n");
    printf("             -a:<Automated mode result XML file>\n");
    printf("             -o:<Lock all pages - TRUE or FALSE>\n");
    printf("             -s:<Sparse - TRUE or FALSE>\n");
    printf("             -m:<Use many file descriptors - TRUE or FALSE>\n");
    printf("             -d:<Use disk direct IO - TRUE or FALSE>\n");
    printf("             -k:<Use AIO - TRUE or FALSE>\n");
    printf("             -e:<Do not do discontiguous IO - TRUE or FALSE>\n");
    printf("             -r:<# threads for AIO>\n");
    printf("             -p:<# Use KXM driver for disk IO>\n");
    printf("             -x:<# Use AIO driver for disk IO>\n");
    printf("\n");
    printf("    -a is mutually exclusive with the -b option. When -a is specified the test runs in an automated mode\n");
    printf("       using various block sizes\n");
    printf("Example: To Run test with KXM driver execute below command\n");
    printf("        ./BlockFileTest -t:write -f:/home/sfdev/test -p:t -b:1 -n:64 -q:64 -l:10000\n");
}

class TestParameters
{
    public:
        TestParameters();
        ~TestParameters();
        
        int Parse(int argc, char* args[]);

        typedef enum { Read = 0, Write = 1} TestOperation;
        typedef enum { Sequential = 0, Random = 1} TestAccess;
    
        TestOperation _TestOperation;
        TestAccess _TestAccess;
        BOOLEAN _WriteThrough;
        BOOLEAN _Sparse;
        BOOLEAN _AutomatedMode;
        BOOLEAN _UseMultipleFD;
        BOOLEAN _UseDiskDirectIo;
        BOOLEAN _UseAIO;
        BOOLEAN _SkipSplitIO;
        BOOLEAN _LockAllPages;
        BOOLEAN _UseKXMDrv;
        BOOLEAN _UseAIODrv;
        unsigned int _TotalNumberIO;
        char* _Filename;
        unsigned int _BlockSizeIn4K;
        unsigned int _NumberBlocks;
        unsigned int _ForegroundQueueDepth;
        unsigned int _NumberThreads;
};

TestParameters::TestParameters()
{
    _TestOperation = TestOperation::Write;
    _TestAccess = TestAccess::Sequential;
    _WriteThrough = TRUE;
    _Sparse = FALSE;
    _TotalNumberIO = 0x10000;
    _Filename = (char *)"/tmp/testfilename";
    _BlockSizeIn4K = 16;
    _NumberBlocks = 0x10000;
    _ForegroundQueueDepth = 32;
    _AutomatedMode = FALSE;
    _UseMultipleFD = FALSE;
    _UseDiskDirectIo = TRUE;
    _UseAIO = TRUE;
    _SkipSplitIO = FALSE;
    _NumberThreads = 1;
    _LockAllPages = FALSE;
    _UseKXMDrv = FALSE;
    _UseAIODrv = FALSE;
}

TestParameters::~TestParameters()
{
}

int
TestParameters::Parse(
    int argc,
    char* args[]
    )
{
    for (int i = 1; i < argc; i++)
    {
        char* arg = args[i];
        int c = toupper(arg[1]);
        char c1 = arg[2];
        char c2 = arg[0];
        arg += 3;

        if ((c1 != ':') || (c2 != '-'))
        {
            Usage();
            printf("Invalid argument %s\n", arg);
            return(ENOEXEC);
        }
        
        switch(c)
        {
            case '?':
            {
                Usage();
                return(ENOEXEC);
            }

            case 'T':              
            {
                if ((*arg == 'R') || (*arg == 'r'))
                {
                    _TestOperation = Read;
                } else if ((*arg == 'W') || (*arg == 'w')) {
                    _TestOperation = Write;
                } else {
                    printf("Invalid argument %s\n", arg);
                    return(ENOEXEC);
                }
                break;
            }

            case 'I':              
            {
                if ((*arg == 'R') || (*arg == 'r'))
                {
                    _TestAccess = Random;
                } else if ((*arg == 'S') || (*arg == 's')) {
                    _TestAccess = Sequential;
                } else {
                    printf("Invalid argument %s\n", arg);
                    return(ENOEXEC);
                }
                break;
            }

            case 'Z':              
            {
                if ((*arg == 'T') || (*arg == 't'))
                {
                    _WriteThrough = TRUE;
                } else if ((*arg == 'F') || (*arg == 'f')) {
                    _WriteThrough = FALSE;
                } else {
                    printf("Invalid argument %s\n", arg);
                    return(ENOEXEC);
                }
                break;
            }

            case 'O':              
            {
                if ((*arg == 'T') || (*arg == 't'))
                {
                    _LockAllPages = TRUE;
                } else if ((*arg == 'F') || (*arg == 'f')) {
                    _LockAllPages = FALSE;
                } else {
                    printf("Invalid argument %s\n", arg);
                    return(ENOEXEC);
                }
                break;
            }

            case 'E':              
            {
                if ((*arg == 'T') || (*arg == 't'))
                {
                    _SkipSplitIO = TRUE;
                } else if ((*arg == 'F') || (*arg == 'f')) {
                    _SkipSplitIO = FALSE;
                } else {
                    printf("Invalid argument %s\n", arg);
                    return(ENOEXEC);
                }
                break;
            }

            case 'S':              
            {
                if ((*arg == 'T') || (*arg == 't'))
                {
                    _Sparse = TRUE;
                } else if ((*arg == 'F') || (*arg == 'f')) {
                    _Sparse = FALSE;
                } else {
                    printf("Invalid argument %s\n", arg);
                    return(ENOEXEC);
                }
                break;
            }

            case 'K':              
            {
                if ((*arg == 'T') || (*arg == 't'))
                {
                    _UseAIO = TRUE;
                } else if ((*arg == 'F') || (*arg == 'f')) {
                    _UseAIO = FALSE;
                } else {
                    printf("Invalid argument %s\n", arg);
                    return(ENOEXEC);
                }
                break;
            }

            case 'X':              
            {
                if ((*arg == 'T') || (*arg == 't'))
                {
                    _UseAIODrv = TRUE;
                } else if ((*arg == 'F') || (*arg == 'f')) {
                    _UseAIODrv = FALSE;
                } else {
                    printf("Invalid argument %s\n", arg);
                    return(ENOEXEC);
                }
                break;
            }

            case 'P':              
            {
                if ((*arg == 'T') || (*arg == 't'))
                {
                    _UseKXMDrv = TRUE;
                } else if ((*arg == 'F') || (*arg == 'f')) {
                    _UseKXMDrv = FALSE;
                } else {
                    printf("Invalid argument %s\n", arg);
                    return(ENOEXEC);
                }
                break;
            }

            case 'M':              
            {
                if ((*arg == 'T') || (*arg == 't'))
                {
                    _UseMultipleFD = TRUE;
                } else if ((*arg == 'F') || (*arg == 'f')) {
                    _UseMultipleFD = FALSE;
                } else {
                    printf("Invalid argument %s\n", arg);
                    return(ENOEXEC);
                }
                break;
            }

            case 'D':              
            {
                if ((*arg == 'T') || (*arg == 't'))
                {
                    _UseDiskDirectIo = TRUE;
                } else if ((*arg == 'F') || (*arg == 'f')) {
                    _UseDiskDirectIo = FALSE;
                } else {
                    printf("Invalid argument %s\n", arg);
                    return(ENOEXEC);
                }
                break;
            }
            
            case 'F':
            {
                _Filename = arg;
                break;
            }

            case 'A':
            {
                 _AutomatedMode = TRUE;
                 break;
            }

            case 'L':
            {
                _TotalNumberIO = atoi(arg);
                if (_TotalNumberIO == 0)
                {
                    printf("Invalid argument %s\n", arg);
                    return(ENOEXEC);
                }
                break;
            }
            
            case 'R':
            {
                _NumberThreads = atoi(arg);
                if (_NumberThreads == 0)
                {
                    printf("Invalid argument %s\n", arg);
                    return(ENOEXEC);
                }
                break;
            }
            
            case 'B':              
            {
                _BlockSizeIn4K = atoi(arg);
                if (_BlockSizeIn4K == 0)
                {
                    printf("Invalid argument %s\n", arg);
                    return(ENOEXEC);
                }
                break;
            }
            
            case 'N':              
            {
                _NumberBlocks = atoi(arg);
                if (_NumberBlocks == 0)
                {
                    printf("Invalid argument %s\n", arg);
                    return(ENOEXEC);
                }
                break;
            }
            
            case 'Q':              
            {
                _ForegroundQueueDepth = atoi(arg);
                if (_ForegroundQueueDepth == 0)
                {
                    printf("Invalid argument %s\n", arg);
                    return(ENOEXEC);
                }
                break;
            }
            
            default:
            {
                printf("Invalid argument %s\n", arg);
                return(ENOEXEC);
            }
        }
    }

    if ((_ForegroundQueueDepth > _MaximumQueueDepth) ||
        (_ForegroundQueueDepth > _TotalNumberIO))
    {
        //
        // Do not allow queue depth to be larger than maximum queue depth or total number of IO
        //
        printf("ForegroundQueueDepth %d must be less than MaximumQueueDepth %d and TotalNumberIO %d\n",
                _ForegroundQueueDepth, _MaximumQueueDepth, _TotalNumberIO);
        return(ENOEXEC);
    }

    return(0);
}

void* AllocSpecificBuffer(
    off64_t FileOffset,
    int BufferSize
    )
{
    //
    // Initialize the file with random data
    //
    void *buffer = nullptr;
    long alignment = sysconf(_SC_PAGESIZE);
    int ret = posix_memalign(&buffer, alignment, BufferSize);
    if (ret < 0)
    {
        return(nullptr);
    }   

    unsigned char* b = (unsigned char*)buffer;
    for (int i = 0; i < BufferSize; i++)
    {
        b[i] = (i * FileOffset) % 255;
    }
    
    return(buffer);
}

struct mntent *mountpoint(char *filename, struct mntent *mnt, char *buf, size_t buflen)
{
    struct stat s;
    FILE *      fp;
    dev_t       dev;

    if (stat(filename, &s) != 0) {
        return nullptr;
    }

    dev = s.st_dev;

    if ((fp = setmntent("/proc/mounts", "r")) == nullptr) {
        return nullptr;
    }

    while (getmntent_r(fp, mnt, buf, buflen)) {
        if (stat(mnt->mnt_dir, &s) != 0) {
            continue;
        }

        if (s.st_dev == dev) {
            endmntent(fp);
            return mnt;
        }
    }

    endmntent(fp);

    // Should never reach here.
    errno = EINVAL;
    return nullptr;
}


//////////////////////////////////////
//////////////////////////////////////
//////////////////////////////////////

int OpenAIODrv(const char *devicename)
{
    int device_fd;
    device_fd = open(devicename, O_RDWR);
    if(device_fd < 0){
        return -1;
    }
    return device_fd;
}


int InitializeAIODrv(int device_fd, char *testfilename, int maxops, int blocksize)
{
    struct test_config config;
    int status;

    config.max_outstanding_ops = maxops;
    config.blocksize = blocksize;
    strcpy(config.filename, testfilename);
    printf("max_outstanding_ops %u, blocksize %u, filename %s\n", maxops, blocksize, testfilename);
    status = ioctl(device_fd,  PERF_TEST_CONFIG, &config);

    if(status != 0){
        printf("IOCTL failed!");
        return -1;
    }
    return 0;
}

int CloseAIODrv(int device_fd)
{
    close(device_fd);
    return 0;
}

//////////////////////////////////////
//////////////////////////////////////
//////////////////////////////////////

int GetDirectDiskDeviceName(
    char* Filename,
    char* Devicename
    )
{
//+++ TODO: Make this my own
    struct mntent mm;
    struct mntent *m;

    char p[4096];
    m = mountpoint(Filename, &mm, p, 4096);
    
    strcpy(Devicename, m->mnt_fsname);  
//+++


    return(0);
}

int
LoadFiemap(
    char* Filename,
    TestParameters::TestOperation Operation,
    struct fiemap* Fiemap,
    int& ExtentCount
    )
{
    int status;
    int fd;
    struct stat fileStat;

    status = stat(Filename, &fileStat);
    FailOnError((status == 0), "stat");
    
    int openFlags = O_RDWR | O_DIRECT | O_CLOEXEC;    
    fd = open(Filename, openFlags, S_IRWXU);
    FailOnError((fd != -1), "open");

    struct fiemap* map;
    struct fiemap_extent* extent;
    unsigned long mapLength = sizeof(struct fiemap) + sizeof(struct fiemap_extent);
    unsigned char mapBuffer[mapLength];
    map = (struct fiemap*)mapBuffer;
    extent = &map->fm_extents[0];

    struct fiemap_extent* extentOut;
    ExtentCount = -1;
    
    __u64 fileOffset;
    __u64 fileLength;
    __u64 expectedPhysical;
    fileOffset = 0;
    fileLength = fileStat.st_size;
    expectedPhysical = (__u64)-1;
    while(1)
    {
        memset(map, 0, mapLength);

        map->fm_start = fileOffset;
        map->fm_length = fileLength;
        map->fm_flags = FIEMAP_FLAG_SYNC;
        map->fm_extent_count = 1;

        status = ioctl(fd, FS_IOC_FIEMAP, map);
        FailOnError((status == 0), "FS_IOC_FIEMAP");
        if (map->fm_mapped_extents == 1)
        {
            FailOnError(((extent->fe_flags & FIEMAP_EXTENT_NOT_ALIGNED) == 0), "Extent FIEMAP_EXTENT_NOT_ALIGNED");
            if (Operation == TestParameters::TestOperation::Read)
            {
                FailOnError(((extent->fe_flags & FIEMAP_EXTENT_UNWRITTEN) == 0), "Extent FIEMAP_EXTENT_UNWRITTEN");
            }
            FailOnError(((extent->fe_flags & FIEMAP_EXTENT_DELALLOC) == 0), "Extent FIEMAP_EXTENT_DELALLOC");
            FailOnError(((extent->fe_flags & FIEMAP_EXTENT_UNKNOWN) == 0), "Extent FIEMAP_EXTENT_UNKNOWN");

            //
            // I've seen these defined in docs but they aren't in our headers
            //
    //      FailOnError(((extent->fe_flags & FIEMAP_EXTENT_HOLE) == 0), "Extent FIEMAP_EXTENT_HOLE");
    //      FailOnError(((extent->fe_flags & FIEMAP_EXTENT_ERROR) == 0), "Extent FIEMAP_EXTENT_ERROR");
    //      FailOnError(((extent->fe_flags & FIEMAP_EXTENT_NO_DIRECT) == 0), "Extent FIEMAP_EXTENT_NO_DIRECT");
    //      FailOnError(((extent->fe_flags & FIEMAP_EXTENT_UNMAPPED) == 0), "Extent FIEMAP_EXTENT_UNMAPPED");
            
            if (extent->fe_physical != expectedPhysical)
            {
                ExtentCount++;
                extentOut = &Fiemap->fm_extents[ExtentCount];
                *extentOut = *extent;
            } else {
                extentOut->fe_length += extent->fe_length;
            }
            expectedPhysical = extentOut->fe_physical + extentOut->fe_length;
            fileOffset = extentOut->fe_logical + extentOut->fe_length;
            fileLength -= extent->fe_length;
            
            if (extent->fe_flags & FIEMAP_EXTENT_LAST)
            {
                break;
            }
        } else {
            break;
        }       
    }
    ExtentCount++;

#if 0  // For testing
    for (int i = 0; i < ExtentCount; i++)
    {
        extentOut = &Fiemap->fm_extents[i];
        printf("Mapping %4d %16lld %16lld %16lld\n", i, extentOut->fe_logical/1024, extentOut->fe_physical/1024, extentOut->fe_length/1024);
    }
    printf("%d extents\n", ExtentCount);
#endif
    
    close(fd);
    
    return(0);
}

BOOLEAN MapLogicalToPhysical(
    off64_t Logical,
    size_t Length,
    off64_t& Physical,
    size_t& PhysicalLength,
    struct fiemap* Fiemap,
    int ExtentCount                          
    )
{
    BOOLEAN isSplit = FALSE;
    off64_t physical;
    for (int i = 0; i < ExtentCount; i++)
    {
        off64_t LogicalStart = Fiemap->fm_extents[i].fe_logical;
        off64_t LogicalEnd = LogicalStart + Fiemap->fm_extents[i].fe_length;
        if ((Logical >= LogicalStart) && (Logical < LogicalEnd))
        {
            off64_t offset = Logical - LogicalStart;
            Physical = (Fiemap->fm_extents[i].fe_physical + offset);
            if ((Logical + Length) <= LogicalEnd)
            {
                PhysicalLength =  Length;
            } else {
                PhysicalLength =  LogicalEnd - Logical;
                isSplit = TRUE;
#if 0
                printf("Cross boundary: Logical extent: %lld  logical: %lld physical: %lld desired length: %lld avail length: %lld\n",
                       LogicalStart/1024, Logical/1024, Physical/1024, Length/1024, PhysicalLength/1024);
#endif
            }
            return(isSplit);
        }
    }
    
    FailOnError(FALSE, "Did not find mapping");
    return TRUE;
}

int
CreateTestFile(
    char* Filename,
    TestParameters::TestOperation Operation,
    unsigned int BlockSizeIn4K,
    unsigned int NumberBlocks,
    BOOLEAN WriteThrough,
    BOOLEAN Sparse
    )
{
    int status;
    int fd;
    unsigned long long totalSize = (unsigned long long)NumberBlocks * ((unsigned long long)BlockSizeIn4K * (unsigned long long)0x1000);
    unsigned long long fileSize = 0;
    unsigned long long fileAllocation = 0;
    int openFlags = O_RDWR | O_DIRECT | O_CLOEXEC | O_CREAT | O_TRUNC;
    if (WriteThrough)
    {
        openFlags |= O_DSYNC;
    }
    
    fd = open(Filename, openFlags, S_IRWXU);
    FailOnError((fd != -1), "open");

    status = ftruncate(fd, totalSize);
    FailOnError((status == 0), "ftruncate");
    
    printf("Created File with total size %llu\n", totalSize);    
    if (Sparse)
    {
        status = fallocate(fd, FALLOC_FL_KEEP_SIZE | FALLOC_FL_PUNCH_HOLE, 0, totalSize);
        FailOnError((status == 0), "fallocate");
    } else {
        status = fallocate(fd, FALLOC_FL_KEEP_SIZE, 0, totalSize);
        FailOnError((status == 0), "fallocate");
    }

    if (Operation == TestParameters::TestOperation::Read)
    {
        size_t writeSize = (size_t)BlockSizeIn4K * 4096;
        off64_t writeOffset;
        struct iovec writeIov;
        int writeIovCount = 1;
        ssize_t bytesWritten;
        void* writeBuffer;
        
        //
        // For read, we need to precreate the file with a known pattern
        //
        close(fd);
        fd = open(Filename, O_RDWR | O_DIRECT | O_CLOEXEC, S_IRWXU);
        FailOnError((fd != -1), "open 2");
        
        for (unsigned int i = 0; i < NumberBlocks; i++)
        {
            
            writeOffset = (size_t)i * writeSize;
            writeBuffer = AllocSpecificBuffer(writeOffset, writeSize);
            FailOnError((writeBuffer != nullptr), "writeBuffer Alloc fail");
            writeIov.iov_base = writeBuffer;
            writeIov.iov_len = writeSize;   
            
            bytesWritten = PWriteFileV(fd, &writeIov, writeIovCount, writeOffset);          
            FailOnError((bytesWritten == writeSize), "Did not write correct number of bytes");
           
     
            if ((i % 1024) == 0)
            {
                printf("%d completed %d total completed\n", i, NumberBlocks);
                fflush(stdout);
            }
            
            free(writeBuffer);
        }       
    }

    close(fd);
        
    return(0);
}

void* AllocRandomBuffer(
    int BufferSize
    )
{
    //
    // Initialize the file with random data
    //
    void* buffer = nullptr;
    long alignment = sysconf(_SC_PAGESIZE);
    int ret = posix_memalign(&buffer, alignment, BufferSize);
    if (ret < 0)
    {
        return(nullptr);
    }   

    unsigned char* b = (unsigned char*)buffer;
    for (int i = 0; i < BufferSize; i++)
    {
        b[i] = rand() % 255;
    }
    
    return(buffer);
}

static volatile off64_t NextWriteOffset;
static volatile int TotalNumberWritten;
static volatile int TotalWakeups;
static unsigned long long EndTicks;
static unsigned long long TotalTicks;
static unsigned long long MinTicks;
static unsigned long long MaxTicks;
   
void GetNextWriteAddress(
    size_t BlockSize,
    unsigned long long FileSize,
    off64_t& WriteOffset
    )
{
    WriteOffset = (__sync_fetch_and_add(&NextWriteOffset, BlockSize) % FileSize);
}

void AccountCompletedOperation(
    int& WriteCounter
    )
{
    WriteCounter = __sync_add_and_fetch(&TotalNumberWritten, (int)1);
}

struct myaiocontext
{
    //
    // Values for the user requested block
    off64_t rwFileOffset;
    size_t rwSize;
    void* rwBuffer;

    //
    // Values for the specific IO
    //
    struct iovec rwIovec;
    off64_t rwNextOffset;
    size_t rwNextSize;
    size_t rwActualSize;
    unsigned long long startTicks;
};

struct kxmiocontext
{
    OVERLAPPED Overlapped;
    void *rwBuffer;
    unsigned long long startTicks;
};

struct StartParameters
{
    int FileDescriptor;
    char* Filename;
    int EventFileDescriptor;
    struct fiemap* Fiemap;
    int FiemapExtentCount;               
    TestParameters::TestOperation Operation;
    TestParameters::TestAccess Access;
    unsigned int BlockSizeIn4K;
    unsigned int NumberBlocks;
    unsigned int TotalNumberIO;
    unsigned int ForegroundQueueDepth;
    unsigned int NumberThreads;
    unsigned int NumberCB;
    BOOLEAN UseDiskDirectIo;
    BOOLEAN WriteThrough;
    BOOLEAN UseMultipleFD;
    BOOLEAN SkipSplitIO;
    aio_context_t AioContext;
    struct iocb* Iocbs;
    struct iocb** IocbPtrs;
    size_t WriteSize;
    void* WriteBuffer;
    pthread_t MasterThread;
    HANDLE CompHandle;
    HANDLE BlockHandle;
    int Aiofd;
    HANDLE Kxmfd;
    struct kxmiocontext *KXMIOContext;
};

//
// AIO stuff that is needed since libs aren't available
//
int io_cancel(aio_context_t ctx, struct iocb *iocb,  struct io_event *result);
int io_setup(unsigned nr, aio_context_t *ctxp);
int io_destroy(aio_context_t ctx) ;
int io_submit(aio_context_t ctx, long nr,  struct iocb **iocbpp) ;
int io_getevents(aio_context_t ctx, long min_nr, long max_nr, struct io_event *events, struct timespec *timeout);

inline int io_setup(unsigned nr, aio_context_t *ctxp)
{
    return syscall(__NR_io_setup, nr, ctxp);
}

inline int io_destroy(aio_context_t ctx) 
{
    return syscall(__NR_io_destroy, ctx);
}

inline int io_submit(aio_context_t ctx, long nr,  struct iocb **iocbpp) 
{
    return syscall(__NR_io_submit, ctx, nr, iocbpp);
}

//`result` argument is no longer supported in 4.4 kernel.
inline int io_cancel(aio_context_t ctx, struct iocb *iocb,  struct io_event *result)
{
    return syscall(__NR_io_cancel, ctx, iocb, result);
}

inline int io_getevents(aio_context_t ctx, long min_nr, long max_nr,
        struct io_event *events, struct timespec *timeout)
{
    return syscall(__NR_io_getevents, ctx, min_nr, max_nr, events, timeout);
}

static void asyio_prep_pwritev(struct iocb *iocb, int fd, struct iovec *iov,
                               int nr_segs, int64_t offset, int afd)
{

    memset(iocb, 0, sizeof(*iocb));

    iocb->aio_fildes = fd;
    iocb->aio_lio_opcode = IOCB_CMD_PWRITEV;
    iocb->aio_reqprio = 0;
    iocb->aio_buf = (u_int64_t) iov;
    iocb->aio_nbytes = nr_segs;
    iocb->aio_offset = offset;
    iocb->aio_flags = 0;
//  iocb->aio_flags = IOCB_FLAG_RESFD;  // TODO: Is this useful ?
//  iocb->aio_resfd = afd;
}

static void asyio_prep_preadv(struct iocb *iocb, int fd, struct iovec *iov,
                  int nr_segs, int64_t offset, int afd)

{
    memset(iocb, 0, sizeof(*iocb));
    iocb->aio_fildes = fd;
    iocb->aio_lio_opcode = IOCB_CMD_PREADV;
    iocb->aio_reqprio = 0;
    iocb->aio_buf = (u_int64_t) iov;
    iocb->aio_nbytes = nr_segs;
    iocb->aio_offset = offset;
    iocb->aio_flags = 0;
//  iocb->aio_flags = IOCB_FLAG_RESFD; // TODO: Is this useful ?
//  iocb->aio_resfd = afd;
}

void InitializeAIOContext(
    StartParameters* startParameters
)
{
    int status;
    struct iocb* iocbs;            // array of actual iocb
    struct myaiocontext* writeContext;
    unsigned int numberCB = startParameters->ForegroundQueueDepth / startParameters->NumberThreads;
    long alignment = sysconf(_SC_PAGESIZE);

    memset(&startParameters->AioContext, 0, sizeof(aio_context_t));
    status = io_setup(numberCB, &startParameters->AioContext);
    FailOnError((status == 0), "io_setup");

    // TODO: Using time for seed won't work well
    srand( (unsigned)time(nullptr) );

    size_t writeSize = (size_t)startParameters->BlockSizeIn4K * 4096;

    void* writeBuffer;
    if (startParameters->Operation == TestParameters::TestOperation::Write)
    {
        writeBuffer = AllocRandomBuffer(writeSize);
        FailOnError((writeBuffer != nullptr), "AllocRandomBuffer");
    }

    size_t iocbsSize = numberCB * sizeof(struct iocb);
    size_t iocbPtrSize = numberCB * sizeof(struct iocb*);
    struct iocb** iocbPtr;         // array of pointers to iocb
    iocbs = (struct iocb*)malloc(iocbsSize + iocbPtrSize);
    FailOnError((iocbs != nullptr), "malloc iocbs");
    memset(iocbs, 0, iocbsSize + iocbPtrSize);
    iocbPtr = (struct iocb**)((char*)iocbs + iocbsSize);

    for (int i = 0; i < numberCB; i++)
    {
        struct iocb *cb = &iocbs[i];
        iocbPtr[i] = cb;

        writeContext = (struct myaiocontext*)malloc(sizeof(struct myaiocontext));
        FailOnError((writeContext != nullptr), "alloc writeContext");

        cb->aio_data = (u_int64_t)writeContext;

        writeContext->rwSize = writeSize;

        if (startParameters->Operation == TestParameters::TestOperation::Write)
        {
            writeContext->rwBuffer = writeBuffer;
        } else {
            void* readBuffer;
            status = posix_memalign((void**)&readBuffer, alignment, writeSize);
            FailOnError((status >= 0), "posix_memalign");
            writeContext->rwBuffer = readBuffer;     
        }
    }

    startParameters->Iocbs = iocbs;
    startParameters->IocbPtrs = iocbPtr;
    startParameters->WriteSize = writeSize;
    startParameters->WriteBuffer = writeBuffer;
    startParameters->NumberCB = numberCB;   
}

void UninitializeAIOContext(
    StartParameters* startParameters
)
{
    struct myaiocontext* writeContext;
    for (unsigned int i = 0; i < startParameters->NumberCB; i++)
    {
        struct iocb *cb = &startParameters->Iocbs[i];
        writeContext = (struct myaiocontext*)cb->aio_data;
        free(writeContext);
    }
    free(startParameters->Iocbs);

    io_destroy(startParameters->AioContext);             
}
                           
void* ReadWorkerAIO(void* Context)
{
    StartParameters startParameters2 = *(StartParameters*)Context;
    StartParameters* startParameters = &startParameters2;
    int status;
    int fd;
    BOOLEAN skipSplitIO = startParameters->SkipSplitIO;
    unsigned int numberReadOutstanding;
    size_t actualReadSize;
    unsigned int totalReads = startParameters->TotalNumberIO;
    size_t readSize;
    unsigned long long fileSize;
    void* readBuffer;
    char* verifyBuffer;
    off64_t fileOffset;
    off64_t readOffset;
    int readIovCount = 1;
    int readCounter;
    ssize_t bytesRead;
    int myCount = 0;
    long alignment = sysconf(_SC_PAGESIZE);

    InitializeAIOContext(startParameters);
    numberReadOutstanding = startParameters->NumberCB;
    readSize = startParameters->WriteSize;
    fileSize = readSize * startParameters->NumberBlocks;
    
    if (startParameters->UseMultipleFD)
    {
        int openFlags = O_RDWR | O_DIRECT | O_CLOEXEC;
        if (startParameters->WriteThrough)
        {
            openFlags |= O_DSYNC;
        }
        fd = open(startParameters->Filename, openFlags, S_IRWXU);
        FailOnError((fd != -1), "open ReadWorkerAIO");        
    } else {
        fd = startParameters->FileDescriptor;
    }

    void* writeBuffer = startParameters->WriteBuffer;
    
    //
    // AIO stuff
    //    
    struct iocb* iocbs = startParameters->Iocbs;            // array of actual iocb
    struct iocb** iocbPtr = startParameters->IocbPtrs;         // array of pointers to iocb
    size_t completionEventsSize = numberReadOutstanding * sizeof(struct io_event);
    struct io_event* completionEvents;
    int completedEventCount;
    struct myaiocontext* readContext;
    BOOLEAN isSplitRead;
    unsigned long long  endTicks;

    completionEvents = (struct io_event*)malloc(completionEventsSize);
    FailOnError((completionEvents != nullptr), "malloc completionEvents");
    memset(completionEvents, 0, completionEventsSize);

    for (int i = 0; i < numberReadOutstanding; i++)
    {
        struct iocb *cb = &iocbs[i];
        readContext = (struct myaiocontext*)cb->aio_data;
        struct iovec* readIov = &readContext->rwIovec;

        //
        // Kick off read for this
        //
        do
        {
            GetNextWriteAddress(readSize, fileSize, fileOffset);

            if (startParameters->UseDiskDirectIo)
            {
                isSplitRead = MapLogicalToPhysical(fileOffset, readSize, readOffset, actualReadSize, startParameters->Fiemap, startParameters->FiemapExtentCount);
            } else {
                readOffset = fileOffset;
                actualReadSize = readSize;
                isSplitRead = FALSE;
            }
        } while (skipSplitIO && isSplitRead);
        readContext->rwFileOffset = fileOffset;

        readIov->iov_base = readContext->rwBuffer;
        readIov->iov_len = actualReadSize;
        readContext->rwActualSize = actualReadSize;
        readContext->rwNextSize = (readSize - actualReadSize);
        readContext->rwNextOffset = fileOffset + actualReadSize;
        readContext->startTicks = 0;

        asyio_prep_preadv(cb, fd, readIov, readIovCount, readOffset, 0);
        cb->aio_data = (u_int64_t)readContext;          
    }
    
    //
    // Wait for wakeup
    //
    int epollFileDescriptor;
    struct epoll_event evnt = {0};

    epollFileDescriptor = epoll_create1(EPOLL_CLOEXEC);
    FailOnError((epollFileDescriptor != -1), "epoll_create1 failed");
    
    evnt.data.fd = startParameters->EventFileDescriptor;
    evnt.events = EPOLLIN | EPOLLONESHOT  ;
    status = epoll_ctl(epollFileDescriptor, EPOLL_CTL_ADD, startParameters->EventFileDescriptor, &evnt);
    FailOnError((status != -1), "epoll_ctl fail");
    
    struct epoll_event evnts[1];
        
    status = epoll_wait(epollFileDescriptor, evnts, 1, -1);
    if ((status == -1) && (errno != EINTR))
    {
        FailOnError((status != -1), "epoll_wait");
    }

    FailOnError((evnts[0].data.fd == startParameters->EventFileDescriptor),"Expect startParameters->EventFileDescriptor on wakeup");

    eventfd_t val;
    status = eventfd_read(startParameters->EventFileDescriptor, &val);
    
    //
    // Prime the pump
    //
    io_submit(startParameters->AioContext, numberReadOutstanding,  iocbPtr);
    
    BOOLEAN allDone = FALSE;
    while (TRUE)
    {
        completedEventCount = io_getevents(startParameters->AioContext, 1, numberReadOutstanding, completionEvents, nullptr);
        endTicks = _GetTickCount64();
        FailOnError((completedEventCount != -1), "io_getevents failed");

        __sync_add_and_fetch(&TotalWakeups, (int)completedEventCount);        

        for (int i = 0; i < completedEventCount; i++)
        {            
            struct iocb* cb = (struct iocb*)completionEvents[i].obj;
            readContext = (struct myaiocontext*)cb->aio_data;
            struct iovec* readIov = (struct iovec*)&readContext->rwIovec;
            FailOnError((completionEvents[i].res == readIov->iov_len), "aio failed");
            
            if (readContext->startTicks != 0)
            {
                unsigned long long ticks = (endTicks - readContext->startTicks);
                
                __sync_add_and_fetch(&TotalTicks, ticks);
                
                if (ticks < MinTicks)
                {
                    MinTicks = ticks;
                }

                if (ticks > MaxTicks)
                {
                    MaxTicks = ticks;
                }       
            }
            
            if (readContext->rwNextSize > 0)
            {
                //
                // We encountered a split read for this request so do
                // the next part of the read
                //
                FailOnError((!skipSplitIO), "Split read not expected");
                FailOnError(startParameters->UseDiskDirectIo, "Split read not expected");
                isSplitRead = MapLogicalToPhysical(readContext->rwNextOffset, readContext->rwNextSize,
                                     readOffset, actualReadSize,
                                     startParameters->Fiemap, startParameters->FiemapExtentCount);

                readIov->iov_base = (unsigned char *)readIov->iov_base + readIov->iov_len;
                readIov->iov_len = actualReadSize;
                readContext->rwActualSize = actualReadSize;
                readContext->rwNextSize = readContext->rwNextSize - actualReadSize;
                readContext->rwNextOffset = readContext->rwNextOffset + actualReadSize;
                asyio_prep_preadv(cb, fd, readIov, readIovCount, readOffset, 0);
                cb->aio_data = (u_int64_t)readContext;

                //
                // By resetting start ticks here we are not taking into
                // account the effect of discontiguous reads. We are
                // computing latency of each partial read.
                //
                readContext->startTicks = _GetTickCount64();
                io_submit(startParameters->AioContext, 1,  &cb);
                continue;
            }

            AccountCompletedOperation(readCounter);         
            if (readCounter > totalReads)
            {
                unsigned long long ticks = _GetTickCount64();
                __sync_val_compare_and_swap(&EndTicks, 0, ticks);
                allDone = TRUE;
                continue;
            }
            
            readBuffer = readContext->rwBuffer;

            verifyBuffer = (char *)AllocSpecificBuffer(readContext->rwFileOffset, readSize);
            FailOnError((verifyBuffer != nullptr), "verifyBuffer Alloc fail");

            status = memcmp(verifyBuffer, readBuffer, readSize);
            FailOnError((status == 0), "ReadVerifyError");

            do
            {
                GetNextWriteAddress(readSize, fileSize, fileOffset);

                if (startParameters->UseDiskDirectIo)
                {
                    isSplitRead = MapLogicalToPhysical(fileOffset, readSize, readOffset, actualReadSize, startParameters->Fiemap, startParameters->FiemapExtentCount);
                } else {
                    readOffset = fileOffset;
                    actualReadSize = readSize;
                    isSplitRead = FALSE;
                }
            } while (skipSplitIO && isSplitRead);
            readContext->rwFileOffset = fileOffset;

            readIov->iov_base = readContext->rwBuffer;          
            readIov->iov_len = actualReadSize;
            readContext->rwActualSize = actualReadSize;
            readContext->rwNextSize = (readSize - actualReadSize);
            readContext->rwNextOffset = fileOffset + actualReadSize;        

            asyio_prep_preadv(cb, fd, readIov, readIovCount, readOffset, 0);
            cb->aio_data = (u_int64_t)readContext;

            readContext->startTicks = _GetTickCount64();
            io_submit(startParameters->AioContext, 1,  &cb);

#if 0   // For testing only
            myCount++;
            if ((myCount % 1024) == 0)
            {
                printf("[%x] %d completed %d total completed\n", pthread_self(), myCount, readCounter);
            }
#endif          
        }

        if (allDone)
        {
            free(completionEvents);
            UninitializeAIOContext(startParameters);
            
            if (startParameters->UseMultipleFD)
            {
                close(fd);
            }
            return(nullptr);           
        }
    }   
}

void* WriteWorkerAIO(void* Context)
{
    StartParameters startParameters2 = *(StartParameters*)Context;
    StartParameters* startParameters = &startParameters2;
    int status;
    int fd;
    unsigned int numberWriteOutstanding;
    BOOLEAN skipSplitIO = startParameters->SkipSplitIO;
    size_t actualWriteSize;
    unsigned int totalWrites = startParameters->TotalNumberIO;
    size_t writeSize;
    unsigned long long fileSize;
    off64_t fileOffset;
    off64_t writeOffset;
    int writeIovCount = 1;
    int writeCounter;
    ssize_t bytesWritten;
    int myCount = 0;
    BOOLEAN isSplitWrite;
    unsigned long long endTicks;
    
    InitializeAIOContext(startParameters);
    numberWriteOutstanding = startParameters->NumberCB;
    writeSize = startParameters->WriteSize;
    fileSize = writeSize * startParameters->NumberBlocks;
    
    if (startParameters->UseMultipleFD)
    {
        int openFlags = O_RDWR | O_DIRECT | O_CLOEXEC;
        if (startParameters->WriteThrough)
        {
            openFlags |= O_DSYNC;
        }
        fd = open(startParameters->Filename, openFlags, S_IRWXU);
        FailOnError((fd != -1), "open WriteWorkerAIO");        
    } else {
        if(startParameters->Aiofd != -1){
            fd = startParameters->Aiofd;
        }else{
            fd = startParameters->FileDescriptor;
        }
    }
    
    void* writeBuffer = startParameters->WriteBuffer;
                       
    // AIO stuff
    struct iocb* iocbs = startParameters->Iocbs;            // array of actual iocb
    struct iocb** iocbPtr = startParameters->IocbPtrs;         // array of pointers to iocb
    size_t completionEventsSize = numberWriteOutstanding * sizeof(struct io_event);
    struct io_event* completionEvents;
    int completedEventCount;
    struct myaiocontext* writeContext;

    completionEvents = (struct io_event*)malloc(completionEventsSize);
    FailOnError((completionEvents != nullptr), "malloc completionEvents");
    memset(completionEvents, 0, completionEventsSize);

    for (int i = 0; i < numberWriteOutstanding; i++)
    {
        struct iocb *cb = &iocbs[i];
        writeContext = (struct myaiocontext*)cb->aio_data;
        struct iovec* writeIov = &writeContext->rwIovec;

        do
        {
            GetNextWriteAddress(writeSize, fileSize, fileOffset);

            if (startParameters->UseDiskDirectIo)
            {
                isSplitWrite = MapLogicalToPhysical(fileOffset, writeSize, writeOffset, actualWriteSize, startParameters->Fiemap, startParameters->FiemapExtentCount);
            } else {
                writeOffset = fileOffset;
                actualWriteSize = writeSize;
                isSplitWrite = FALSE;
            }
        } while (skipSplitIO && isSplitWrite);

        writeIov->iov_base = writeContext->rwBuffer;
        writeIov->iov_len = actualWriteSize;
        writeContext->rwActualSize = actualWriteSize;
        writeContext->rwNextSize = (writeSize - actualWriteSize);
        writeContext->rwNextOffset = fileOffset + actualWriteSize;
        writeContext->startTicks = 0;

        asyio_prep_pwritev(cb, fd, writeIov, writeIovCount, writeOffset, 0);
        cb->aio_data = (u_int64_t)writeContext;
    }
    
    //
    // Wait for wakeup
    //
    int epollFileDescriptor;
    struct epoll_event evnt = {0};

    epollFileDescriptor = epoll_create1(EPOLL_CLOEXEC);
    FailOnError((epollFileDescriptor != -1), "epoll_create1 failed");
    
    evnt.data.fd = startParameters->EventFileDescriptor;
    evnt.events = EPOLLIN | EPOLLONESHOT  ;
    status = epoll_ctl(epollFileDescriptor, EPOLL_CTL_ADD, startParameters->EventFileDescriptor, &evnt);
    FailOnError((status != -1), "epoll_ctl fail");
    
    struct epoll_event evnts[1];
        
    status = epoll_wait(epollFileDescriptor, evnts, 1, -1);
    if ((status == -1) && (errno != EINTR))
    {
        FailOnError((status != -1), "epoll_wait");
    }

    FailOnError((evnts[0].data.fd == startParameters->EventFileDescriptor),"Expect startParameters->EventFileDescriptor on wakeup");

    eventfd_t val;
    status = eventfd_read(startParameters->EventFileDescriptor, &val);
    
    //
    // Prime the pump
    //
    printf("[%s @ %d] -- numberwriteoutstanding %d\n", __FUNCTION__, __LINE__, numberWriteOutstanding); 

    io_submit(startParameters->AioContext, numberWriteOutstanding,  iocbPtr);

    printf("[%s @ %d]\n", __FUNCTION__, __LINE__); 
    BOOLEAN allDone = FALSE;
    while (TRUE)
    {
        completedEventCount = io_getevents(startParameters->AioContext, 1, numberWriteOutstanding, completionEvents, nullptr);
        endTicks = _GetTickCount64();
        FailOnError((completedEventCount != -1), "io_getevents failed");

        if (completedEventCount > 0)
        {
            __sync_add_and_fetch(&TotalWakeups, (int)1);
        }
        
        for (int i = 0; i < completedEventCount; i++)
        {
            struct iocb* cb = (struct iocb*)completionEvents[i].obj;
            
            writeContext = (struct myaiocontext*)cb->aio_data;
            struct iovec* writeIov = (struct iovec*)&writeContext->rwIovec;
            //FailOnError((completionEvents[i].res == writeIov->iov_len), "aio failed");

            if (writeContext->startTicks != 0)
            {
                unsigned long long ticks = (endTicks - writeContext->startTicks);
                
                __sync_add_and_fetch(&TotalTicks, ticks);

                if (ticks < MinTicks)
                {
                    MinTicks = ticks;
                }

                if (ticks > MaxTicks)
                {
                    MaxTicks = ticks;
                }
            }
            
            if (writeContext->rwNextSize > 0)
            {
                //
                // We encountered a split write for this request so do
                // the next part of the write
                //
                FailOnError((!skipSplitIO), "Split write not expected");
                FailOnError(startParameters->UseDiskDirectIo, "Split write not expected");
                MapLogicalToPhysical(writeContext->rwNextOffset, writeContext->rwNextSize,
                                     writeOffset, actualWriteSize,
                                     startParameters->Fiemap, startParameters->FiemapExtentCount);

                writeIov->iov_base = (unsigned char *)writeIov->iov_base + writeIov->iov_len;
                writeIov->iov_len = actualWriteSize;
                writeContext->rwActualSize = actualWriteSize;
                writeContext->rwNextSize = writeContext->rwNextSize - actualWriteSize;
                writeContext->rwNextOffset = writeContext->rwNextOffset + actualWriteSize;
                asyio_prep_pwritev(cb, fd, writeIov, writeIovCount, writeOffset, 0);
                printf("[%s@%d] WRITEOFFSET %#lx\n", __FUNCTION__, __LINE__, writeOffset);
                cb->aio_data = (u_int64_t)writeContext;
                
                //
                // By resetting start ticks here we are not taking into
                // account the effect of discontiguous reads. We are
                // computing latency of each partial read.
                //
                writeContext->startTicks = _GetTickCount64();
                io_submit(startParameters->AioContext, 1,  &cb);
                continue;
            }
            
            AccountCompletedOperation(writeCounter);            
            if (writeCounter > totalWrites)
            {
                unsigned long long ticks = _GetTickCount64();
                __sync_val_compare_and_swap(&EndTicks, 0, ticks);
                
                allDone = TRUE;
                continue;
            }

            do
            {
                GetNextWriteAddress(writeSize, fileSize, fileOffset);

                if (startParameters->UseDiskDirectIo)
                {
                    isSplitWrite = MapLogicalToPhysical(fileOffset, writeSize, writeOffset, actualWriteSize, startParameters->Fiemap, startParameters->FiemapExtentCount);
                } else {
                    writeOffset = fileOffset;
                    actualWriteSize = writeSize;
                    isSplitWrite = FALSE;
                }
            } while (skipSplitIO && isSplitWrite);
            writeContext->rwFileOffset = fileOffset;

            writeIov->iov_base = writeContext->rwBuffer;            
            writeIov->iov_len = actualWriteSize;
            writeContext->rwActualSize = actualWriteSize;
            writeContext->rwNextSize = (writeSize - actualWriteSize);
            writeContext->rwNextOffset = fileOffset + actualWriteSize;      

            asyio_prep_pwritev(cb, fd, writeIov, writeIovCount, writeOffset, 0);
            cb->aio_data = (u_int64_t)writeContext;
            
            //printf("[%s@%d] IOVBASE %lld, IOVLEN %lld, WRITEOFFSET %#lx\n", __FUNCTION__, __LINE__, writeIov->iov_base, writeIov->iov_len, writeOffset);
            writeContext->startTicks = _GetTickCount64();
            io_submit(startParameters->AioContext, 1,  &cb);

#if 0   // For testing only
            myCount++;
            if ((myCount % 1024) == 0)
            {
                printf("[%x] %d completed %d total completed\n", pthread_self(), myCount, writeCounter);
            }
#endif          
        }

        if (allDone)
        {
            free(completionEvents);
            UninitializeAIOContext(startParameters);
            
            if (startParameters->UseMultipleFD)
            {
                close(fd);
            }
            return(nullptr);           
        }
    }   
}


void DoBasicBlockFileTest(StartParameters *StartParam)
{
    int i = 0;
    unsigned long long fileSize;
    off64_t fileOffset;
    off64_t writeOffset;
    size_t actualWriteSize;
    BOOLEAN isSplitWrite;
    int numReq = 0;
    int numWriteOutstanding;
    size_t writeSize = (size_t)StartParam->BlockSizeIn4K * 4096;
    struct kxmiocontext *writeContext;
    struct kxmiocontext *writeContextArr;
    //KXMCompletionEvent completionEvent;
    NTSTATUS status;
    unsigned long completionKey;
    DWORD numbytes;
    bool result;

    fileSize = writeSize * StartParam->NumberBlocks;
    numWriteOutstanding = StartParam->ForegroundQueueDepth;
    writeContextArr = (struct kxmiocontext *)malloc(
                                    sizeof(struct kxmiocontext) * 
                                    numWriteOutstanding);

    FailOnError((writeContextArr != nullptr), "alloc writeContextArr");
    StartParam->KXMIOContext = writeContextArr; 

    printf("NumWriteOutstanding %d\n", numWriteOutstanding);
    while(numReq < numWriteOutstanding)
    {
        GetNextWriteAddress(writeSize, fileSize, fileOffset);
        isSplitWrite = MapLogicalToPhysical(fileOffset, writeSize, writeOffset,
                                    actualWriteSize, StartParam->Fiemap, StartParam->FiemapExtentCount);
        if(isSplitWrite)
            continue;

        writeContextArr[numReq].startTicks = 0;
        writeContextArr[numReq].rwBuffer = AllocRandomBuffer(writeSize);
        FailOnError((writeContextArr[numReq].rwBuffer != nullptr), "alloc rand buffer");
        writeContextArr[numReq].Overlapped.Offset = LSB_32(writeOffset);
        writeContextArr[numReq].Overlapped.OffsetHigh = MSB_32(writeOffset);
        printf("GotNextWrite: RWBuffer %p, NumPages %ld, SectorOffset %ld,"
                    "Overlapped %p\n", writeContextArr[numReq].rwBuffer, 
                    actualWriteSize/4096, writeOffset/512, 
                    &writeContextArr[numReq]);
        KXMWriteFileAsync(StartParam->Kxmfd,
                                    StartParam->BlockHandle,
                                    writeContextArr[numReq].rwBuffer,
                                    actualWriteSize/4096,
                                    (LPOVERLAPPED)&writeContextArr[numReq]);

        numReq++;
    }

    //Wait for Response.
    numReq = 0;
    while(numReq < numWriteOutstanding)
    {
        result = KXMGetQueuedCompletionStatus(StartParam->Kxmfd, 
                                StartParam->CompHandle, &numbytes,
                                &completionKey,
                                (LPOVERLAPPED *)&writeContext, INFINITE);

        printf("Get event returned status %d\n", status);
        printf("Status %llu, CompletionKey %lld\n",
                            writeContext->Overlapped.Internal, completionKey);
        free(writeContext->rwBuffer);
        numReq++;
    }

    free(writeContextArr);
}

void SubmitInitialRequest(StartParameters *StartParam, int RW)
{
    int i = 0;
    unsigned long long fileSize;
    off64_t fileOffset;
    off64_t offset;
    size_t actualWriteSize;
    BOOLEAN isSplitWrite;
    int numReq = 0;
    int numWriteOutstanding;
    size_t writeSize = (size_t)StartParam->BlockSizeIn4K * 4096;
    struct kxmiocontext *context;

    fileSize = writeSize * StartParam->NumberBlocks;
    numWriteOutstanding = StartParam->ForegroundQueueDepth;
    context = (struct kxmiocontext *)malloc(
                                    sizeof(struct kxmiocontext) * 
                                    StartParam->ForegroundQueueDepth);

    FailOnError((context != nullptr), "alloc context");
    StartParam->KXMIOContext = context; 

    while(numReq < numWriteOutstanding)
    {
        GetNextWriteAddress(writeSize, fileSize, fileOffset);
        isSplitWrite = MapLogicalToPhysical(fileOffset, writeSize, offset,
                                    actualWriteSize, StartParam->Fiemap, StartParam->FiemapExtentCount);
        if(isSplitWrite)
            continue;
        context[numReq].startTicks = 0;
        context[numReq].rwBuffer = AllocRandomBuffer(writeSize);
        FailOnError((context[numReq].rwBuffer != nullptr), "alloc rand buffer");

        context[numReq].Overlapped.Offset = LSB_32(offset);
        context[numReq].Overlapped.OffsetHigh = MSB_32(offset);

        if(RW == 1)
        {
            KXMWriteFileAsync(StartParam->Kxmfd,
                StartParam->BlockHandle, context[numReq].rwBuffer,
                actualWriteSize/4096,
                (LPOVERLAPPED)&context[numReq]);
        }
        else
        {
            KXMReadFileAsync(StartParam->Kxmfd,
                StartParam->BlockHandle, context[numReq].rwBuffer,
                actualWriteSize/4096,
                (LPOVERLAPPED)&context[numReq]);
        }
        numReq++;
    }
    printf("Submiited %d request.\n", numReq);
}

void FreeInitialRequestMemory(StartParameters *StartParam)
{
    struct kxmiocontext *context = StartParam->KXMIOContext;
    int numReq = 0;
    int numWriteOutstanding = StartParam->ForegroundQueueDepth;

    if(!context)
        return;

    while(numReq < numWriteOutstanding)
    {
        if(context[numReq].rwBuffer)
            free(context[numReq].rwBuffer);
        numReq++;
    }
    free(context);
}

void* KXMReadWorker(void* Context)
{
    StartParameters startParameters2 = *(StartParameters*)Context;
    StartParameters* startParameters = &startParameters2;
    int status;
    HANDLE fd;
    unsigned int numberWriteOutstanding;
    BOOLEAN skipSplitIO = startParameters->SkipSplitIO;
    size_t actualWriteSize;
    unsigned int totalWrites = startParameters->TotalNumberIO;
    size_t readSize;
    unsigned long long fileSize;
    off64_t fileOffset;
    off64_t readOffset;
    int readCounter;
    BOOLEAN isSplitWrite;
    unsigned long long endTicks;
    struct kxmiocontext* readContext;
    unsigned long completionKey;
    DWORD numbytes;
    bool result;

    numberWriteOutstanding = startParameters->ForegroundQueueDepth;
    readSize = startParameters->BlockSizeIn4K * 4096;
    fileSize = readSize * startParameters->NumberBlocks;
    
    fd = startParameters->Kxmfd;

    //
    // Wait for wakeup
    //
    int epollFileDescriptor;
    struct epoll_event evnt = {0};

    epollFileDescriptor = epoll_create1(EPOLL_CLOEXEC);
    FailOnError((epollFileDescriptor != -1), "epoll_create1 failed");
    
    evnt.data.fd = startParameters->EventFileDescriptor;
    evnt.events = EPOLLIN | EPOLLONESHOT  ;
    status = epoll_ctl(epollFileDescriptor, EPOLL_CTL_ADD, startParameters->EventFileDescriptor, &evnt);
    FailOnError((status != -1), "epoll_ctl fail");
    
    struct epoll_event evnts[1];
        
    status = epoll_wait(epollFileDescriptor, evnts, 1, -1);
    if ((status == -1) && (errno != EINTR))
    {
        FailOnError((status != -1), "epoll_wait");
    }

    FailOnError((evnts[0].data.fd == startParameters->EventFileDescriptor),"Expect startParameters->EventFileDescriptor on wakeup");

    eventfd_t val;
    status = eventfd_read(startParameters->EventFileDescriptor, &val);
    
    printf("[%s @ %d]\n", __FUNCTION__, __LINE__); 

    //KXMRegister
    BOOL b = KXMRegisterCompletionThread(fd, startParameters->CompHandle);
    FailOnError(b, "Registeration of Thread failed!\n");

    while (TRUE)
    {
        result = KXMGetQueuedCompletionStatus(fd, 
                        startParameters->CompHandle, &numbytes, 
                        &completionKey,
                        (LPOVERLAPPED *)&readContext, INFINITE);

        endTicks = _GetTickCount64();
        FailOnError(result == true, "GetqueuedCompletionStatus failed");

        if(readContext->Overlapped.Internal != STATUS_SUCCESS)
        {
            printf("Operation completed with error!");
        }

        if (readContext->startTicks != 0)
        {
            unsigned long long ticks = (endTicks - readContext->startTicks);

            __sync_add_and_fetch(&TotalTicks, ticks);

            if (ticks < MinTicks)
            {
                MinTicks = ticks;
            }

            if (ticks > MaxTicks)
            {
                MaxTicks = ticks;
            }
        }

        AccountCompletedOperation(readCounter);            
        if (readCounter > totalWrites)
        {
            unsigned long long ticks = _GetTickCount64();
            __sync_val_compare_and_swap(&EndTicks, 0, ticks);
            break;
        }

        do
        {
            GetNextWriteAddress(readSize, fileSize, fileOffset);
            isSplitWrite = MapLogicalToPhysical(fileOffset, readSize, 
                                    readOffset, actualWriteSize, 
                                    startParameters->Fiemap, 
                                    startParameters->FiemapExtentCount);
        } while (isSplitWrite);

        //printf("[%s@%d], PerfFD %d, BlockHandle %d, rwBuffer %p, "
                        //"NumPages %lu, Sector %lu\n", __FUNCTION__, __LINE__,
                        //startParameters->PerfFD, 
                        //startParameters->BlockHandle, readContext->rwBuffer, 
                        //actualWriteSize/4096, readOffset/512); 

        readContext->startTicks = _GetTickCount64();
        readContext->Overlapped.Offset = LSB_32(readOffset);
        readContext->Overlapped.OffsetHigh = MSB_32(readOffset);
        KXMReadFileAsync(fd, startParameters->BlockHandle,
                        readContext->rwBuffer, actualWriteSize/4096, 
                        (LPOVERLAPPED)readContext);

    }
    //KXMUnregister
    b = KXMUnregisterCompletionThread(fd, startParameters->CompHandle);
    FailOnError(b, "Unregisteration of Thread failed!\n");

    return(nullptr);           
}

void* KXMWriteWorker(void* Context)
{
    StartParameters startParameters2 = *(StartParameters*)Context;
    StartParameters* startParameters = &startParameters2;
    int status;
    unsigned int numberWriteOutstanding;
    BOOLEAN skipSplitIO = startParameters->SkipSplitIO;
    size_t actualWriteSize;
    unsigned int totalWrites = startParameters->TotalNumberIO;
    size_t writeSize;
    unsigned long long fileSize;
    off64_t fileOffset;
    off64_t writeOffset;
    int writeCounter;
    BOOLEAN isSplitWrite;
    unsigned long long endTicks;
    struct kxmiocontext* writeContext;
    unsigned long completionKey;
    HANDLE fd;
    DWORD numbytes;
    bool result;

    numberWriteOutstanding = startParameters->ForegroundQueueDepth;
    writeSize = startParameters->BlockSizeIn4K * 4096;
    fileSize = writeSize * startParameters->NumberBlocks;
    
    fd = startParameters->Kxmfd;

    //
    // Wait for wakeup
    //
    int epollFileDescriptor;
    struct epoll_event evnt = {0};

    epollFileDescriptor = epoll_create1(EPOLL_CLOEXEC);
    FailOnError((epollFileDescriptor != -1), "epoll_create1 failed");
    
    evnt.data.fd = startParameters->EventFileDescriptor;
    evnt.events = EPOLLIN | EPOLLONESHOT  ;
    status = epoll_ctl(epollFileDescriptor, EPOLL_CTL_ADD, startParameters->EventFileDescriptor, &evnt);
    FailOnError((status != -1), "epoll_ctl fail");
    
    struct epoll_event evnts[1];
        
    status = epoll_wait(epollFileDescriptor, evnts, 1, -1);
    if ((status == -1) && (errno != EINTR))
    {
        FailOnError((status != -1), "epoll_wait");
    }

    FailOnError((evnts[0].data.fd == startParameters->EventFileDescriptor),"Expect startParameters->EventFileDescriptor on wakeup");

    eventfd_t val;
    status = eventfd_read(startParameters->EventFileDescriptor, &val);
    
    //Register
    BOOL b = KXMRegisterCompletionThread(fd, startParameters->CompHandle);
    FailOnError(b, "Registeration of Thread failed!\n");

    while (TRUE)
    {
            result = KXMGetQueuedCompletionStatus(fd, 
                            startParameters->CompHandle, &numbytes, 
                            &completionKey, 
                            (LPOVERLAPPED *)&writeContext, INFINITE);
        
            endTicks = _GetTickCount64();
            FailOnError(result == true, "GetqueuedCompletionStatus failed");

            if(writeContext->Overlapped.Internal != STATUS_SUCCESS)
            {
                printf("Operation completed with error!\n");
            }

            if (writeContext->startTicks != 0)
            {
                    unsigned long long ticks = (endTicks - writeContext->startTicks);

                    __sync_add_and_fetch(&TotalTicks, ticks);

                    if (ticks < MinTicks)
                    {
                            MinTicks = ticks;
                    }

                    if (ticks > MaxTicks)
                    {
                            MaxTicks = ticks;
                    }
            }

            AccountCompletedOperation(writeCounter);            
            if (writeCounter > totalWrites)
            {
                    unsigned long long ticks = _GetTickCount64();
                    __sync_val_compare_and_swap(&EndTicks, 0, ticks);
                    break;
            }

            do
            {
                    GetNextWriteAddress(writeSize, fileSize, fileOffset);
                    isSplitWrite = MapLogicalToPhysical(fileOffset, writeSize, writeOffset, 
                                            actualWriteSize, startParameters->Fiemap, 
                                            startParameters->FiemapExtentCount);
            } while (isSplitWrite);

            //printf("[%s@%d], BlockHandle %d, rwBuffer %p, "
                            //"NumPages %lu, Sector %lu\n", __FUNCTION__, __LINE__,
                            //startParameters->BlockHandle, writeContext->rwBuffer, 
                            //actualWriteSize/4096, writeOffset); 
            writeContext->startTicks = _GetTickCount64();
            writeContext->Overlapped.Offset = LSB_32(writeOffset);
            writeContext->Overlapped.OffsetHigh = MSB_32(writeOffset);
    
            KXMWriteFileAsync(fd, startParameters->BlockHandle,
                            writeContext->rwBuffer, actualWriteSize/4096, 
                            (LPOVERLAPPED)writeContext);
    }

    //KXMUnregister
    b = KXMUnregisterCompletionThread(fd, startParameters->CompHandle);
    FailOnError(b, "Unregisteration of Thread failed!\n");

    return(nullptr);           
}

void* WriteWorker(void* Context)
{
    StartParameters* startParameters = (StartParameters*)Context;
    int status;
    int fd;
    BOOLEAN skipSplitIO = startParameters->SkipSplitIO;
    size_t writeSize = (size_t)startParameters->BlockSizeIn4K * 4096;
    size_t actualWriteSize;
    unsigned int totalWrites = startParameters->TotalNumberIO;
    unsigned long long fileSize = writeSize * startParameters->NumberBlocks;
    void* writeBuffer;
    off64_t fileOffset;
    off64_t writeOffset;
    struct iovec writeIov;
    int writeIovCount = 1;
    int writeCounter;
    ssize_t bytesWritten;
    int myCount = 0;
    unsigned long long startTicks, endTicks;

    if (startParameters->UseMultipleFD)
    {
        int openFlags = O_RDWR | O_DIRECT | O_CLOEXEC;
        if (startParameters->WriteThrough)
        {
            openFlags |= O_DSYNC;
        }
        fd = open(startParameters->Filename, openFlags, S_IRWXU);
        FailOnError((fd != -1), "open WriteWorker");        
    } else {
        fd = startParameters->FileDescriptor;
    }
    
    // TODO: Using time for seed won't work well
    srand( (unsigned)time(nullptr) );
    
    writeBuffer = AllocRandomBuffer(writeSize);
    FailOnError((writeBuffer != nullptr), "AllocRandomBuffer");

    writeIov.iov_base = writeBuffer;
    writeIov.iov_len = writeSize;   

    //
    // Wait for wakeup
    //
    int epollFileDescriptor;
    struct epoll_event evnt = {0};

    epollFileDescriptor = epoll_create1(EPOLL_CLOEXEC);
    FailOnError((epollFileDescriptor != -1), "epoll_create1 failed");
    
    evnt.data.fd = startParameters->EventFileDescriptor;
    evnt.events = EPOLLIN | EPOLLONESHOT  ;
    status = epoll_ctl(epollFileDescriptor, EPOLL_CTL_ADD, startParameters->EventFileDescriptor, &evnt);
    FailOnError((status != -1), "epoll_ctl fail");
    
    struct epoll_event evnts[1];
    while (TRUE)
    {
        status = epoll_wait(epollFileDescriptor, evnts, 1, -1);
        if ((status == -1) && (errno != EINTR))
        {
            FailOnError((status != -1), "epoll_wait");
        }

        FailOnError((evnts[0].data.fd == startParameters->EventFileDescriptor),"Expect startParameters->EventFileDescriptor on wakeup");
        
        eventfd_t val;
        status = eventfd_read(startParameters->EventFileDescriptor, &val);
        break;
    }    
    
    off64_t rwNextOffset;
    size_t rwNextSize = 0;
    
    while(TRUE)
    {
        BOOLEAN splitWrite;
        if (rwNextSize > 0)
        {
            FailOnError((! skipSplitIO), "Split write not expected");
            FailOnError(startParameters->UseDiskDirectIo, "Split write not expected");
            MapLogicalToPhysical(rwNextOffset, rwNextSize,
                                 writeOffset, actualWriteSize,
                                 startParameters->Fiemap, startParameters->FiemapExtentCount);

            
            writeIov.iov_base = (unsigned char *)writeIov.iov_base + writeIov.iov_len;
            writeIov.iov_len = actualWriteSize;
            rwNextSize = rwNextSize - actualWriteSize;
            rwNextOffset = rwNextOffset + actualWriteSize;
            splitWrite = TRUE;
        } else {
            BOOLEAN isSplitWrite;

            do
            {
                GetNextWriteAddress(writeSize, fileSize, fileOffset);

                if (startParameters->UseDiskDirectIo)
                {
                    isSplitWrite = MapLogicalToPhysical(fileOffset, writeSize, writeOffset, actualWriteSize, startParameters->Fiemap, startParameters->FiemapExtentCount);
                } else {
                    writeOffset = fileOffset;
                    actualWriteSize = writeSize;
                    isSplitWrite = FALSE;
                }
            } while (skipSplitIO && isSplitWrite);
            
            writeIov.iov_base = writeBuffer;
            writeIov.iov_len = actualWriteSize;
            
            rwNextOffset = fileOffset + actualWriteSize;
            rwNextSize = writeSize - actualWriteSize;
            splitWrite = FALSE;
        }

        startTicks = _GetTickCount64();
        bytesWritten = PWriteFileV(fd, &writeIov, writeIovCount, writeOffset);
        endTicks = _GetTickCount64();
        FailOnError((bytesWritten == actualWriteSize), "Did not write correct number of bytes");

        unsigned int long long ticks = (endTicks - startTicks);
        __sync_add_and_fetch(&TotalTicks, ticks);
        if (ticks < MinTicks)
        {
            MinTicks = ticks;
        }

        if (ticks > MaxTicks)
        {
            MaxTicks = ticks;
        }                   
        
        if (! splitWrite)
        {
            AccountCompletedOperation(writeCounter);
            if (writeCounter > totalWrites)
            {
                unsigned long long ticks = _GetTickCount64();
                __sync_val_compare_and_swap(&EndTicks, 0, ticks);

                if (startParameters->UseMultipleFD)
                {
                    close(fd);
                }
                return(nullptr);
            }
        }
        
#if 0   // For testing only
        myCount++;
        if ((myCount % 1024) == 0)
        {
            printf("[%x] %d completed %d total completed\n", pthread_self(), myCount, writeCounter);
        }
#endif
    }
}


void* ReadWorker(void* Context)
{
    StartParameters* startParameters = (StartParameters*)Context;
    int status;
    int fd;
    BOOLEAN skipSplitIO = startParameters->SkipSplitIO;
    size_t readSize = (size_t)startParameters->BlockSizeIn4K * 4096;
    size_t actualReadSize;
    unsigned int totalReads = startParameters->TotalNumberIO;
    unsigned long long fileSize = readSize * startParameters->NumberBlocks;
    char* readBuffer;
    char* verifyBuffer;
    off64_t fileOffset;
    off64_t readOffset;
    struct iovec readIov;
    int readIovCount = 1;
    int readCounter;
    ssize_t bytesRead;
    int myCount = 0;
    unsigned long long startTicks, endTicks;
    long alignment = sysconf(_SC_PAGESIZE);
    
    if (startParameters->UseMultipleFD)
    {
        int openFlags = O_RDWR | O_DIRECT | O_CLOEXEC;
        if (startParameters->WriteThrough)
        {
            openFlags |= O_DSYNC;
        }
        fd = open(startParameters->Filename, openFlags, S_IRWXU);
        FailOnError((fd != -1), "open WriteWorker");        
    } else {
        fd = startParameters->FileDescriptor;
    }
    
    status = posix_memalign((void**)&readBuffer, alignment, readSize);
    FailOnError((status >= 0), "posix_memalign");
    
    readIov.iov_base = readBuffer;
    readIov.iov_len = readSize;   

    //
    // Wait for wakeup
    //
    int epollFileDescriptor;
    struct epoll_event evnt = {0};

    epollFileDescriptor = epoll_create1(EPOLL_CLOEXEC);
    FailOnError((epollFileDescriptor != -1), "epoll_create1 failed");
    
    evnt.data.fd = startParameters->EventFileDescriptor;
    evnt.events = EPOLLIN | EPOLLONESHOT  ;
    status = epoll_ctl(epollFileDescriptor, EPOLL_CTL_ADD, startParameters->EventFileDescriptor, &evnt);
    FailOnError((status != -1), "epoll_ctl fail");
    
    struct epoll_event evnts[1];
    while (TRUE)
    {        
        status = epoll_wait(epollFileDescriptor, evnts, 1, -1);
        if ((status == -1) && (errno != EINTR))
        {
            FailOnError((status != -1), "epoll_wait");
        }

        FailOnError((evnts[0].data.fd == startParameters->EventFileDescriptor),"Expect startParameters->EventFileDescriptor on wakeup");
        
        eventfd_t val;
        status = eventfd_read(startParameters->EventFileDescriptor, &val);
        break;
    }
    
    off64_t rwNextOffset;
    size_t rwNextSize = 0;
    while(TRUE)
    {
        if (rwNextSize > 0)
        {
            FailOnError((! skipSplitIO), "Split write not expected");
            FailOnError(startParameters->UseDiskDirectIo, "Split write not expected");
            MapLogicalToPhysical(rwNextOffset, rwNextSize,
                                 readOffset, actualReadSize,
                                 startParameters->Fiemap, startParameters->FiemapExtentCount);

            
            readIov.iov_base = (unsigned char *)readIov.iov_base + readIov.iov_len;
            readIov.iov_len = actualReadSize;
            rwNextSize = rwNextSize - actualReadSize;
            rwNextOffset = rwNextOffset + actualReadSize;
        } else {
            BOOLEAN isSplitRead;

            do
            {
                GetNextWriteAddress(readSize, fileSize, fileOffset);

                if (startParameters->UseDiskDirectIo)
                {
                    MapLogicalToPhysical(fileOffset, readSize, readOffset, actualReadSize, startParameters->Fiemap, startParameters->FiemapExtentCount);
                } else {
                    readOffset = fileOffset;
                    actualReadSize = readSize;
                }
            } while (skipSplitIO && isSplitRead);
            
            readIov.iov_base = readBuffer;
            readIov.iov_len = actualReadSize;
            
            rwNextOffset = fileOffset + actualReadSize;
            rwNextSize = readSize - actualReadSize;
        }
        
        startTicks = _GetTickCount64();
        bytesRead = PReadFileV(fd, &readIov, readIovCount, readOffset);
        endTicks = _GetTickCount64();
        FailOnError((bytesRead == actualReadSize), "Did not read correct number of bytes");

        unsigned int long long ticks = (endTicks - startTicks);
        __sync_add_and_fetch(&TotalTicks, ticks);
        if (ticks < MinTicks)
        {
            MinTicks = ticks;
        }

        if (ticks > MaxTicks)
        {
            MaxTicks = ticks;
        }                   
        
        if (rwNextSize == 0)
        {
            AccountCompletedOperation(readCounter);
            if (readCounter > totalReads)
            {
                unsigned long long ticks = _GetTickCount64();
                __sync_val_compare_and_swap(&EndTicks, 0, ticks);
                return(nullptr);
            }

            
            verifyBuffer = (char *)AllocSpecificBuffer(fileOffset, readSize);
            FailOnError((verifyBuffer != nullptr), "verifyBuffer Alloc fail");

#if 0  // Use for testing
            for (int i = 0; i < readSize; i++)
            {
                if (verifyBuffer[i] != readBuffer[i])
                {
                    printf("ReadVerifyError: FileOffset: %lld BufferOffset: %d\n", fileOffset, i);
                    FailOnError(FALSE, "ReadVerifyError");
                }
            }
#else
            status = memcmp(verifyBuffer, readBuffer, readSize);
            FailOnError((status == 0), "ReadVerifyError");
#endif
        }
        
#if 0   // For testing only
        myCount++;
        if ((myCount % 1024) == 0)
        {
            printf("[%x] %d completed %d total completed\n", pthread_self(), myCount, readCounter);
        }
#endif
    }
}

void ReadFileValue(
    const char* FileName,
    char* string,
    int stringLen
)
{
    int fd;
    int count;

    fd = open(FileName, O_RDONLY);
    FailOnError((fd != -1), FileName);

    *string = 0;
    count = read(fd, string, stringLen);
    FailOnError((count != -1), "ReadFileValue");

    count = strlen(string);
    if ((count > 0) && (string[count-1] == 10))
    {
        string[count-1] = 0;
    }
    
    close(fd);
}

int
PerformTest(
    char *Filename1,
    char* Filename,
    TestParameters::TestOperation Operation,
    TestParameters::TestAccess Access,
    BOOLEAN WriteThrough,           
    unsigned int BlockSizeIn4K,
    unsigned int NumberBlocks,
    unsigned int TotalNumberIO,
    unsigned int ForegroundQueueDepth,
    unsigned int NumberThreads,
    BOOLEAN UseDiskDirectIo,
    BOOLEAN UseMultipleFD,
    BOOLEAN UseAIO,
    BOOLEAN SkipSplitIO,
    BOOLEAN LockAllPages,
    BOOLEAN UseKXMDrv,
    BOOLEAN UseAIODrv,
    struct fiemap* Fiemap,
    int FiemapExtentCount,               
    float& IoPerSecond,
    float& MBPerSecond
    )
{
    int status;
    int fd;
    float ticks;
    pthread_t* threads;
    unsigned long long startTicks, endTicks;
    StartParameters startParameters;
    long alignment = sysconf(_SC_PAGESIZE);
    HANDLE kxmfd = INVALID_HANDLE_VALUE;
    HANDLE blockHandle = INVALID_HANDLE_VALUE;
    HANDLE compHandle = INVALID_HANDLE_VALUE;
    HANDLE tmp;
    int aiofd = -1;

    int openFlags = O_RDWR | O_DIRECT | O_CLOEXEC;
    if (WriteThrough)
    {
        openFlags |= O_DSYNC;
    }

    printf("****Opening FileName %s****\n", Filename);

    fd = open(Filename, openFlags, S_IRWXU);
    if ((fd == -1) && (errno == EACCES))
    {
        printf("Did you remember to do sudo %s\n", Filename);
    }
    FailOnError((fd != -1), "open 3");


    if(UseKXMDrv)
    {
        char *blockfile = (char *)malloc(strlen(Filename1));
        if(!blockfile)
        {
            printf("Couldn't allocate block file!");
            return -1;
        }

        printf("****Opening DeviceFileName %s****\n", KXM_DEVICE_FILENAME);
        //Setup Disk Perf IO
        kxmfd = KXMOpenDev();
        FailOnError(kxmfd != nullptr, "KXM file open failed!");

        //Open Completion Port
        printf("Creating Completion Port.");
        compHandle = KXMCreateIoCompletionPort(kxmfd, INVALID_HANDLE_VALUE, nullptr, 0ULL, 0);
        FailOnError(compHandle != nullptr, "Completion port creation failed!"); 

		// TODO: Also test using KXM User emulation
		
		// TODO: Don't use strcpy
        strcpy(blockfile, Filename1);
        //Open Block File
		ULONG flags = 0;
        blockHandle = KXMCreateFile(kxmfd, -1, blockfile, nullptr, FALSE, BLOCK_FLUSH, 0, flags);
        FailOnError(blockHandle != INVALID_HANDLE_VALUE, "BlockFile creation failed!"); 

        //Associate BlockFile completion events with Completion Port.
        printf("Linking Completion Port with blockHandle");
        tmp = KXMCreateIoCompletionPort(kxmfd, blockHandle, compHandle, 0ULL, 0);

        FailOnError(tmp == compHandle, "Failed setting up completion port!");
        free(blockfile);
    }
    else if (UseAIODrv)
    {
        if(Operation != TestParameters::TestOperation::Write)
        {
            printf("Only write operation is supported with AIO Driver.\n");
            exit(0);
        }

        printf("****Opening DeviceFileName %s****\n", AIO_DEVICE_FILENAME);
        //Setup Disk Perf IO
        aiofd = OpenAIODrv(AIO_DEVICE_FILENAME);
        if (aiofd < 0)
        {
            printf("Opening file %s failed.\n", AIO_DEVICE_FILENAME);
            perror("");
            exit(0);
        }
        //Overwrite AIO/MultipleFD parameter.
        UseAIO = TRUE;
        UseMultipleFD = FALSE;
    }

    //
    // For both reads and writes, we want to fully write to the file
    //
    threads = (pthread_t*)malloc(ForegroundQueueDepth * sizeof(pthread_t));
    FailOnError((threads != nullptr), "OOM for threads");
    
    startParameters.EventFileDescriptor = eventfd(0, EFD_CLOEXEC | EFD_SEMAPHORE);
    FailOnError((startParameters.EventFileDescriptor != -1), "Create eventfd");

    startParameters.FileDescriptor = fd;
    startParameters.Filename = Filename;
    startParameters.Access = Access;
    startParameters.BlockSizeIn4K = BlockSizeIn4K;
    startParameters.NumberBlocks = NumberBlocks;
    startParameters.TotalNumberIO = TotalNumberIO;
    startParameters.ForegroundQueueDepth = ForegroundQueueDepth;
    startParameters.Fiemap = Fiemap;
    startParameters.FiemapExtentCount = FiemapExtentCount;
    startParameters.UseDiskDirectIo = UseDiskDirectIo;
    startParameters.WriteThrough = WriteThrough;
    startParameters.UseMultipleFD = UseMultipleFD;
    startParameters.SkipSplitIO = SkipSplitIO;
    startParameters.NumberThreads = NumberThreads;
    startParameters.Operation = Operation;
    startParameters.CompHandle = compHandle;
    startParameters.BlockHandle = blockHandle;
    startParameters.Kxmfd = kxmfd;
    startParameters.Aiofd = aiofd;

    NextWriteOffset = 0;
    TotalNumberWritten = 0;
    TotalWakeups = 0;

    //DoBasic Test.
    //DoBasicBlockFileTest(&startParameters);

    if(aiofd != -1){
        InitializeAIODrv(aiofd, Filename1, ForegroundQueueDepth, BlockSizeIn4K*4096);
    }

    if(UseKXMDrv) {
        //Prime the pump
        SubmitInitialRequest(&startParameters, 
                    Operation == TestParameters::TestOperation::Write ? 1 : 0);

        for (int i = 0; i < NumberThreads; i++)
        {
            if (Operation == TestParameters::TestOperation::Write)
            {
                status = pthread_create(&threads[i], nullptr, KXMWriteWorker, &startParameters);
            } else {
                status = pthread_create(&threads[i], nullptr, KXMReadWorker, &startParameters);
            }
        }
        startParameters.MasterThread = threads[0];
    } else if (! UseAIO) {
        for (int i = 0; i < ForegroundQueueDepth; i++)
        {
            if (Operation == TestParameters::TestOperation::Write)
            {
                status = pthread_create(&threads[i], nullptr, WriteWorker, &startParameters);
            } else {
                status = pthread_create(&threads[i], nullptr, ReadWorker, &startParameters);
            }

            FailOnError((status == 0), "Thread Create");
        }
    } else {
        for (int i = 0; i < NumberThreads; i++)
        {
            if (Operation == TestParameters::TestOperation::Write)
            {
                status = pthread_create(&threads[i], nullptr, WriteWorkerAIO, &startParameters);
            } else {
                status = pthread_create(&threads[i], nullptr, ReadWorkerAIO, &startParameters);
            }
        }
        startParameters.MasterThread = threads[0];
        FailOnError((status == 0), "Thread Create");    
    }

    
    //
    // Signal all threads to start after a short wait to get them
    // all ready to go.
    //
    printf("Sleeping 3 seconds to let threads settle\n");
    sleep(3);

    TotalTicks = 0;
    MinTicks = 0x7FFFFFFFFFFFFFFF;
    MaxTicks = 0;
    EndTicks = 0;
    startTicks = _GetTickCount64();

    if (LockAllPages)
    {
        status = mlockall(MCL_CURRENT | MCL_FUTURE);
        FailOnError((status == 0), "mlockall");
    }
    
    status = eventfd_write(startParameters.EventFileDescriptor, ForegroundQueueDepth);
    FailOnError((status == 0), "eventfd_write");

    // TODO: Use eventfd/epoll instead of waiting for pthread_join

    if (! UseAIO && !UseKXMDrv)
    {
        for (int i = 0; i < ForegroundQueueDepth; i++)
        {
            void* result; 

            status = pthread_join(threads[i], &result);
            FailOnError((status == 0), "Thread Join");
        }
    } else {

        for (int i = 0; i < NumberThreads; i++)
        {
            void* result; 
            status = pthread_join(threads[i], &result);
            FailOnError((status == 0), "Thread Join");
        }

    }

    if (LockAllPages)
    {
        status = munlockall();
        FailOnError((status == 0), "mlockall");
    }

    
    endTicks = EndTicks;

    ticks = (float)(endTicks - startTicks);

    free(threads);
    close(startParameters.EventFileDescriptor);
    close(fd);

    if(UseKXMDrv){
        FreeInitialRequestMemory(&startParameters);
        KXMCloseFile(kxmfd, blockHandle);
        KXMCloseIoCompletionPort(kxmfd, compHandle);
        KXMCloseDev(kxmfd);
    }

    //
    // Compute results
    //
    char scheduler[30];
    char nomerges[5];
    char nr_requests[20];
    
    ReadFileValue("/sys/block/sda/queue/scheduler", scheduler, 30);
    ReadFileValue("/sys/block/sda/queue/nomerges", nomerges, 30);
    ReadFileValue("/sys/block/sda/queue/nr_requests", nr_requests, 20);
    
    float fBlockSize = (float)(BlockSizeIn4K*0x1000);
    float fTotalNumberIO = (float)TotalNumberIO;
    float totalNumberBytes = fBlockSize * fTotalNumberIO;
    float seconds = ticks / 1000;
    float bytesPerSecond = (totalNumberBytes / seconds);
    float megaBytesPerSecond = bytesPerSecond / (0x100000);
    float numberIOs = (float)TotalNumberIO;
    float ioPerSecond = (float)numberIOs / seconds;
    float latency = (float)TotalTicks / fTotalNumberIO;
    
    printf("\n");
    printf("%f BlockSize, %f TotalNumberIo\n", fBlockSize, fTotalNumberIO);
    printf("%f bytes in %f seconds\n", totalNumberBytes, seconds);
    printf("%f bytes or %f MBytes per second\n", bytesPerSecond, megaBytesPerSecond);
    printf("    %f IO/sec\n", ioPerSecond);
    printf("    %f IO latency in ms\n", latency);

    printf("    %d Total Wakeups\n", TotalWakeups);
    printf("\n\n");
    printf("***, User, %s, %s, %s, %f, %d, %f, %d, %s, %s, %s, %s, %d, %d, %lld, %lld, %f, %f, %f",
           scheduler,
           nomerges,
           nr_requests,
           fBlockSize, ForegroundQueueDepth, fTotalNumberIO, NumberBlocks,
           UseMultipleFD ? "TRUE" : "FALSE",
           UseAIO ? "TRUE" : "FALSE",
           UseDiskDirectIo ? "TRUE" : "FALSE",
           Filename,
           NumberThreads,
           FiemapExtentCount,
           MinTicks, MaxTicks, latency,
           megaBytesPerSecond,         
           ioPerSecond);
           
    printf("\n\n");
    
    IoPerSecond = ioPerSecond;
    MBPerSecond = megaBytesPerSecond;

    return(0);
}



int
KBlockFileTestX(
    int argc, 
    char* args[]
    )
{
    int status = 0;
    char deviceName[4096];
    TestParameters testParameters;
    int fiemapExtentCount = 4096;
    void* fiemapBuffer;
    int fiemapBufferSize = sizeof(struct fiemap) + (fiemapExtentCount * sizeof(struct fiemap_extent));
    struct fiemap* fiemap1 = (struct fiemap*)fiemapBuffer;

    if (argc == 1)
    {
        Usage();
        return(ENOEXEC);
    }
    
    status = testParameters.Parse(argc, args);
    if (status != 0)
    {
        return(status);
    }

    //
    // Number of bytes written for each test is constant so we scale the number of blocks
    // based on the block size
    //
    unsigned int automationBlockSizes[] = { 1, 4, 16, 32, 64, 128, 256, 512, 1024 };
    unsigned int numberOfBlocksMultiple[] = { 1024, 512, 256, 128, 64, 32, 16, 4, 1 };
    const unsigned int automationBlockCount = (sizeof(automationBlockSizes) / sizeof(unsigned int));

    printf("\n");
    printf("Running LinuxIoPerf with the following:\n");
    printf("    TestOperation: %d\n", testParameters._TestOperation);
    printf("    TestAccess: %d\n", testParameters._TestAccess);
    printf("    WriteThrough: %s\n", testParameters._WriteThrough ? "TRUE" : "FALSE");
    printf("    Sparse: %s\n", testParameters._Sparse ? "TRUE" : "FALSE");
    printf("    UseDiskDirectIo: %s\n", testParameters._UseDiskDirectIo ? "TRUE" : "FALSE");
    printf("    UseAIO: %s\n", testParameters._UseAIO ? "TRUE" : "FALSE");
    printf("    Number AIO Threads: %d\n", testParameters._NumberThreads);
    printf("    UseMultipleFD: %s\n", testParameters._UseMultipleFD ? "TRUE" : "FALSE");
    printf("    TotalNumberIO: %d\n", testParameters._TotalNumberIO);
    printf("    Filename: %s\n", testParameters._Filename);
    if (testParameters._AutomatedMode)
    {
        printf("    BlockSizeIn4K: ");
        for (unsigned int i = 0; i < automationBlockCount; i++)
        {
            printf("%d, ", automationBlockSizes[i]);
        }
        printf("\n");
    }
    else 
    {
        printf("    BlockSizeIn4K: %d\n", testParameters._BlockSizeIn4K);
    }
    printf("    NumberBlocks: %d\n", testParameters._NumberBlocks);
    printf("    ForegroundQueueDepth: %d\n", testParameters._ForegroundQueueDepth);
    printf("\n");

    //
    // Test file needs to be created to account for the largest run
    //
    unsigned int blockSizeIn4K;
    if (testParameters._AutomatedMode)
    {
        blockSizeIn4K = 1024;
    }
    else 
    {
        blockSizeIn4K = testParameters._BlockSizeIn4K;
    }

    printf("Creating test file %d blocks %d bytes per block\n", testParameters._NumberBlocks, (blockSizeIn4K * (4 * 1024)));
    status = CreateTestFile(testParameters._Filename, testParameters._TestOperation,
                            blockSizeIn4K, testParameters._NumberBlocks, testParameters._WriteThrough, testParameters._Sparse);
    if (status != 0)
    {
        return(status);
    }
    printf("\n");

    if (testParameters._UseDiskDirectIo)
    {
        fiemapBuffer = malloc(fiemapBufferSize);
        FailOnError((fiemapBuffer != nullptr), "Alloc fiemapBuffer");
        memset(fiemapBuffer, 0, fiemapBufferSize);
        fiemap1 = (struct fiemap*)fiemapBuffer;

        status = LoadFiemap(testParameters._Filename, testParameters._TestOperation, fiemap1, fiemapExtentCount);
        if (status != 0)
        {
            return(status);
        }

        status = GetDirectDiskDeviceName(testParameters._Filename, deviceName);
        if (status != 0)
        {
            return(status);
        }

    } else {
        strcpy(deviceName, testParameters._Filename);
    }
    
    if (testParameters._AutomatedMode)
    {
        float ioPerSecond[automationBlockCount];
        float mbPerSecond[automationBlockCount];

        for (unsigned int i = 0; i < automationBlockCount; i++)
        {
            unsigned int numberBlocks = testParameters._NumberBlocks * numberOfBlocksMultiple[i];
            unsigned int totalNumberIO = testParameters._TotalNumberIO * numberOfBlocksMultiple[i];
            blockSizeIn4K = automationBlockSizes[i];

            printf("Starting test %d blocks %d total IO %d bytes per block....\n", numberBlocks, totalNumberIO, blockSizeIn4K * 4096);
            status = PerformTest(testParameters._Filename, deviceName, testParameters._TestOperation, testParameters._TestAccess,
                testParameters._WriteThrough,
                blockSizeIn4K, numberBlocks,
                totalNumberIO,
                testParameters._ForegroundQueueDepth,
                testParameters._NumberThreads,
                testParameters._UseDiskDirectIo,                                
                testParameters._UseMultipleFD,                                
                testParameters._UseAIO,                                
                testParameters._SkipSplitIO,                                
                testParameters._LockAllPages,                                
                testParameters._UseKXMDrv,
                testParameters._UseAIODrv,
                fiemap1,
                fiemapExtentCount,
                ioPerSecond[i],
                mbPerSecond[i]);
            printf("\n");

            if (status != 0)
            {
                return(status);
            }
        }
    }
    else 
    {
        float ioPerSecond, mbPerSecond;

        printf("Starting test %d bytes per block....\n", (testParameters._BlockSizeIn4K * 4096));
        status = PerformTest(testParameters._Filename, deviceName, testParameters._TestOperation, testParameters._TestAccess,
            testParameters._WriteThrough,
            testParameters._BlockSizeIn4K, testParameters._NumberBlocks,
            testParameters._TotalNumberIO,
            testParameters._ForegroundQueueDepth,
            testParameters._NumberThreads,
            testParameters._UseDiskDirectIo,                                
            testParameters._UseMultipleFD,                                
            testParameters._UseAIO,                                
            testParameters._SkipSplitIO,
            testParameters._LockAllPages,
            testParameters._UseKXMDrv,
            testParameters._UseAIODrv,
            fiemap1,
            fiemapExtentCount,
            ioPerSecond,
            mbPerSecond);

        if (status != 0)
        {
            return(status);
        }
    }

    return status;
}

int
KBlockFileTest(
    int argc, 
    char* args[]
    )
{
    int status;

    status = KBlockFileTestX(argc, args);

    return status;
}


int main(int argc, char* cargs[])
{
    return KBlockFileTest(argc, cargs); 
}
