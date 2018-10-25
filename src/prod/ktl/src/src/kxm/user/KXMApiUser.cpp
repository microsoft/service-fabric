/*++

Copyright (c) Microsoft Corporation

Module Name:

    KXMApiUser.cpp

Abstract:

    Implementation of user space APIs to acces KXM driver.

--*/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <mntent.h>
#include <errno.h>
#include <palio.h>
#include <palerr.h>
#include "KXMApi.h"
#include <ktl.h>
#include <ktrace.h>

#include <palhandle.h>
#include "KXMUKApi.h"

#define Message(a, b...) //printf("[%s@%d] "a"\n", __FUNCTION__, __LINE__, ##b)

ULONG pageSize = sysconf(_SC_PAGESIZE);

//////////////////////////////////////
//////////////////////////////////////
////////CompletionPort APIs///////////
//////////////////////////////////////
//////////////////////////////////////

const ULONG KTL_TAG_IOCOMPLETION = 'pcoI';
struct TpIoCompletionMessage
{
    DWORD        _NumBytes;
    ULONG_PTR    _Key;
    LPOVERLAPPED _LpOverlapped;
};

class TpIoCompletionPort : public KObject<TpIoCompletionPort>
{
public:

    TpIoCompletionPort(KAllocator& Allocator)
    : _Msgs(Allocator)
    {
        pthread_mutex_init(&_Mutex, NULL);
        sem_init(&_Sem, 0, 0);
    };

    ~TpIoCompletionPort()
    {
        pthread_mutex_destroy(&_Mutex);
        sem_destroy(&_Sem);
    };

    TpIoCompletionMessage Get()
    {
        sem_wait(&_Sem);
        pthread_mutex_lock(&_Mutex);
        TpIoCompletionMessage msg = _Msgs.Peek();
        _Msgs.Deq();
        pthread_mutex_unlock(&_Mutex);
        return msg;
    };

    void Post(TpIoCompletionMessage msg)
    {
        pthread_mutex_lock(&_Mutex);
        _Msgs.Enq(msg);
        pthread_mutex_unlock(&_Mutex);
        sem_post(&_Sem);
    };

private:
    sem_t                         _Sem;
    pthread_mutex_t               _Mutex;
    KQueue<TpIoCompletionMessage> _Msgs;
};

BOOL
WINAPI
GetQueuedCompletionStatusU(
    _In_  HANDLE       CompletionPort,
    _Out_ LPDWORD      lpNumberOfBytes,
    _Out_ PULONG_PTR   lpCompletionKey,
    _Out_ LPOVERLAPPED *lpOverlapped,
    _In_  DWORD        dwMilliseconds
    )
{
    TpIoCompletionPort *cp = (TpIoCompletionPort*)CompletionPort;
    TpIoCompletionMessage msg = cp->Get();
    *lpNumberOfBytes = msg._NumBytes;
    *lpCompletionKey = msg._Key;
    *lpOverlapped = msg._LpOverlapped;
    return TRUE;
}

BOOL
WINAPI
PostQueuedCompletionStatusU(
    _In_     HANDLE       CompletionPort,
    _In_     DWORD        dwNumberOfBytesTransferred,
    _In_     ULONG_PTR    dwCompletionKey,
    _In_opt_ LPOVERLAPPED lpOverlapped
    )
{
    TpIoCompletionPort *cp = (TpIoCompletionPort*)CompletionPort;
    TpIoCompletionMessage msg {dwNumberOfBytesTransferred, dwCompletionKey, lpOverlapped};
    cp->Post(msg);
    return TRUE;
}

HANDLE
CreateIoCompletionPortU(
    _In_     HANDLE      FileHandle,
    _In_opt_ HANDLE      ExistingCompletionPort,
    _In_     ULONG_PTR   CompletionKey,
    _In_     DWORD       NumberOfConcurrentThreads
    )
{
    TpIoCompletionPort *cp = static_cast<TpIoCompletionPort*>(ExistingCompletionPort);
    KAllocator& allocator = KtlSystemCoreImp::TryGetDefaultKtlSystemCore()->NonPagedAllocator();
    
    if (!cp) {
        cp = _new(KTL_TAG_IOCOMPLETION, allocator) TpIoCompletionPort(allocator);
        if (!cp) {
            KTraceFailedAsyncRequest(STATUS_INSUFFICIENT_RESOURCES, NULL, sizeof(TpIoCompletionPort), (ULONGLONG)&allocator);
            return nullptr;
        }
    }

    return (HANDLE)cp;
}

BOOL RegisterCompletionThreadU(HANDLE CompHandle)
{
    return true;
}

BOOL UnregisterCompletionThreadU(HANDLE CompHandle)
{
    return true;
}

BOOL CloseIoCompletionPortU(HANDLE CompHandle)
{
    TpIoCompletionPort *cp = static_cast<TpIoCompletionPort*>(CompHandle);   
    _delete(cp);
    return true;
}

//////////////////////////////////////
//////////////////////////////////////
////////////KXM File APIs/////////////
//////////////////////////////////////
//////////////////////////////////////

// User

class ReadWriteHelpers
{
    public:
    
    static ssize_t PReadFile(int fd, void* buf, size_t count, off64_t offset)
    {
        ssize_t numRead = 0;
        while (numRead < count) {
            ssize_t num = pread64(fd, buf, count, offset);
            if (num == 0) {
                return numRead;
            }
            if (num < 0 && errno != EINTR) {
                return num;
            }
            numRead += num;
            offset += num;
        }
        return numRead;
    }

    static ssize_t PWriteFile(int fd, const void* buf, size_t count, off64_t offset)
    {
        ssize_t numWrite = 0;
        while (numWrite < count) {
            ssize_t num = pwrite(fd, buf, count, offset);
            if (num == 0) {
                return numWrite;
            }
            else if (num < 0) {
                return num;
            }
            numWrite += num;
            offset += num;
        }
        return numWrite;
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

};


//
// This is the open routine for those files that need to be opened and
// accessed via the file system only.
//
HANDLE KXMCreateFileUFS(int FileFD, BOOLEAN IsEventFile, FileMode Mode,
                        pid_t RemotePID, ULONG Flags)
{
    HANDLE handle;
    
    if (! IsEventFile)
    {
        //
        // For Blockfile, the file is opened by the upper layer so we
        // just hang on to the FD here
        //
        handle = (HANDLE)FileFD;
    } else {
        // TODO:  emulate event files
        KTraceFailedAsyncRequest(STATUS_NOT_SUPPORTED, NULL, Mode, (ULONGLONG)FileFD);
        handle = INVALID_HANDLE_VALUE;
        SetLastError(ERROR_NOT_SUPPORTED);
    }
    
    return(handle);
}

NTSTATUS GetDirectDiskDeviceFD(
    const char *FileName,
    int& DiskDeviceFD
    )
{
    NTSTATUS status = STATUS_NOT_FOUND;
    struct mntent mm;
    struct mntent *m = NULL;
    struct stat s;
    FILE * fp;
    dev_t  dev;
    const ULONG buflen = 4096;
    char buf[buflen];
    const char *devdirprefix = "/dev/";

    //
    // Map the file to the underlying disk device name
    //
    if (stat(FileName, &s) != 0) {
        status = LinuxError::LinuxErrorToNTStatus(errno);
        KTraceFailedAsyncRequest(status, NULL, errno, (ULONGLONG)0);
        return status;
    }

    dev = s.st_dev;

    if ((fp = setmntent("/proc/mounts", "r")) == NULL) {
        status = LinuxError::LinuxErrorToNTStatus(errno);
        KTraceFailedAsyncRequest(status, NULL, errno, (ULONGLONG)0);
        return(status);
    }

    while (getmntent_r(fp, &mm, buf, buflen)) {
        if (stat(mm.mnt_dir, &s) != 0) {
            continue;
        }

        if (s.st_dev == dev && (strstr(mm.mnt_fsname, devdirprefix) != nullptr) ) {
            m = &mm;
            //
            // Try to open the disk device and get its FD
            // TODO: We can be smart and cache the FD for the different disk
            //       block devices
            //
            DiskDeviceFD = open(m->mnt_fsname, O_RDWR | O_SYNC | O_DIRECT | O_CLOEXEC, S_IRWXU);
            if (DiskDeviceFD == -1)
            {
                status = LinuxError::LinuxErrorToNTStatus(errno);
                KTraceFailedAsyncRequest(status, NULL, errno, (ULONGLONG)0);
                //TODO: we can continue and see if file is mapped by any other dev file. 
                break;
            }
            endmntent(fp);
            return(STATUS_SUCCESS);
        }
    }

    endmntent(fp);
    KTraceFailedAsyncRequest(status, NULL, errno, (ULONGLONG)0);
    return (status);
}

//
// This is the open routine for those files that need to be opened and
// accessed via direct write to disk blocks.
//
HANDLE KXMCreateFileUDW(const char *FileName, BOOLEAN IsEventFile, FileMode Mode,
                        pid_t RemotePID, ULONG Flags)
{
    NTSTATUS status;
    HANDLE handle;
    int diskDeviceFD;
    
    if (! IsEventFile)
    {
        //
        // For Blockfile, the file will be read and written using
        // direct disk access. Need to map the file handle from file to
        // disk.
        //
        status = GetDirectDiskDeviceFD(FileName, diskDeviceFD);
        if (! NT_SUCCESS(status))
        {
            SetLastError(WinError::NTStatusToWinError(status));
            handle = INVALID_HANDLE_VALUE;
        } else {        
            handle = (HANDLE)diskDeviceFD;
        }
    } else {
        // TODO:  emulate event files
        KTraceFailedAsyncRequest(STATUS_NOT_SUPPORTED, NULL, Mode, (ULONGLONG)0);
        handle = INVALID_HANDLE_VALUE;
        SetLastError(ERROR_NOT_SUPPORTED);
    }
    
    return(handle);
}

class KXMFileOperation : public KShared<KXMFileOperation>, public SystemThreadWorker<KXMFileOperation>
{
    K_FORCE_SHARED(KXMFileOperation);

    public:
        static KXMFileOperation::SPtr KXMFileOperation::Create();
        VOID Execute();

    public:
        enum { WriteFileAsync, ReadFileAsync, ReadFileScatterAsync, WriteFileGatherAsync } _Operation;

        HANDLE _DevFD;
        KXMFileHandle::SPtr _FileHandle;
        void* _UserAddress;
        int _NumPages;
        LPOVERLAPPED _Overlapped;
        int _NumEntries;
        struct iovec _Iov[KXM_MAX_FILE_SG];
        KXMFileOffset _Frags[KXM_MAX_FILE_SG];
};

KXMFileOperation::KXMFileOperation()
{
}

KXMFileOperation::~KXMFileOperation()
{
}

KXMFileOperation::SPtr KXMFileOperation::Create()
{
    return _new(KXM_FILEHANDLE_TAG,
                KtlSystemCoreImp::TryGetDefaultKtlSystemCore()->PagedAllocator())
            KXMFileOperation();
}

VOID KXMFileOperation::Execute()
{
    NTSTATUS status;
    ssize_t bytes;
    int fd;
    ULONGLONG offset = (((unsigned long long)_Overlapped->OffsetHigh) << 32) |
                        ((unsigned long long)_Overlapped->Offset & 0xffffffffU);
    
    fd = (int)_FileHandle->GetKxmHandle();
    if (! _FileHandle->IsOpen())
    {
        status = STATUS_INVALID_HANDLE;
        KTraceFailedAsyncRequest(status, NULL, 0, (ULONGLONG)_FileHandle.RawPtr());
    } else {    
        switch(_Operation)
        {
            case KXMFileOperation::WriteFileAsync:
            {
                ULONG length = _NumPages * pageSize;

                bytes = ReadWriteHelpers::PWriteFile(fd, _UserAddress, length, offset);

                if (bytes <= 0)
                {
                    status = LinuxError::LinuxErrorToNTStatus(errno);
                    KDbgCheckpointWDataInformational(0, "KXMFileOperation::WriteFileAsync", status,
                                        (ULONGLONG)_FileHandle.RawPtr(),
                                        (ULONGLONG)_UserAddress,
                                        (ULONGLONG)length,
                                        (ULONGLONG)offset);                                
                } else {
                    status = STATUS_SUCCESS;
                    _Overlapped->InternalHigh = bytes;
                }

                break;
            }

            case KXMFileOperation::ReadFileAsync:
            {
                ULONG length = _NumPages * pageSize;

                bytes = ReadWriteHelpers::PReadFile(fd, _UserAddress, length, offset);

                if (bytes <= 0)
                {
                    status = LinuxError::LinuxErrorToNTStatus(errno);
                    KDbgCheckpointWDataInformational(0, "KXMFileOperation::ReadFileAsync", status,
                                        (ULONGLONG)_FileHandle.RawPtr(),
                                        (ULONGLONG)_UserAddress,
                                        (ULONGLONG)length,
                                        (ULONGLONG)offset);                                
                } else {
                    status = STATUS_SUCCESS;
                    _Overlapped->InternalHigh = bytes;
                }

                break;
            }

            case KXMFileOperation::ReadFileScatterAsync:
            {
                BOOLEAN isDirectRead;

                isDirectRead = ! _FileHandle->UseFileSystemOnly();

                if (isDirectRead)
                {
                    status = STATUS_SUCCESS;
                    
                    ssize_t bytes1;
                    
                    bytes = 0;
                    for (ULONG i = 0; i < _NumEntries; i++)
                    {
                        bytes1 = ReadWriteHelpers::PReadFile(fd,
                                                             (void*)_Frags[i].UserAddress,
                                                             _Frags[i].NumPages * pageSize,
                                                             _Frags[i].PhysicalAddress);
                        if (bytes1 <= 0)
                        {
                            status = LinuxError::LinuxErrorToNTStatus(errno);
                            KDbgCheckpointWDataInformational(0, "KXMFileOperation::ReadFileScatterAsync", status,
                                                (ULONGLONG)_FileHandle.RawPtr(),
                                                (ULONGLONG)_Frags[i].UserAddress,
                                                (ULONGLONG)_Frags[i].NumPages * pageSize,
                                                (ULONGLONG)_Frags[i].PhysicalAddress);                                
                            break;
                        }
                        
                        bytes += bytes1;
                    }
                } else {
                    struct iovec* iov = _Iov;
                    int iovCount = _NumEntries;
                    
                    for (ULONG i = 0; i < iovCount; i++)
                    {
                        iov[i].iov_base = (void *)_Frags[i].UserAddress;
                        iov[i].iov_len = _Frags[i].NumPages * pageSize;
                    }

                    bytes = ReadWriteHelpers::PReadFileV(fd, iov, iovCount, offset);
                    if (bytes <= 0)
                    {
                        status = LinuxError::LinuxErrorToNTStatus(errno);
                        KDbgCheckpointWDataInformational(0, "KXMFileOperation::ReadFileScatterAsync", status,
                                            (ULONGLONG)_FileHandle.RawPtr(),
                                            (ULONGLONG)iov,
                                            (ULONGLONG)iovCount,
                                            (ULONGLONG)offset);
                        
                        for (ULONG i = 0; i < iovCount; i++)
                        {
                            KDbgCheckpointWDataInformational(0, "KXMFileOperation::ReadFileScatterAsync", status,
                                                (ULONGLONG)_FileHandle.RawPtr(),
                                                (ULONGLONG)iov[i].iov_base,
                                                (ULONGLONG)iov[i].iov_len,
                                                (ULONGLONG)0);
                        }
                        
                    } else {
                        status = STATUS_SUCCESS;
                        _Overlapped->InternalHigh = bytes;
                    }
                }
                
                break;
            }

            case KXMFileOperation::WriteFileGatherAsync:
            {
                BOOLEAN isDirectWrite;

                isDirectWrite = ! _FileHandle->UseFileSystemOnly();

                if (isDirectWrite)
                {
                    status = STATUS_SUCCESS;
                    
                    ssize_t bytes1;
                    
                    bytes = 0;
                    for (ULONG i = 0; i < _NumEntries; i++)
                    {
                        bytes1 = ReadWriteHelpers::PWriteFile(fd,
                                                             (void*)_Frags[i].UserAddress,
                                                             _Frags[i].NumPages * pageSize,
                                                             _Frags[i].PhysicalAddress);
                        if (bytes1 <= 0)
                        {
                            status = LinuxError::LinuxErrorToNTStatus(errno);
                            KDbgCheckpointWDataInformational(0, "KXMFileOperation::WriteFileGatherAsync", status,
                                                (ULONGLONG)_FileHandle.RawPtr(),
                                                (ULONGLONG)_Frags[i].UserAddress,
                                                (ULONGLONG)_Frags[i].NumPages * pageSize,
                                                (ULONGLONG)_Frags[i].PhysicalAddress);                                
                            break;
                        }
                        
                        bytes += bytes1;
                    }
                } else {
                    struct iovec* iov = _Iov;
                    int iovCount = _NumEntries;

                    for (ULONG i = 0; i < iovCount; i++)
                    {
                        iov[i].iov_base = (void *)_Frags[i].UserAddress;
                        iov[i].iov_len = _Frags[i].NumPages * pageSize;
                    }

                    bytes = ReadWriteHelpers::PWriteFileV(fd, iov, iovCount, offset);
                    if (bytes <= 0)
                    {
                        status = LinuxError::LinuxErrorToNTStatus(errno);
                        KDbgCheckpointWDataInformational(0, "KXMFileOperation::WriteFileGatherAsync", status,
                                            (ULONGLONG)_FileHandle.RawPtr(),
                                            (ULONGLONG)iov,
                                            (ULONGLONG)iovCount,
                                            (ULONGLONG)offset);
                        
                        for (ULONG i = 0; i < iovCount; i++)
                        {
                            KDbgCheckpointWDataInformational(0, "KXMFileOperation::WriteFileGatherAsync", status,
                                                (ULONGLONG)_FileHandle.RawPtr(),
                                                (ULONGLONG)iov[i].iov_base,
                                                (ULONGLONG)iov[i].iov_len,
                                                (ULONGLONG)0);
                        }
                    } else {
                        status = STATUS_SUCCESS;
                        _Overlapped->InternalHigh = bytes;
                    }
                }
                break;
            }

            default:
            {
                KInvariant(FALSE);
            }
        }
    }

    //
    // Complete the request back through the completion port
    //
    BOOL bl;

    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, NULL, _Operation, (ULONGLONG)_FileHandle.RawPtr());
        KTraceFailedAsyncRequest(status, NULL, errno, (ULONGLONG)_FileHandle.RawPtr());
        KInvariant(status != STATUS_INVALID_PARAMETER);
    }
    
    _Overlapped->Internal = status;
    bl = KXMPostQueuedCompletionStatus(_DevFD,
                                       _FileHandle->GetCompletionPort(),
                                       bytes,
                                       _FileHandle->GetCompletionKey(),
                                       _Overlapped);
    
    // TODO: What else can we do if we can't post ??
    if (! bl)
    {
        KTraceFailedAsyncRequest(status, NULL, GetLastError(), (ULONGLONG)_FileHandle.RawPtr());
        KTraceFailedAsyncRequest(status, NULL, errno, (ULONGLONG)_FileHandle.RawPtr());
        KInvariant(bl);
    }
    
    //
    // Remove ref taken when thread started
    //
    Release();
}

NTSTATUS KXMReadFileAsyncU(HANDLE DevFD, KXMFileHandle& FileHandle, void *UserAddress,
                           int NumPages, LPOVERLAPPED Overlapped)
{
    NTSTATUS status;
    KXMFileOperation::SPtr operation;
    
    operation = KXMFileOperation::Create();
    if (! operation)
    {
        KTraceFailedAsyncRequest(STATUS_INSUFFICIENT_RESOURCES, NULL, 0, (ULONGLONG)&FileHandle);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }
    
    operation->_Operation = KXMFileOperation::ReadFileAsync;

    operation->_DevFD = DevFD;
    operation->_FileHandle = &FileHandle;
    operation->_UserAddress = UserAddress;
    operation->_NumPages = NumPages;
    operation->_Overlapped = Overlapped;

    //
    // Since we are on a KTL thread, we cannot do a blocking operation
    // on this thread. So kick the work off to a new thread.
    // TODO: Use a more linux oriented approach such as epoll and IoSubmit.
    //

    //
    // Take refcount on the operation to keep it alive; Will be
    // released when thread completes
    //
    operation->AddRef();
    
    status = operation->StartThread();
    if (NT_SUCCESS(status))
    {
        status = STATUS_PENDING;
    } else {
        //
        // If thread didn't start then release the refcount taken above
        //
        KTraceFailedAsyncRequest(status, NULL, errno, (ULONGLONG)&FileHandle);
        operation->Release();
    }
    
    return(status);
}

NTSTATUS KXMWriteFileAsyncU(HANDLE DevFD, KXMFileHandle& FileHandle, void *UserAddress,
                        int NumPages, LPOVERLAPPED Overlapped)
{
    NTSTATUS status;
    KXMFileOperation::SPtr operation;
    
    operation = KXMFileOperation::Create();
    if (! operation)
    {
        KTraceFailedAsyncRequest(STATUS_INSUFFICIENT_RESOURCES, NULL, 0, (ULONGLONG)&FileHandle);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }
    
    operation->_Operation = KXMFileOperation::WriteFileAsync;

    operation->_DevFD = DevFD;
    operation->_FileHandle = &FileHandle;
    operation->_UserAddress = UserAddress;
    operation->_NumPages = NumPages;
    operation->_Overlapped = Overlapped;

    //
    // Since we are on a KTL thread, we cannot do a blocking operation
    // on this thread. So kick the work off to a new thread.
    // TODO: Use a more linux oriented approach such as epoll and IoSubmit.
    //
    
    //
    // Take refcount on the operation to keep it alive; Will be
    // released when thread completes
    //
    operation->AddRef();
        
    status = operation->StartThread();
    if (NT_SUCCESS(status))
    {
        status = STATUS_PENDING;
    } else {
        //
        // If thread didn't start then release the refcount taken above
        //
        KTraceFailedAsyncRequest(status, NULL, errno, (ULONGLONG)&FileHandle);
        operation->Release();
    }
    
    return(status);
}

NTSTATUS KXMReadFileScatterAsyncU(HANDLE DevFD, KXMFileHandle& FileHandle, 
                        KXMFileOffset Frag[], int NumEntries, 
                        LPOVERLAPPED Overlapped)
{
    NTSTATUS status;
    KXMFileOperation::SPtr operation;
    ULONG fragSize;

    operation = KXMFileOperation::Create();
    if (! operation)
    {
        KTraceFailedAsyncRequest(STATUS_INSUFFICIENT_RESOURCES, NULL, 0, (ULONGLONG)&FileHandle);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    operation->_Operation = KXMFileOperation::ReadFileScatterAsync;

    fragSize = NumEntries * sizeof(KXMFileOffset);
    memcpy(operation->_Frags, Frag, fragSize);
    operation->_DevFD = DevFD;
    operation->_FileHandle = &FileHandle;
    operation->_NumEntries = NumEntries;
    operation->_Overlapped = Overlapped;

    //
    // Since we are on a KTL thread, we cannot do a blocking operation
    // on this thread. So kick the work off to a new thread.
    // TODO: Use a more linux oriented approach such as epoll and IoSubmit.
    //
    
    //
    // Take refcount on the operation to keep it alive; Will be
    // released when thread completes
    //
    operation->AddRef();
    
    status = operation->StartThread();
    if (NT_SUCCESS(status))
    {
        status = STATUS_PENDING;
    } else {
        //
        // If thread didn't start then release the refcount taken above
        //
        KTraceFailedAsyncRequest(status, NULL, errno, (ULONGLONG)&FileHandle);
        operation->Release();
    }
    
    return(status);
}

NTSTATUS KXMWriteFileGatherAsyncU(HANDLE DevFD, KXMFileHandle& FileHandle, 
                        KXMFileOffset Frag[], int NumEntries, 
                        LPOVERLAPPED Overlapped)
{
    NTSTATUS status;
    KXMFileOperation::SPtr operation;
    ULONG fragSize;

    operation = KXMFileOperation::Create();
    if (! operation)
    {
        KTraceFailedAsyncRequest(STATUS_INSUFFICIENT_RESOURCES, NULL, 0, (ULONGLONG)&FileHandle);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    operation->_Operation = KXMFileOperation::WriteFileGatherAsync;

    fragSize = NumEntries * sizeof(KXMFileOffset);
    memcpy(operation->_Frags, Frag, fragSize);  
    operation->_DevFD = DevFD;
    operation->_FileHandle = &FileHandle;
    operation->_NumEntries = NumEntries;
    operation->_Overlapped = Overlapped;

    //
    // Since we are on a KTL thread, we cannot do a blocking operation
    // on this thread. So kick the work off to a new thread.
    // TODO: Use a more linux oriented approach such as epoll and IoSubmit.
    //
    
    //
    // Take refcount on the operation to keep it alive; Will be
    // released when thread completes
    //
    operation->AddRef();
    
    status = operation->StartThread();
    if (NT_SUCCESS(status))
    {
        status = STATUS_PENDING;
    } else {
        //
        // If thread didn't start then release the refcount taken above
        //
        KTraceFailedAsyncRequest(status, NULL, errno, (ULONGLONG)&FileHandle);
        operation->Release();
    }
    
    return(status);
}

NTSTATUS KXMFlushBuffersFileU(HANDLE DevFD, KXMFileHandle& FileHandle)
{
    NTSTATUS status;
    int ret;
    int fd;
    
    //
    // This is assumed to be invoked in a thread that can block
    //
    fd = (int)FileHandle.GetKxmHandle();
    if (! FileHandle.IsOpen())
    {
        status = STATUS_INVALID_HANDLE;
        KTraceFailedAsyncRequest(status, NULL, 0, (ULONGLONG)&FileHandle);
    } else {
        ret = fsync(fd);
        if (ret == -1)
        {
            status = LinuxError::LinuxErrorToNTStatus(errno);
            KTraceFailedAsyncRequest(status, NULL, errno, (ULONGLONG)&FileHandle);
        } else {
            status = STATUS_SUCCESS;
        }
    }
    
    return(status);
}

NTSTATUS KXMCloseFileU(HANDLE DevFD, KXMFileHandle& FileHandle)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (! FileHandle.IsOpen())
    {
        status = STATUS_INVALID_HANDLE;
        KTraceFailedAsyncRequest(status, NULL, 0, (ULONGLONG)&FileHandle);
    } else {
        BOOLEAN isDirectRead;
        isDirectRead = ! FileHandle.UseFileSystemOnly();
        if (isDirectRead)
        {
            int fd = (int)FileHandle.GetKxmHandle();
            close(fd);
        }
    }
    
    return(status);
}
