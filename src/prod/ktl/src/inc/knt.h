/*++

Copyright (c) 2010  Microsoft Corporation

Module Name:

    knt.h

Abstract:

    This file defines a KNt class for a user/kernel agnostic way of calling some of the NT API set.

Author:

    Norbert P. Kusters (norbertk) 17-Nov-2010

Environment:

    Kernel mode and User mode

Notes:

--*/

class KNt {

    public:

        static
        NTSTATUS
        CreateFile(
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
            );

        static
        NTSTATUS
        OpenFile(
            __out HANDLE* FileHandle,
            __in ACCESS_MASK DesiredAccess,
            __in OBJECT_ATTRIBUTES* ObjectAttributes,
            __out IO_STATUS_BLOCK* IoStatusBlock,
            __in ULONG ShareAccess,
            __in ULONG OpenOptions
            );

        static
        NTSTATUS
        DeleteFile(
            __in_z PCWCHAR FileName
            );

        static
        NTSTATUS
        Close(
            __in HANDLE Handle
            );

        // Summary:
        // Abstracts User/Kernel Mode for Nt/ZwCreateSection;
        // MCoskun:
        // As part of adding Linux support,
        // we will want to consider whether KNt APIs will keep taking OBJECT_ATTRIBUTES
        // or take each attribute (objectname, rootdirectory, security description) explicitly.
        static
        NTSTATUS
        CreateSection(
            __out    HANDLE&                    SectionHandle,
            __in     ACCESS_MASK                DesiredAccess,
            __in_opt OBJECT_ATTRIBUTES* const   ObjectAttributes,
            __in_opt LARGE_INTEGER* const       MaximumSize,
            __in     ULONG                      SectionPageProtection,
            __in     ULONG                      AllocationAttributes,
            __in_opt HANDLE const               FileHandle
            );

        // Summary:
        // Abstracts User/Kernel Mode for Nt/ZwMapViewOfSection;
        // MCoskun:
        // Note that explicit decision has been made to hide process handle for making
        // Linux port simpler.
        static
        NTSTATUS
        MapViewOfSection(
            __in        HANDLE                  SectionHandle,
            __inout     VOID*&                  BaseAddress,
            __in        ULONG_PTR               ZeroBits,
            __in        SIZE_T                  CommitSize,
            __inout_opt LARGE_INTEGER* const    SectionOffset,
            __inout     SIZE_T&                 ViewSize,
            __in        SECTION_INHERIT         InheritDisposition,
            __in        ULONG                   AllocationType,
            __in        ULONG                   Win32Protect
            );

        // Summary:
        // Abstracts User/Kernel Mode for Nt/ZwUnmapViewOfSection;
        // MCoskun:
        // Note that explicit decision has been made to hide process handle for making
        // Linux port simpler.
        static
        NTSTATUS
        UnmapViewOfSection(
            __in_opt VOID* const        BaseAddress
            );

        // Summary:
        // Abstracts User/Kernel Mode for Nt/ZwFlushVirtualMemory;
        static
        NTSTATUS
        FlushVirtualMemory(
            __inout VOID*&              BaseAddress,
            __inout SIZE_T&             RegionSize,
            __out   IO_STATUS_BLOCK&    IoStatus
            );

        static
        NTSTATUS
        QueryInformationFile(
            __in HANDLE FileHandle,
            __out IO_STATUS_BLOCK* IoStatusBlock,
            __out_bcount(Length) VOID* FileInformation,
            __in ULONG Length,
            __in FILE_INFORMATION_CLASS FileInformationClass
            );

        static
        NTSTATUS
        SetInformationFile(
            __in HANDLE FileHandle,
            __out IO_STATUS_BLOCK* IoStatusBlock,
            __in_bcount(Length) VOID* FileInformation,
            __in ULONG Length,
            __in FILE_INFORMATION_CLASS FileInformationClass
            );

        static
        NTSTATUS
        FsControlFile(
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
            );

        static
        NTSTATUS
        DeviceIoControlFile(
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
            );

        static
        NTSTATUS
        WriteFile(
            __in HANDLE FileHandle,
            __in_opt HANDLE Event,
            __in_opt PIO_APC_ROUTINE ApcRoutine,
            __in_opt VOID* ApcContext,
            __out IO_STATUS_BLOCK* IoStatusBlock,
            __in_bcount(Length) VOID* Buffer,
            __in ULONG Length,
            __in_opt LARGE_INTEGER* ByteOffset,
            __in_opt ULONG* Key
            );

        static
        NTSTATUS
        ReadFile(
            __in HANDLE FileHandle,
            __in_opt HANDLE Event,
            __in_opt PIO_APC_ROUTINE ApcRoutine,
            __in_opt VOID* ApcContext,
            __out IO_STATUS_BLOCK* IoStatusBlock,
            __in_bcount(Length) VOID* Buffer,
            __in ULONG Length,
            __in_opt LARGE_INTEGER* ByteOffset,
            __in_opt ULONG* Key
            );

        static
        NTSTATUS
        FlushBuffersFile(
            __in HANDLE FileHandle,
            __out IO_STATUS_BLOCK* IoStatusBlock
            );

        static
        NTSTATUS
        WaitForSingleObject(
            __in HANDLE Handle,
            __in BOOLEAN Alertable,
            __in_opt LARGE_INTEGER* Timeout
            );

        static
        NTSTATUS
        QueryDirectoryFile(
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
            );

        static
        NTSTATUS
        QuerySystemInformation(
            __in SYSTEM_INFORMATION_CLASS SystemInformationClass,
            __out_bcount_part_opt(SystemInformationLength, *ReturnLength) VOID* SystemInformation,
            __in ULONG SystemInformationLength,
            __out_opt ULONG* ReturnLength
            );

        static
        NTSTATUS
        CreateEvent(
            __out HANDLE* EventHandle,
            __in ACCESS_MASK DesiredAccess,
            __in_opt OBJECT_ATTRIBUTES* ObjectAttributes,
            __in EVENT_TYPE EventType,
            __in BOOLEAN InitialState
            );

        static
        NTSTATUS
        QueryVolumeInformationFile(
            __in HANDLE FileHandle,
            __out PIO_STATUS_BLOCK IoStatusBlock,
            __out_bcount(Length) PVOID FsInformation,
            __in ULONG Length,
            __in FS_INFORMATION_CLASS FsInformationClass
            );

        static
        NTSTATUS
        QueryFullAttributesFile(
            __in OBJECT_ATTRIBUTES& ObjectAttributes,
            __out FILE_NETWORK_OPEN_INFORMATION& Result
            );

        static
        NTSTATUS
        OpenDirectoryObject(
            __out HANDLE& DirectoryHandle,
            __in ACCESS_MASK DesiredAccess,
            __in OBJECT_ATTRIBUTES& ObjectAttributes
            );

        static
        NTSTATUS
        OpenSymbolicLinkObject(
            __out HANDLE& LinkHandle,
            __in ACCESS_MASK DesiredAccess,
            __in OBJECT_ATTRIBUTES& ObjectAttributes
            );

        static
        NTSTATUS
        QuerySymbolicLinkObject(
            __in HANDLE LinkHandle,
            __inout UNICODE_STRING& LinkTarget,
            __out_opt ULONG* ReturnedLength
            );

        static
        NTSTATUS
        QueryDirectoryObject(
            __in HANDLE DirectoryHandle,
            _Out_writes_bytes_opt_(Length) VOID* Buffer,
            __in ULONG Length,
            __in BOOLEAN ReturnSingleEntry,
            _In_ BOOLEAN RestartScan,
            __inout ULONG& Context,
            __out_opt ULONG* ReturnLength
            );


        static
        ULONGLONG
        GetTickCount64(
            );


        static
        LONGLONG
        GetSystemTime(
            );

        static
        LONGLONG
        GetPerformanceTime(
            );

        static
        BOOLEAN
        IsWithinStack(
            __in PVOID Address
            );

        static
        VOID
        Sleep(
            __in ULONG Milliseconds
            );

#if KTL_USER_MODE

        static
        NTSTATUS
        UrlCanonicalize(
            __in PCWSTR pszUrl,
            __out_ecount_part(*pcchCanonicalized, *pcchCanonicalized) PWSTR pszCanonicalized,
            __inout ULONG *pcchCanonicalized,
            __in ULONG flags);

        static
        NTSTATUS
        UrlEscape(
            __in PCWSTR pszUrl,
            __out_ecount_part(*pcchEscaped, *pcchEscaped) PWSTR pszEscaped,
            __inout ULONG *pcchEscaped,
            __in ULONG flags);

#endif
};
