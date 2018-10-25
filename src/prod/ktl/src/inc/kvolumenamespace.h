/*++

Copyright (c) 2010  Microsoft Corporation

Module Name:

    kvolumenamespace.h

Abstract:

    This file defines a KVolumeNamespace class for navigating the local file systems.

Author:

    Norbert P. Kusters (norbertk) 14-Nov-2010

Environment:

    Kernel mode and User mode

Notes:

--*/
#pragma once

//
// NOTE: Volume guids and driver letters are a Windows concept and not used for Linux.
//       A number of apis and data structures related to mapping between them and formatting
//       filenames for NT are #ifdef out. On Linux, filename are able to be used "as-is"
//
//       Hardlink support is also not available on Linix  (WHY ???)
//


class KVolumeNamespace
{
public:

    //
    // Helpful definitions for the path separator character as a string or char
    //
    static const WCHAR* PathSeparator;
    static const WCHAR  PathSeparatorChar;
    
    typedef KArray<KWString> NameArray;
    
#if !defined(PLATFORM_UNIX)
    struct VolumeInformation
    {
        GUID VolumeId;
        BOOLEAN IsStableId;
        UCHAR DriveLetter;
    };

    typedef KArray<GUID> VolumeIdArray;
    typedef KArray<VolumeInformation> VolumeInformationArray;

    static
    NTSTATUS
    QueryVolumeList(
        __out VolumeIdArray& VolumeList,
        __in KAllocator& Allocator,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = NULL
        );

#if defined(K_UseResumable)
    static
    ktl::Awaitable<NTSTATUS>
    QueryVolumeListAsync(
        __out VolumeIdArray& VolumeList,
        __in KAllocator& Allocator,
        __in_opt KAsyncContextBase* const ParentAsync = NULL
        );
#endif  
    
    static
    NTSTATUS
    QueryVolumeListEx(
        __out VolumeInformationArray& VolumeInformationList,
        __in KAllocator& Allocator,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = NULL
        );

#if defined(K_UseResumable) 
    static
    ktl::Awaitable<NTSTATUS>
    QueryVolumeListExAsync(
        __out VolumeInformationArray& VolumeInformationList,
        __in KAllocator& Allocator,
        __in_opt KAsyncContextBase* const ParentAsync = NULL
        );
#endif  

    static
    BOOLEAN
    QueryVolumeIdFromDriveLetter(
        __in VolumeInformationArray& VolumeInformationList,
        __in UCHAR DriveLetter,
        __out GUID& VolumeId
        );

    static
    NTSTATUS
    QueryVolumeIdFromRootDirectoryName(
        __in KWString& FullyQualifiedRootDirectoryName,
        __in KAllocator& Allocator,
        __out GUID& VolumeId,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = NULL
        );

#if defined(K_UseResumable) 
    static
    ktl::Awaitable<NTSTATUS>
    QueryVolumeIdFromRootDirectoryNameAsync(
        __in KWString& FullyQualifiedRootDirectoryName,
        __in KAllocator& Allocator,
        __out GUID& VolumeId,
        __in_opt KAsyncContextBase* const ParentAsync = NULL
        );
#endif

    static
    NTSTATUS
    CreateFullyQualifiedRootDirectoryName(
        __in const GUID& VolumeId,
        __out KWString& FullyQualifiedRootDirectoryName
        );

    //** Convert a fully qualified volume name to corresponding "\GLOBAL??\volume{guid}" form
    //
    //  Parameters:
    //      FullyQualifiedVolumeDirectoryName - FQ Name of the volume - Examples:
    //
    //          "\??\C:"
    //          "\global??\c:"
    //          "\GLOBAL??\Volume{d498b863-8a6d-44f9-b040-48eab3b42b02}"
    //
    //      GlobalDirectoryName - Resulting normalized form of the volume directory name in the form of:
    //
    //          "\GLOBAL??\Volume{d498b863-8a6d-44f9-b040-48eab3b42b02}"
    //
    static
    NTSTATUS
    QueryGlobalVolumeDirectoryName(
        __in KWString& FullyQualifiedVolumeDirectoryName,
        __in KAllocator& Allocator,
        __out KWString& GlobalDirectoryName,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = NULL
        );

#if defined(K_UseResumable) 
    static
    ktl::Awaitable<NTSTATUS>
    QueryGlobalVolumeDirectoryNameAsync(
        __in KWString& FullyQualifiedVolumeDirectoryName,
        __in KAllocator& Allocator,
        __out KWString& GlobalDirectoryName,
        __in_opt KAsyncContextBase* const ParentAsync = NULL
        );    
#endif
    
#endif
    
    static
    NTSTATUS
    CreateFullyQualifiedName(
        __in KWString& FullyQualifiedDiretoryName,
        __in KWString& Name,
        __out KWString& FullyQualifiedName
        );

    //** Split and normalize the supplied Windows Object path into it's Root and Root-relative parts
    //
    //  Parameters:
    //      FullyQualifiedObjectPath - FQN or volume rooted (e.g. "\Global??\..." or "C:\..." or "\device\..."
    //      FullyQualifiedObjectRoot - resulting normalized root portion of FullyQualifiedObjectPath
    //      RootRelativePath - Remainder of FullyQualifiedObjectPath after FullyQualifiedObjectRoot
    //
    //  Returns:
    //      STATUS_SUCCESS - valid path components of FullyQualifiedObjectPath have been returned
    //      STATUS_INVALID_PARAMETER - FullyQualifiedDiretoryName's format is invalid
    //
    static
    NTSTATUS
    SplitAndNormalizeObjectPath(
        __in KWString& FullyQualifiedObjectPath,
        __out KWString& FullyQualifiedObjectRoot,
        __out KWString& RootRelativePath
        );

    static
    NTSTATUS
    SplitAndNormalizeObjectPath(
        __in KStringView& FullyQualifiedObjectPath,
        __out KWString& FullyQualifiedObjectRoot,
        __out KWString& RootRelativePath
        );


    //** Split and normalize the supplied Windows Object path into it's
    //** full pathname and filename components
    //
    //  Parameters:
    //      FullyQualifiedObjectPath - FQN or volume rooted (e.g. "\Global??\..." or "C:\..." or "\device\..."
    //                                 or relative path (e.g. \foo\bar\baz)
    //      Path - resulting path to the file
    //      Filename - resulting filename for file
    //
    //  Returns:
    //      STATUS_SUCCESS - valid path components of FullyQualifiedObjectPath have been returned
    //      STATUS_INVALID_PARAMETER - FullyQualifiedDiretoryName's format is invalid
    //
    static NTSTATUS
    SplitObjectPathInPathAndFilename(
        __in KAllocator& Allocator,
        __in KWString& FullyQualifiedObjectPath,
        __out KWString& Path,
        __out KWString& Filename
       );

    static NTSTATUS
    SplitObjectPathInPathAndFilename(
        __in KAllocator& Allocator,
        __in KStringView& FullyQualifiedObjectPath,
        __out KWString& Path,
        __out KWString& Filename
       );
    
    static
    NTSTATUS
    CreateDirectory(
        __in KWString& FullyQualifiedDirectoryName,
        __in KAllocator& Allocator,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = NULL
        );

#if defined(K_UseResumable) 
    static
    ktl::Awaitable<NTSTATUS>
    CreateDirectoryAsync(
        __in KWString& FullyQualifiedDirectoryName,
        __in KAllocator& Allocator,
        __in_opt KAsyncContextBase* const ParentAsync = NULL
        );
	
    static
    ktl::Awaitable<NTSTATUS>
    CreateDirectoryAsync(
        __in LPCWSTR FullyQualifiedDirectoryName,
        __in KAllocator& Allocator,
        __in_opt KAsyncContextBase* const ParentAsync = NULL
        );
#endif
    
    static
    NTSTATUS
    DeleteFileOrDirectory(
        __in KWString& FullyQualifiedDirectoryName,
        __in KAllocator& Allocator,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = NULL
        );

#if defined(K_UseResumable) 
    static
    ktl::Awaitable<NTSTATUS>
    DeleteFileOrDirectoryAsync(
        __in KWString& FullyQualifiedDirectoryName,
        __in KAllocator& Allocator,
        __in_opt KAsyncContextBase* const ParentAsync = NULL
        );
	
    static
    ktl::Awaitable<NTSTATUS>
    DeleteFileOrDirectoryAsync(
        __in LPCWSTR FullyQualifiedDirectoryName,
        __in KAllocator& Allocator,
        __in_opt KAsyncContextBase* const ParentAsync = NULL
        );
#endif
    
    static
    NTSTATUS
    QueryDirectories(
        __in KWString& FullyQualifiedDirectoryName,
        __out NameArray& DirectoryNameArray,
        __in KAllocator& Allocator,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = NULL
        );

#if defined(K_UseResumable) 
    static
    ktl::Awaitable<NTSTATUS>
    QueryDirectoriesAsync(
        __in KWString& FullyQualifiedDirectoryName,
        __out NameArray& DirectoryNameArray,
        __in KAllocator& Allocator,
        __in_opt KAsyncContextBase* const ParentAsync = NULL
        );
	
    static
    ktl::Awaitable<NTSTATUS>
    QueryDirectoriesAsync(
        __in LPCWSTR FullyQualifiedDirectoryName,
        __out NameArray& DirectoryNameArray,
        __in KAllocator& Allocator,
        __in_opt KAsyncContextBase* const ParentAsync = NULL
        );
#endif

    static
    NTSTATUS
    QueryFiles(
        __in KWString& FullyQualifiedDirectoryName,
        __out NameArray& FileNameArray,
        __in KAllocator& Allocator,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = NULL
        );

#if defined(K_UseResumable) 
    static
    ktl::Awaitable<NTSTATUS>
    QueryFilesAsync(
        __in KWString& FullyQualifiedDirectoryName,
        __out NameArray& FileNameArray,
        __in KAllocator& Allocator,
        __in_opt KAsyncContextBase* const ParentAsync = NULL
        );

    static
    ktl::Awaitable<NTSTATUS>
    QueryFilesAsync(
        __in LPCWSTR FullyQualifiedDirectoryName,
        __out NameArray& FileNameArray,
        __in KAllocator& Allocator,
        __in_opt KAsyncContextBase* const ParentAsync = NULL
        );
#endif

    static
    NTSTATUS
    QueryFullFileAttributes(
        __in KWString& FullyQualifiedPath,
        __in KAllocator& Allocator,
        __out FILE_NETWORK_OPEN_INFORMATION& Result,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = NULL
        );

#if defined(K_UseResumable) 
    static
    ktl::Awaitable<NTSTATUS>
    QueryFullFileAttributesAsync(
        __in KWString& FullyQualifiedPath,
        __in KAllocator& Allocator,
        __out FILE_NETWORK_OPEN_INFORMATION& Result,
        __in_opt KAsyncContextBase* const ParentAsync = NULL
        );

    static
    ktl::Awaitable<NTSTATUS>
    QueryFullFileAttributesAsync(
        __in LPCWSTR FullyQualifiedPath,
        __in KAllocator& Allocator,
        __out FILE_NETWORK_OPEN_INFORMATION& Result,
        __in_opt KAsyncContextBase* const ParentAsync = NULL
        );	
#endif

    static
    NTSTATUS
    RenameFile(
    __in KWString const & FromPathName,
    __in KWString const & ToPathName,
    __in BOOLEAN OverwriteIfExists,
    __in KAllocator& Allocator,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync);
    
#if defined(K_UseResumable) 
    static
    ktl::Awaitable<NTSTATUS>
    RenameFileAsync(
    __in KWString const & FromPathName,
    __in KWString const & ToPathName,
    __in BOOLEAN OverwriteIfExists,
    __in KAllocator& Allocator,
    __in_opt KAsyncContextBase* const ParentAsync);
#endif

    static
    NTSTATUS
    RenameFile(
    __in LPCWSTR FromPathName,
    __in LPCWSTR ToPathName,
    __in BOOLEAN OverwriteIfExists,
    __in KAllocator& Allocator,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync);
    
#if defined(K_UseResumable) 
    static
    ktl::Awaitable<NTSTATUS>
    RenameFileAsync(
    __in LPCWSTR FromPathName,
    __in LPCWSTR ToPathName,
    __in BOOLEAN OverwriteIfExists,
    __in KAllocator& Allocator,
    __in_opt KAsyncContextBase* const ParentAsync);
#endif
    
//
// Hard link support is Windows only
//
#if !defined(PLATFORM_UNIX)
    static
    NTSTATUS
    QueryHardLinks(
        __in KWString& FullyQualifiedLinkName,
        __in KAllocator& Allocator,
        __out NameArray& FileNameArray,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = NULL
        );

#if defined(K_UseResumable) 
    static
    ktl::Awaitable<NTSTATUS>
    QueryHardLinksAsync(
        __in KWString& FullyQualifiedLinkName,
        __in KAllocator& Allocator,
        __out NameArray& FileNameArray,
        __in_opt KAsyncContextBase* const ParentAsync = NULL
        );
#endif

    static
    NTSTATUS
    SetHardLink(
        __in KWString& FullyQualifiedExistingLinkName,
        __in KWString& FullyQualifiedNewLinkName,
        __in KAllocator& Allocator,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = NULL
        );

#if defined(K_UseResumable) 
    static
    ktl::Awaitable<NTSTATUS>
    SetHardLinkAsync(
        __in KWString& FullyQualifiedExistingLinkName,
        __in KWString& FullyQualifiedNewLinkName,
        __in KAllocator& Allocator,
        __in_opt KAsyncContextBase* const ParentAsync = NULL
        );
#endif

    static
    NTSTATUS
    SetHardLink(
        __in HANDLE LinkHandle,
        __in KWString& FullyQualifiedNewLinkName,
        __in KAllocator& Allocator,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = NULL
        );
    
#if defined(K_UseResumable) 
    static
    ktl::Awaitable<NTSTATUS>
    SetHardLinkAsync(
        __in HANDLE LinkHandle,
        __in KWString& FullyQualifiedNewLinkName,
        __in KAllocator& Allocator,
        __in_opt KAsyncContextBase* const ParentAsync = NULL
        );
#endif
        
#endif
};
