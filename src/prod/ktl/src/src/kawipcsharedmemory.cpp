/*++

Module Name:

    KAWIpc.cpp

Abstract:

    This file contains the implementation for the KAWIpc mechanism.
    This is a cross process IPC mechanism.

Author:

    Alan Warwick 05/2/2017

Environment:

    User mode

Notes:

Revision History:

--*/

#include <ktl.h>
#include <ktrace.h>
#include <kawipc.h>

#if defined(PLATFORM_UNIX)
#include <palio.h>

#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>

#endif

#if !defined(PLATFORM_UNIX)
WCHAR const * const KAWIpcSharedMemory::_SharedMemoryNamePrefix = L"_KtlLogger_Microsoft_";
#else
WCHAR const * const KAWIpcSharedMemory::_SharedMemoryNamePrefix = L"/_KtlLogger_Microsoft_";
#endif
CHAR const * const KAWIpcSharedMemory::_SharedMemoryNamePrefixAnsi = "_KtlLogger_Microsoft_";


KAWIpcSharedMemory::KAWIpcSharedMemory()
{
    // Not used
    KInvariant(FALSE);
}

KAWIpcSharedMemory::~KAWIpcSharedMemory()
{
    if (_BaseAddress != nullptr)
    {
#if !defined(PLATFORM_UNIX)
        UnmapViewOfFile(_BaseAddress);
#else
        munmap(_BaseAddress, _Size);
#endif
        _BaseAddress = nullptr;
    }
    
#if !defined(PLATFORM_UNIX)
    CloseHandle(_SectionHandle);
#else
    if (_SectionFd != -1)
    {
        close(_SectionFd);
    }
    
    if (_OwnSection)
    {
        NTSTATUS status;
        KDynString name(GetThisAllocator(), _SharedMemoryNameLength);
        
        status = CreateSectionName(_Id.Get(), name);
        if (NT_SUCCESS(status))
        {
            shm_unlink(Utf16To8(name).c_str());
        }
    }
#endif
}

KAWIpcSharedMemory::KAWIpcSharedMemory(
    __in ULONG Size    
    )
{
    NTSTATUS status;
    KGuid guid;
    KDynString name(GetThisAllocator(), _SharedMemoryNameLength);

    _OwnSection = FALSE;
    _BaseAddress = nullptr;
#if !defined(PLATFORM_UNIX)
    _SectionHandle = nullptr;
#else
    _SectionFd = -1;
#endif
    
    guid.CreateNew();
    _Id = guid; 
    status = CreateSectionName(guid, name);
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }
        
    _Size = Size;
#if !defined(PLATFORM_UNIX)
    DWORD error;
    
    _SectionHandle = CreateFileMapping(INVALID_HANDLE_VALUE,
                                       nullptr,
                                       PAGE_EXECUTE_READWRITE,
                                       0,
                                       Size,
                                       (PWCHAR)name);
    if (_SectionHandle == nullptr)
    {
        error = GetLastError();
        status = STATUS_UNSUCCESSFUL;
        SetConstructorStatus(status);
        return;
    }   
    _OwnSection = TRUE;
#else
    int err;
    _SectionFd = shm_open(Utf16To8(name).c_str(), O_RDWR | O_CREAT | O_EXCL | O_TRUNC, S_IRWXU);
    if (_SectionFd == -1)
    {
        status = LinuxError::LinuxErrorToNTStatus(errno);
        SetConstructorStatus(status);
        return;
    }
    _OwnSection = TRUE;
    
    err = ftruncate(_SectionFd, Size);
    if (err == -1)
    {
        status = LinuxError::LinuxErrorToNTStatus(errno);
        SetConstructorStatus(status);
        return;
    }
#endif

    _ClientBaseOffset = 0;

#if !defined(PLATFORM_UNIX)
    _BaseAddress = MapViewOfFile(_SectionHandle,
                                 FILE_MAP_ALL_ACCESS,
                                 0,
                                 0,
                                 _Size);
    if (_BaseAddress == nullptr)
    {
        error = GetLastError();
        status = STATUS_UNSUCCESSFUL;
        SetConstructorStatus(status);
        return;
    }
#else
    _BaseAddress = mmap(nullptr, Size, PROT_READ | PROT_WRITE, MAP_SHARED, _SectionFd, 0);
    if (_BaseAddress == MAP_FAILED)
    {
        status = LinuxError::LinuxErrorToNTStatus(errno);
        SetConstructorStatus(status);
        return;
    }
#endif
}


KAWIpcSharedMemory::KAWIpcSharedMemory(
    __in SHAREDMEMORYID SharedMemoryId,
    __in ULONG StartingOffset,
    __in ULONG Size,
    __in ULONG ClientBaseOffset
    )
{
    NTSTATUS status;
    KDynString name(GetThisAllocator(), _SharedMemoryNameLength);

    _OwnSection = FALSE;
    
    status = CreateSectionName(SharedMemoryId.Get(), name);
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }
        
    _Size = Size;
    _ClientBaseOffset = ClientBaseOffset;

#if !defined(PLATFORM_UNIX)
    _SectionHandle = OpenFileMapping(FILE_MAP_ALL_ACCESS,
                                     FALSE,
                                     (PWCHAR)name);                                  
    if (_SectionHandle == nullptr)
    {
        DWORD error;
        error = GetLastError();
        status = STATUS_UNSUCCESSFUL;
    }
#else
    _SectionFd = shm_open(Utf16To8(name).c_str(), O_RDWR, 0);
    if (_SectionFd == -1)
    {
        status = LinuxError::LinuxErrorToNTStatus(errno);
        SetConstructorStatus(status);
        return;
    }
#endif
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }

#if !defined(PLATFORM_UNIX)
    DWORD error;
    _BaseAddress = MapViewOfFile(_SectionHandle,
                                 FILE_MAP_ALL_ACCESS,
                                 0,
                                 StartingOffset,
                                 _Size);
    if (_BaseAddress == nullptr)
    {
        error = GetLastError();
        status = STATUS_UNSUCCESSFUL;
    }
#else
    _BaseAddress = mmap(nullptr, Size, PROT_READ | PROT_WRITE, MAP_SHARED, _SectionFd, StartingOffset);
    if (_BaseAddress == MAP_FAILED)
    {
        status = LinuxError::LinuxErrorToNTStatus(errno);
        SetConstructorStatus(status);
        return;
    }
#endif
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }
    
}

NTSTATUS KAWIpcSharedMemory::CreateSectionName(
    __in SHAREDMEMORYID SharedMemoryId,
    __out KDynString& Name
    )
{

    NTSTATUS status = STATUS_SUCCESS;
    KStringView prefix(_SharedMemoryNamePrefix);
    BOOLEAN b;

    b = Name.Concat(prefix);
    if (! b)
    {
        status = STATUS_UNSUCCESSFUL;
        return(status);
    }
    
    b = Name.FromGUID(SharedMemoryId.Get(), TRUE);
    if (! b)
    {
        status = STATUS_UNSUCCESSFUL;
        return(status);
    }

    b = Name.SetNullTerminator();
    if (! b)
    {
        status = STATUS_UNSUCCESSFUL;
    }
    
    return(status);
}

NTSTATUS KAWIpcSharedMemory::Create(
    __in KAllocator& Allocator,
    __in ULONG AllocationTag,
    __in ULONG Size,
    __out KAWIpcSharedMemory::SPtr& Context
)
{
    KAWIpcSharedMemory::SPtr context;
    
    context = _new(AllocationTag, Allocator) KAWIpcSharedMemory(Size);
    if (! context)
    {
        KTraceOutOfMemory(0, STATUS_INSUFFICIENT_RESOURCES, nullptr, 0, 0);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    if (! NT_SUCCESS(context->Status()))
    {
        return(context->Status());
    }

    Context = Ktl::Move(context);
    return(STATUS_SUCCESS); 
}

NTSTATUS KAWIpcSharedMemory::Create(
    __in KAllocator& Allocator,
    __in ULONG AllocationTag,
    __in SHAREDMEMORYID SharedMemoryId,
    __in ULONG StartingOffset,
    __in ULONG Size,
    __in ULONG ClientBaseOffset,
    __out KAWIpcSharedMemory::SPtr& Context
)
{
    NTSTATUS status;
    KAWIpcSharedMemory::SPtr context;
    
    context = _new(AllocationTag, Allocator) KAWIpcSharedMemory(SharedMemoryId, StartingOffset, Size, ClientBaseOffset);
    if (! context)
    {
        KTraceOutOfMemory(0, STATUS_INSUFFICIENT_RESOURCES, nullptr, 0, 0);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    status = context->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    Context = Ktl::Move(context);
    return(STATUS_SUCCESS);
}

#if defined(PLATFORM_UNIX)
ktl::Awaitable<NTSTATUS> KAWIpcSharedMemory::CleanupFromPreviousCrashes(
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    )
{
    NTSTATUS status;
    ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acs;
    using MyWorkItemType = KThreadPool::ParameterizedWorkItem<ktl::AwaitableCompletionSource<NTSTATUS>&>;

    status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(Allocator, AllocationTag, acs);
    if (! NT_SUCCESS(status))
    {
        co_return(status);
    }

    MyWorkItemType::WorkItemProcessor worker = [](ktl::AwaitableCompletionSource<NTSTATUS>& Acs)
    {
        
        // CONSIDER: Encode PID in the filename so that only old files are
        //           deleted.
        NTSTATUS status = STATUS_SUCCESS;
        DIR *d;
        struct dirent *dir;

        d = opendir("/dev/shm");
        if (d)
        {
            while ((dir = readdir(d)) != nullptr)
            {
                if (memcmp(dir->d_name, _SharedMemoryNamePrefixAnsi, sizeof(_SharedMemoryNamePrefixAnsi)) == 0)
                {
                    shm_unlink(dir->d_name);
                }
            }

            closedir(d);
        } else {
            status = LinuxError::LinuxErrorToNTStatus(errno);
        }
                
        Acs.SetResult(status);
    };

    MyWorkItemType      workItem(*acs, worker);

    Allocator.GetKtlSystem().DefaultSystemThreadPool().QueueWorkItem(workItem);
    
    status = co_await acs->GetAwaitable();

    co_return(status);  
}

#endif

