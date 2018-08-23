/*++

    (c) 2017 by Microsoft Corp. All Rights Reserved.

    palio.h
    
    Description:
        Platform abstraction layer for linux - IO

    History:

--*/

#pragma once

#include <ktl.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <ktlpal.h>
#include <HLPalFunctions.h>
#include "KXMApi.h"

#define KTL_TAG_WORKBUFFERS 'PIWB'

class NtHandle {

  private:
    NTSTATUS _status;

  public:
    NtHandle();
    virtual ~NtHandle() = 0;

    void SetConstructorStatus(NTSTATUS status)
    {
        _status = status;
    }

    NTSTATUS Status()
    {
        return _status;
    }

    virtual NTSTATUS Close() = 0;
};

class NtEventHandle : public NtHandle {

  private:
    KEvent _Event;

    NtEventHandle(const NtEventHandle& rhs) = delete;
    NtEventHandle& operator=(const NtEventHandle& rhs) = delete;


  public:
    static const ULONG fstatBlockSize = 512;

    static void SetEvent(union sigval val);

    NtEventHandle(ACCESS_MASK        DesiredAccess,
                  POBJECT_ATTRIBUTES ObjectAttributes,
                  EVENT_TYPE         EventType,
                  BOOLEAN            InitialState);

    virtual ~NtEventHandle();

    NTSTATUS Close();

    NTSTATUS WaitForSingleObject(BOOLEAN        Alertable,
                                 PLARGE_INTEGER Timeout);

    void SetEvent();
};

class NtFileHandle : public NtHandle {

  public:
    
    //
    // TRUE when IO must always go via the file system and not direct
    // IO
    //
    inline BOOLEAN UseFileSystemOnly()
    {
        return((_Flags & NTCF_FLAG_USE_FILE_SYSTEM_ONLY) != 0);
    }

    //
    // TRUE when IO must no go via the file system and must go via direct
    // IO instead
    //
    inline BOOLEAN AvoidFileSystem()
    {
        return((_Flags & NTCF_FLAG_AVOID_FILE_SYSTEM) != 0);
    }

    inline int GetFileDescriptor()
    {
        if (UseFileSystemOnly())
        {
            return(_FileDescriptor);
        }

        int fd;
        char *filePath = (char *)_PathToFile.c_str();
        fd = open(filePath, _OpenFlags | O_SYNC | O_CLOEXEC);
        
        return(fd);
    }

    inline VOID DoneWithFileDescriptor(int fd)
    {
        if (! UseFileSystemOnly())
        {
            close(fd);
        }
    }

    NTSTATUS MapLogicalToPhysical(
        off64_t Logical,
        size_t Length,
        off64_t& Physical,
        size_t& PhysicalLength
        );

    NTSTATUS
    BuildLogicalToPhysicalMapping(
        __in ULONG NumberAlignedBuffers,
        __in HLPalFunctions::ALIGNED_BUFFER* AlignedBuffers,
        __in LPOVERLAPPED lpOverlapped,
        __in ULONG FragsCount,  
        __out KXMFileOffset *Frags,
        __out ULONG& UsedFragsCount,
        __in ULONG PageSize
        );

	NTSTATUS AdjustExtentTable(
		__in ULONGLONG OldFileSize,
		__in ULONGLONG NewFileSize,
		__in KAllocator& Allocator
		);
	

    NTSTATUS SetValidDataLength(
        __in LONGLONG OldEndOfFileOffset,
        __in LONGLONG ValidDataLength,
		__in KAllocator& Allocator
        );
    
    
  protected:
    std::string _PathToFile;
    ULONG       _OpenFlags;
    KBuffer::SPtr _Fiemap;
    ULONG _FiemapExtentCount;
    KReaderWriterSpinLock _MappingLock;

      
    int         _FileDescriptor;
    bool        _DeleteOnClose;
    HANDLE      _KXMDescriptor;
    HANDLE      _KXMFileDescriptor;
    ULONG       _CreateOptions;
    ULONG       _Flags;

    NtFileHandle(const NtFileHandle& rhs) = delete;
    NtFileHandle& operator=(const NtFileHandle& rhs) = delete;

  public:
    NtFileHandle(ACCESS_MASK        DesiredAccess,
                 POBJECT_ATTRIBUTES ObjectAttributes,
                 PIO_STATUS_BLOCK   IoStatusBlock,
                 PLARGE_INTEGER     AllocationSize,
                 ULONG              FileAttributes,
                 ULONG              ShareAccess,
                 ULONG              CreateDisposition,
                 ULONG              CreateOptions,
                 PVOID              EaBuffer,
                 ULONG              EaLength,
                 ULONG              Flags);

    virtual ~NtFileHandle();

    NTSTATUS Close();

    //TODO Find a better place for this ...
    HANDLE GetKXMFileDescriptor();

    virtual NTSTATUS Read(HANDLE           Event,
                          PIO_APC_ROUTINE  ApcRoutine,
                          PVOID            ApcContext,
                          PIO_STATUS_BLOCK IoStatusBlock,
                          PVOID            Buffer,
                          ULONG            Length,
                          PLARGE_INTEGER   ByteOffset,
                          PULONG           Key) = 0;

    virtual NTSTATUS Write(HANDLE           Event,
                           PIO_APC_ROUTINE  ApcRoutine,
                           PVOID            ApcContext,
                           PIO_STATUS_BLOCK IoStatusBlock,
                           PVOID            Buffer,
                           ULONG            Length,
                           PLARGE_INTEGER   ByteOffset,
                           PULONG           Key) = 0;

    virtual NTSTATUS FlushBuffersFile(
        PIO_STATUS_BLOCK IoStatusBlock
        ) = 0;

    NTSTATUS QueryInformationFile(PIO_STATUS_BLOCK       IoStatusBlock,
                                  PVOID                  FileInformation,
                                  ULONG                  Length,
                                  FILE_INFORMATION_CLASS FileInformationClass);

    NTSTATUS SetInformationFile(PIO_STATUS_BLOCK       IoStatusBlock,
                                PVOID                  FileInformation,
                                ULONG                  Length,
                                FILE_INFORMATION_CLASS FileInformationClass);

    NTSTATUS FsControlFile(HANDLE           Event,
                           PIO_APC_ROUTINE  ApcRoutine,
                           PVOID            ApcContext,
                           PIO_STATUS_BLOCK IoStatusBlock,
                           ULONG            FsControlCode,
                           PVOID            InputBuffer,
                           ULONG            InputBufferLength,
                           PVOID            OutputBuffer,
                           ULONG            OutputBufferLength);

  private:
    NTSTATUS Initialize(ACCESS_MASK        DesiredAccess,
                        POBJECT_ATTRIBUTES ObjectAttributes,
                        PIO_STATUS_BLOCK   IoStatusBlock,
                        PLARGE_INTEGER     AllocationSize,
                        ULONG              FileAttributes,
                        ULONG              ShareAccess,
                        ULONG              CreateDisposition,
                        ULONG              CreateOptions,
                        PVOID              EaBuffer,
                        ULONG              EaLength,
                        ULONG              Flags);
};

class NtSyncFileHandle : public NtFileHandle {

  private:
    NtSyncFileHandle(const NtSyncFileHandle& rhs) = delete;
    NtSyncFileHandle& operator=(const NtSyncFileHandle& rhs) = delete;

  public:
    NtSyncFileHandle(ACCESS_MASK        DesiredAccess,
                     POBJECT_ATTRIBUTES ObjectAttributes,
                     PIO_STATUS_BLOCK   IoStatusBlock,
                     PLARGE_INTEGER     AllocationSize,
                     ULONG              FileAttributes,
                     ULONG              ShareAccess,
                     ULONG              CreateDisposition,
                     ULONG              CreateOptions,
                     PVOID              EaBuffer,
                     ULONG              EaLength,
                     ULONG              Flags);

    virtual ~NtSyncFileHandle();

    virtual NTSTATUS Read(HANDLE           Event,
                          PIO_APC_ROUTINE  ApcRoutine,
                          PVOID            ApcContext,
                          PIO_STATUS_BLOCK IoStatusBlock,
                          PVOID            Buffer,
                          ULONG            Length,
                          PLARGE_INTEGER   ByteOffset,
                          PULONG           Key);

    virtual NTSTATUS Write(HANDLE           Event,
                           PIO_APC_ROUTINE  ApcRoutine,
                           PVOID            ApcContext,
                           PIO_STATUS_BLOCK IoStatusBlock,
                           PVOID            Buffer,
                           ULONG            Length,
                           PLARGE_INTEGER   ByteOffset,
                           PULONG           Key);

    virtual NTSTATUS FlushBuffersFile(
        PIO_STATUS_BLOCK IoStatusBlock
        );
    
};

class NtAsyncFileHandle : public NtFileHandle {

  private:
    std::mutex _mutex;
    HANDLE     _IoCompletionPort;
    ULONG_PTR  _CompletionKey;

    NtAsyncFileHandle(const NtAsyncFileHandle& rhs) = delete;
    NtAsyncFileHandle& operator=(const NtAsyncFileHandle& rhs) = delete;

  public:
    NtAsyncFileHandle(ACCESS_MASK        DesiredAccess,
                      POBJECT_ATTRIBUTES ObjectAttributes,
                      PIO_STATUS_BLOCK   IoStatusBlock,
                      PLARGE_INTEGER     AllocationSize,
                      ULONG              FileAttributes,
                      ULONG              ShareAccess,
                      ULONG              CreateDisposition,
                      ULONG              CreateOptions,
                      PVOID              EaBuffer,
                      ULONG              EaLength,
                      ULONG              Flags);

    virtual ~NtAsyncFileHandle();

    virtual NTSTATUS Read(HANDLE           Event,
                          PIO_APC_ROUTINE  ApcRoutine,
                          PVOID            ApcContext,
                          PIO_STATUS_BLOCK IoStatusBlock,
                          PVOID            Buffer,
                          ULONG            Length,
                          PLARGE_INTEGER   ByteOffset,
                          PULONG           Key);

    virtual NTSTATUS Write(HANDLE           Event,
                           PIO_APC_ROUTINE  ApcRoutine,
                           PVOID            ApcContext,
                           PIO_STATUS_BLOCK IoStatusBlock,
                           PVOID            Buffer,
                           ULONG            Length,
                           PLARGE_INTEGER   ByteOffset,
                           PULONG           Key);

    BOOL ReadFileScatterPal(
        __in ULONG NumberAlignedBuffers,
        __in HLPalFunctions::ALIGNED_BUFFER* AlignedBuffers,
        __in DWORD  nNumberOfBytesToRead,
        __in LPOVERLAPPED lpOverlapped,
        __in ULONG PageSize
        );

    BOOL WriteFileGatherPal(
        __in ULONG NumberAlignedBuffers,
        __in HLPalFunctions::ALIGNED_BUFFER* AlignedBuffers,
        __in DWORD  nNumberOfBytesToRead,
        __in LPOVERLAPPED lpOverlapped,
        __in ULONG PageSize
        );

    BOOL WINAPI ReadFile(
      _In_       LPVOID               lpBuffer,
      _In_       DWORD                nNumberOfBytesToRead,
      _Out_opt_  LPDWORD              lpNumberBytesRead,
      _Inout_    LPOVERLAPPED         lpOverlapped);
    

    BOOL WINAPI WriteFile(
      _In_       LPVOID               lpBuffer,
      _In_       DWORD                nNumberOfBytesToWrite,
      _Out_opt_  LPDWORD              lpNumberBytesWriten,
      _Inout_    LPOVERLAPPED         lpOverlapped);
    
    NTSTATUS
    SetUseFileSystemOnlyFlag(
        __in BOOLEAN UseFileSystemOnly
        );

    virtual NTSTATUS FlushBuffersFile(
        PIO_STATUS_BLOCK IoStatusBlock
        );
};

class NtDirHandle : public NtHandle {

  private:
    DIR* _Dir;

    NtDirHandle(const NtDirHandle& rhs) = delete;
    NtDirHandle& operator=(const NtDirHandle& rhs) = delete;

  public:
    NtDirHandle(ACCESS_MASK        DesiredAccess,
                POBJECT_ATTRIBUTES ObjectAttributes,
                PIO_STATUS_BLOCK   IoStatusBlock,
                PLARGE_INTEGER     AllocationSize,
                ULONG              FileAttributes,
                ULONG              ShareAccess,
                ULONG              CreateDisposition,
                ULONG              CreateOptions,
                PVOID              EaBuffer,
                ULONG              EaLength);

    virtual ~NtDirHandle();

    NTSTATUS Close();

    NTSTATUS QueryDirectoryFile(HANDLE                 Event,
                                PIO_APC_ROUTINE        ApcRoutine,
                                PVOID                  ApcContext,
                                PIO_STATUS_BLOCK       IoStatusBlock,
                                PVOID                  FileInformation,
                                ULONG                  Length,
                                FILE_INFORMATION_CLASS FileInformationClass,
                                BOOLEAN                ReturnSingleEntry,
                                PUNICODE_STRING        FileName,
                                BOOLEAN                RestartScan);

  private:
    NTSTATUS Initialize(ACCESS_MASK        DesiredAccess,
                        POBJECT_ATTRIBUTES ObjectAttributes,
                        PIO_STATUS_BLOCK   IoStatusBlock,
                        PLARGE_INTEGER     AllocationSize,
                        ULONG              FileAttributes,
                        ULONG              ShareAccess,
                        ULONG              CreateDisposition,
                        ULONG              CreateOptions,
                        PVOID              EaBuffer,
                        ULONG              EaLength);
};

