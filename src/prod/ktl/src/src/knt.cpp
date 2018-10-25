/*++

Module Name:

    KNt.cpp

Abstract:

    This file contains the user mode and kernel mode implementations of the KNt routines.

Author:

    Norbert P. Kusters (norbertk) 17-Nov-2010

Environment:

    Kernel mode and User mode

Notes:

Revision History:

--*/

#include <ktl.h>

NTSTATUS
KNt::CreateFile(
    __out HANDLE* FileHandle,
    __in ACCESS_MASK DesiredAccess,
    __in OBJECT_ATTRIBUTES* ObjectAttributes,
    __out IO_STATUS_BLOCK* IoStatusBlock,
    __in_opt LARGE_INTEGER* AllocationSize,
    __in ULONG FileAttributes,
    __in ULONG ShareAccess,
    __in ULONG CreateDisposition,
    __in ULONG CreateOptions,
    __in_bcount_opt(EaLength) VOID* EaBuffer,
    __in ULONG EaLength
    )
{
#if KTL_USER_MODE
    return NtCreateFile(
#else
    ObjectAttributes->Attributes |= OBJ_KERNEL_HANDLE;
    return ZwCreateFile(
#endif
            FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, AllocationSize, FileAttributes, ShareAccess,
            CreateDisposition, CreateOptions, EaBuffer, EaLength);
}

NTSTATUS
KNt::OpenFile(
    __out HANDLE* FileHandle,
    __in ACCESS_MASK DesiredAccess,
    __in OBJECT_ATTRIBUTES* ObjectAttributes,
    __out IO_STATUS_BLOCK* IoStatusBlock,
    __in ULONG ShareAccess,
    __in ULONG OpenOptions
    )
{
#if KTL_USER_MODE
    return NtOpenFile(
#else
    ObjectAttributes->Attributes |= OBJ_KERNEL_HANDLE;
    return ZwOpenFile(
#endif
            FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, OpenOptions);
}

NTSTATUS
KNt::DeleteFile(
    __in_z PCWCHAR FileName
    )
{
    NTSTATUS LocalStatus;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING String;

    RtlInitUnicodeString(&String, FileName);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &String,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

#if KTL_USER_MODE
    LocalStatus = NtDeleteFile( &ObjectAttributes );
#else
    ObjectAttributes.Attributes |= OBJ_KERNEL_HANDLE;
    LocalStatus = ZwDeleteFile( &ObjectAttributes );
#endif

    return(LocalStatus);
}

NTSTATUS
KNt::Close(
    __in HANDLE Handle
    )
{
#if KTL_USER_MODE
    return NtClose(
#else
    return ZwClose(
#endif
            Handle);
}

#if !defined(PLATFORM_UNIX)
NTSTATUS
KNt::CreateSection(
    __out    HANDLE&                    SectionHandle,
    __in     ACCESS_MASK                DesiredAccess,
    __in_opt OBJECT_ATTRIBUTES* const   ObjectAttributes,
    __in_opt LARGE_INTEGER* const       MaximumSize,
    __in     ULONG                      SectionPageProtection,
    __in     ULONG                      AllocationAttributes,
    __in_opt HANDLE const               FileHandle
    )
{
#if KTL_USER_MODE
    return NtCreateSection(&SectionHandle, DesiredAccess, ObjectAttributes, MaximumSize, SectionPageProtection, AllocationAttributes, FileHandle);
#else
    ObjectAttributes->Attributes |= OBJ_KERNEL_HANDLE;
    return ZwCreateSection(&SectionHandle, DesiredAccess, ObjectAttributes, MaximumSize, SectionPageProtection, AllocationAttributes, FileHandle);
#endif
}

NTSTATUS
KNt::MapViewOfSection(
    __in        HANDLE                  SectionHandle,
    __inout     VOID*&                  BaseAddress,
    __in        ULONG_PTR               ZeroBits,
    __in        SIZE_T                  CommitSize,
    __inout_opt LARGE_INTEGER* const    SectionOffset,
    __inout     SIZE_T&                 ViewSize,
    __in        SECTION_INHERIT         InheritDisposition,
    __in        ULONG                   AllocationType,
    __in        ULONG                   Win32Protect
    )
{
#if KTL_USER_MODE
    HANDLE currentProcessHandle = NtCurrentProcess();
    return NtMapViewOfSection(SectionHandle, currentProcessHandle, &BaseAddress, ZeroBits, CommitSize, SectionOffset, &ViewSize, InheritDisposition, AllocationType, Win32Protect);
#else
    HANDLE currentProcessHandle = ZwCurrentProcess();
    return ZwMapViewOfSection(SectionHandle, currentProcessHandle, &BaseAddress, ZeroBits, CommitSize, SectionOffset, &ViewSize, InheritDisposition, AllocationType, Win32Protect);
#endif

}

NTSTATUS
KNt::UnmapViewOfSection(
    __in_opt VOID* const    BaseAddress
    )
{
#if KTL_USER_MODE
    HANDLE currentProcessHandle = NtCurrentProcess();
    return NtUnmapViewOfSection(currentProcessHandle, BaseAddress);
#else
    HANDLE currentProcessHandle = ZwCurrentProcess();
    return ZwUnmapViewOfSection(currentProcessHandle, BaseAddress);
#endif
}

NTSTATUS
KNt::FlushVirtualMemory(
    __inout VOID*&              BaseAddress,
    __inout SIZE_T&             RegionSize,
    __out   IO_STATUS_BLOCK&    IoStatus
    )
{
#if KTL_USER_MODE
    HANDLE currentProcessHandle = NtCurrentProcess();
    return  NtFlushVirtualMemory(currentProcessHandle, &BaseAddress, &RegionSize, &IoStatus);
#else
    HANDLE currentProcessHandle = ZwCurrentProcess();
    return  ZwFlushVirtualMemory(currentProcessHandle, &BaseAddress, &RegionSize, &IoStatus);
#endif
}
#endif

NTSTATUS
KNt::QueryInformationFile(
    __in HANDLE FileHandle,
    __out IO_STATUS_BLOCK* IoStatusBlock,
    __out_bcount(Length) VOID* FileInformation,
    __in ULONG Length,
    __in FILE_INFORMATION_CLASS FileInformationClass
    )
{
#if KTL_USER_MODE
    return NtQueryInformationFile(
#else
    return ZwQueryInformationFile(
#endif
            FileHandle, IoStatusBlock, FileInformation, Length, FileInformationClass);
}

NTSTATUS
KNt::QueryFullAttributesFile(
    __in OBJECT_ATTRIBUTES& ObjectAttributes,
    __out FILE_NETWORK_OPEN_INFORMATION& Result)
{
#if KTL_USER_MODE
    return NtQueryFullAttributesFile(&ObjectAttributes, &Result);
#else
    ObjectAttributes.Attributes |= OBJ_KERNEL_HANDLE;
    return ZwQueryFullAttributesFile(&ObjectAttributes, &Result);
#endif
}

NTSTATUS
KNt::SetInformationFile(
    __in HANDLE FileHandle,
    __out IO_STATUS_BLOCK* IoStatusBlock,
    __in_bcount(Length) VOID* FileInformation,
    __in ULONG Length,
    __in FILE_INFORMATION_CLASS FileInformationClass
    )
{
#if KTL_USER_MODE
    return NtSetInformationFile(
#else
    return ZwSetInformationFile(
#endif
            FileHandle, IoStatusBlock, FileInformation, Length, FileInformationClass);
}

NTSTATUS
KNt::FsControlFile(
    __in HANDLE FileHandle,
    __in_opt HANDLE Event,
    __in_opt PIO_APC_ROUTINE ApcRoutine,
    __in_opt VOID* ApcContext,
    __out IO_STATUS_BLOCK* IoStatusBlock,
    __in ULONG FsControlCode,
    __in_bcount_opt(InputBufferLength) VOID* InputBuffer,
    __in ULONG InputBufferLength,
    __out_bcount_opt(OutputBufferLength) VOID* OutputBuffer,
    __in ULONG OutputBufferLength
    )
{
#if KTL_USER_MODE
    return NtFsControlFile(
#else
    return ZwFsControlFile(
#endif
            FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, FsControlCode, InputBuffer, InputBufferLength,
            OutputBuffer, OutputBufferLength);
}

NTSTATUS
KNt::DeviceIoControlFile(
    __in HANDLE FileHandle,
    __in_opt HANDLE Event,
    __in_opt PIO_APC_ROUTINE ApcRoutine,
    __in_opt VOID* ApcContext,
    __out IO_STATUS_BLOCK* IoStatusBlock,
    __in ULONG IoControlCode,
    __in_bcount_opt(InputBufferLength) VOID* InputBuffer,
    __in ULONG InputBufferLength,
    __out_bcount_opt(OutputBufferLength) VOID* OutputBuffer,
    __in ULONG OutputBufferLength
    )
{
#if KTL_USER_MODE
    return NtDeviceIoControlFile(
#else
    return ZwDeviceIoControlFile(
#endif
            FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, IoControlCode, InputBuffer, InputBufferLength,
            OutputBuffer, OutputBufferLength);
}

NTSTATUS
KNt::WriteFile(
    __in HANDLE FileHandle,
    __in_opt HANDLE Event,
    __in_opt PIO_APC_ROUTINE ApcRoutine,
    __in_opt VOID* ApcContext,
    __out IO_STATUS_BLOCK* IoStatusBlock,
    __in_bcount(Length) VOID* Buffer,
    __in ULONG Length,
    __in_opt LARGE_INTEGER* ByteOffset,
    __in_opt ULONG* Key
    )
{
#if KTL_USER_MODE
    return NtWriteFile(
#else
    return ZwWriteFile(
#endif
            FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffset, Key);
}

NTSTATUS
KNt::ReadFile(
    __in HANDLE FileHandle,
    __in_opt HANDLE Event,
    __in_opt PIO_APC_ROUTINE ApcRoutine,
    __in_opt VOID* ApcContext,
    __out IO_STATUS_BLOCK* IoStatusBlock,
    __in_bcount(Length) VOID* Buffer,
    __in ULONG Length,
    __in_opt LARGE_INTEGER* ByteOffset,
    __in_opt ULONG* Key
    )
{
#if KTL_USER_MODE
    return NtReadFile(
#else
    return ZwReadFile(
#endif
            FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffset, Key);
}

NTSTATUS
KNt::FlushBuffersFile(
    __in HANDLE FileHandle,
    __out IO_STATUS_BLOCK* IoStatusBlock
    )
{
#if KTL_USER_MODE
    return NtFlushBuffersFile(
#else
    return ZwFlushBuffersFile(
#endif
            FileHandle, IoStatusBlock);
}

NTSTATUS
KNt::WaitForSingleObject(
    __in HANDLE Handle,
    __in BOOLEAN Alertable,
    __in_opt LARGE_INTEGER* Timeout
    )
{
#if KTL_USER_MODE
    return NtWaitForSingleObject(
#else
    return ZwWaitForSingleObject(
#endif
            Handle, Alertable, Timeout);
}

NTSTATUS
KNt::QueryDirectoryFile(
    __in HANDLE FileHandle,
    __in_opt HANDLE Event,
    __in_opt PIO_APC_ROUTINE ApcRoutine,
    __in_opt VOID* ApcContext,
    __out IO_STATUS_BLOCK* IoStatusBlock,
    __out_bcount(Length) VOID* FileInformation,
    __in ULONG Length,
    __in FILE_INFORMATION_CLASS FileInformationClass,
    __in BOOLEAN ReturnSingleEntry,
    __in_opt UNICODE_STRING* FileName,
    __in BOOLEAN RestartScan
    )
{
#if KTL_USER_MODE
    return NtQueryDirectoryFile(
#else
    return ZwQueryDirectoryFile(
#endif
            FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, FileInformation, Length, FileInformationClass,
            ReturnSingleEntry, FileName, RestartScan);
}


NTSTATUS
KNt::QuerySystemInformation(
    __in SYSTEM_INFORMATION_CLASS SystemInformationClass,
    __out_bcount_part_opt(SystemInformationLength, *ReturnLength) VOID* SystemInformation,
    __in ULONG SystemInformationLength,
    __out_opt ULONG* ReturnLength
    )
{
#if KTL_USER_MODE
    return NtQuerySystemInformation(
#else
    return ZwQuerySystemInformation(
#endif
            SystemInformationClass, SystemInformation, SystemInformationLength, ReturnLength);
}


NTSTATUS
KNt::CreateEvent(
    __out HANDLE* EventHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt OBJECT_ATTRIBUTES* ObjectAttributes,
    __in EVENT_TYPE EventType,
    __in BOOLEAN InitialState
    )
{
#if KTL_USER_MODE
    return NtCreateEvent(
#else
    if (ObjectAttributes != nullptr)
    {
        ObjectAttributes->Attributes |= OBJ_KERNEL_HANDLE;
    }
    return ZwCreateEvent(
#endif
            EventHandle, DesiredAccess, ObjectAttributes, EventType, InitialState);
}

NTSTATUS
KNt::QueryVolumeInformationFile(
    __in HANDLE FileHandle,
    __out PIO_STATUS_BLOCK IoStatusBlock,
    __out_bcount(Length) PVOID FsInformation,
    __in ULONG Length,
    __in FS_INFORMATION_CLASS FsInformationClass
    )
{
#if KTL_USER_MODE
    return NtQueryVolumeInformationFile(
#else
    return ZwQueryVolumeInformationFile(
#endif
            FileHandle, IoStatusBlock, FsInformation, Length, FsInformationClass);
}

//** Object Manager API
NTSTATUS
KNt::OpenDirectoryObject(
    __out HANDLE& DirectoryHandle,
    __in ACCESS_MASK DesiredAccess,
    __in OBJECT_ATTRIBUTES& ObjectAttributes)
{
#if KTL_USER_MODE
    return NtOpenDirectoryObject(&DirectoryHandle, DesiredAccess, &ObjectAttributes);
#else
    ObjectAttributes.Attributes |= OBJ_KERNEL_HANDLE;
    return ZwOpenDirectoryObject(&DirectoryHandle, DesiredAccess, &ObjectAttributes);
#endif
}

NTSTATUS
KNt::OpenSymbolicLinkObject(
    __out HANDLE& LinkHandle,
    __in ACCESS_MASK DesiredAccess,
    __in OBJECT_ATTRIBUTES& ObjectAttributes)
{
#if KTL_USER_MODE
    return NtOpenSymbolicLinkObject(&LinkHandle, DesiredAccess, &ObjectAttributes);
#else
    ObjectAttributes.Attributes |= OBJ_KERNEL_HANDLE;
    return ZwOpenSymbolicLinkObject(&LinkHandle, DesiredAccess, &ObjectAttributes);
#endif
}

NTSTATUS
KNt::QuerySymbolicLinkObject(
    __in HANDLE LinkHandle,
    __inout UNICODE_STRING& LinkTarget,
    __out_opt ULONG* ReturnedLength)
{
#if KTL_USER_MODE
    return NtQuerySymbolicLinkObject(LinkHandle, &LinkTarget, ReturnedLength);
#else
    return ZwQuerySymbolicLinkObject(LinkHandle, &LinkTarget, ReturnedLength);
#endif
}

NTSTATUS
KNt::QueryDirectoryObject(
    __in HANDLE DirectoryHandle,
    _Out_writes_bytes_opt_(Length) VOID* Buffer,
    __in ULONG Length,
    __in BOOLEAN ReturnSingleEntry,
    _In_ BOOLEAN RestartScan,
    __inout ULONG& Context,
    __out_opt ULONG* ReturnLength)
{
#if KTL_USER_MODE
    return NtQueryDirectoryObject(DirectoryHandle, Buffer, Length, ReturnSingleEntry, RestartScan, &Context, ReturnLength);
#else
    return ZwQueryDirectoryObject(DirectoryHandle, Buffer, Length, ReturnSingleEntry, RestartScan, &Context, ReturnLength);
#endif
}


ULONGLONG
KNt::GetTickCount64(
    )

/*++

Routine Description:

    This routine returns number of milliseconds that have elapsed since the system was started.

Arguments:

    None.

Return Value:

    The number of milliseconds that have elapsed since the system was started.

--*/

{
#if KTL_USER_MODE
    return ::GetTickCount64();
#else

    LARGE_INTEGER tickCount;

    KeQueryTickCount(&tickCount);

    return ((ULONGLONG) tickCount.QuadPart)*KeQueryTimeIncrement()/10000;

#endif
}


LONGLONG
KNt::GetSystemTime(
    )

/*++

Routine Description:

    This routine returns the current system in UTC format.
    It is the number of 100-nanosecond intervals since January 1, 1601 (UTC).
    System time is typically updated approximately every ten milliseconds.

Arguments:

    None.

Return Value:

    The number of 100-nanosecond intervals since January 1, 1601 (UTC).

--*/

{
    LARGE_INTEGER li;

#if KTL_USER_MODE
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    li.u.LowPart = ft.dwLowDateTime;
    li.u.HighPart = (LONG)ft.dwHighDateTime;
#else
    KeQuerySystemTime(&li);
#endif

    return li.QuadPart;
}

LONGLONG
KNt::GetPerformanceTime(
    )

/*++

Routine Description:

    This routine returns preformance counter adjusted to 100ns increments.

Arguments:

    None.

Return Value:

    Preformance counter

--*/

{
    LARGE_INTEGER perfCount;
    LARGE_INTEGER perfFreq;

    static ULONGLONG Multiplier = 0;

    if (Multiplier == 0)
    {

#if KTL_USER_MODE
        QueryPerformanceFrequency( &perfFreq );
#else
        KeQueryPerformanceCounter( &perfFreq );
#endif
        Multiplier = (256 * 1000 * 1000 * 10i64 )/ perfFreq.QuadPart;
    }

#if KTL_USER_MODE
    QueryPerformanceCounter( &perfCount );
#else
    perfCount = KeQueryPerformanceCounter(nullptr);
#endif
    return((perfCount.QuadPart * Multiplier)/256);
}

#if !defined(PLATFORM_UNIX)
BOOLEAN
KNt::IsWithinStack(
    __in PVOID Address
    )
/*++
 *
 * Routine Description:
 *      This return tests to see if the addres is on the current thread stack.
 *
 * Arguments:
 *      Address - Address to test, it may be zero.
 *
 * Return Value:
 *      Returns true of the address is on the current thread stack.
 *
 * Note:
 *
-*/
{
#if KTL_USER_MODE
    ULONG_PTR StackBase = (ULONG_PTR)(((PNT_TIB)NtCurrentTeb())->StackBase);
    ULONG_PTR StackLimit = (ULONG_PTR)(((PNT_TIB)NtCurrentTeb())->StackLimit);

    if (((ULONG_PTR)Address >= StackLimit) && ((ULONG_PTR) Address < StackBase))
    {
        return(TRUE);
    }

    return(FALSE);
#else
    return(IoWithinStackLimits((ULONG_PTR) Address, 1) != 0);
#endif
}

#endif

VOID
KNt::Sleep(
    __in ULONG Milliseconds
    )
/*++
 *
 * Routine Description:
 *      This routine causes the thread to sleep for the specified number of
 *      milliseconds.  This routine is called with alerts enabled, so it may
 *      terminate early.
 *
 * Arguments:
 *      Millisconds - Specifies the number of milliseconds to wait.
 *
 * Return Value:
 *      None
 *
 * Note:
 *      This routine should not be called by threads that are part of the RVD thread
 *      pool.
 *
-*/

{

#if KTL_USER_MODE

        SleepEx(Milliseconds, TRUE);

#else
         LARGE_INTEGER  WaitTime;

         WaitTime.QuadPart = -(Milliseconds * 10 * 1000I64);

         //
         // Wait with kernel mode to prevent the thread from paging out.  Which is
         // likely to cause problems if there are objects on the stack as part of
         // tests.
         //

         KeDelayExecutionThread( KernelMode, TRUE, &WaitTime);

#endif
}

#if !defined(PLATFORM_UNIX)
#if KTL_USER_MODE

NTSTATUS
KNt::UrlCanonicalize(
    __in PCWSTR pszUrl,
    __out_ecount_part(*pcchCanonicalized, *pcchCanonicalized) PWSTR pszCanonicalized,
    __inout ULONG *pcchCanonicalized,
    __in ULONG dwFlags)
{
    HRESULT hr = ::UrlCanonicalizeW(pszUrl, pszCanonicalized, pcchCanonicalized, dwFlags);
    if (FAILED(hr))
    {
        return STATUS_INVALID_ADDRESS;
    }
    else
    {
        return STATUS_SUCCESS;
    }
}

NTSTATUS
KNt::UrlEscape(
    __in PCWSTR pszUrl,
    __out_ecount_part(*pcchEscaped, *pcchEscaped) PWSTR pszEscaped,
    __inout ULONG *pcchEscaped,
    __in ULONG dwFlags)
{
    HRESULT hr = ::UrlEscapeW(pszUrl, pszEscaped, pcchEscaped, dwFlags);
    if (FAILED(hr))
    {
        // If the pcchEscaped buffer was too small to contain the result, UrlEscapeW returns E_POINTER and the value pointed to by pcchEscaped is set to the required buffer size. 
        if (hr == E_POINTER)
        {
            return STATUS_BUFFER_TOO_SMALL;
        }
        else
        {
            return STATUS_INVALID_ADDRESS;
        }
    }
    else
    {
        return STATUS_SUCCESS;
    }
}

#endif
#endif

