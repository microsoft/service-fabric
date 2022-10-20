/*++

Module Name:

    winpal.cpp

Abstract:

    This file contains the windows specific implementations for
    platform independent functions. 

Author:

    Alan Warwick

Environment:

    Kernel mode and User mode

Notes:

Revision History:

--*/


#include <ktl.h>
#include <ktrace.h>
#include <ntdddisk.h>

#if KTL_USER_MODE
#else
#include <mountmgr.h>
#endif

#include <HLPalFunctions.h>

NTSTATUS HLPalFunctions::CreateSecurityDescriptor(
    __out SECURITY_DESCRIPTOR& SecurityDescriptor,
    __out ACL*& Acl,
    __in KAllocator& Allocator
    )
{
    static const SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    NTSTATUS status;
    ULONG aclLength;
    SID adminSid[2];

    Acl = NULL;

    status = RtlCreateSecurityDescriptor(&SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);

    if (!NT_SUCCESS(status)) {
        KTraceFailedAsyncRequest(status, NULL, 0, 0);
        goto Finish;
    }

    adminSid->Revision = SID_REVISION;
    adminSid->SubAuthorityCount = 2;
    adminSid->IdentifierAuthority = ntAuthority;
    adminSid->SubAuthority[0] = SECURITY_BUILTIN_DOMAIN_RID;
    adminSid->SubAuthority[1] = DOMAIN_ALIAS_RID_ADMINS;

    aclLength = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) + RtlLengthSid(adminSid) - sizeof(ULONG);

    Acl = (ACL*) _newArray<UCHAR>(KTL_TAG_FILE, Allocator, aclLength);

    if (!Acl) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceFailedAsyncRequest(status, NULL, 0, aclLength);
        goto Finish;
    }

    status = RtlCreateAcl(Acl, aclLength, ACL_REVISION);

    if (!NT_SUCCESS(status)) {
        KTraceFailedAsyncRequest(status, NULL, 0, aclLength);
        goto Finish;
    }

    status = RtlAddAccessAllowedAce(Acl, ACL_REVISION, FILE_ALL_ACCESS, adminSid);

    if (!NT_SUCCESS(status)) {
        KTraceFailedAsyncRequest(status, NULL, 0, 0);
        goto Finish;
    }

    status = RtlSetDaclSecurityDescriptor(&SecurityDescriptor, TRUE, Acl, FALSE);

    if (!NT_SUCCESS(status)) {
        KTraceFailedAsyncRequest(status, NULL, 0, 0);
        goto Finish;
    }

Finish:

    if (!NT_SUCCESS(status)) {
        _delete(Acl);
        Acl = NULL;
    }

    return status;
}

#if KTL_USER_MODE
BOOL HLPalFunctions::ManageVolumePrivilege(
    )
{
    BOOL b;
    HANDLE hThread;
    HANDLE hProcess;
    TOKEN_PRIVILEGES tokenPrivileges;
    TOKEN_PRIVILEGES oldTokenPrivileges;
    DWORD oldPrivilegesLength;

    b = OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, TRUE, &hThread);
    if (!b) {
        ULONG error = GetLastError();
        if (error != ERROR_NO_TOKEN) {
            KTraceFailedAsyncRequest(error, NULL, 0, 0);
            return b;
        }

        b = OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE, &hProcess);
        if (!b) {
            KTraceFailedAsyncRequest(GetLastError(), NULL, 0, 0);
            return b;
        }

        b = DuplicateTokenEx(hProcess, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY | TOKEN_IMPERSONATE, NULL, SecurityImpersonation,
                TokenImpersonation, &hThread);
        if (!b) {
            KTraceFailedAsyncRequest(GetLastError(), NULL, 0, 0);
            CloseHandle(hProcess);
            return b;
        }

        b = SetThreadToken(NULL, hThread);
        if (!b) {
            KTraceFailedAsyncRequest(GetLastError(), NULL, 0, 0);
            CloseHandle(hProcess);
            CloseHandle(hThread);
            return b;
        }

        CloseHandle(hProcess);
    }

    ZeroMemory(&tokenPrivileges, sizeof(tokenPrivileges));

    tokenPrivileges.PrivilegeCount = 1;
    tokenPrivileges.Privileges[0].Luid.LowPart = SE_MANAGE_VOLUME_PRIVILEGE;
    tokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    b = AdjustTokenPrivileges(hThread, FALSE, &tokenPrivileges, sizeof(tokenPrivileges), &oldTokenPrivileges,
            &oldPrivilegesLength);

    CloseHandle(hThread);

    return b;
    
}

NTSTATUS HLPalFunctions::AdjustExtentTable(
    __in HANDLE Handle,
    __in ULONGLONG OldFileSize,
    __in ULONGLONG NewFileSize,
    __in KAllocator& Allocator
    )
{
    UNREFERENCED_PARAMETER(Handle);
    UNREFERENCED_PARAMETER(OldFileSize);
    UNREFERENCED_PARAMETER(NewFileSize);
    UNREFERENCED_PARAMETER(Allocator);

    return STATUS_SUCCESS;
}
#endif

NTSTATUS HLPalFunctions::SetValidDataLength(
    __in HANDLE Handle,
    __in LONGLONG OldEndOfFileOffset,
    __in LONGLONG ValidDataLength,
    __in KAllocator& Allocator
    )
{
    UNREFERENCED_PARAMETER(OldEndOfFileOffset);
    UNREFERENCED_PARAMETER(Allocator);
    
    NTSTATUS status;    
    IO_STATUS_BLOCK ioStatus;
    FILE_VALID_DATA_LENGTH_INFORMATION validDataLength;

    //
    // We assume we are not running on a KTL thread and so can do
    // blocking IO calls
    //  
    validDataLength.ValidDataLength.QuadPart = ValidDataLength; 
    status = KNt::SetInformationFile(Handle, &ioStatus, &validDataLength, sizeof(validDataLength),
        FileValidDataLengthInformation);

    if (!NT_SUCCESS(status)) {
        KTraceFailedAsyncRequest(status, NULL, OldEndOfFileOffset, ValidDataLength);
    }

    return status;
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
    UNREFERENCED_PARAMETER(Allocator);
    
    UCHAR* buffer = NULL;
    HANDLE eventHandle = NULL;
    FILE_ALLOCATION_INFORMATION allocInfo;
    FILE_END_OF_FILE_INFORMATION endOfFile;
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatus;
    LARGE_INTEGER offset;
    FILE_VALID_DATA_LENGTH_INFORMATION validDataLength;

    //
    // Round the size up to the nearest block multiple.
    //

    endOfFile.EndOfFile.QuadPart = ((NewFileSize + BlockSize - 1)/BlockSize)*BlockSize;

    //
    // If the prior file size was zero, then zero the first block of the file so that growing the valid data length doesn't
    // "find" a prior format.
    //

    if (! OldFileSize && NewFileSize) {

        //
        // Set the allocation length, first, so that we can safely zero the first block without changing the size and
        // possibly exposing uninitialized data on the first block.
        //

        allocInfo.AllocationSize.QuadPart = endOfFile.EndOfFile.QuadPart;

        status = KNt::SetInformationFile(Handle, &ioStatus, &allocInfo, sizeof(allocInfo), FileAllocationInformation);

        if (!NT_SUCCESS(status)) {
            KTraceFailedAsyncRequest(status, nullptr, NewFileSize, allocInfo.AllocationSize.QuadPart);
            goto Finish;
        }

        buffer = _newArray<UCHAR>(KTL_TAG_FILE, Allocator, BlockSize);

        if (!buffer) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Finish;
        }

        RtlZeroMemory(buffer, BlockSize);

        status = KNt::CreateEvent(&eventHandle, EVENT_ALL_ACCESS, NULL, NotificationEvent, FALSE);

        if (!NT_SUCCESS(status)) {
            eventHandle = NULL;
            KTraceFailedAsyncRequest(status, nullptr, (ULONGLONG)Handle, NewFileSize);
            goto Finish;
        }

        offset.QuadPart = 0;

        status = KNt::WriteFile(Handle, eventHandle, NULL, NULL, &ioStatus, buffer, BlockSize, &offset, NULL);

        if (status == STATUS_PENDING) {
            KNt::WaitForSingleObject(eventHandle, FALSE, NULL);
            status = ioStatus.Status;
        }

        if (!NT_SUCCESS(status)) {
            KTraceFailedAsyncRequest(status, nullptr, (ULONGLONG)Handle, offset.QuadPart);
            goto Finish;
        }
    }

    //
    // We are in the context of a system worker thread where we can perform blocking operations.  Set the file size,
    // and valid data length for this file.
    //

    status = KNt::SetInformationFile(Handle, &ioStatus, &endOfFile, sizeof(endOfFile), FileEndOfFileInformation);

    if (!NT_SUCCESS(status)) {
        KTraceFailedAsyncRequest(status, nullptr, (ULONGLONG)Handle, endOfFile.EndOfFile.HighPart);
        goto Finish;
    }

    if (IsSparseFile)
    {
        //
        // Make sparse only the newly allocated part of the file
        //
        if (NewFileSize > OldFileSize)
        {
            HANDLE eventHandle1 = NULL;
            status = KNt::CreateEvent(&eventHandle1, EVENT_ALL_ACCESS, NULL, NotificationEvent, FALSE);

            if (!NT_SUCCESS(status)) {
                KTraceFailedAsyncRequest(status, nullptr, (ULONGLONG)Handle, NewFileSize);
                eventHandle1 = NULL;
                goto Finish;
            }

            FILE_ZERO_DATA_INFORMATION fzInfo;
            fzInfo.FileOffset.QuadPart = OldFileSize;
            fzInfo.BeyondFinalZero.QuadPart = NewFileSize+1;
            status = KNt::FsControlFile(Handle, eventHandle1, NULL, NULL, &ioStatus,
                FSCTL_SET_ZERO_DATA,
                &fzInfo, sizeof(fzInfo),
                NULL, 0);

            if (status == STATUS_PENDING) {
                KNt::WaitForSingleObject(eventHandle1, FALSE, NULL);
                status = ioStatus.Status;
            }

            KNt::Close(eventHandle1);
            eventHandle1 = NULL;

            if (!NT_SUCCESS(status)) {
                KTraceFailedAsyncRequest(status, nullptr, OldFileSize, NewFileSize);
                KTraceFailedAsyncRequest(status, nullptr, fzInfo.FileOffset.QuadPart, fzInfo.BeyondFinalZero.QuadPart);
                goto Finish;
            }
        }
    } else {
        //
        // Set the valid data length.
        //
        validDataLength.ValidDataLength.QuadPart = NewFileSize;

        status = KNt::SetInformationFile(Handle, &ioStatus, &validDataLength, sizeof(validDataLength),
                FileValidDataLengthInformation);

#if KTL_USER_MODE
        if (status == STATUS_PRIVILEGE_NOT_HELD || status == STATUS_NOT_SUPPORTED)
        {
            //
            // Allow underpriveged processes to move forward
            // without preallocation
            //
            status = STATUS_SUCCESS;
        }
#endif
        
        if (!NT_SUCCESS(status)) {
            KTraceFailedAsyncRequest(status, nullptr, (ULONGLONG)Handle, validDataLength.ValidDataLength.QuadPart);
            goto Finish;
        }
    }

Finish:

    //
    // Cleanup and complete the async request.
    //

    _deleteArray(buffer);

    if (eventHandle) {
        KNt::Close(eventHandle);
    }

    return(status);
}

#if KTL_USER_MODE

//
// Note that MAX_FILE_SEGMENT_ELEMENT must match KBlockFile::MaxBlocksPerIo
//
#define MAX_FILE_SEGMENT_ELEMENT 256
#define KTL_TAG_WORKBUFFERS 'PIWB'

BOOL
HLPalFunctions::ReadFileScatterPal(
    __in HANDLE Handle,
    __in ULONG NumberAlignedBuffers,
    __in HLPalFunctions::ALIGNED_BUFFER* AlignedBuffers,
    __in DWORD  NumberOfBytesToRead,
    __in LPOVERLAPPED Overlapped,
    __in ULONG PageSize
)
{    
    BOOL b;
    FILE_SEGMENT_ELEMENT* fileSegmentElements;
    ULONG index;
    ULONG numberBytes;
    PUCHAR buf;
    PUCHAR workBuffer;
    ULONG workBufferUsed;

    workBufferUsed = MAX_FILE_SEGMENT_ELEMENT * sizeof(FILE_SEGMENT_ELEMENT);
    workBuffer = _new(KTL_TAG_WORKBUFFERS, KtlSystemCoreImp::TryGetDefaultKtlSystemCore()->PagedAllocator())
                        UCHAR[workBufferUsed];
    if (workBuffer == nullptr)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        Overlapped->Internal = (ULONG_PTR)STATUS_INSUFFICIENT_RESOURCES;
        KTraceFailedAsyncRequest(STATUS_INSUFFICIENT_RESOURCES, NULL, 0, 0);
        return(FALSE);      
    }

    KFinally([&]() {
        _delete(workBuffer);
    });
    
    
    fileSegmentElements = (FILE_SEGMENT_ELEMENT*)workBuffer;    
    index = 0;
    numberBytes = 0;
    for (ULONG i = 0; i < NumberAlignedBuffers; i++)
    {
        buf = (PUCHAR)AlignedBuffers[i].buf;
        for (ULONG j = 0; j < AlignedBuffers[i].npages; j++)
        {
            fileSegmentElements[index].Buffer = buf;
            index++;
            buf += PageSize;
            numberBytes += PageSize;
        }
    }

    KInvariant(numberBytes == NumberOfBytesToRead);
    KInvariant(index <= MAX_FILE_SEGMENT_ELEMENT);

    b = ::ReadFileScatter(Handle,
                            fileSegmentElements,
                            NumberOfBytesToRead,
                            NULL,
                            Overlapped);
    
    return(b);
}

BOOL
HLPalFunctions::WriteFileGatherPal(
    __in HANDLE Handle,
    __in ULONG NumberAlignedBuffers,
    __in HLPalFunctions::ALIGNED_BUFFER* AlignedBuffers,
    __in DWORD  NumberOfBytesToWrite,
    __in LPOVERLAPPED Overlapped,
    __in ULONG PageSize
)
{
    BOOL b;
    ULONG index;
    ULONG numberBytes;
    PUCHAR buf;
    PUCHAR workBuffer;
    ULONG workBufferUsed;
    FILE_SEGMENT_ELEMENT* fileSegmentElements;

    workBufferUsed = MAX_FILE_SEGMENT_ELEMENT * sizeof(FILE_SEGMENT_ELEMENT);
    workBuffer = _new(KTL_TAG_WORKBUFFERS, KtlSystemCoreImp::TryGetDefaultKtlSystemCore()->PagedAllocator())
                        UCHAR[workBufferUsed];
    if (workBuffer == nullptr)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        Overlapped->Internal = (ULONG_PTR)STATUS_INSUFFICIENT_RESOURCES;
        KTraceFailedAsyncRequest(STATUS_INSUFFICIENT_RESOURCES, NULL, 0, 0);
        return(FALSE);      
    }

    KFinally([&]() {
        _delete(workBuffer);
    });
    
    
    fileSegmentElements = (FILE_SEGMENT_ELEMENT*)workBuffer;    

    index = 0;
    numberBytes = 0;
    for (ULONG i = 0; i < NumberAlignedBuffers; i++)
    {
        buf = (PUCHAR)AlignedBuffers[i].buf;
        for (ULONG j = 0; j < AlignedBuffers[i].npages; j++)
        {
            fileSegmentElements[index].Buffer = buf;
            index++;
            buf += PageSize;
            numberBytes += PageSize;
        }
    }

    KInvariant(numberBytes == NumberOfBytesToWrite);
    KInvariant(index <= MAX_FILE_SEGMENT_ELEMENT);

    b = ::WriteFileGather(Handle,
                            fileSegmentElements,
                            NumberOfBytesToWrite,
                            NULL,
                            Overlapped);
    
    return(b);
}
#endif

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
    UNREFERENCED_PARAMETER(Flags);
    
    return KNt::CreateFile(FileHandle,
                              DesiredAccess,
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

NTSTATUS HLPalFunctions::SetUseFileSystemOnlyFlag(
    __in HANDLE Handle,
    __in BOOLEAN UseFileSystemOnly
    )
{
    UNREFERENCED_PARAMETER(UseFileSystemOnly);
    UNREFERENCED_PARAMETER(Handle);
    
    return(STATUS_SUCCESS);
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
    HANDLE sourceHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatus;
    UNICODE_STRING sourceFileName;
    FILE_RENAME_INFORMATION* fileRenameInformation;
    ULONG sizeNeeded;

    RtlInitUnicodeString(&sourceFileName, FromPathName);
    InitializeObjectAttributes(&objectAttributes, &sourceFileName, OBJ_CASE_INSENSITIVE, NULL, NULL);
    
    status =  KNt::CreateFile(&sourceHandle,
                           DELETE,
                           &objectAttributes,
                           &ioStatus,
                           NULL,
                           FILE_ATTRIBUTE_NORMAL,
                           0,                       // ShareAccess
                           FILE_OPEN,
                           0,                       // CreateOptions
                           NULL,
                           0);

    if (! NT_SUCCESS(status))
    {
        return(status);
    }
    KFinally([&]() {
        KNt::Close(sourceHandle);
    });

    sizeNeeded = sizeof(FILE_RENAME_INFORMATION) + ToPathNameLength;    
    fileRenameInformation = (FILE_RENAME_INFORMATION*)_new(KTL_TAG_TEST,
                                                           Allocator) UCHAR[sizeNeeded];

    if (fileRenameInformation == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        return(status);
    }
    KFinally([&]() {
        _delete(fileRenameInformation);
    });
    RtlZeroMemory(fileRenameInformation, sizeNeeded);

    fileRenameInformation->ReplaceIfExists = OverwriteIfExists;
    fileRenameInformation->RootDirectory = nullptr;
    fileRenameInformation->FileNameLength = ToPathNameLength;
    KMemCpySafe(fileRenameInformation->FileName, ToPathNameLength,
                ToPathName, ToPathNameLength);

    status = KNt::SetInformationFile(sourceHandle,
                                     &ioStatus,
                                     fileRenameInformation,
                                     sizeNeeded,
                                     FileRenameInformation);
    return(status);
}

                
