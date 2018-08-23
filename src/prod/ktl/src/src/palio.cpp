#include <unistd.h>
#include <uuid/uuid.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
// TODO fix https://bugzilla.redhat.com/show_bug.cgi?id=1476120
#if defined(LINUX_DISTRIB_REDHAT)
#include <linux/falloc.h>
#endif
#include <sys/syscall.h>
#include <errno.h>
#include <string.h>
#include <thread>
#include <aio.h>
#include <dirent.h>
#include <linux/fs.h>
#include <linux/fiemap.h>
#include <linux/unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>

#include <palio.h>
#include <KXMApi.h>
#include <ktlpal.h>
#include <paldef.h>
#include "palhandle.h"
#include "palerr.h"
#include <kinternal.h>

inline BOOLEAN IsInvalidHandleValue(HANDLE h)
{
    if ((h == NULL))
    {
        return(TRUE);
    } else {
        return(FALSE);
    }
}

#define FAIL_UNIMPLEMENTED(funcname) KInvariant(FALSE);

NTSTATUS LinuxError::LinuxErrorToNTStatus(int LinuxError)
{
    NTSTATUS status = STATUS_INTERNAL_ERROR;

    switch(LinuxError)
    {
        case ETXTBSY:
        case EROFS:
        case EPERM:
        case EACCES:
        {
            status = STATUS_ACCESS_DENIED;
            break;
        }

        case EDQUOT:
        {
            status = STATUS_QUOTA_EXCEEDED;
            break;
        }

        case EEXIST:
        {
            status = STATUS_OBJECT_NAME_COLLISION;
            break;
        }

        case EFAULT:
        {
            status = STATUS_OBJECT_PATH_INVALID;
            break;
        }

        case EISDIR:
        {
            status = STATUS_FILE_IS_A_DIRECTORY;
            break;
        }

        case ELOOP:
        {
            status = STATUS_REPARSE_POINT_NOT_RESOLVED;
            break;
        }

        case ENFILE:
        case EMFILE:
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        case ENAMETOOLONG:
        {
            status = STATUS_OBJECT_PATH_SYNTAX_BAD;
            break;
        }

        case ENXIO:
        case ENOENT:
        {
            status = STATUS_OBJECT_NAME_NOT_FOUND;
            break;
        }

        case ENOSPC:
        {
            status = STATUS_DISK_FULL;
            break;
        }
        
        case ENOMEM:
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        case ENOTDIR:
        {
            status = STATUS_NOT_A_DIRECTORY;
            break;
        }

        case ECONNREFUSED:
        {
            status = STATUS_PIPE_DISCONNECTED;
            break;
        }

        case EPIPE:
        case ECONNRESET:
        {
            status = STATUS_PIPE_BROKEN;
            break;
        }

        case EBADF:
        {
            status = STATUS_INVALID_HANDLE;
            break;
        }

        case EINVAL:
        {
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        
        case EIO:
        {
            status = STATUS_UNEXPECTED_IO_ERROR;
            break;
        }

        case ESPIPE:
        {
            status = STATUS_NONEXISTENT_SECTOR;
            break;
        }

        case  EILSEQ:
        {
            status = STATUS_REQUEST_OUT_OF_SEQUENCE;
            break;
        }
        
        default:
        {
            KTraceFailedAsyncRequest(status, NULL, LinuxError, 0);
            KAssert(FALSE);
        }
    }

    return status;
}

int LinuxError::NTStatusToLinuxError(NTSTATUS Status)
{
    int linuxerr = EIO;

    switch(Status)
    {
        case STATUS_FILE_CLOSED:
        {
            linuxerr = ESTALE;
            break;
        }

        case STATUS_TIMEOUT:
        {
            linuxerr = ETIMEDOUT;
            break;
        }

        case STATUS_INVALID_PARAMETER:
        {
            linuxerr = EINVAL;
            break;
        }

        case STATUS_INVALID_HANDLE:
        {
            linuxerr = EBADF;
            break;
        }

        case STATUS_NO_MEMORY:
        case STATUS_INSUFFICIENT_RESOURCES:
        {
            linuxerr = ENOMEM;
            break;
        }

        case STATUS_RESOURCE_IN_USE:
        {
            linuxerr = EAGAIN;
            break;
        }

        case STATUS_OPEN_FAILED:
        {
            linuxerr = ENOENT;
            break;
        }

        case STATUS_INVALID_DEVICE_REQUEST:
        {
            linuxerr = EIO;
            break;
        }

        case STATUS_ENCOUNTERED_WRITE_IN_PROGRESS:
        {
            linuxerr = EBUSY;
            break;
        }

        case STATUS_NOT_FOUND:
        {
            linuxerr = ENXIO;
            break;
        }       
        
        default:
        {
            KAssert(FALSE);
            break;
        }
    }
    return linuxerr;
}

DWORD LinuxError::LinuxErrorToWinError(int error)
{
    DWORD winerror = ERROR_GEN_FAILURE;

    switch(error)
    {
        case ETXTBSY:
        case EROFS:
        case EPERM:
        case EACCES:
        {
            winerror = ERROR_ACCESS_DENIED;
            break;
        }

        case EEXIST:
        {
            winerror = ERROR_ALREADY_EXISTS;
            break;
        }

        case EFAULT:
        {
            winerror = ERROR_BAD_PATHNAME;
            break;
        }

        case EISDIR:
        {
            winerror = ERROR_ACCESS_DENIED;
            break;
        }

        case ENFILE:
        case EMFILE:
        {
            winerror = ERROR_NO_SYSTEM_RESOURCES;
            break;
        }

        case ENAMETOOLONG:
        {
            winerror = ERROR_FILENAME_EXCED_RANGE;
            break;
        }

        case ENXIO:
        case ENOENT:
        {
            winerror = ERROR_FILE_NOT_FOUND;
            break;
        }

        case ENOMEM:
        case ENOSPC:
        {
            winerror = ERROR_NO_SYSTEM_RESOURCES;
            break;
        }

        case ENOTDIR:
        {
            winerror = ERROR_DIRECTORY;
            break;
        }

        case EPIPE:
        case ECONNRESET:
        {
            winerror = ERROR_BROKEN_PIPE;
            break;
        }

        case EBADF:
        {
            winerror = ERROR_INVALID_HANDLE;
            break;
        }

        case ESTALE:
        {
            winerror = ERROR_ABANDONED_WAIT_0;
            break;
        }

        case EINVAL:
        {
            winerror = ERROR_INVALID_PARAMETER;
            break;
        }

        case ETIMEDOUT:
        {
            winerror = ERROR_TIMEOUT;
            break;
        }

        case EIO:
        {
            winerror = ERROR_IO_DEVICE;
            break;
        }

        case EAGAIN:
        {
            winerror = ERROR_INTERNAL_ERROR;
            break;
        }

        case ESPIPE:
        {
            winerror = ERROR_SEEK;
        }
        
        default:
        {
            KAssert(FALSE);
        }
    }
    return winerror;
}

DWORD WinError::NTStatusToWinError(NTSTATUS Status)
{
    DWORD winerror = ERROR_GEN_FAILURE;

    switch (Status)
    {
        case STATUS_SUCCESS:
        {
            winerror = NO_ERROR;
            break;
        }

        case STATUS_TIMEOUT:
        {
            winerror = ERROR_TIMEOUT;
            break;
        }

        case STATUS_UNSUCCESSFUL:
        {
            winerror = ERROR_GEN_FAILURE;
            break;
        }

        case STATUS_NO_MEMORY:
        {
            winerror = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        case STATUS_INSUFFICIENT_RESOURCES:
        {
            winerror = ERROR_NO_SYSTEM_RESOURCES;
            break;
        }
        
        case STATUS_INVALID_DEVICE_REQUEST:
        {
            winerror = ERROR_INVALID_FUNCTION;
            break;
        }

        case STATUS_INVALID_PARAMETER:
        {
            winerror = ERROR_INVALID_PARAMETER;
            break;
        }

        case STATUS_CANCELLED:
        {
            winerror = ERROR_OPERATION_ABORTED;
            break;
        }

        case STATUS_RESOURCE_IN_USE:
        {
            winerror = ERROR_INTERNAL_ERROR;
            break;
        }

        case STATUS_INVALID_HANDLE:
        {
            winerror = ERROR_INVALID_HANDLE;
            break;
        }

        case STATUS_FILE_CLOSED:
        {
            winerror = ERROR_ABANDONED_WAIT_0;
            break;
        }

        case STATUS_OPEN_FAILED:
        {
            winerror = ERROR_NET_OPEN_FAILED;
            break;
        }

        case STATUS_ENCOUNTERED_WRITE_IN_PROGRESS:
        {
            winerror = ERROR_BUSY;
            break;
        }

        case STATUS_ACCESS_DENIED:
        {
            winerror = ERROR_ACCESS_DENIED ;
            break;
        }

        case STATUS_OBJECT_NAME_NOT_FOUND:
        {
            winerror = ERROR_FILE_NOT_FOUND;
            break;
        }

        case STATUS_NOT_FOUND:
        {
            winerror = ERROR_NOT_FOUND;
            break;
        }
        
        default:
        {
            KAssert(FALSE);
        }
    }
    return winerror;
}

namespace {

ULONG pageSize = sysconf(_SC_PAGESIZE);

const ULONG KTL_TAG_NTEH = 'hetN';
const ULONG KTL_TAG_NTAF = 'fatN';
const ULONG KTL_TAG_NTSF = 'fstN';
const ULONG KTL_TAG_NTDR = 'rdtN';

class ReadWriteHelpers
{
    public:

    static ssize_t ReadFile(int fd, void* buf, size_t count)
    {
        ssize_t numRead = 0;
        while (numRead < count) {
            ssize_t num = read(fd, buf, count);
            if (num == 0) {
                return numRead;
            }
            if (num < 0 && errno != EINTR) {
                return num;
            }
            numRead += num;
        }
        return numRead;
    }

    static ssize_t WriteFile(int fd, const void* buf, size_t count)
    {
        ssize_t numWrite = 0;
        while (numWrite < count) {
            ssize_t num = write(fd, buf, count);
            if (num == 0) {
                return numWrite;
            }
            else if (num < 0) {
                return num;
            }
            numWrite += num;
        }
        return numWrite;
    }

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

#if 0  // TODO: Delete me
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
                return num;
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
                return num;
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
#endif
    
};

}

//
// class NtHandle
//

NtHandle::NtHandle()
: _status(STATUS_SUCCESS)
{
}

NtHandle::~NtHandle()
{
}

//
// class NtEventHandle
//

void
NtEventHandle::SetEvent(union sigval val)
{
    NtEventHandle* event = (NtEventHandle*)(val.sival_ptr);
    event->SetEvent();
}

NtEventHandle::NtEventHandle(ACCESS_MASK        DesiredAccess,
                             POBJECT_ATTRIBUTES ObjectAttributes,
                             EVENT_TYPE         EventType,
                             BOOLEAN            InitialState)
: NtHandle()
, _Event(true, InitialState)
{
}

NtEventHandle::~NtEventHandle()
{
}

NTSTATUS
NtEventHandle::Close()
{
    return STATUS_SUCCESS;
}

NTSTATUS
NtEventHandle::WaitForSingleObject(BOOLEAN        Alertable,
                                   PLARGE_INTEGER Timeout)
{
    return _Event.WaitUntilSet(Timeout ? Timeout->QuadPart : KEvent::Infinite) ? STATUS_SUCCESS
                                                                               : STATUS_INTERNAL_ERROR; // Is this the right error ?
}

void
NtEventHandle::SetEvent()
{
    _Event.SetEvent();
}

//
// class NtFileHandle
//

NtFileHandle::NtFileHandle(ACCESS_MASK        DesiredAccess,
                           POBJECT_ATTRIBUTES ObjectAttributes,
                           PIO_STATUS_BLOCK   IoStatusBlock,
                           PLARGE_INTEGER     AllocationSize,
                           ULONG              FileAttributes,
                           ULONG              ShareAccess,
                           ULONG              CreateDisposition,
                           ULONG              CreateOptions,
                           PVOID              EaBuffer,
                           ULONG              EaLength,
                           ULONG              Flags
                          )
: NtHandle()
, _FileDescriptor(-1)
, _DeleteOnClose(false)
, _PathToFile(Utf16To8(ObjectAttributes->ObjectName->Buffer))
, _KXMDescriptor(INVALID_HANDLE_VALUE)
, _KXMFileDescriptor(INVALID_HANDLE_VALUE)
{
    SetConstructorStatus(Initialize(DesiredAccess,
                                    ObjectAttributes,
                                    IoStatusBlock,
                                    AllocationSize,
                                    FileAttributes,
                                    ShareAccess,
                                    CreateDisposition,
                                    CreateOptions,
                                    EaBuffer,
                                    EaLength,
                                    Flags));
}

NtFileHandle::~NtFileHandle()
{
    Close();
}


NTSTATUS
NtFileHandle::Close() {
    int ret = 0;

    if (_KXMFileDescriptor != INVALID_HANDLE_VALUE) {
        KXMCloseFile(_KXMDescriptor, _KXMFileDescriptor);
        _KXMFileDescriptor = INVALID_HANDLE_VALUE;
    }

    if (_FileDescriptor >= 0) {
        ret = close(_FileDescriptor);
        if (ret < 0) {
            return LinuxError::LinuxErrorToNTStatus(errno);
        }
        if (_DeleteOnClose) {
#if KTL_USER_MODE
            // TODO: Is this correct to use std::remove ? Why not remove()
            ret = std::remove(_PathToFile.c_str());   // TODO: Remove STL usage
#else
            // TODO: Kernel mode
#endif
            if (ret < 0) {
                return STATUS_INTERNAL_ERROR;         // TODO: Map to NTStatus
            }
        }
        _FileDescriptor = -1;
    }
    return STATUS_SUCCESS;
}

HANDLE
NtFileHandle::GetKXMFileDescriptor()
{
    return _KXMFileDescriptor;
}


NTSTATUS
NtFileHandle::QueryInformationFile(PIO_STATUS_BLOCK       IoStatusBlock,
                                   PVOID                  FileInformation,
                                   ULONG                  Length,
                                   FILE_INFORMATION_CLASS FileInformationClass)
{
    NTSTATUS status;
    
    KInvariant( (! UseFileSystemOnly()) || ( _FileDescriptor >= 0));
    KInvariant(FileInformationClass == FileStandardInformation);

    int fd = GetFileDescriptor();
    if (fd < 0) {
        status = LinuxError::LinuxErrorToNTStatus(errno);
        KTraceFailedAsyncRequest(status, NULL, errno, 0);
        return status;
    }
    KFinally([&]() {
        DoneWithFileDescriptor(fd);
    });
    
    struct stat buf;
    int ret = fstat(fd, &buf);
    if (ret < 0) {
        return LinuxError::LinuxErrorToNTStatus(errno);
    }

    FILE_STANDARD_INFORMATION *StandardInformation = (FILE_STANDARD_INFORMATION*) FileInformation;
    StandardInformation->AllocationSize.QuadPart = buf.st_blocks * NtEventHandle::fstatBlockSize;
    StandardInformation->EndOfFile.QuadPart = buf.st_size;
    StandardInformation->NumberOfLinks = buf.st_nlink;
    StandardInformation->DeletePending = false;
    StandardInformation->Directory = S_ISDIR(buf.st_mode);

    return STATUS_SUCCESS;
}

NTSTATUS
NtFileHandle::SetInformationFile(PIO_STATUS_BLOCK       IoStatusBlock,
                                 PVOID                  FileInformation,
                                 ULONG                  Length,
                                 FILE_INFORMATION_CLASS FileInformationClass)
{
    NTSTATUS status;
    
    KInvariant((FileInformationClass == FileEndOfFileInformation) ||
               (FileInformationClass == FileDispositionInformation) ||
               (FileInformationClass == FileAllocationInformation) ||
               (FileInformationClass == FileIoPriorityHintInformation));

    switch (FileInformationClass) {
      case FileDispositionInformation: {
        FILE_DISPOSITION_INFORMATION *DispositionInformation = (FILE_DISPOSITION_INFORMATION*) FileInformation;
        _DeleteOnClose = DispositionInformation->DeleteFile;
        break;
      }

      case FileEndOfFileInformation: {
        FILE_END_OF_FILE_INFORMATION *EndOfFileInformation = (FILE_END_OF_FILE_INFORMATION*) FileInformation;
        int fd = GetFileDescriptor();
        if (fd < 0) {
            status = LinuxError::LinuxErrorToNTStatus(errno);
            KTraceFailedAsyncRequest(status, NULL, errno, 0);
            return status;
        }
        KFinally([&]() {
            DoneWithFileDescriptor(fd);
        });
        
        int ret = ftruncate(fd, EndOfFileInformation->EndOfFile.QuadPart);
        if (ret < 0) {
            return LinuxError::LinuxErrorToNTStatus(errno);
        }
        break;
      }

      case FileAllocationInformation: {
        FILE_ALLOCATION_INFORMATION *AllocationInformation = (FILE_ALLOCATION_INFORMATION*) FileInformation;

        int fd = GetFileDescriptor();
        if (fd < 0) {
            status = LinuxError::LinuxErrorToNTStatus(errno);
            KTraceFailedAsyncRequest(status, NULL, errno, 0);
            return status;
        }
        KFinally([&]() {
            DoneWithFileDescriptor(fd);
        });
        
        int ret = fallocate(fd,
                            FALLOC_FL_KEEP_SIZE,
                            0,                   // TODO: Will this overwrite existing allocation
                            AllocationInformation->AllocationSize.QuadPart);
        if (ret < 0) {
            return LinuxError::LinuxErrorToNTStatus(errno);
        }

        break;
      }

      case FileIoPriorityHintInformation:
      {
          // TODO: Is there an equivalent for Linux ??
          return STATUS_NOT_SUPPORTED;
      }

    default:
        KInvariant(false);
        break;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NtFileHandle::FsControlFile(HANDLE           Event,
                            PIO_APC_ROUTINE  ApcRoutine,
                            PVOID            ApcContext,
                            PIO_STATUS_BLOCK IoStatusBlock,
                            ULONG            FsControlCode,
                            PVOID            InputBuffer,
                            ULONG            InputBufferLength,
                            PVOID            OutputBuffer,
                            ULONG            OutputBufferLength)
{
    NTSTATUS status;
    
    KInvariant((FsControlCode == FSCTL_SET_ZERO_DATA) ||
               (FsControlCode == FSCTL_QUERY_ALLOCATED_RANGES) ||
               (FsControlCode == FSCTL_SET_SPARSE));


    KFinally([&]() {
        if (Event)
        {
            ((NtEventHandle*)Event)->SetEvent();
        }
    });

    switch(FsControlCode)
    {
        case FSCTL_QUERY_ALLOCATED_RANGES:
        {
            FILE_ALLOCATED_RANGE_BUFFER* in = (FILE_ALLOCATED_RANGE_BUFFER*)InputBuffer;
            FILE_ALLOCATED_RANGE_BUFFER* out = (FILE_ALLOCATED_RANGE_BUFFER*)OutputBuffer;

            if ((in->FileOffset.QuadPart == 0) && (in->Length.QuadPart == 0))
            {
                IoStatusBlock->Information = 0;
                return STATUS_SUCCESS;
            }

            int fd = GetFileDescriptor();
            if (fd < 0) {
                status = LinuxError::LinuxErrorToNTStatus(errno);
                KTraceFailedAsyncRequest(status, NULL, errno, 0);
                return status;
            }
            KFinally([&]() {
                DoneWithFileDescriptor(fd);
            });
            
            //
            // CONSIDER: Read the extents more than one at a time
            //
            struct fiemap* map;
            struct fiemap_extent* extent;
            ULONG mapLength = sizeof(struct fiemap) + sizeof(struct fiemap_extent);
            UCHAR mapBuffer[mapLength];

            map = (struct fiemap*)mapBuffer;
            memset(map, 0, mapLength);

            map->fm_start = in->FileOffset.QuadPart;
            map->fm_length = in->Length.QuadPart;
            map->fm_flags = FIEMAP_FLAG_SYNC;
            map->fm_extent_count = 1;

            int ret = ioctl(fd, FS_IOC_FIEMAP, map);

            if (ret < 0) {
                return LinuxError::LinuxErrorToNTStatus(errno);
            }

            KInvariant((map->fm_mapped_extents == 0) || (map->fm_mapped_extents == 1));
            if (map->fm_mapped_extents == 1)
            {
                extent = (fiemap_extent*)&map->fm_extents[0];
                out->FileOffset.QuadPart = extent->fe_logical;
                out->Length.QuadPart = extent->fe_length;

                IoStatusBlock->Information = sizeof(FILE_ALLOCATED_RANGE_BUFFER);

                if (! (extent->fe_flags & FIEMAP_EXTENT_LAST))
                {
                    return(STATUS_BUFFER_OVERFLOW);
                }
            } else {
                IoStatusBlock->Information = 0;
                return STATUS_SUCCESS;
            }

            break;
        }

        case FSCTL_SET_ZERO_DATA:
        {
            FILE_ZERO_DATA_INFORMATION* zeroDataInfo = (FILE_ZERO_DATA_INFORMATION*) InputBuffer;
            
            int fd = GetFileDescriptor();
            if (fd < 0) {
                status = LinuxError::LinuxErrorToNTStatus(errno);
                KTraceFailedAsyncRequest(status, NULL, errno, 0);
                return status;
            }
            KFinally([&]() {
                DoneWithFileDescriptor(fd);
            });
            
            int ret = fallocate(fd,
                                FALLOC_FL_KEEP_SIZE | FALLOC_FL_PUNCH_HOLE,
                                zeroDataInfo->FileOffset.QuadPart,
                                zeroDataInfo->BeyondFinalZero.QuadPart - zeroDataInfo->FileOffset.QuadPart);
            if (ret < 0) {
                return LinuxError::LinuxErrorToNTStatus(errno);
            }
            break;
        }

        case FSCTL_SET_SPARSE:
        {
            // No op
            break;
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS NtFileHandle::AdjustExtentTable(
    __in ULONGLONG OldFileSize,
    __in ULONGLONG NewFileSize,
    __in KAllocator& Allocator
    )
{
    int ret, fd, prev, cur;
    size_t fiemap_size;
    struct fiemap query;
    struct fiemap* fiemap;
    struct fiemap_extent *extents = nullptr;

    NTSTATUS status;
    ULONGLONG numExtents;
    KBuffer::SPtr fiemapKBuffer;

    if (UseFileSystemOnly())
    {
        return(STATUS_SUCCESS);
    }   
    
    //
    // File creation, do nothing except NULL out extents
    //
    if (NewFileSize == 0) {
        _MappingLock.AcquireExclusive();
        _Fiemap = nullptr;
        _FiemapExtentCount = 0;
        _MappingLock.ReleaseExclusive();      
        return STATUS_SUCCESS;
    }

    //
    // Otherwise load the mappings
    //
    fd = GetFileDescriptor();
    if (fd < 0) {
        status = LinuxError::LinuxErrorToNTStatus(errno);
        KTraceFailedAsyncRequest(status, NULL, errno, 0);
        return status;
    }
    KFinally([&]() {
        DoneWithFileDescriptor(fd);
    });

    //
    // Figure out the number of new extents
    //
    query.fm_start = 0;
    query.fm_length = NewFileSize;
    query.fm_flags = FIEMAP_FLAG_SYNC;
    query.fm_extent_count = 0;

    ret = ioctl(fd, FS_IOC_FIEMAP, &query);

    if (ret != 0) {
        status = LinuxError::LinuxErrorToNTStatus(errno);
        KTraceFailedAsyncRequest(status, NULL, fd, errno);
        return status;
    }

    //
    // fm_mapped_extents should now tell us how much space we need
    //
    numExtents = query.fm_mapped_extents;
    fiemap_size = sizeof(struct fiemap) + (sizeof(struct fiemap_extent) * numExtents);

    status = KBuffer::Create(fiemap_size, fiemapKBuffer, Allocator);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, NULL, fd, fiemap_size);
        return status;
    }

    fiemap = (struct fiemap *)fiemapKBuffer->GetBuffer();
    
    memset(fiemap, 0, fiemap_size);
    fiemap->fm_start = 0;
    fiemap->fm_length = NewFileSize;
    fiemap->fm_flags = FIEMAP_FLAG_SYNC;
    fiemap->fm_extent_count = numExtents;

    //
    // Get the actual extents
    //
    ret = ioctl(fd, FS_IOC_FIEMAP, fiemap);

    if (ret != 0) {
        status = LinuxError::LinuxErrorToNTStatus(errno);
        KTraceFailedAsyncRequest(status, NULL, fd, errno);
        return status;
    }

    // Merge the extents in-place, should probably go in a helper function
    // Works on the assumption that the extents are sorted by physical
    // offset.
    ULONG invalidFiemapFlags = (FIEMAP_EXTENT_NOT_ALIGNED |
                                FIEMAP_EXTENT_DELALLOC |
                                FIEMAP_EXTENT_UNKNOWN);
    
    extents = fiemap->fm_extents;
    prev = 0;
    cur = 1;
    numExtents--;

    if ((extents[0].fe_flags & invalidFiemapFlags) != 0)
    {
        status = STATUS_UNSUCCESSFUL;
        KTraceFailedAsyncRequest(status, NULL, fd, extents[0].fe_flags);
        KDbgPrintf("InvalidFieMap File: %s", _PathToFile.c_str());
        KAssert(FALSE);
        return status;
    }
    
    while (numExtents) {
        if ((extents[cur].fe_flags & invalidFiemapFlags) != 0)
        {
            status = STATUS_UNSUCCESSFUL;
            KTraceFailedAsyncRequest(status, NULL, fd, extents[cur].fe_flags);
            KDbgPrintf("InvalidFieMap File: %s", _PathToFile.c_str());
            KAssert(FALSE);
            return status;
        }
                
        // If the current extent begins where the previous ended ...
        if (extents[cur].fe_physical == (extents[prev].fe_physical + extents[prev].fe_length)) {
            // ... update the length of the previous extent (effectively merging prev + cur)
            extents[prev].fe_length += extents[cur].fe_length;
        } else {
            // ... otherwise shift the current extent down as it's not adjacent
            prev++;
            if (prev != cur)
            {
                (void) memcpy(&extents[prev], &extents[cur], sizeof (struct fiemap_extent));
            }
        }

        cur++;
        numExtents--;
    }

    numExtents = prev + 1;
    fiemap->fm_extent_count = numExtents;
    fiemap->fm_mapped_extents = numExtents;
    
    //
    // Remember the mapping
    //
    _MappingLock.AcquireExclusive();
    _Fiemap = nullptr;
    _Fiemap = Ktl::Move(fiemapKBuffer);
    _FiemapExtentCount = numExtents;
    _MappingLock.ReleaseExclusive();
    return(STATUS_SUCCESS);
}


NTSTATUS NtFileHandle::SetValidDataLength(
    __in LONGLONG OldEndOfFileOffset,
    __in LONGLONG ValidDataLength,
    __in KAllocator& Allocator
    )
{
    NTSTATUS status;
    LONGLONG fileSize;

    fileSize = OldEndOfFileOffset;

    if (fileSize >= ValidDataLength)
    {
        //
        // Since there is no expansion of the file then there is no
        // new region of the file to zero out.
        //
        return(STATUS_SUCCESS);
    }

    //
    // Since the file may be accessed via KXM direct apis,
    // we want to zero out the new region of the file so that all
    // KXM reads are zero and the file system will record that data
    // has been written to all blocks and thus the file accessible
    // via the file system.
    //
    const ULONG oneMB = 1024 * 1024;
    LONGLONG newRangeSize = ValidDataLength - OldEndOfFileOffset;
    LONGLONG oneMBWritesLL = newRangeSize / oneMB;
    LONGLONG lastWriteSizeLL = newRangeSize - (oneMBWritesLL * oneMB);
    ULONG oneMBWrites = (ULONG)oneMBWritesLL;
    ULONG lastWriteSize = (ULONG)lastWriteSizeLL;
    KIoBuffer::SPtr oneMBKIoBuffer;
    PVOID p;
    PUCHAR oneMBBuffer;
    int vdlFD;

    //
    // Create a special FD that is only used for this purpose so we
    // can take advantage of the file system cache.
    //
    char *filePath = (char *)_PathToFile.c_str();
    vdlFD = open(filePath, O_RDWR | O_CLOEXEC);
    if (vdlFD < 0)
    {
        status = LinuxError::LinuxErrorToNTStatus(errno);
        KTraceFailedAsyncRequest(status, NULL, errno, 0);
        return(status);
    }

    KFinally([&]() {
        close(vdlFD);
    });

    status = KIoBuffer::CreateSimple(oneMB, oneMBKIoBuffer, p, Allocator);
    if ( ! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, NULL, oneMBKIoBuffer, 0);
        return(status);
    }
    oneMBBuffer = (PUCHAR)oneMBKIoBuffer->First()->GetBuffer();
    memset(oneMBBuffer, oneMB, 0);

    //
    // We assume we are not running on a KTL thread and so can do
    // blocking IO calls
    //
    ssize_t bytesWritten;
    off64_t offset;
    int ret;

    offset = OldEndOfFileOffset;

    for (ULONG i = 0; i < oneMBWrites; i++)
    {
        bytesWritten = ReadWriteHelpers::PWriteFile(vdlFD, oneMBBuffer, oneMB, offset);
        if (bytesWritten != oneMB)
        {
            status = LinuxError::LinuxErrorToNTStatus(errno);
            KTraceFailedAsyncRequest(status, NULL, errno, offset);
            KTraceFailedAsyncRequest(status, NULL, errno, ValidDataLength);
            return status;
        }

        offset += bytesWritten;
    }

    if (lastWriteSize > 0)
    {
        bytesWritten = ReadWriteHelpers::PWriteFile(vdlFD, oneMBBuffer, lastWriteSize, offset);
        if (bytesWritten != lastWriteSize)
        {
            status = LinuxError::LinuxErrorToNTStatus(errno);
            KTraceFailedAsyncRequest(status, NULL, errno, offset);
            KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)bytesWritten, ValidDataLength);
            return status;
        }
    }

    ret = fsync(vdlFD);
    if (ret < 0)
    {
        status = LinuxError::LinuxErrorToNTStatus(errno);
        KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)bytesWritten, ValidDataLength);
        return status;
    }
#if VERBOSE
    KDbgCheckpointWDataInformational(0, "SetVdl", STATUS_SUCCESS,
                        (ULONGLONG)_FileDescriptor,
                        (ULONGLONG)OldEndOfFileOffset,
                        (ULONGLONG)ValidDataLength,
                        (ULONGLONG)0);
#endif

    return(STATUS_SUCCESS);
}


NTSTATUS
NtFileHandle::Initialize(ACCESS_MASK        DesiredAccess,
                         POBJECT_ATTRIBUTES ObjectAttributes,
                         PIO_STATUS_BLOCK   IoStatusBlock,
                         PLARGE_INTEGER     AllocationSize,
                         ULONG              FileAttributes,
                         ULONG              ShareAccess,
                         ULONG              CreateDisposition,
                         ULONG              CreateOptions,
                         PVOID              EaBuffer,
                         ULONG              EaLength,
                         ULONG              Flags)
{
    NTSTATUS status;
    
    KInvariant(ObjectAttributes->Length >= sizeof(OBJECT_ATTRIBUTES));

    //
    // Map KBlockFile flags into their KXM equivalent
    //
    _Flags = Flags;
    
    int openFlags = 0;
    if ((DesiredAccess & FILE_READ_DATA)
     && (DesiredAccess & FILE_WRITE_DATA))
    {
        openFlags |= O_RDWR;
    }
    else if (DesiredAccess & FILE_READ_DATA) {
        openFlags |= O_RDONLY;
    }
    else if (DesiredAccess & FILE_WRITE_DATA) {
        openFlags |= O_WRONLY;
    }
    else if (DesiredAccess & DELETE) {
        // Do nothing.
    }
    else {
        // TODO: Return error code when all features have been ported.
        KInvariant(false);
    }

    if (CreateDisposition == FILE_SUPERSEDE) {
        // Remove the original file if it exists.
        int ret = remove(_PathToFile.c_str());
        if (ret < 0 && errno != ENOENT) {
            return LinuxError::LinuxErrorToNTStatus(errno);
        }
        openFlags |= O_CREAT;
    }
    else if (CreateDisposition == FILE_CREATE) {
        openFlags |= O_CREAT | O_EXCL;
    }
    else if (CreateDisposition == FILE_OPEN_IF) {
        openFlags |= O_CREAT;
    }
    else if ((CreateDisposition == FILE_OVERWRITE)
          || (CreateDisposition == FILE_OVERWRITE_IF))
    {
        openFlags |= O_CREAT | O_TRUNC;
    }

    // Although these are the semantics that KTL expects and are needed to correctly implement
    // the KTLLogger, the performance penalty is many orders of magnitude. Leave this disabled
    // for now until KBlockFile is ready for performance testing. At that point we will properly
    // evaluate the impact.
    //
    if (CreateOptions & FILE_NO_INTERMEDIATE_BUFFERING)
    {
        openFlags |= O_DIRECT;
    }
    if (CreateOptions & FILE_WRITE_THROUGH)
    {
        openFlags |= O_SYNC;
    }
    _CreateOptions = CreateOptions;
    _OpenFlags = openFlags & ~(O_CREAT | O_TRUNC | O_EXCL);

    char *filePath = (char *)_PathToFile.c_str();
    int fd = open(filePath, openFlags | O_CLOEXEC, S_IRWXU);

    if (fd < 0) {
        return LinuxError::LinuxErrorToNTStatus(errno);
    }

    _KXMDescriptor = TlsGetKxmHandle();

    ULONG kxmFlags = UseFileSystemOnly() ? KXMFLAG_USE_FILESYSTEM_ONLY : 0;

    HANDLE kxmDesc = KXMCreateFile(_KXMDescriptor,
                                   fd,
                                   _PathToFile.c_str(),
                                   NULL,
                                   FALSE,
                                   CreateOptions & FILE_WRITE_THROUGH ? BLOCK_FLUSH : BLOCK_NOFLUSH,
                                   0,
                                   kxmFlags);  // Flags

    if (kxmDesc != INVALID_HANDLE_VALUE) {
        _KXMFileDescriptor = kxmDesc;

        //
        // See if KXM needed to fall back from direct write to file
        // system only
        //
        if ((kxmFlags & KXMFLAG_USE_FILESYSTEM_ONLY) != 0)
        {
            _Flags |= NTCF_FLAG_USE_FILE_SYSTEM_ONLY;
        }

        if (! UseFileSystemOnly())
        {
            //
            // In the case where KXM direct writes are used, we do not need
            // to keep a file system file descriptor open
            //
            close(fd);
            fd = -1;
        } else {
            _FileDescriptor = fd;
        }       
        
        status = STATUS_SUCCESS;        
    } else {
        close(fd);
        status = STATUS_UNSUCCESSFUL;
        KTraceFailedAsyncRequest(status, NULL, errno, 0);       
    }

    _Fiemap = nullptr;
    
    return status;
}


//
// class NtSyncFileHandle
//

NtSyncFileHandle::NtSyncFileHandle(ACCESS_MASK        DesiredAccess,
                                   POBJECT_ATTRIBUTES ObjectAttributes,
                                   PIO_STATUS_BLOCK   IoStatusBlock,
                                   PLARGE_INTEGER     AllocationSize,
                                   ULONG              FileAttributes,
                                   ULONG              ShareAccess,
                                   ULONG              CreateDisposition,
                                   ULONG              CreateOptions,
                                   PVOID              EaBuffer,
                                   ULONG              EaLength,
                                   ULONG              Flags)
: NtFileHandle(DesiredAccess,
               ObjectAttributes,
               IoStatusBlock,
               AllocationSize,
               FileAttributes,
               ShareAccess,
               CreateDisposition,
               CreateOptions,
               EaBuffer,
               EaLength,           // Always use file system for sync apis
               Flags | NTCF_FLAG_USE_FILE_SYSTEM_ONLY)
{
}

NtSyncFileHandle::~NtSyncFileHandle()
{
}

NTSTATUS
NtSyncFileHandle::Read(HANDLE           Event,
                       PIO_APC_ROUTINE  ApcRoutine,
                       PVOID            ApcContext,
                       PIO_STATUS_BLOCK IoStatusBlock,
                       PVOID            Buffer,
                       ULONG            Length,
                       PLARGE_INTEGER   ByteOffset,
                       PULONG           Key)
{
    KInvariant(_FileDescriptor >= 0);
    KInvariant(UseFileSystemOnly());
    KInvariant(Event == nullptr);
    KInvariant(ApcRoutine == nullptr);
    KInvariant(ApcContext == nullptr);

    ssize_t num;

    if (ByteOffset != NULL)
    {
        num = ReadWriteHelpers::PReadFile(_FileDescriptor, Buffer, Length, ByteOffset->QuadPart);
    } else {
        num = ReadWriteHelpers::ReadFile(_FileDescriptor, Buffer, Length);
    }

    if (num < 0) {
        return LinuxError::LinuxErrorToNTStatus(errno);
    } else {
        IoStatusBlock->Information = num;
    }

    return STATUS_SUCCESS;
}
    

NTSTATUS
NtSyncFileHandle::Write(HANDLE           Event,
                        PIO_APC_ROUTINE  ApcRoutine,
                        PVOID            ApcContext,
                        PIO_STATUS_BLOCK IoStatusBlock,
                        PVOID            Buffer,
                        ULONG            Length,
                        PLARGE_INTEGER   ByteOffset,
                        PULONG           Key)
{
    KInvariant(_FileDescriptor >= 0);
    KInvariant(UseFileSystemOnly());
    KInvariant(ApcRoutine == nullptr);
    KInvariant(ApcContext == nullptr);

    ssize_t num;
    
    if (ByteOffset != NULL)
    {
        num = ReadWriteHelpers::PWriteFile(_FileDescriptor, Buffer, Length, ByteOffset->QuadPart);
    } else {
        num = ReadWriteHelpers::WriteFile(_FileDescriptor, Buffer, Length);
    }
    
    if (num < 0) {
        return LinuxError::LinuxErrorToNTStatus(errno);
    } else {
        IoStatusBlock->Information = num;
    }

    return STATUS_SUCCESS;}

NTSTATUS
NtSyncFileHandle::FlushBuffersFile(
    PIO_STATUS_BLOCK IoStatusBlock
    )
{
    KInvariant(FALSE);

    // TODO: Do I need to do this ??
    
    return STATUS_SUCCESS;
}


//
// class NtAsyncFileHandle
//

NtAsyncFileHandle::NtAsyncFileHandle(ACCESS_MASK        DesiredAccess,
                                     POBJECT_ATTRIBUTES ObjectAttributes,
                                     PIO_STATUS_BLOCK   IoStatusBlock,
                                     PLARGE_INTEGER     AllocationSize,
                                     ULONG              FileAttributes,
                                     ULONG              ShareAccess,
                                     ULONG              CreateDisposition,
                                     ULONG              CreateOptions,
                                     PVOID              EaBuffer,
                                     ULONG              EaLength,
                                     ULONG              Flags)
: NtFileHandle(DesiredAccess,
               ObjectAttributes,
               IoStatusBlock,
               AllocationSize,
               FileAttributes,
               ShareAccess,
               CreateDisposition,
               CreateOptions,
               EaBuffer,
               EaLength,
               Flags)
, _IoCompletionPort(nullptr)
, _CompletionKey(0)
{
}

NtAsyncFileHandle::~NtAsyncFileHandle()
{
}

NTSTATUS
NtAsyncFileHandle::Read(HANDLE           Event,
                        PIO_APC_ROUTINE  ApcRoutine,
                        PVOID            ApcContext,
                        PIO_STATUS_BLOCK IoStatusBlock,
                        PVOID            Buffer,
                        ULONG            Length,
                        PLARGE_INTEGER   ByteOffset,
                        PULONG           Key)
{
    FAIL_UNIMPLEMENTED(__func__);
}

NTSTATUS
NtAsyncFileHandle::Write(HANDLE           Event,
                         PIO_APC_ROUTINE  ApcRoutine,
                         PVOID            ApcContext,
                         PIO_STATUS_BLOCK IoStatusBlock,
                         PVOID            Buffer,
                         ULONG            Length,
                         PLARGE_INTEGER   ByteOffset,
                         PULONG           Key)
{
    FAIL_UNIMPLEMENTED(__func__);
}

BOOL
WINAPI
NtAsyncFileHandle::ReadFile(LPVOID       lpBuffer,
                            DWORD        nNumberOfBytesToRead,
                            LPDWORD      lpNumberBytesRead,
                            LPOVERLAPPED lpOverlapped)
{
    KInvariant(FALSE);
    *lpNumberBytesRead = 0;
    return FALSE;
}

BOOL
WINAPI
NtAsyncFileHandle::WriteFile(LPVOID       lpBuffer,
                             DWORD        nNumberOfBytesToWrite,
                             LPDWORD      lpNumberBytesWritten,
                             LPOVERLAPPED lpOverlapped)
{
    KInvariant(FALSE);
    *lpNumberBytesWritten = 0;
    return FALSE;
}

NTSTATUS NtFileHandle::MapLogicalToPhysical(
    off64_t Logical,
    size_t Length,
    off64_t& Physical,
    size_t& PhysicalLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    off64_t physical;
    struct fiemap* map; 

    KInvariant(_Fiemap != nullptr);
    
    _MappingLock.AcquireShared();
    KFinally([&]() {
        _MappingLock.ReleaseShared();
    });

    map = (struct fiemap*)_Fiemap->GetBuffer();
    
    for (int i = 0; i < _FiemapExtentCount; i++)
    {
        off64_t LogicalStart = map->fm_extents[i].fe_logical;
        off64_t LogicalEnd = LogicalStart + map->fm_extents[i].fe_length;
        if ((Logical >= LogicalStart) && (Logical < LogicalEnd))
        {
            off64_t offset = Logical - LogicalStart;
            Physical = (map->fm_extents[i].fe_physical + offset);
            if ((Logical + Length) <= LogicalEnd)
            {
                PhysicalLength =  Length;
            } else {
                PhysicalLength =  LogicalEnd - Logical;
            }
            return(status);
        }
    }

    //
    // We did not find the mapping, which is unfortunate
    //
    status = STATUS_INVALID_PARAMETER;
    KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)Logical, Length);
    KAssert(FALSE);
    return(status);
}

NTSTATUS
NtFileHandle::BuildLogicalToPhysicalMapping(
    __in ULONG NumberAlignedBuffers,
    __in HLPalFunctions::ALIGNED_BUFFER* AlignedBuffers,
    __in LPOVERLAPPED lpOverlapped,
    __in ULONG FragsCount,  
    __out KXMFileOffset *Frags,
    __out ULONG& UsedFragsCount,
    __in ULONG PageSize
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    unsigned long long offset, userp;
    off64_t physical;
    size_t physicalLength;
    size_t remaining;
    ULONG fragIdx;

    offset = (((unsigned long long)lpOverlapped->OffsetHigh) << 32) |
              ((unsigned long long)lpOverlapped->Offset & 0xffffffffU);

    fragIdx = 0;
    for (int i = 0; i < NumberAlignedBuffers; i++) {
        remaining = AlignedBuffers[i].npages * PageSize;
        userp = (unsigned long long) AlignedBuffers[i].buf;

        while (remaining) {             
            status = MapLogicalToPhysical(offset, remaining, physical, physicalLength);
            if (! NT_SUCCESS(status))
            {
                return(status);
            }

            remaining -= physicalLength;

            Frags[fragIdx].UserAddress = userp;
            Frags[fragIdx].PhysicalAddress = (unsigned long long)physical;
            Frags[fragIdx].NumPages = physicalLength / PageSize;

            offset += physicalLength;
            userp += physicalLength;
            fragIdx++;

            if (fragIdx >= FragsCount)
            {
                status = STATUS_NO_MORE_ENTRIES;
                KTraceFailedAsyncRequest(status, NULL, fragIdx, FragsCount);
                KAssert(FALSE);
                return(status);
            }
        }
    }

    UsedFragsCount = fragIdx;
    return(status);
}

BOOL
NtAsyncFileHandle::ReadFileScatterPal(
    __in ULONG NumberAlignedBuffers,
    __in HLPalFunctions::ALIGNED_BUFFER* AlignedBuffers,
    __in DWORD nNumberOfBytesToRead,
    __in LPOVERLAPPED lpOverlapped,
    __in ULONG PageSize
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOL b;
    KXMFileOffset *frags;
    ULONG fragIdx;
    ULONG workBufferUsed;
    PUCHAR workBuffer;

    workBufferUsed = (KXM_MAX_FILE_SG + 1) * sizeof(KXMFileOffset);
    workBuffer = _new(KTL_TAG_WORKBUFFERS, KtlSystemCoreImp::TryGetDefaultKtlSystemCore()->PagedAllocator())
                        UCHAR[workBufferUsed];
    if (workBuffer == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        lpOverlapped->Internal = status;
        KTraceFailedAsyncRequest(status, NULL, 0, 0);
        return(FALSE);
    }
    KFinally([&]() {
        _delete(workBuffer);
    });
        
    frags = (KXMFileOffset *)workBuffer;
    
    if (UseFileSystemOnly())
    {
        KInvariant(NumberAlignedBuffers <= KXM_MAX_FILE_SG);
        
        //
        // File system IO uses regular old logical file offsets
        //
        for (ULONG i = 0; i < NumberAlignedBuffers; i++)
        {
            frags[i].UserAddress = (unsigned long long) AlignedBuffers[i].buf;
            frags[i].NumPages = AlignedBuffers[i].npages;
            frags[i].PhysicalAddress = 0;
        }
        
        fragIdx = NumberAlignedBuffers;

    } else {
        //
        // Non file system IO uses direct block access and so we need
        // to perform mappings from file offsets to disk block offsets
        //
        
        status = BuildLogicalToPhysicalMapping(NumberAlignedBuffers,
                                               AlignedBuffers,
                                               lpOverlapped,
                                               KXM_MAX_FILE_SG + 1,
                                               frags,
                                               fragIdx,
                                               PageSize);

        if (! NT_SUCCESS(status))
        {       
            SetLastError(WinError::NTStatusToWinError(status));
            lpOverlapped->Internal = status;
            return(FALSE);
        }

        workBufferUsed = fragIdx * sizeof(KXMFileOffset);
    }
    
    status = KXMReadFileScatterAsync(_KXMDescriptor,
                                     _KXMFileDescriptor,
                                     frags,
                                     fragIdx,
                                     lpOverlapped);

    if (status == STATUS_PENDING)
    {
        b = FALSE;
        SetLastError(ERROR_IO_PENDING);
    } else if (! NT_SUCCESS(status)) {
        b = FALSE;
        SetLastError(WinError::NTStatusToWinError(status));
        lpOverlapped->Internal = status;
        KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)0, 0);
    } else {
        //
        // Successful synchronous completions are not expected
        //
        b = TRUE;
        KInvariant(FALSE);
    }
    
    return b;
}

BOOL
NtAsyncFileHandle::WriteFileGatherPal(
    __in ULONG NumberAlignedBuffers,
    __in HLPalFunctions::ALIGNED_BUFFER* AlignedBuffers,
    __in DWORD   nNumberOfBytesToWrite,
    __in LPOVERLAPPED lpOverlapped,
    __in ULONG PageSize
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOL b;
    KXMFileOffset *frags;
    ULONG fragIdx;
    ULONG workBufferUsed;
    PUCHAR workBuffer;

    workBufferUsed = (KXM_MAX_FILE_SG + 1) * sizeof(KXMFileOffset);
    workBuffer = _new(KTL_TAG_WORKBUFFERS, KtlSystemCoreImp::TryGetDefaultKtlSystemCore()->PagedAllocator())
                        UCHAR[workBufferUsed];
    if (workBuffer == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        lpOverlapped->Internal = status;
        KTraceFailedAsyncRequest(status, NULL, 0, 0);
        return(FALSE);
    }
    KFinally([&]() {
        _delete(workBuffer);
    });
        
    frags = (KXMFileOffset *)workBuffer;

    if (UseFileSystemOnly())
    {        
        KInvariant(NumberAlignedBuffers <= KXM_MAX_FILE_SG);
        
        //
        // File system IO uses regular old logical file offsets
        //
        for (ULONG i = 0; i < NumberAlignedBuffers; i++)
        {
            frags[i].UserAddress = (unsigned long long) AlignedBuffers[i].buf;
            frags[i].NumPages = AlignedBuffers[i].npages;
            frags[i].PhysicalAddress = 0;
        }
        
        fragIdx = NumberAlignedBuffers;
    } else {
        //
        // Non file system IO uses direct block access and so we need
        // to perform mappings from file offsets to disk block offsets
        //
        status = BuildLogicalToPhysicalMapping(NumberAlignedBuffers,
                                               AlignedBuffers,
                                               lpOverlapped,
                                               KXM_MAX_FILE_SG+1,
                                               frags,
                                               fragIdx,
                                               PageSize);

        if (! NT_SUCCESS(status))
        {       
            SetLastError(WinError::NTStatusToWinError(status));
            lpOverlapped->Internal = status;
            return(FALSE);
        }       
    }

    status = KXMWriteFileGatherAsync(_KXMDescriptor,
                                     _KXMFileDescriptor,
                                     frags,
                                     fragIdx,
                                     lpOverlapped);

    if (status == STATUS_PENDING)
    {
        b = FALSE;
        SetLastError(ERROR_IO_PENDING);
    } else if (! NT_SUCCESS(status)) {
        b = FALSE;
        SetLastError(WinError::NTStatusToWinError(status));
        lpOverlapped->Internal = status;
        KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)0, 0);
    } else {
        //
        // Synchronous completions are not expected
        //
        b = TRUE;
        KInvariant(FALSE);
    }
    
    return b;
}

NTSTATUS
NtAsyncFileHandle::SetUseFileSystemOnlyFlag(
    __in BOOLEAN UseFileSystemOnlyFlag
    )
{
    NTSTATUS status;
    int fd;

    //
    // Only allow switching to file system only
    //
    KInvariant(UseFileSystemOnlyFlag);

    if (UseFileSystemOnly())
    {
        return(STATUS_SUCCESS);
    }
    
    //
    // In order to switch between sparse/non sparse, we need to reopen
    // the KXM file descriptor
    //

    HANDLE completionPort;
    ULONG_PTR completionKey;
    ULONG maxConcurrentThreads; 
    KXMGetCompletionPortInfo(_KXMFileDescriptor, completionPort, completionKey, maxConcurrentThreads);
    
    status = KXMCloseFile(_KXMDescriptor, _KXMFileDescriptor);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)_KXMFileDescriptor, 0);
        return(status);
    }
    
    _KXMFileDescriptor = INVALID_HANDLE_VALUE;

    ULONG kxmFlags = UseFileSystemOnlyFlag ? KXMFLAG_USE_FILESYSTEM_ONLY : 0;

    if (UseFileSystemOnlyFlag)
    {
        char *filePath = (char *)_PathToFile.c_str();
        fd = open(filePath, _OpenFlags | O_CLOEXEC, S_IRWXU);
        if (fd < 0)
        {
            status = LinuxError::LinuxErrorToNTStatus(errno);
            KTraceFailedAsyncRequest(status, NULL, errno, 0);
            return(status);
        }
        _FileDescriptor = fd;
    } else {
        close(_FileDescriptor);
        _FileDescriptor = -1;
        fd = -1;
    }

    HANDLE kxmDesc = KXMCreateFile(_KXMDescriptor,
                                   fd,
                                   _PathToFile.c_str(),
                                   NULL,
                                   FALSE,
                                   _CreateOptions & FILE_WRITE_THROUGH ? BLOCK_FLUSH : BLOCK_NOFLUSH,
                                   0,
                                   kxmFlags);  // Flags

    if (kxmDesc != INVALID_HANDLE_VALUE) {
        _KXMFileDescriptor = kxmDesc;
    } else {
        status = STATUS_UNSUCCESSFUL;
        KTraceFailedAsyncRequest(status, NULL, GetLastError(), 0);
        return(status);
    }   

    HANDLE cHandle = KXMCreateIoCompletionPort(_KXMDescriptor, kxmDesc, completionPort, completionKey, maxConcurrentThreads);
    if (cHandle == NULL)
    {
        status = STATUS_UNSUCCESSFUL;
        KTraceFailedAsyncRequest(status, NULL, GetLastError(), 0);
        return(status);
    }

    if (UseFileSystemOnlyFlag)
    {
        _Flags &= ~KXMFLAG_AVOID_FILESYSTEM;
        _Flags |= KXMFLAG_USE_FILESYSTEM_ONLY;
    } else {
        _Flags &= ~KXMFLAG_USE_FILESYSTEM_ONLY;
    }
    
    return(STATUS_SUCCESS);
}

NTSTATUS
NtAsyncFileHandle::FlushBuffersFile(
    PIO_STATUS_BLOCK IoStatusBlock
    )
{
    NTSTATUS status;

    status = KXMFlushBuffersFile(_KXMDescriptor,
                                 _KXMFileDescriptor);

    IoStatusBlock->Status = status;
    return(status);
}


//
// class NtDirFileHandle
//

NtDirHandle::NtDirHandle(ACCESS_MASK        DesiredAccess,
                         POBJECT_ATTRIBUTES ObjectAttributes,
                         PIO_STATUS_BLOCK   IoStatusBlock,
                         PLARGE_INTEGER     AllocationSize,
                         ULONG              FileAttributes,
                         ULONG              ShareAccess,
                         ULONG              CreateDisposition,
                         ULONG              CreateOptions,
                         PVOID              EaBuffer,
                         ULONG              EaLength)
: NtHandle()
, _Dir(nullptr)
{
    SetConstructorStatus(Initialize(DesiredAccess,
                                    ObjectAttributes,
                                    IoStatusBlock,
                                    AllocationSize,
                                    FileAttributes,
                                    ShareAccess,
                                    CreateDisposition,
                                    CreateOptions,
                                    EaBuffer,
                                    EaLength));
}

NtDirHandle::~NtDirHandle()
{
    Close();
}

NTSTATUS
NtDirHandle::Close()
{
    if (_Dir) {
        int ret = closedir(_Dir);
        if (ret != 0) {
            return LinuxError::LinuxErrorToNTStatus(errno);
        }
    }
    _Dir = nullptr;
    return STATUS_SUCCESS;
}

NTSTATUS
NtDirHandle::QueryDirectoryFile(HANDLE                 Event,
                                PIO_APC_ROUTINE        ApcRoutine,
                                PVOID                  ApcContext,
                                PIO_STATUS_BLOCK       IoStatusBlock,
                                PVOID                  FileInformation,
                                ULONG                  Length,
                                FILE_INFORMATION_CLASS FileInformationClass,
                                BOOLEAN                ReturnSingleEntry,
                                PUNICODE_STRING        FileName,
                                BOOLEAN                RestartScan)
{
    NTSTATUS status;
    
    KInvariant(_Dir != nullptr);
    KInvariant(Event == nullptr);
    KInvariant(ApcRoutine == nullptr);
    KInvariant(ApcContext == nullptr);
    KInvariant(FileName == nullptr);

    KInvariant(FileInformationClass == FileDirectoryInformation);
    errno = 0;
    struct dirent* entry = readdir(_Dir);
    if (!entry) {
        if (errno == 0)
        {
            status = STATUS_NO_MORE_FILES;
        } else {
            status = LinuxError::LinuxErrorToNTStatus(errno);
        }
        return(status);
    }

    PFILE_DIRECTORY_INFORMATION dirInfo = (PFILE_DIRECTORY_INFORMATION) FileInformation;

    switch (entry->d_type) {
      case DT_DIR: {
        dirInfo->FileAttributes = FILE_ATTRIBUTE_DIRECTORY;
      } break;
      case DT_REG: {
        dirInfo->FileAttributes = FILE_ATTRIBUTE_NORMAL;
      } break;
      default: KInvariant(false); break;
    }

    // TODO: Remove STL usage
    std::wstring fileName = Utf8To16(entry->d_name);
    dirInfo->FileNameLength = fileName.length() * sizeof(wchar_t);
    memcpy(dirInfo->FileName, fileName.c_str(), fileName.length() * sizeof(wchar_t));

    return STATUS_SUCCESS;
}

NTSTATUS
NtDirHandle::Initialize(ACCESS_MASK        DesiredAccess,
                        POBJECT_ATTRIBUTES ObjectAttributes,
                        PIO_STATUS_BLOCK   IoStatusBlock,
                        PLARGE_INTEGER     AllocationSize,
                        ULONG              FileAttributes,
                        ULONG              ShareAccess,
                        ULONG              CreateDisposition,
                        ULONG              CreateOptions,
                        PVOID              EaBuffer,
                        ULONG              EaLength)
{
    KInvariant(ObjectAttributes->Length >= sizeof(OBJECT_ATTRIBUTES));
    KInvariant(ObjectAttributes->ObjectName);
    // TODO: Remove STL usage
    std::string pathname = Utf16To8(ObjectAttributes->ObjectName->Buffer);

    DIR *dir = nullptr;
    mode_t mode = S_IFDIR | S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH;
    switch (CreateDisposition)
    {
      case FILE_CREATE:
      {
          // Create directory
          int ret = mkdir(pathname.c_str(), mode);
          if (ret != 0) {
              if (errno == EEXIST) {
                  return STATUS_OBJECT_NAME_COLLISION;
              }
              return LinuxError::LinuxErrorToNTStatus(errno);
          }

          // Open directory
          dir = opendir(pathname.c_str());
          if (!dir) {
              return LinuxError::LinuxErrorToNTStatus(errno);
          }
          break;
      }

      case FILE_OPEN:
      {
          // Open directory
          dir = opendir(pathname.c_str());
          if (!dir) {
              if (errno == ENOENT) {
                  return STATUS_NOT_A_DIRECTORY;
              }
              return LinuxError::LinuxErrorToNTStatus(errno);
          }
          break;
      }

      default: ASSERT(FALSE);
    }

    _Dir = dir;

    return STATUS_SUCCESS;
}

NTSTATUS
NtCreateFileForKBlockFile(PHANDLE            FileHandle,
             ACCESS_MASK        DesiredAccess,
             POBJECT_ATTRIBUTES ObjectAttributes,
             PIO_STATUS_BLOCK   IoStatusBlock,
             PLARGE_INTEGER     AllocationSize,
             ULONG              FileAttributes,
             ULONG              ShareAccess,
             ULONG              CreateDisposition,
             ULONG              CreateOptions,
             PVOID              EaBuffer,
             ULONG              EaLength,
             ULONG              Flags)
{
    NtHandle *handle = nullptr;
    
    if (CreateOptions & FILE_DIRECTORY_FILE) {
        //
        // these are used for directory type operations from
        // KVolumeNamspace.cpp
        //
        handle = new NtDirHandle(DesiredAccess,
                                 ObjectAttributes,
                                 IoStatusBlock,
                                 AllocationSize,
                                 FileAttributes,
                                 ShareAccess,
                                 CreateDisposition,
                                 CreateOptions,
                                 EaBuffer,
                                 EaLength);
    }
    else if (DesiredAccess & SYNCHRONIZE) {
        //
        // these are used for volume type operations from
        // KVolumeNamspace.cpp
        //
        handle = new NtSyncFileHandle(DesiredAccess,
                                      ObjectAttributes,
                                      IoStatusBlock,
                                      AllocationSize,
                                      FileAttributes,
                                      ShareAccess,
                                      CreateDisposition,
                                      CreateOptions,
                                      EaBuffer,
                                      EaLength,
                                      Flags);
    }
    else {
        //
        // These are typical file type operations and so this gets
        // passed onward to KXM
        //              
        handle = new NtAsyncFileHandle(DesiredAccess,
                                       ObjectAttributes,
                                       IoStatusBlock,
                                       AllocationSize,
                                       FileAttributes,
                                       ShareAccess,
                                       CreateDisposition,
                                       CreateOptions,
                                       EaBuffer,
                                       EaLength,
                                       Flags);
    }

    NTSTATUS ret = handle->Status();
    if (NT_SUCCESS(ret)) {
        *FileHandle = handle;
    }
    else {
        delete handle;
    }
    return ret;
}

NTSTATUS
NtCreateFile(PHANDLE            FileHandle,
             ACCESS_MASK        DesiredAccess,
             POBJECT_ATTRIBUTES ObjectAttributes,
             PIO_STATUS_BLOCK   IoStatusBlock,
             PLARGE_INTEGER     AllocationSize,
             ULONG              FileAttributes,
             ULONG              ShareAccess,
             ULONG              CreateDisposition,
             ULONG              CreateOptions,
             PVOID              EaBuffer,
             ULONG              EaLength)
{
    return NtCreateFileForKBlockFile(FileHandle,
                              DesiredAccess,
                              ObjectAttributes,
                              IoStatusBlock,
                              AllocationSize,
                              FileAttributes,
                              ShareAccess,
                              CreateDisposition,
                              CreateOptions,
                              EaBuffer,
                              EaLength,
                              TRUE);
                              
}

NTSTATUS
NtDeleteFile(POBJECT_ATTRIBUTES ObjectAttributes)
{
    KInvariant(ObjectAttributes->Length >= sizeof(OBJECT_ATTRIBUTES));
    KInvariant(ObjectAttributes->ObjectName);
    // TODO: Remove STL usage
    std::string pathname = Utf16To8(ObjectAttributes->ObjectName->Buffer);

    return remove(pathname.c_str()) == 0 ? STATUS_SUCCESS : LinuxError::LinuxErrorToNTStatus(errno);
}

NTSTATUS
WINAPI
NtClose(HANDLE Handle)
{
    NtHandle* ntHandle = (NtHandle*) Handle;
    NTSTATUS ret = ntHandle->Close();
    delete ntHandle;
    return ret;
}

NTSTATUS
NtOpenFile(PHANDLE            FileHandle,
           ACCESS_MASK        DesiredAccess,
           POBJECT_ATTRIBUTES ObjectAttributes,
           PIO_STATUS_BLOCK   IoStatusBlock,
           ULONG              ShareAccess,
           ULONG              OpenOptions)
{
    return NtCreateFile(FileHandle,
                        DesiredAccess,
                        ObjectAttributes,
                        IoStatusBlock,
                        0,
                        0,
                        ShareAccess,
                        FILE_OPEN,
                        OpenOptions,
                        NULL,
                        0);
}

NTSTATUS
NtQueryInformationFile(HANDLE                 FileHandle,
                       PIO_STATUS_BLOCK       IoStatusBlock,
                       PVOID                  FileInformation,
                       ULONG                  Length,
                       FILE_INFORMATION_CLASS FileInformationClass)
{
    NtFileHandle* handle = static_cast<NtFileHandle*>(FileHandle);
    return handle->QueryInformationFile(IoStatusBlock,
                                        FileInformation,
                                        Length,
                                        FileInformationClass);
}

NTSTATUS
NtQueryFullAttributesFile(POBJECT_ATTRIBUTES             ObjectAttributes,
                          PFILE_NETWORK_OPEN_INFORMATION FileInformation)
{
    KInvariant(ObjectAttributes->Length >= sizeof(OBJECT_ATTRIBUTES));
    KInvariant(ObjectAttributes->ObjectName);
    // TODO: Remove STL usage
    std::string pathname = Utf16To8(ObjectAttributes->ObjectName->Buffer);

    struct stat buf;
    int ret = stat(pathname.c_str(), &buf);
    if (ret < 0) {
        if (errno == ENOENT) {
            return STATUS_OBJECT_NAME_NOT_FOUND;
        }
        return LinuxError::LinuxErrorToNTStatus(errno);
    }

    auto TimeConverter = [] (const struct timespec& t) -> LONGLONG {
        return t.tv_sec * 1e9 + t.tv_nsec;
    };

    FileInformation->CreationTime.QuadPart = 0;
    FileInformation->LastAccessTime.QuadPart = TimeConverter(buf.st_atim);
    FileInformation->LastWriteTime.QuadPart = TimeConverter(buf.st_mtim);
    FileInformation->ChangeTime.QuadPart = TimeConverter(buf.st_ctim);
    FileInformation->AllocationSize.QuadPart = buf.st_blocks * 512;
    FileInformation->EndOfFile.QuadPart = buf.st_size;

    if (S_ISDIR(buf.st_mode)) {
        FileInformation->FileAttributes = FILE_ATTRIBUTE_DIRECTORY;
    }
    else {
        FileInformation->FileAttributes = FILE_ATTRIBUTE_NORMAL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NtSetInformationFile(HANDLE                 FileHandle,
                     PIO_STATUS_BLOCK       IoStatusBlock,
                     PVOID                  FileInformation,
                     ULONG                  Length,
                     FILE_INFORMATION_CLASS FileInformationClass)
{
    NtFileHandle* handle = static_cast<NtFileHandle*>(FileHandle);
    return handle->SetInformationFile(IoStatusBlock,
                                      FileInformation,
                                      Length,
                                      FileInformationClass);
}

NTSTATUS
NtFsControlFile(HANDLE           FileHandle,
                HANDLE           Event,
                PIO_APC_ROUTINE  ApcRoutine,
                PVOID            ApcContext,
                PIO_STATUS_BLOCK IoStatusBlock,
                ULONG            FsControlCode,
                PVOID            InputBuffer,
                ULONG            InputBufferLength,
                PVOID            OutputBuffer,
                ULONG            OutputBufferLength)
{
    NtFileHandle* handle = static_cast<NtFileHandle*>(FileHandle);
    return handle->FsControlFile(Event,
                                 ApcRoutine,
                                 ApcContext,
                                 IoStatusBlock,
                                 FsControlCode,
                                 InputBuffer,
                                 InputBufferLength,
                                 OutputBuffer,
                                 OutputBufferLength);
}

NTSTATUS
WINAPI
NtDeviceIoControlFile(HANDLE           FileHandle,
                      HANDLE           Event,
                      PIO_APC_ROUTINE  ApcRoutine,
                      PVOID            ApcContext,
                      PIO_STATUS_BLOCK IoStatusBlock,
                      ULONG            IoControlCode,
                      PVOID            InputBuffer,
                      ULONG            InputBufferLength,
                      PVOID            OutputBuffer,
                      ULONG            OutputBufferLength)
{
    FAIL_UNIMPLEMENTED(__func__);
}

NTSTATUS
NtWriteFile(HANDLE           FileHandle,
            HANDLE           Event,
            PIO_APC_ROUTINE  ApcRoutine,
            PVOID            ApcContext,
            PIO_STATUS_BLOCK IoStatusBlock,
            PVOID            Buffer,
            ULONG            Length,
            PLARGE_INTEGER   ByteOffset,
            PULONG           Key)
{
    NtFileHandle *handle = static_cast<NtFileHandle*>(FileHandle);
    return handle->Write(Event,
                         ApcRoutine,
                         ApcContext,
                         IoStatusBlock,
                         Buffer,
                         Length,
                         ByteOffset,
                         Key);
}

NTSTATUS
NtReadFile(HANDLE           FileHandle,
           HANDLE           Event,
           PIO_APC_ROUTINE  ApcRoutine,
           PVOID            ApcContext,
           PIO_STATUS_BLOCK IoStatusBlock,
           PVOID            Buffer,
           ULONG            Length,
           PLARGE_INTEGER   ByteOffset,
           PULONG           Key)
{
    NtFileHandle *handle = static_cast<NtFileHandle*>(FileHandle);
    return handle->Read(Event,
                        ApcRoutine,
                        ApcContext,
                        IoStatusBlock,
                        Buffer,
                        Length,
                        ByteOffset,
                        Key);
}

NTSTATUS
NtFlushBuffersFile(HANDLE           FileHandle,
                   PIO_STATUS_BLOCK IoStatusBlock)
{
    NtFileHandle *handle = static_cast<NtFileHandle*>(FileHandle);
    return handle->FlushBuffersFile(IoStatusBlock);
}


NTSTATUS
NtWaitForSingleObject(HANDLE         Handle,
                      BOOLEAN        Alertable,
                      PLARGE_INTEGER Timeout)
{
    NtEventHandle* eventHandle = static_cast<NtEventHandle*>(Handle);
    return eventHandle->WaitForSingleObject(Alertable, Timeout);
}

NTSTATUS
NtQueryDirectoryFile(HANDLE                 FileHandle,
                     HANDLE                 Event,
                     PIO_APC_ROUTINE        ApcRoutine,
                     PVOID                  ApcContext,
                     PIO_STATUS_BLOCK       IoStatusBlock,
                     PVOID                  FileInformation,
                     ULONG                  Length,
                     FILE_INFORMATION_CLASS FileInformationClass,
                     BOOLEAN                ReturnSingleEntry,
                     PUNICODE_STRING        FileName,
                     BOOLEAN                RestartScan)
{
    NtDirHandle* dirHandle = static_cast<NtDirHandle*>(FileHandle);
    return dirHandle->QueryDirectoryFile(Event,
                                         ApcRoutine,
                                         ApcContext,
                                         IoStatusBlock,
                                         FileInformation,
                                         Length,
                                         FileInformationClass,
                                         ReturnSingleEntry,
                                         FileName,
                                         RestartScan);
}

NTSTATUS
NtCreateEvent(PHANDLE            EventHandle,
              ACCESS_MASK        DesiredAccess,
              POBJECT_ATTRIBUTES ObjectAttributes,
              EVENT_TYPE         EventType,
              BOOLEAN            InitialState)
{
    NtHandle* handle = new NtEventHandle(DesiredAccess,
                                         ObjectAttributes,
                                         EventType,
                                         InitialState);
    NTSTATUS ret = handle->Status();
    if (NT_SUCCESS(ret)) {
        *EventHandle = handle;
    }
    else {
        delete handle;
    }
    return ret;
}

#if KTL_USER_MODE
BOOL
WINAPI
WriteFile(HANDLE       hFile,
          LPVOID       lpBuffer,
          DWORD        nNumberOfBytesToWrite,
          LPDWORD      lpNumberBytesWriten,
          LPOVERLAPPED lpOverlapped)
{
    if (IsInvalidHandleValue(hFile))
    {
        NTSTATUS status;
        
        status = STATUS_INVALID_HANDLE;
        SetLastError(ERROR_INVALID_HANDLE);
        lpOverlapped->Internal = status;
        KTraceFailedAsyncRequest(status, NULL, 0, 0);
        return(FALSE);      
    }
    
    NtAsyncFileHandle* handle = static_cast<NtAsyncFileHandle*>(hFile); 
    return handle->WriteFile(lpBuffer, nNumberOfBytesToWrite, lpNumberBytesWriten, lpOverlapped);
}

BOOL
WINAPI
ReadFile(HANDLE       hFile,
         LPVOID       lpBuffer,
         DWORD        nNumberOfBytesToRead,
         LPDWORD      lpNumberBytesRead,
         LPOVERLAPPED lpOverlapped)
{
    if (IsInvalidHandleValue(hFile))
    {
        NTSTATUS status;
        
        status = STATUS_INVALID_HANDLE;
        SetLastError(ERROR_INVALID_HANDLE);
        lpOverlapped->Internal = status;
        KTraceFailedAsyncRequest(status, NULL, 0, 0);
        return(FALSE);      
    }
    
    NtAsyncFileHandle* handle = static_cast<NtAsyncFileHandle*>(hFile);
    return handle->ReadFile(lpBuffer, nNumberOfBytesToRead, lpNumberBytesRead, lpOverlapped);
}
#endif

NTSTATUS HLPalFunctions::CreateSecurityDescriptor(
    __out SECURITY_DESCRIPTOR& SecurityDescriptor,
    __out ACL*& Acl,
    __in KAllocator& Allocator
    )
{
    // TODO: Linux equivalent
    Acl = NULL;
    return STATUS_SUCCESS;
}

BOOL HLPalFunctions::ManageVolumePrivilege(
    )
{
    // No linux equivalent
    return TRUE;
}


NTSTATUS HLPalFunctions::AdjustExtentTable(
    __in HANDLE Handle,
    __in ULONGLONG OldFileSize,
    __in ULONGLONG NewFileSize,
    __in KAllocator& Allocator
    )
{
    if (IsInvalidHandleValue(Handle))
    {
        NTSTATUS status;
        
        status = STATUS_INVALID_HANDLE;
        KTraceFailedAsyncRequest(status, NULL, 0, 0);
        return(status);     
    }
    
    NtAsyncFileHandle* File = static_cast<NtAsyncFileHandle*>(Handle);

    return File->AdjustExtentTable(OldFileSize, NewFileSize, Allocator);
}

NTSTATUS HLPalFunctions::SetValidDataLength(
    __in HANDLE Handle,
    __in LONGLONG OldEndOfFileOffset,
    __in LONGLONG ValidDataLength,
    __in KAllocator& Allocator
    )
{
    NTSTATUS status;
    
    if (IsInvalidHandleValue(Handle))
    {
        status = STATUS_INVALID_HANDLE;
        KTraceFailedAsyncRequest(status, NULL, 0, 0);
        return(status);     
    }
    
    NtAsyncFileHandle* handle = static_cast<NtAsyncFileHandle*>(Handle);
    status = handle->SetValidDataLength(OldEndOfFileOffset, ValidDataLength, Allocator);

    return(status);
}

BOOL
HLPalFunctions::WriteFileGatherPal(
    __in HANDLE Handle,
    __in ULONG NumberAlignedBuffers,
    __in HLPalFunctions::ALIGNED_BUFFER* AlignedBuffers,
    __in DWORD  nNumberOfBytesToRead,
    __in LPOVERLAPPED lpOverlapped,
    __in ULONG PageSize                     
    )
{
    if (IsInvalidHandleValue(Handle))
    {
        NTSTATUS status;
        
        status = STATUS_INVALID_HANDLE;
        SetLastError(ERROR_INVALID_HANDLE);
        lpOverlapped->Internal = status;
        KTraceFailedAsyncRequest(status, NULL, 0, 0);
        return(FALSE);      
    }
    
    NtAsyncFileHandle* handle = static_cast<NtAsyncFileHandle*>(Handle);
    return handle->WriteFileGatherPal(NumberAlignedBuffers,
                                   AlignedBuffers,
                                   nNumberOfBytesToRead,
                                   lpOverlapped,
                                   PageSize);
}

BOOL
HLPalFunctions::ReadFileScatterPal(
    __in HANDLE Handle,
    __in ULONG NumberAlignedBuffers,
    __in HLPalFunctions::ALIGNED_BUFFER* AlignedBuffers,
    __in DWORD  nNumberOfBytesToRead,
    __in LPOVERLAPPED lpOverlapped,
    __in ULONG PageSize                     
    )
{
    if (IsInvalidHandleValue(Handle))
    {
        NTSTATUS status;
        
        status = STATUS_INVALID_HANDLE;
        SetLastError(ERROR_INVALID_HANDLE);
        lpOverlapped->Internal = status;
        KTraceFailedAsyncRequest(status, NULL, 0, 0);
        return(FALSE);      
    }
    
    NtAsyncFileHandle* handle = static_cast<NtAsyncFileHandle*>(Handle);
    return handle->ReadFileScatterPal(NumberAlignedBuffers,
                                   AlignedBuffers,
                                   nNumberOfBytesToRead,
                                   lpOverlapped,
                                   PageSize);
}


NTSTATUS
HLPalFunctions::NtCreateFileForKBlockFile(
    PHANDLE            FileHandle,
    ACCESS_MASK        DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    PIO_STATUS_BLOCK   IoStatusBlock,
    PLARGE_INTEGER     AllocationSize,
    ULONG              FileAttributes,
    ULONG              ShareAccess,
    ULONG              CreateDisposition,
    ULONG              CreateOptions,
    PVOID              EaBuffer,
    ULONG              EaLength,
    ULONG              Flags)
{
    return ::NtCreateFileForKBlockFile(FileHandle,
                              DesiredAccess,
                              ObjectAttributes,
                              IoStatusBlock,
                              AllocationSize,
                              FileAttributes,
                              ShareAccess,
                              CreateDisposition,
                              CreateOptions,
                              EaBuffer,
                              EaLength,
                              Flags);   
}

NTSTATUS
HLPalFunctions::SetUseFileSystemOnlyFlag(
    __in HANDLE Handle,
    __in BOOLEAN UseFileSystemOnly
    )
{
    NTSTATUS status;

    if (IsInvalidHandleValue(Handle))
    {
        status = STATUS_INVALID_HANDLE;
        KTraceFailedAsyncRequest(status, NULL, 0, 0);
        return(status);     
    }
    
    NtAsyncFileHandle* handle = static_cast<NtAsyncFileHandle*>(Handle);
    status = handle->SetUseFileSystemOnlyFlag(UseFileSystemOnly);
    return(status);
}

NTSTATUS
HLPalFunctions::SetFileSize(
    HANDLE Handle,
    BOOLEAN IsSparseFile,
    ULONG BlockSize,
    ULONGLONG OldFileSize,  // _File->_FileSize
    ULONGLONG NewFileSize,  // _FIleSize
    KAllocator& Allocator
    )
{
    NTSTATUS status;
    FILE_END_OF_FILE_INFORMATION endOfFile;
    FILE_ALLOCATION_INFORMATION allocInfo;
    IO_STATUS_BLOCK ioStatus;
    ULONGLONG newFileSize;

    if (IsInvalidHandleValue(Handle))
    {
        status = STATUS_INVALID_HANDLE;
        KTraceFailedAsyncRequest(status, NULL, 0, 0);
        return(status);     
    }
    
    newFileSize = ((NewFileSize + BlockSize - 1)/BlockSize)*BlockSize;

    if (! IsSparseFile)
    {
        //
        // For non-sparse file that is growing, update the allocation size
        //
        if (NewFileSize > OldFileSize)
        {
            allocInfo.AllocationSize.QuadPart = newFileSize;

            status = KNt::SetInformationFile(Handle, &ioStatus, &allocInfo, sizeof(allocInfo), FileAllocationInformation);

            if (!NT_SUCCESS(status)) {
                KTraceFailedAsyncRequest(status, nullptr, OldFileSize, newFileSize);
                goto Finish;
            }
        }
        
        status = HLPalFunctions::SetValidDataLength(Handle, OldFileSize, newFileSize, Allocator);
        if (!NT_SUCCESS(status)) {
            goto Finish;
        }
    }
    
    //
    // Update End of file. This will deallocate any blocks above EOF.
    //
    endOfFile.EndOfFile.QuadPart = newFileSize;

    status = KNt::SetInformationFile(Handle, &ioStatus, &endOfFile, sizeof(endOfFile), FileEndOfFileInformation);

    if (!NT_SUCCESS(status)) {
        KTraceFailedAsyncRequest(status, nullptr, OldFileSize, newFileSize);
        goto Finish;
    }

    if (! IsSparseFile)
    {
        //
        // Adjust the extent table.
        //
        status = HLPalFunctions::AdjustExtentTable(Handle, OldFileSize, NewFileSize, Allocator);
        
        if (!NT_SUCCESS(status)) {
            goto Finish;
        }
    }

Finish:

    return(status);
}




NTSTATUS HLPalFunctions::RenameFile(
    __in PWCHAR FromPathName,
    __in PWCHAR ToPathName,
    __in ULONG ToPathNameLength,
    __in BOOLEAN OverwriteIfExists,
    __in KAllocator& Allocator                                  
)
{
    NTSTATUS status;
    int result;
    std::string fromPathname = Utf16To8(FromPathName);
    std::string toPathname = Utf16To8(ToPathName);

    result = syscall(SYS_renameat2,
                         0,
                         fromPathname.c_str(),
                         0,
                         toPathname.c_str(),
                         OverwriteIfExists ? 0 : RENAME_NOREPLACE);
        
    if (result == -1)
    {
        status = LinuxError::LinuxErrorToNTStatus(errno);
    } else {
        status = STATUS_SUCCESS;
    }   
    
    return(status);
}
