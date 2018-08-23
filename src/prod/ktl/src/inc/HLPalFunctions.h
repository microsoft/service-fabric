 /*++

Module Name:

    HLPalFunctions.h

Abstract:

    This file contains the definitions for high level pal functionality

Author:

    Alan Warwick

Environment:

    Kernel mode and User mode

Notes:

Revision History:

--*/



//
// Flags for NtCreateFileForBlockFile. These must match the flags in
// KXMApi.h named KXMFLAG_
//
#define NTCF_FLAG_USE_FILE_SYSTEM_ONLY   1  // All IO must go via file system apis
#define NTCF_FLAG_AVOID_FILE_SYSTEM      2  // All IO must not go via file system apis

class HLPalFunctions
{
    public:
        static
        NTSTATUS
        CreateSecurityDescriptor(
            __out SECURITY_DESCRIPTOR& SecurityDescriptor,
            __out ACL*& Acl,
            __in KAllocator& Allocator
            );

#if KTL_USER_MODE
        static
        BOOL
        ManageVolumePrivilege(
            );

        static
        NTSTATUS
        AdjustExtentTable(
            __in HANDLE Handle,
            __in ULONGLONG OldFileSize,
            __in ULONGLONG NewFileSize,
            __in KAllocator& Allocator
            );
#endif
        
        static
        NTSTATUS
        SetValidDataLength(
            __in HANDLE Handle,
            __in LONGLONG OldEndOfFileOffset,
            __in LONGLONG ValidDataLength,
            __in KAllocator& Allocator
            );

        static
        NTSTATUS
        SetFileSize(
            HANDLE Handle,
            BOOLEAN IsSparseFile,
            ULONG BlockSize,
            ULONGLONG OldFileSize,
            ULONGLONG NewFileSize,
            KAllocator& Allocator
        );
        
#if KTL_USER_MODE
        typedef struct
        {
            PVOID buf;
            ULONG npages;
        } ALIGNED_BUFFER;

        static
        BOOL
        ReadFileScatterPal(
            __in HANDLE Handle,
            __in ULONG NumberAlignedBuffers,
            __in ALIGNED_BUFFER* AlignedBuffers,
            __in DWORD  NumberOfBytesToRead,
            __in LPOVERLAPPED Overlapped,
            __in ULONG PageSize                     
        );

        static
        BOOL
        WriteFileGatherPal(
            __in HANDLE Handle,
            __in ULONG NumberAlignedBuffers,
            __in ALIGNED_BUFFER* AlignedBuffers,
            __in DWORD  NumberOfBytesToRead,
            __in LPOVERLAPPED Overlapped,
            __in ULONG PageSize                     
        );
#endif      
        
        static
        NTSTATUS
        NtCreateFileForKBlockFile(
            PHANDLE FileHandle,
            ACCESS_MASK DesiredAccess,
            POBJECT_ATTRIBUTES ObjectAttributes,
            PIO_STATUS_BLOCK IoStatusBlock,
            PLARGE_INTEGER AllocationSize,
            ULONG FileAttributes,
            ULONG ShareAccess,
            ULONG CreateDisposition,
            ULONG CreateOptions,
            PVOID EaBuffer,
            ULONG EaLength,
            ULONG CreateFlags
            );

        static
        NTSTATUS SetUseFileSystemOnlyFlag(
            __in HANDLE Handle,
            __in BOOLEAN UseFileSystemOnly
            );

        static
        NTSTATUS RenameFile(
            __in PWCHAR FromPathName,
            __in PWCHAR ToPathName,
            __in ULONG ToPathNameLength,
            __in BOOLEAN OverwriteIfExists,
            __in KAllocator& Allocator                                  
        );        
};
