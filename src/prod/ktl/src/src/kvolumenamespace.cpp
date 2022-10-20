/*++

Module Name:

    kvolumenamespace.cpp

Abstract:

    This file contains the user mode and kernel mode implementations of a KVolumeNamespace object.

Author:

    Norbert P. Kusters (norbertk) 15-Nov-2010

Environment:

    Kernel mode and User mode

Notes:

Revision History:

--*/

#include <ktl.h>
#if !defined(PLATFORM_UNIX)
#include <mountdev.h>
#endif
#include <ktrace.h>

#include <HLPalFunctions.h>

#if !defined(PLATFORM_UNIX)
const WCHAR* KVolumeNamespace::PathSeparator = L"\\";
const WCHAR  KVolumeNamespace::PathSeparatorChar = L'\\';
#else
const WCHAR* KVolumeNamespace::PathSeparator =  L"/";
const WCHAR  KVolumeNamespace::PathSeparatorChar = L'/';
#endif

class KVolumeNamespaceStandard : public KVolumeNamespace
{

    public:
#if !defined(PLATFORM_UNIX)     
        class QueryVolumeListContext : public KAsyncContextBase, public KThreadPool::WorkItem
        {

            K_FORCE_SHARED(QueryVolumeListContext);

            NOFAIL
            QueryVolumeListContext(
                __in_opt VolumeIdArray* VolumeList,
                __in_opt VolumeInformationArray* VolumeInformationList
                );

            private:

                VOID
                Initialize(
                    __in_opt VolumeIdArray* VolumeList,
                    __in_opt VolumeInformationArray* VolumeInformationList
                    );

                VOID
                OnStart(
                    );

                VOID
                Execute(
                    );

                friend KVolumeNamespace;

                VolumeIdArray* _VolumeList;
                VolumeInformationArray* _VolumeInformationList;

        };
#endif
        
        class CreateDirectoryContext : public KAsyncContextBase, public KThreadPool::WorkItem
        {

            K_FORCE_SHARED(CreateDirectoryContext);

            FAILABLE
            CreateDirectoryContext(
                __in KWString& FullyQualifiedDirectoryName
                );

            private:

                NTSTATUS
                Initialize(
                    __in KWString& FullyQualifiedDirectoryName
                    );

                VOID
                OnStart(
                    );

                VOID
                Execute(
                    );

                friend KVolumeNamespace;

                KWString _FullyQualifiedDirectoryName;

        };

        class DeleteFileOrDirectoryContext : public KAsyncContextBase, public KThreadPool::WorkItem
        {

            K_FORCE_SHARED(DeleteFileOrDirectoryContext);

            FAILABLE
            DeleteFileOrDirectoryContext(
                __in KWString& FullyQualifiedDirectoryName
                );

            private:

                NTSTATUS
                Initialize(
                    __in KWString& FullyQualifiedDirectoryName
                    );

                VOID
                OnStart(
                    );

                VOID
                Execute(
                    );

                friend KVolumeNamespace;

                KWString _FullyQualifiedDirectoryName;

        };

        class QueryDirectoriesContext : public KAsyncContextBase, public KThreadPool::WorkItem
        {

            K_FORCE_SHARED(QueryDirectoriesContext);

            FAILABLE
            QueryDirectoriesContext(
                __in KWString& FullyQualifiedDirectoryName,
                __inout NameArray& DirectoryNameArray
                );

            private:

                NTSTATUS
                Initialize(
                    __in KWString& FullyQualifiedDirectoryName,
                    __inout NameArray& DirectoryNameArray
                    );

                VOID
                OnStart(
                    );

                VOID
                Execute(
                    );

                friend KVolumeNamespace;

                KWString _FullyQualifiedDirectoryName;
                NameArray* _DirectoryNameArray;

        };

        class QueryFilesContext : public KAsyncContextBase, public KThreadPool::WorkItem
        {

            K_FORCE_SHARED(QueryFilesContext);

            FAILABLE
            QueryFilesContext(
                __in KWString& FullyQualifiedDirectoryName,
                __inout NameArray& FileNameArray
                );

            private:

                NTSTATUS
                Initialize(
                    __in KWString& FullyQualifiedDirectoryName,
                    __inout NameArray& FileNameArray
                    );

                VOID
                OnStart(
                    );

                VOID
                Execute(
                    );

                friend KVolumeNamespace;

                KWString _FullyQualifiedDirectoryName;
                NameArray* _FileNameArray;

        };

        class QueryHardLinksOp;
        class SetHardLinksOp;
        class QueryGlobalVolumeDirectoryNameOp;
        class GetVolumeIdOp;
        class QueryFullFileAttributesOp;
        class RenameFileOp;
        class IsHardLinkOp;

    private:
        
#if !defined(PLATFORM_UNIX)
        struct DeviceNameNode
        {
            DeviceNameNode(KAllocator& Allocator);

            KTableEntry TableEntry;
            KWString DeviceName;
            GUID VolumeId;
            UCHAR DriveLetter;

        private:
            DeviceNameNode();
        };

        typedef KNodeTable<DeviceNameNode> DeviceNameTable;

        static
        NTSTATUS
        QueryStableGuid(
            __in KWString& VolumeName,
            __out GUID& StableGuid
            );

        static
        NTSTATUS
        QueryVolumeGuid(
            __in KAllocator& Allocator,
            __in KWString& VolumeName,
            __out GUID& VolumeGuid
            );
#endif
        
        static
        NTSTATUS
        QueryIsRemovableMedia(
            __in KWString& VolumeName,
            __out BOOLEAN& IsRemovableMedia
            );

#if !defined(PLATFORM_UNIX)
        static
        NTSTATUS
        QueryMountPoints(
            __out KBuffer::SPtr& OutputBuffer,
            __in KAllocator& Allocator
            );

        static
        LONG
        Compare(
            __in DeviceNameNode& First,
            __in DeviceNameNode& Second
            );
#endif
};

#if !defined(PLATFORM_UNIX)

NTSTATUS
KVolumeNamespace::QueryVolumeList(
    __out VolumeIdArray& VolumeList,
    __in KAllocator& Allocator,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync
    )

/*++

Routine Description:

    This routine will return the list of volume ids for volumes available on this local system.

Arguments:

    VolumeList  - Returns the list of volumes.

    Completion  - Supplies the completion callback.

    ParentAsync - Supplies, optionally, the parent async context to use for synchronizing the completion.

Return Value:

    STATUS_PENDING                  - The given completion routine will be called when the request is complete.

    STATUS_INSUFFICIENT_RESOURCES   - Insufficient memory.

--*/

{
    KVolumeNamespaceStandard::QueryVolumeListContext* context;
    NTSTATUS status;

    context = _new(KTL_TAG_NAMESPACE, Allocator) KVolumeNamespaceStandard::QueryVolumeListContext(&VolumeList, NULL);

    if (!context) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = context->Status();

    if (!NT_SUCCESS(status)) {
        _delete(context);
        return status;
    }

    context->Start(ParentAsync, Completion);

    return STATUS_PENDING;
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KVolumeNamespace::QueryVolumeListAsync(
    __out VolumeIdArray& VolumeList,
    __in KAllocator& Allocator,
    __in_opt KAsyncContextBase* const ParentAsync
    )
{
    NTSTATUS status;
    KVolumeNamespaceStandard::QueryVolumeListContext* context;
    context = _new(KTL_TAG_NAMESPACE, Allocator) KVolumeNamespaceStandard::QueryVolumeListContext(&VolumeList, NULL);

    if (!context) {
        co_return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = context->Status();

    if (!NT_SUCCESS(status)) {
        _delete(context);
        co_return status;
    }
    
    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)context), ParentAsync, NULL);

    status = co_await awaiter;
    co_return status;   
}
#endif

NTSTATUS
KVolumeNamespace::QueryVolumeListEx(
    __out VolumeInformationArray& VolumeInformationList,
    __in KAllocator& Allocator,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync
    )

/*++

Routine Description:

    This routine will return the list all volumes in the system, both those with stable guids and those without stable ids,
    and also return the drive letters, if present.

Arguments:

    VolumeInformationList   - Returns the volume information list.

    Completion              - Supplies the completion callback.

    ParentAsync             - Supplies, optionally, the parent async context to use for synchronizing the completion.

Return Value:

    STATUS_PENDING                  - The given completion routine will be called when the request is complete.

    STATUS_INSUFFICIENT_RESOURCES   - Insufficient memory.

--*/

{
    KVolumeNamespaceStandard::QueryVolumeListContext* context;
    NTSTATUS status;

    context = _new(KTL_TAG_NAMESPACE, Allocator) KVolumeNamespaceStandard::QueryVolumeListContext(NULL, &VolumeInformationList);

    if (!context) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = context->Status();

    if (!NT_SUCCESS(status)) {
        _delete(context);
        return status;
    }

    context->Start(ParentAsync, Completion);

    return STATUS_PENDING;
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KVolumeNamespace::QueryVolumeListExAsync(
    __out VolumeInformationArray& VolumeInformationList,
    __in KAllocator& Allocator,
    __in_opt KAsyncContextBase* const ParentAsync
    )
{
    KVolumeNamespaceStandard::QueryVolumeListContext* context;
    NTSTATUS status;

    context = _new(KTL_TAG_NAMESPACE, Allocator) KVolumeNamespaceStandard::QueryVolumeListContext(NULL, &VolumeInformationList);

    if (!context) {
        co_return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = context->Status();

    if (!NT_SUCCESS(status)) {
        _delete(context);
        co_return status;
    }
    
    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)context), ParentAsync, NULL);

    status = co_await awaiter;
    co_return status;   
}
#endif

BOOLEAN
KVolumeNamespace::QueryVolumeIdFromDriveLetter(
    __in VolumeInformationArray& VolumeInformationList,
    __in UCHAR DriveLetter,
    __out GUID& VolumeId
    )

/*++

Routine Description:

    This routine will return the volume id for the given drive letter.

Arguments:

    VolumeInformationList   - Supplies the current volume information list as supplied by 'QueryVolumeListEx'.

    DriveLetter             - Supplies the drive letter.

    VolumeId                - Returns the volume id.

Return Value:

    FALSE   - The drive letter does not exits.

    TRUE    - Success.

--*/

{
    ULONG i;

    for (i = 0; i < VolumeInformationList.Count(); i++) {
        if (VolumeInformationList[i].DriveLetter == DriveLetter) {
            VolumeId = VolumeInformationList[i].VolumeId;
            return TRUE;
        }
    }

    return FALSE;
}

NTSTATUS
KVolumeNamespace::CreateFullyQualifiedRootDirectoryName(
    __in const GUID& VolumeId,
    __out KWString& FullyQualifiedRootDirectoryName
    )

/*++

Routine Description:

    This routine will create a fully qualified root directory name to the root directory of the given volume.

Arguments:

    VolumeId                        - Supplies the volume id.

    FullyQualifiedRootDirectoryName - Returns a fully qualified root directory name to the given volume.

Return Value:

    NTSTATUS

--*/

{
    FullyQualifiedRootDirectoryName = L"\\GLOBAL??\\Volume";
    FullyQualifiedRootDirectoryName += VolumeId;

    return FullyQualifiedRootDirectoryName.Status();
}
#endif


NTSTATUS
KVolumeNamespace::CreateFullyQualifiedName(
    __in KWString& FullyQualifiedDirectoryName,
    __in KWString& Name,
    __out KWString& FullyQualifiedName
    )

/*++

Routine Description:

    This routine will create a fully qualified name by combining the given fully qualified directory name with the given
    name.

Arguments:

    FullyQualifiedDiretoryName  - Supplies the fully qualified directory name.

    Name                        - Supplies a file or directory name.

    FullyQualifiedName          - Returns a fully qualified name that is the combination of the name and the fully qualified
                                    directory name.

Return Value:

    NTSTATUS

--*/

{
    FullyQualifiedName = FullyQualifiedDirectoryName;
#if !defined(PLATFORM_UNIX)    
    FullyQualifiedName += KVolumeNamespace::PathSeparatorChar;
#else
    //
    // Check for root directory / special case
    // TODO: Add unit test
    if (FullyQualifiedDirectoryName.CompareTo(L"/") != 0)
    {
        FullyQualifiedName += KVolumeNamespace::PathSeparatorChar;        
    }
#endif
    FullyQualifiedName += Name;

    return FullyQualifiedName.Status();
}

//** Split and normalize the supplied Windows Object path into it's Root and Root-relative parts
//
//  Parameters (Windows):
//      FullyQualifiedObjectPath - FQN or volume rooted (e.g. "\Global??\..." or "C:\..." or "\device\..."
//      FullyQualifiedObjectRoot - resulting normalized root portion of FullyQualifiedObjectPath
//      RootRelativePath - Remainder of FullyQualifiedObjectPath after FullyQualifiedObjectRoot
//
//  Parameters (Linux):
//      FullyQualifiedObjectPath - FQN or rooted (e.g. "/fred/joe/bar"
//      FullyQualifiedObjectRoot - since there is no volume, L""
//      RootRelativePath - Remainder of FullyQualifiedObjectPath after FullyQualifiedObjectRoot
//      
//  Returns:
//      STATUS_SUCCESS - valid path components of FullyQualifiedObjectPath have been returned
//      STATUS_INVALID_PARAMETER - FullyQualifiedDiretoryName's format is invalid
//
NTSTATUS
KVolumeNamespace::SplitAndNormalizeObjectPath(
    __in KWString& FullyQualifiedObjectPath,
    __out KWString& FullyQualifiedObjectRoot,
    __out KWString& RootRelativePath)
{
    KStringView     fullyQualifiedObjectPath(*(PUNICODE_STRING)FullyQualifiedObjectPath);
    return KVolumeNamespace::SplitAndNormalizeObjectPath(fullyQualifiedObjectPath, FullyQualifiedObjectRoot, RootRelativePath);
}

NTSTATUS
KVolumeNamespace::SplitAndNormalizeObjectPath(
    __in KStringView& FullyQualifiedObjectPath,
    __out KWString& FullyQualifiedObjectRoot,
    __out KWString& RootRelativePath)
{
    ULONG           splitPoint = 0;
    NTSTATUS        status = STATUS_SUCCESS;

    if ((FullyQualifiedObjectPath.Length() > 0) && (FullyQualifiedObjectPath.PeekFirst() == KVolumeNamespace::PathSeparatorChar))
    {
        // Possible FQN for object - must be one of the following to be valid
        static WCHAR const *   validFqnPreambles[] =
        {
#if !defined(PLATFORM_UNIX)
            L"\\global??\\",
            L"\\??\\",
            L"\\device\\",
#else
            L"/",
#endif
            nullptr
        };

        WCHAR const **     validFqnPreamblePtrPtr = &validFqnPreambles[0];

        while ((*validFqnPreamblePtrPtr) != nullptr)
        {
            KStringView validFqnPreamble(*validFqnPreamblePtrPtr);
            ULONG       validFqnPreambleLength = validFqnPreamble.Length();

            if (FullyQualifiedObjectPath.LeftString(validFqnPreambleLength)
                .CompareNoCase(validFqnPreamble, validFqnPreambleLength) == 0)
            {
                splitPoint = validFqnPreambleLength;
                break;
            }

            validFqnPreamblePtrPtr++;
        }

        if (splitPoint == 0)
        {
            // Not a wellknown object root name
            status = STATUS_INVALID_PARAMETER;
        }
    }

    if (NT_SUCCESS(status))
    {
#if !defined(PLATFORM_UNIX)
        KWString        globalRoot(FullyQualifiedObjectRoot.GetAllocator(), L"\\global??\\");
#else
        KWString        globalRoot(FullyQualifiedObjectRoot.GetAllocator(), L"/");
#endif
        KStringView     rootName;
        KStringView     rootRelPath;
        KStringView     delimiter(KVolumeNamespace::PathSeparator);

        if (splitPoint != 0)
        {
#if !defined(PLATFORM_UNIX)
            // FQN object path supplied - simple split to do
            if (FullyQualifiedObjectPath.Search(delimiter, splitPoint, splitPoint))
            {
                rootName = FullyQualifiedObjectPath.LeftString(splitPoint);
                rootRelPath = FullyQualifiedObjectPath.Remainder(splitPoint);
            }
            else
            {
                // No delimiter between root portion and rest of path or rest of path is missing
                status = STATUS_INVALID_PARAMETER;
            }
#else
            rootName = L"";
            rootRelPath = FullyQualifiedObjectPath;
#endif
        }
        else
        {
            // CONSIDER: How is this code ever reached ??

            // Not FQN preamble - split and build normalized object path
            if (FullyQualifiedObjectPath.Search(delimiter, splitPoint, 0))
            {
                rootName = FullyQualifiedObjectPath.LeftString(splitPoint);
                rootRelPath = FullyQualifiedObjectPath.Remainder(splitPoint);

                status = globalRoot.Status();
                if (NT_SUCCESS(status))
                {
                    UNICODE_STRING  us = {(USHORT)rootName.LengthInBytes(), (USHORT)rootName.LengthInBytes(), rootName};
                    globalRoot += us;
                    status = globalRoot.Status();
                }

                if (NT_SUCCESS(status))
                {
                    rootName.Set(globalRoot, globalRoot.Length() / 2, globalRoot.Length() / 2);
                }
            }
            else
            {
                // No delimiter between root portion and rest of path
                status = STATUS_INVALID_PARAMETER;
            }
        }

        if (NT_SUCCESS(status))
        {
            // Stuff results - try
            KAssert(!rootName.IsNull());
            FullyQualifiedObjectRoot = Ktl::Move(rootName);
            status = FullyQualifiedObjectRoot.Status();
            if (NT_SUCCESS(status))
            {
                KAssert(!rootRelPath.IsNull());
                RootRelativePath = Ktl::Move(rootRelPath);
                status = RootRelativePath.Status();
            }
        }
    }

    if (!NT_SUCCESS(status))
    {
        // Something failed - clear results
        FullyQualifiedObjectRoot.Reset();
        RootRelativePath.Reset();
    }

    return status;
}

NTSTATUS
SplitObjectPathInPathAndFilename(
    __in KAllocator& Allocator,
    __in KWString& FullyQualifiedObjectPath,
    __out KWString& Path,
    __out KWString& Filename
    )
{
    KStringView     fullyQualifiedObjectPath(*(PUNICODE_STRING)FullyQualifiedObjectPath);
    return KVolumeNamespace::SplitObjectPathInPathAndFilename(Allocator,
                                                              fullyQualifiedObjectPath,
                                                              Path,
                                                              Filename);
}

NTSTATUS
KVolumeNamespace::SplitObjectPathInPathAndFilename(
    __in KAllocator& Allocator,
    __in KStringView& FullyQualifiedObjectPath,
    __out KWString& Path,
    __out KWString& Filename
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN b;
    ULONG pos;
    KWString rootPathString(Allocator);
    KWString relativePathString(Allocator);

    //
    // Parse into root and relative paths
    //
    status = KVolumeNamespace::SplitAndNormalizeObjectPath(FullyQualifiedObjectPath,
                                                           rootPathString,
                                                           relativePathString);

    if (! NT_SUCCESS(status))
    {
        rootPathString = L"";
        relativePathString = FullyQualifiedObjectPath;
    }

    //
    // Separate rest of path and filename from RelativePath
    //
    KStringView     relativePath(*(PUNICODE_STRING)relativePathString);

    b = relativePath.RFind(KVolumeNamespace::PathSeparatorChar, pos);
    if (! b)
    {
        Path.Reset();
        Filename.Reset();
        return(STATUS_INVALID_PARAMETER);
    }

    //
    // Put the full path together and grab the filename
    //

    KStringView p = relativePath.LeftString(pos+1);
    UNICODE_STRING pu;
    pu.Buffer = p;
    pu.Length = (USHORT)p.LengthInBytes();
    KWString pString(Allocator, pu);
    status = pString.Status();
    if (!NT_SUCCESS(status))
    {
        Path.Reset();
        Filename.Reset();
        return(status);
    }

    Path = rootPathString;
    Path += pString;
    Filename = relativePath.Remainder(pos+1);

    status = Path.Status();
    if (NT_SUCCESS(status))
    {
        status = Filename.Status();
        if (!NT_SUCCESS(status))
        {
            Path.Reset();
            Filename.Reset();
        }
    }
    return(status);
}


NTSTATUS
KVolumeNamespace::CreateDirectory(
    __in KWString& FullyQualifiedDirectoryName,
    __in KAllocator& Allocator,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync
    )

/*++

Routine Description:

    This routine will create a new directory with the given fully qualified name.

Arguments:

    FullyQualifiedDirectoryName - Supplies a fully qualified directory name.

    Completion                  - Supplies the completion callback.

    ParentAsync                 - Supplies, optionally, the parent async context to use for synchronizing the completion.

Return Value:

    STATUS_PENDING                  - The given completion routine will be called when the request is complete.

    STATUS_INSUFFICIENT_RESOURCES   - Insufficient memory.

--*/

{
    KVolumeNamespaceStandard::CreateDirectoryContext* context;
    NTSTATUS status;

    context = _new(KTL_TAG_NAMESPACE, Allocator) KVolumeNamespaceStandard::CreateDirectoryContext(FullyQualifiedDirectoryName);

    if (!context) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = context->Status();

    if (!NT_SUCCESS(status)) {
        _delete(context);
        return status;
    }

    context->Start(ParentAsync, Completion);

    return STATUS_PENDING;
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KVolumeNamespace::CreateDirectoryAsync(
    __in KWString& FullyQualifiedDirectoryName,
    __in KAllocator& Allocator,
    __in_opt KAsyncContextBase* const ParentAsync
    )
{
    KVolumeNamespaceStandard::CreateDirectoryContext* context;
    NTSTATUS status;

    context = _new(KTL_TAG_NAMESPACE, Allocator) KVolumeNamespaceStandard::CreateDirectoryContext(FullyQualifiedDirectoryName);

    if (!context) {
        co_return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = context->Status();

    if (!NT_SUCCESS(status)) {
        _delete(context);
        co_return status;
    }

    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)context), ParentAsync, NULL);

    status = co_await awaiter;
    co_return status;       
}

ktl::Awaitable<NTSTATUS>
KVolumeNamespace::CreateDirectoryAsync(
    __in LPCWSTR FullyQualifiedDirectoryName,
    __in KAllocator& Allocator,
    __in_opt KAsyncContextBase* const ParentAsync
    )
{
	KWString s(Allocator);
	s = FullyQualifiedDirectoryName;
	if (! NT_SUCCESS(s.Status()))
	{
		co_return s.Status();
	}

	co_return co_await KVolumeNamespace::CreateDirectoryAsync(s, Allocator, ParentAsync);
}
#endif

NTSTATUS
KVolumeNamespace::DeleteFileOrDirectory(
    __in KWString& FullyQualifiedDirectoryName,
    __in KAllocator& Allocator,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync
    )

/*++

Routine Description:

    This routine will delete the directory with the given fully qualified name.

Arguments:

    FullyQualifiedDirectoryName - Supplies a fully qualified directory name.

    Completion                  - Supplies the completion callback.

    ParentAsync                 - Supplies, optionally, the parent async context to use for synchronizing the completion.

Return Value:

    STATUS_PENDING                  - The given completion routine will be called when the request is complete.

    STATUS_INSUFFICIENT_RESOURCES   - Insufficient memory.

--*/

{
    KVolumeNamespaceStandard::DeleteFileOrDirectoryContext* context;
    NTSTATUS status;

    context = _new(KTL_TAG_NAMESPACE, Allocator) KVolumeNamespaceStandard::DeleteFileOrDirectoryContext(FullyQualifiedDirectoryName);

    if (!context) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = context->Status();

    if (!NT_SUCCESS(status)) {
        _delete(context);
        return status;
    }

    context->Start(ParentAsync, Completion);

    return STATUS_PENDING;
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KVolumeNamespace::DeleteFileOrDirectoryAsync(
    __in KWString& FullyQualifiedDirectoryName,
    __in KAllocator& Allocator,
    __in_opt KAsyncContextBase* const ParentAsync
    )
{
    KVolumeNamespaceStandard::DeleteFileOrDirectoryContext* context;
    NTSTATUS status;

    context = _new(KTL_TAG_NAMESPACE, Allocator) KVolumeNamespaceStandard::DeleteFileOrDirectoryContext(FullyQualifiedDirectoryName);

    if (!context) {
        co_return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = context->Status();

    if (!NT_SUCCESS(status)) {
        _delete(context);
        co_return status;
    }
    
    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)context), ParentAsync, NULL);

    status = co_await awaiter;
    co_return status;       
}

ktl::Awaitable<NTSTATUS>
KVolumeNamespace::DeleteFileOrDirectoryAsync(
    __in LPCWSTR FullyQualifiedDirectoryName,
    __in KAllocator& Allocator,
    __in_opt KAsyncContextBase* const ParentAsync
    )
{
	KWString s(Allocator);
	s = FullyQualifiedDirectoryName;
	if (! NT_SUCCESS(s.Status()))
	{
		co_return s.Status();
	}

	co_return co_await KVolumeNamespace::DeleteFileOrDirectoryAsync(s, Allocator, ParentAsync);
}
#endif

NTSTATUS
KVolumeNamespace::QueryDirectories(
    __in KWString& FullyQualifiedDirectoryName,
    __out NameArray& DirectoryNameArray,
    __in KAllocator& Allocator,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync
    )

/*++

Routine Description:

    This routine will return the list of directories in the given directory.

Arguments:

    FullyQualifiedDirectoryName - Supplies a fully qualified directory name.

    DirectoryNameArray          - Returns a list of directory names in the given directory.

    Completion                  - Supplies the completion callback.

    ParentAsync                 - Supplies, optionally, the parent async context to use for synchronizing the completion.

Return Value:

    STATUS_PENDING                  - The given completion routine will be called when the request is complete.

    STATUS_INSUFFICIENT_RESOURCES   - Insufficient memory.

--*/

{
    KVolumeNamespaceStandard::QueryDirectoriesContext* context;
    NTSTATUS status;

    context = _new(KTL_TAG_NAMESPACE, Allocator) KVolumeNamespaceStandard::QueryDirectoriesContext(FullyQualifiedDirectoryName,
            DirectoryNameArray);

    if (!context) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = context->Status();

    if (!NT_SUCCESS(status)) {
        _delete(context);
        return status;
    }

    context->Start(ParentAsync, Completion);

    return STATUS_PENDING;
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KVolumeNamespace::QueryDirectoriesAsync(
    __in KWString& FullyQualifiedDirectoryName,
    __out NameArray& DirectoryNameArray,
    __in KAllocator& Allocator,
    __in_opt KAsyncContextBase* const ParentAsync
    )
{
    KVolumeNamespaceStandard::QueryDirectoriesContext* context;
    NTSTATUS status;

    context = _new(KTL_TAG_NAMESPACE, Allocator) KVolumeNamespaceStandard::QueryDirectoriesContext(FullyQualifiedDirectoryName,
            DirectoryNameArray);

    if (!context) {
        co_return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = context->Status();

    if (!NT_SUCCESS(status)) {
        _delete(context);
        co_return status;
    }

    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)context), ParentAsync, NULL);

    status = co_await awaiter;
    co_return status;       
}

ktl::Awaitable<NTSTATUS>
KVolumeNamespace::QueryDirectoriesAsync(
    __in LPCWSTR FullyQualifiedDirectoryName,
    __out NameArray& DirectoryNameArray,
    __in KAllocator& Allocator,
    __in_opt KAsyncContextBase* const ParentAsync
    )
{
	KWString s(Allocator);
	s = FullyQualifiedDirectoryName;
	if (! NT_SUCCESS(s.Status()))
	{
		co_return s.Status();
	}

	co_return co_await KVolumeNamespace::QueryDirectoriesAsync(s, DirectoryNameArray, Allocator, ParentAsync);
}
#endif


NTSTATUS
KVolumeNamespace::QueryFiles(
    __in KWString& FullyQualifiedDirectoryName,
    __out NameArray& FileNameArray,
    __in KAllocator& Allocator,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync
    )

/*++

Routine Description:

    This routine will return the list of files in the given directory.

Arguments:

    FullyQualifiedDirectoryName - Supplies the directory name.

    FileNameArray               - Returns the list of files in the current directory.

    Completion                  - Supplies the completion callback.

    ParentAsync                 - Supplies, optionally, the parent async context to use for synchronizing the completion.

Return Value:

    STATUS_PENDING                  - The given completion routine will be called when the request is complete.

    STATUS_INSUFFICIENT_RESOURCES   - Insufficient memory.

--*/

{
    KVolumeNamespaceStandard::QueryFilesContext* context;
    NTSTATUS status;

    context = _new(KTL_TAG_NAMESPACE, Allocator) KVolumeNamespaceStandard::QueryFilesContext(FullyQualifiedDirectoryName,
            FileNameArray);

    if (!context) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = context->Status();

    if (!NT_SUCCESS(status)) {
        _delete(context);
        return status;
    }

    context->Start(ParentAsync, Completion);

    return STATUS_PENDING;
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KVolumeNamespace::QueryFilesAsync(
    __in KWString& FullyQualifiedDirectoryName,
    __out NameArray& FileNameArray,
    __in KAllocator& Allocator,
    __in_opt KAsyncContextBase* const ParentAsync
    )
{
    KVolumeNamespaceStandard::QueryFilesContext* context;
    NTSTATUS status;

    context = _new(KTL_TAG_NAMESPACE, Allocator) KVolumeNamespaceStandard::QueryFilesContext(FullyQualifiedDirectoryName,
            FileNameArray);

    if (!context) {
        co_return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = context->Status();

    if (!NT_SUCCESS(status)) {
        _delete(context);
        co_return status;
    }

    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)context), ParentAsync, NULL);

    status = co_await awaiter;
    co_return status;       
}

ktl::Awaitable<NTSTATUS>
KVolumeNamespace::QueryFilesAsync(
    __in LPCWSTR FullyQualifiedDirectoryName,
    __out NameArray& FileNameArray,
    __in KAllocator& Allocator,
    __in_opt KAsyncContextBase* const ParentAsync
    )
{
	KWString s(Allocator);
	s = FullyQualifiedDirectoryName;
	if (! NT_SUCCESS(s.Status()))
	{
		co_return s.Status();
	}

	co_return co_await KVolumeNamespace::QueryFilesAsync(s, FileNameArray, Allocator, ParentAsync);
}
#endif

#if !defined(PLATFORM_UNIX)

NTSTATUS
KVolumeNamespaceStandard::QueryStableGuid(
    __in KWString& VolumeName,
    __out GUID& StableGuid
    )

/*++

Routine Description:

    This routine returns the volume's stable guid, if the volume has a stable guid.

Arguments:

    VolumeName  - Supplies the volume name.

    StableGuid  - Returns the stable guid for the volume.

Return Value:

    NTSTATUS

--*/

{
    OBJECT_ATTRIBUTES oa;
    NTSTATUS status;
    HANDLE h;
    IO_STATUS_BLOCK ioStatus;
    MOUNTDEV_STABLE_GUID stableGuid;

    InitializeObjectAttributes(&oa, VolumeName, OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = KNt::OpenFile(&h, FILE_READ_ATTRIBUTES | SYNCHRONIZE, &oa, &ioStatus,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, FILE_SYNCHRONOUS_IO_NONALERT);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = KNt::DeviceIoControlFile(h, NULL, NULL, NULL, &ioStatus, IOCTL_MOUNTDEV_QUERY_STABLE_GUID, NULL, 0, &stableGuid,
            sizeof(stableGuid));

    StableGuid = stableGuid.StableGuid;

    KNt::Close(h);

    return status;
}

NTSTATUS
KVolumeNamespaceStandard::QueryVolumeGuid(
    __in KAllocator& Allocator,
    __in KWString& VolumeName,
    __out GUID& VolumeGuid
    )

/*++

Routine Description:

    This routine returns the volume's guid, if the volume has a guid.

Arguments:

    VolumeName  - Supplies fully qualified volume name or volume file/dir path

    VolumeGuid  - Returns the guid for the volume.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    HANDLE              dirHandle = 0;
    HANDLE              linkHandle = 0;
    KFinally([&]()
    {
        if (!NT_SUCCESS(status))
        {
            memset(&VolumeGuid, 0, sizeof(GUID));
        }

        if (dirHandle != 0)
        {
            KNt::Close(dirHandle);
        }

        if (linkHandle != 0)
        {
            KNt::Close(linkHandle);
        }
    });

    KWString            rootPath(Allocator);
    KWString            rootRelativePath(Allocator);
    KWString            terminatedVolumeName(Allocator);
    KStringView         volumePath((UNICODE_STRING)VolumeName);

    status = KVolumeNamespace::SplitAndNormalizeObjectPath(volumePath, rootPath, rootRelativePath);
    if (status == STATUS_INVALID_PARAMETER)
    {
        // Potentially VolumeName does not have a volume-relative path attached - add one
        terminatedVolumeName = VolumeName;
        status = terminatedVolumeName.Status();
        if (NT_SUCCESS(status))
        {
            terminatedVolumeName += L"\\";
            status = terminatedVolumeName.Status();
            if (NT_SUCCESS(status))
            {
                volumePath = (UNICODE_STRING)terminatedVolumeName;
                status = KVolumeNamespace::SplitAndNormalizeObjectPath(volumePath, rootPath, rootRelativePath);
            }
        }
    }

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    //* Split a volume name into its object directory and object names
    KStringView         objPath((UNICODE_STRING)rootPath);
    KStringView         dirName;

    if (!objPath.RightString(objPath.Length() - 1).MatchUntil(KStringView(L"\\"), dirName))
    {
        return STATUS_INVALID_PARAMETER;
    }

    dirName = objPath.LeftString(dirName.Length() + 1);
    KStringView         objectName(objPath.SubString(dirName.Length() + 1, objPath.Length() - dirName.Length() - 1));

    OBJECT_ATTRIBUTES   oa;
    UNICODE_STRING      ucs;

    //* Retrieve the underlying device name for VolumeName (subjectDeviceName)
    ucs = dirName.ToUNICODE_STRING();
    InitializeObjectAttributes(&oa, &ucs, OBJ_CASE_INSENSITIVE, NULL, NULL);
    status = KNt::OpenDirectoryObject(dirHandle, DIRECTORY_QUERY, oa);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    ucs = objectName.ToUNICODE_STRING();
    InitializeObjectAttributes(&oa, &ucs, OBJ_CASE_INSENSITIVE, dirHandle, nullptr);
    status = KNt::OpenSymbolicLinkObject(linkHandle, SYMBOLIC_LINK_QUERY, oa);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    KNt::Close(dirHandle);
    dirHandle = 0;

    // Compute size and allocate subjectDeviceName buffer
    ULONG           requiredSize = 0;
    KBuffer::SPtr   subjectDeviceNameBuffer;

    ucs.Buffer = nullptr;
    ucs.Length = ucs.MaximumLength = 0;

    status = KNt::QuerySymbolicLinkObject(linkHandle, ucs, &requiredSize);
    KInvariant(status == STATUS_BUFFER_TOO_SMALL);
    KInvariant(requiredSize <= MAXUSHORT);

    status = KBuffer::Create(requiredSize, subjectDeviceNameBuffer, Allocator);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    // Retrieve the device name (MUST BE "\Device\HarddiskVolume...")
    ucs.Buffer = (WCHAR*)subjectDeviceNameBuffer->GetBuffer();
    ucs.Length = ucs.MaximumLength = (USHORT)subjectDeviceNameBuffer->QuerySize();
    status = KNt::QuerySymbolicLinkObject(linkHandle, ucs, nullptr);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    KNt::Close(linkHandle);
    linkHandle = 0;

    KStringView         subjectDeviceName(ucs);
    KStringView         constVolumePreamble(L"\\Device\\HarddiskVolume");
    KInvariant(subjectDeviceName.Length() > constVolumePreamble.Length());
    KInvariant(subjectDeviceName.LeftString(constVolumePreamble.Length()).CompareNoCase(constVolumePreamble) == 0);

    //* Retreive all of the "\GLOBAL??" object directory - to scan for volume symbolic links
    ucs = KStringView(L"\\GLOBAL??").ToUNICODE_STRING();
    InitializeObjectAttributes(&oa, &ucs, OBJ_CASE_INSENSITIVE, NULL, NULL);
    status = KNt::OpenDirectoryObject(dirHandle, DIRECTORY_QUERY, oa);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    ULONG           ctx;
    KBuffer::SPtr   globalDirBuffer;

    status = KBuffer::Create(4096, globalDirBuffer, Allocator);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    // Retreive and re-allocate until we have a big enough buffer
    #pragma warning(disable:4127)   // C4127: conditional expression is constant
    while (TRUE)
    {
        KInvariant(globalDirBuffer->QuerySize() < (MAXULONG / 2));
        status = KNt::QueryDirectoryObject(
            dirHandle,
            globalDirBuffer->GetBuffer(),
            globalDirBuffer->QuerySize(),
            FALSE,
            TRUE,
            ctx,
            nullptr);

        if (!NT_SUCCESS(status))
        {
            return status;
        }

        if (status == STATUS_SUCCESS)
        {
            break;
        }

        KInvariant(status == STATUS_MORE_ENTRIES);
        status = globalDirBuffer->SetSize(globalDirBuffer->QuerySize() * 2);
        if (!NT_SUCCESS(status))
        {
            return status;
        }
    };

    // Scan the retreived global obj dir for matching subjectDeviceName volume guid name - that will be the subject guid
    KStringView                     constVolumeObjPreamble(L"Volume{");
    KStringView                     constSymbolicLinkType(L"SymbolicLink");
    KDynString                      objName(Allocator);
    KDynString                      objType(Allocator);
    KBuffer::SPtr                   currentEntryBuffer;

    OBJECT_DIRECTORY_INFORMATION    zeroEntry;
        memset(&zeroEntry, 0, sizeof(zeroEntry));

    status = KBuffer::Create(256, currentEntryBuffer, Allocator);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    OBJECT_DIRECTORY_INFORMATION*   currentEntry = (OBJECT_DIRECTORY_INFORMATION*)globalDirBuffer->GetBuffer();
    while (memcmp(&zeroEntry, currentEntry, sizeof(zeroEntry)) != 0)
    {
        if ((KStringView(currentEntry->Name).LeftString(constVolumeObjPreamble.Length()).Compare(constVolumeObjPreamble) == 0) &&
            (KStringView(currentEntry->TypeName).Compare(constSymbolicLinkType) == 0))
        {
            KAssert(linkHandle ==0);
            KFinally([&] ()
            {
                if (linkHandle != 0)
                {
                    KNt::Close(linkHandle);
                    linkHandle = 0;
                }
            });

            InitializeObjectAttributes(&oa, &currentEntry->Name, OBJ_CASE_INSENSITIVE, dirHandle, nullptr);
            status = KNt::OpenSymbolicLinkObject(linkHandle, SYMBOLIC_LINK_QUERY, oa);
            if (!NT_SUCCESS(status))
            {
                return status;
            }

            // Get Symbolic link info for current entry
            {
                ucs.Buffer = (WCHAR*)currentEntryBuffer->GetBuffer();
                ucs.Length = ucs.MaximumLength = (USHORT)currentEntryBuffer->QuerySize();
                ULONG       sizeRequired = 0;

                status = KNt::QuerySymbolicLinkObject(linkHandle, ucs, &sizeRequired);
                if (status == STATUS_BUFFER_TOO_SMALL)
                {
                    status = currentEntryBuffer->SetSize(currentEntryBuffer->QuerySize() * 2);
                    if (!NT_SUCCESS(status))
                    {
                        return status;
                    }

                    KInvariant(currentEntryBuffer->QuerySize() <= MAXUSHORT);
                    ucs.Buffer = (WCHAR*)currentEntryBuffer->GetBuffer();
                    ucs.Length = ucs.MaximumLength = (USHORT)currentEntryBuffer->QuerySize();
                    ULONG       sizeRequired1 = 0;

                    status = KNt::QuerySymbolicLinkObject(linkHandle, ucs, &sizeRequired1);
                }
            }

            if (!NT_SUCCESS(status))
            {
                return status;
            }

            KStringView     currentSymbolicLinkName(ucs);

            if (subjectDeviceName.CompareNoCase(currentSymbolicLinkName) == 0)
            {
                //** Found matching volume symbolic link - extract GUID
                if (!KStringView(currentEntry->Name)
                        .SubString(constVolumeObjPreamble.Length() - 1, KStringView::GuidLengthInChars)
                            .ToGUID(VolumeGuid))
                {
                    status = STATUS_UNRECOGNIZED_VOLUME;
                    return status;
                }

                status = STATUS_SUCCESS;
                return status;
            }
        }

        currentEntry++;
    }

    status = STATUS_UNRECOGNIZED_VOLUME;
    return status;
}
#endif

NTSTATUS
KVolumeNamespaceStandard::QueryIsRemovableMedia(
    __in KWString& VolumeName,
    __out BOOLEAN& IsRemovableMedia
    )

/*++

Routine Description:

    This routine will return whether or not the given volume is a volume that deals with removable media, such as a floppy,
    cd-rom, jaz, etc...

Arguments:

    VolumeName          - Supplies the volume name.

    IsRemovableMedia    - Returns whether or not this device is a removable-media volume.

Return Value:

    NTSTATUS

--*/

{
    OBJECT_ATTRIBUTES oa;
    NTSTATUS status;
    HANDLE h;
    IO_STATUS_BLOCK ioStatus;
    FILE_FS_DEVICE_INFORMATION deviceInfo;


    InitializeObjectAttributes(&oa, VolumeName, OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = KNt::OpenFile(&h, FILE_READ_ATTRIBUTES | SYNCHRONIZE, &oa, &ioStatus,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, FILE_SYNCHRONOUS_IO_NONALERT);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = KNt::QueryVolumeInformationFile(h, &ioStatus, &deviceInfo, sizeof(deviceInfo), FileFsDeviceInformation);

    if (!NT_SUCCESS(status)) {
        goto Finish;
    }

    if (deviceInfo.Characteristics&FILE_REMOVABLE_MEDIA) {
        IsRemovableMedia = TRUE;
    } else {
        IsRemovableMedia = FALSE;
    }

Finish:

    KNt::Close(h);

    return status;
}



#if !defined(PLATFORM_UNIX)

NTSTATUS
KVolumeNamespaceStandard::QueryMountPoints(
    __out KBuffer::SPtr& OutputBuffer,
    __in KAllocator& Allocator
    )

/*++

Routine Description:

    This routine queries the full list of mount points from the mount point manager.  This list can be used to find
    all PNP visible volumes in the system.

Arguments:

    OutputBuffer    - Returns an allocated buffer that points to a full MOUNTMGR_MOUNT_POINTS structure.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE h = NULL;
    KWString deviceName(Allocator);
    OBJECT_ATTRIBUTES oa;
    IO_STATUS_BLOCK ioStatus;
    MOUNTMGR_MOUNT_POINT mountPoint;
    MOUNTMGR_MOUNT_POINTS* mountPoints;

    //
    // Create a string for the MOUNTMGR device name.
    //

    deviceName = MOUNTMGR_DEVICE_NAME;
    status = deviceName.Status();

    if (!NT_SUCCESS(status)) {
        goto Finish;
    }

    //
    // Open a handle to the mount point manager.
    //

    InitializeObjectAttributes(&oa, deviceName, OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = KNt::OpenFile(&h, FILE_READ_ATTRIBUTES | SYNCHRONIZE, &oa, &ioStatus,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, FILE_SYNCHRONOUS_IO_NONALERT);

    if (!NT_SUCCESS(status)) {
        h = NULL;
        goto Finish;
    }

    //
    // Allocate a receive buffer the list of volume mount points.
    //

    status = KBuffer::Create(sizeof(MOUNTMGR_MOUNT_POINTS), OutputBuffer, Allocator, KTL_TAG_NAMESPACE);

    if (!NT_SUCCESS(status)) {
        goto Finish;
    }


    //
    // Put in a query for all of the mount points in the system.  This gives us, as a superset, all of the PNP visible
    // volumes in the system.  Allocate a receive buffer of a reasonable size and repeat the call until the entire buffer
    // is queried.
    //

    RtlZeroMemory(&mountPoint, sizeof(mountPoint));

    for (;;) {

        //
        // Repeat this call until some status other than STATUS_BUFFER_OVERFLOW.
        //

        status = KNt::DeviceIoControlFile(h, NULL, NULL, NULL, &ioStatus, IOCTL_MOUNTMGR_QUERY_POINTS, &mountPoint,
                sizeof(mountPoint), OutputBuffer->GetBuffer(), OutputBuffer->QuerySize());

        mountPoints = (MOUNTMGR_MOUNT_POINTS*) OutputBuffer->GetBuffer();

        if (status != STATUS_BUFFER_OVERFLOW) {
            break;
        }

        status = KBuffer::Create(mountPoints->Size, OutputBuffer, Allocator, KTL_TAG_NAMESPACE);

        if (!NT_SUCCESS(status)) {
            goto Finish;
        }
    }

Finish:

    if (h) {
        KNt::Close(h);
    }

    return status;
}

LONG
KVolumeNamespaceStandard::Compare(
    __in DeviceNameNode& First,
    __in DeviceNameNode& Second
    )

/*++

Routine Description:

    This routine compares 2 device names.

Arguments:

    First   - Supplies the first device name.

    Seconds - Supplies the second device name.

Return Value:

    <0  - The first is less than the second.

    0   - The first and second are equal

    >0  - The first is greater than the second.

--*/

{
    return First.DeviceName.CompareTo(Second.DeviceName);
}

KVolumeNamespaceStandard::QueryVolumeListContext::~QueryVolumeListContext(
    )
{
    // Nothing.
}

KVolumeNamespaceStandard::QueryVolumeListContext::QueryVolumeListContext(
    __in_opt VolumeIdArray* VolumeList,
    __in_opt VolumeInformationArray* VolumeInformationList
    )
{
    Initialize(VolumeList, VolumeInformationList);
}

VOID
KVolumeNamespaceStandard::QueryVolumeListContext::Initialize(
    __in_opt VolumeIdArray* VolumeList,
    __in_opt VolumeInformationArray* VolumeInformationList
    )
{
    _VolumeList = VolumeList;
    _VolumeInformationList = VolumeInformationList;
}

VOID
KVolumeNamespaceStandard::QueryVolumeListContext::OnStart(
    )
{
    GetThisKtlSystem().DefaultSystemThreadPool().QueueWorkItem(*this);
}

VOID
KVolumeNamespaceStandard::QueryVolumeListContext::Execute(
    )

/*++

Routine Description:

    This routine will return a list of all of the volumes on the local system.

Arguments:

    None.

Return Value:

    None.

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    KBuffer::SPtr buffer;
    MOUNTMGR_MOUNT_POINTS* mountPoints;
    DeviceNameTable::CompareFunction compare(&KVolumeNamespaceStandard::Compare);
    DeviceNameTable deviceNameTable(FIELD_OFFSET(DeviceNameNode, TableEntry), compare);
    ULONG i;
    UNICODE_STRING deviceName;
    UNICODE_STRING symbolicLinkName;
    DeviceNameNode* node;
    BOOLEAN b;
    DeviceNameNode* inTableNode;
    KWString volumeName(GetThisAllocator());
    GUID stableGuid;
    BOOLEAN isRemovableMedia;
    VolumeInformation volInfo;

    //
    // Get the full list of mount points.
    //

    status = QueryMountPoints(buffer, GetThisAllocator());

    if (!NT_SUCCESS(status)) {
        goto Finish;
    }

    mountPoints = (MOUNTMGR_MOUNT_POINTS*) buffer->GetBuffer();

    //
    // Create a sort of all of the mount points by device name, eliminating duplicates.
    //

    for (i = 0; i < mountPoints->NumberOfMountPoints; i++) {

        //
        // Create unicode strings that points to the device name and symbolic link name for this mount point entry.
        //

        deviceName.Length = deviceName.MaximumLength = mountPoints->MountPoints[i].DeviceNameLength;
        deviceName.Buffer = (WCHAR*) ((UCHAR*) mountPoints + mountPoints->MountPoints[i].DeviceNameOffset);
        symbolicLinkName.Length = symbolicLinkName.MaximumLength = mountPoints->MountPoints[i].SymbolicLinkNameLength;
        symbolicLinkName.Buffer = (WCHAR*) ((UCHAR*) mountPoints + mountPoints->MountPoints[i].SymbolicLinkNameOffset);

        //
        // Allocate a node to insert into the table.
        //

        node = _new(KTL_TAG_NAMESPACE, GetThisAllocator()) DeviceNameNode(GetThisAllocator());

        if (!node) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Finish;
        }

        node->DeviceName = deviceName;

        status = node->DeviceName.Status();

        if (!NT_SUCCESS(status)) {
            _delete(node);
            goto Finish;
        }

        b = deviceNameTable.Insert(*node);

        if (!b) {

            //
            // This device name is already in the table.  Get 'node' to point to the entry that is already there.
            //

            inTableNode = deviceNameTable.Lookup(*node);
            _delete(node);
            node = inTableNode;
        }

        //
        // Now check for a drive letter or volume name that can be set in the 'node' structure.
        //

        if (MOUNTMGR_IS_DRIVE_LETTER(&symbolicLinkName)) {

            //
            // We have a drive letter.
            //

            node->DriveLetter = (UCHAR) symbolicLinkName.Buffer[12];

        } else if (MOUNTMGR_IS_VOLUME_NAME(&symbolicLinkName)) {

            //
            // We have a volume name.
            //

            volumeName = symbolicLinkName;

            status = volumeName.Status();

            if (!NT_SUCCESS(status)) {
                goto Finish;
            }

            //
            // Extract the volume id.
            //

            status = volumeName.ExtractGuidSuffix(node->VolumeId);

            if (!NT_SUCCESS(status)) {
                goto Finish;
            }
        }
    }

    //
    // Now we have a unique list of device names that are visible volumes in the system.  The list needs to be reduced
    // further by checking to see if the volume supports STABLE_GUID.  This will filter out the floppies, cds, and MBR partitions.
    //

    if (_VolumeList) {
        _VolumeList->Clear();
    }

    if (_VolumeInformationList) {
        _VolumeInformationList->Clear();
    }

    for (node = deviceNameTable.First(); node; node = deviceNameTable.Next(*node)) {

        status = QueryStableGuid(node->DeviceName, stableGuid);

        if (NT_SUCCESS(status)) {

            //
            // If this is a stable volume then return it in both cases.
            //

            if (_VolumeList) {

                status = _VolumeList->Append(stableGuid);

                if (!NT_SUCCESS(status)) {
                    goto Finish;
                }
            }

            if (_VolumeInformationList) {

                volInfo.VolumeId = stableGuid;
                volInfo.IsStableId = TRUE;
                volInfo.DriveLetter = node->DriveLetter;

                status = _VolumeInformationList->Append(volInfo);

                if (!NT_SUCCESS(status)) {
                    goto Finish;
                }
            }

            continue;
        }

        //
        // This is not really a failure, just an indication that there isn't a stable guid here.
        //

        status = STATUS_SUCCESS;

        //
        // If this is not a stable volume then only return in the case of the full information query.
        //

        if (!_VolumeInformationList) {
            continue;
        }

        status = QueryIsRemovableMedia(node->DeviceName, isRemovableMedia);

        if (!NT_SUCCESS(status)) {
            goto Finish;
        }

        //
        // Don't return removable media devices such as floppies and cdroms.
        //

        if (isRemovableMedia) {
            continue;
        }

        volInfo.VolumeId = node->VolumeId;
        volInfo.IsStableId = FALSE;
        volInfo.DriveLetter = node->DriveLetter;

        status = _VolumeInformationList->Append(volInfo);

        if (!NT_SUCCESS(status)) {
            goto Finish;
        }
    }

Finish:

    //
    // Clean up the table.
    //

    while ((node = deviceNameTable.First()) != NULL) {
        deviceNameTable.Remove(*node);
        _delete(node);
    }

    //
    // Complete the request.
    //

    if (!NT_SUCCESS(status)) {
        KTraceFailedAsyncRequest(status, this, 0, 0);
    }

    Complete(status);
}
#endif


KVolumeNamespaceStandard::CreateDirectoryContext::~CreateDirectoryContext(
    )
{
    // Nothing.
}

KVolumeNamespaceStandard::CreateDirectoryContext::CreateDirectoryContext(
    __in KWString& FullyQualifiedDirectoryName
    )
    :   _FullyQualifiedDirectoryName(GetThisAllocator())
{
    SetConstructorStatus(Initialize(FullyQualifiedDirectoryName));
}

NTSTATUS
KVolumeNamespaceStandard::CreateDirectoryContext::Initialize(
    __in KWString& FullyQualifiedDirectoryName
    )
{
    _FullyQualifiedDirectoryName = FullyQualifiedDirectoryName;

    return _FullyQualifiedDirectoryName.Status();
}


VOID
KVolumeNamespaceStandard::CreateDirectoryContext::OnStart(
    )
{
    GetThisKtlSystem().DefaultSystemThreadPool().QueueWorkItem(*this);
}

VOID
KVolumeNamespaceStandard::CreateDirectoryContext::Execute(
    )

/*++

Routine Description:

    This routine will simply create a directory.

Arguments:

    None.

Return Value:

    None.

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE h = NULL;
    OBJECT_ATTRIBUTES oa;
    IO_STATUS_BLOCK ioStatus;


    InitializeObjectAttributes(&oa, _FullyQualifiedDirectoryName, OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = KNt::CreateFile(&h, FILE_LIST_DIRECTORY | SYNCHRONIZE, &oa, &ioStatus, NULL, FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, FILE_CREATE,
            FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT, NULL, 0);

    if (!NT_SUCCESS(status)) {
        h = NULL;
        goto Finish;
    }

Finish:

    //
    // Close the handle.
    //

    if (h) {
        KNt::Close(h);
    }

    //
    // Complete the request.
    //

    if ((!NT_SUCCESS(status) && (status != STATUS_OBJECT_NAME_COLLISION))) {
        KTraceFailedAsyncRequest(status, this, 0, 0);
    }

    Complete(status);
}

KVolumeNamespaceStandard::DeleteFileOrDirectoryContext::~DeleteFileOrDirectoryContext(
    )
{
    // Nothing.
}

KVolumeNamespaceStandard::DeleteFileOrDirectoryContext::DeleteFileOrDirectoryContext(
    __in KWString& FullyQualifiedDirectoryName
    )
    :   _FullyQualifiedDirectoryName(GetThisAllocator())
{
    SetConstructorStatus(Initialize(FullyQualifiedDirectoryName));
}

NTSTATUS
KVolumeNamespaceStandard::DeleteFileOrDirectoryContext::Initialize(
    __in KWString& FullyQualifiedDirectoryName
    )
{
    _FullyQualifiedDirectoryName = FullyQualifiedDirectoryName;

    return _FullyQualifiedDirectoryName.Status();
}

VOID
KVolumeNamespaceStandard::DeleteFileOrDirectoryContext::OnStart(
    )
{
    GetThisKtlSystem().DefaultSystemThreadPool().QueueWorkItem(*this);
}

VOID
KVolumeNamespaceStandard::DeleteFileOrDirectoryContext::Execute(
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE h = NULL;
    OBJECT_ATTRIBUTES oa;
    IO_STATUS_BLOCK ioStatus;
    FILE_DISPOSITION_INFORMATION dispInfo;

    //
    // Open the directory.
    //

    InitializeObjectAttributes(&oa, _FullyQualifiedDirectoryName, OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = KNt::OpenFile(&h, DELETE | SYNCHRONIZE, &oa, &ioStatus, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT);

    if (!NT_SUCCESS(status)) {
        h = NULL;
        goto Finish;
    }

    //
    // Set the disposition information.
    //


#if KTL_USER_MODE
#undef DeleteFile
#endif

    dispInfo.DeleteFile = TRUE;

    status = KNt::SetInformationFile(h, &ioStatus, &dispInfo, sizeof(dispInfo), FileDispositionInformation);

Finish:

    //
    // Close the handle.
    //

    if (h) {
        KNt::Close(h);
    }

    //
    // Complete the request.
    //

    Complete(status);
}

KVolumeNamespaceStandard::QueryDirectoriesContext::~QueryDirectoriesContext(
    )
{
    // Nothing.
}

KVolumeNamespaceStandard::QueryDirectoriesContext::QueryDirectoriesContext(
    __in KWString& FullyQualifiedDirectoryName,
    __inout NameArray& DirectoryNameArray
    )
    :   _FullyQualifiedDirectoryName(GetThisAllocator())
{
    SetConstructorStatus(Initialize(FullyQualifiedDirectoryName, DirectoryNameArray));
}

NTSTATUS
KVolumeNamespaceStandard::QueryDirectoriesContext::Initialize(
    __in KWString& FullyQualifiedDirectoryName,
    __inout NameArray& DirectoryNameArray
    )
{
    _FullyQualifiedDirectoryName = FullyQualifiedDirectoryName;
    _DirectoryNameArray = &DirectoryNameArray;

    return _FullyQualifiedDirectoryName.Status();
}

VOID
KVolumeNamespaceStandard::QueryDirectoriesContext::OnStart(
    )
{
    GetThisKtlSystem().DefaultSystemThreadPool().QueueWorkItem(*this);
}

VOID
KVolumeNamespaceStandard::QueryDirectoriesContext::Execute(
    )

/*++

Routine Description:

    This routine enumerates the given directory and then returns the directories within that directory.

Arguments:

    None.

Return Value:

    None.

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE h = NULL;
    OBJECT_ATTRIBUTES oa;
    IO_STATUS_BLOCK ioStatus;
    KBuffer::SPtr dirInfoBuffer;
    FILE_DIRECTORY_INFORMATION* dirInfo;
    UNICODE_STRING fileName;
    KWString fileNameString(GetThisAllocator());
    LONG r;

    //
    // Open the directory.
    //

    InitializeObjectAttributes(&oa, _FullyQualifiedDirectoryName, OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = KNt::OpenFile(&h, FILE_LIST_DIRECTORY | SYNCHRONIZE, &oa, &ioStatus,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT);

    if (status == STATUS_INVALID_PARAMETER || status == STATUS_NOT_A_DIRECTORY) {

        //
        // This might be the root of the device, so, add a backslash to denote the root directory, and try again.
        //

        _FullyQualifiedDirectoryName += KVolumeNamespace::PathSeparator;

        status = _FullyQualifiedDirectoryName.Status();

        if (!NT_SUCCESS(status)) {
            goto Finish;
        }

        InitializeObjectAttributes(&oa, _FullyQualifiedDirectoryName, OBJ_CASE_INSENSITIVE, NULL, NULL);

        status = KNt::OpenFile(&h, FILE_LIST_DIRECTORY | SYNCHRONIZE, &oa, &ioStatus,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT);
    }

    if (!NT_SUCCESS(status)) {
        h = NULL;
        goto Finish;
    }

    //
    // Create a large buffer to receive a single directory entry.
    //

    status = KBuffer::Create(0x1000, dirInfoBuffer, GetThisAllocator(), KTL_TAG_NAMESPACE);

    if (!NT_SUCCESS(status)) {
        goto Finish;
    }

    dirInfo = (FILE_DIRECTORY_INFORMATION*) dirInfoBuffer->GetBuffer();

    //
    // Enumerate the directory, one entry at a time.
    //

    _DirectoryNameArray->Clear();

    for (;;) {

        status = KNt::QueryDirectoryFile(h, NULL, NULL, NULL, &ioStatus, dirInfoBuffer->GetBuffer(), dirInfoBuffer->QuerySize(),
                FileDirectoryInformation, TRUE, NULL, FALSE);

        if (!NT_SUCCESS(status)) {

            //
            // A failure here marks the end of the enumeration.
            //

            status = STATUS_SUCCESS;

            break;
        }

        //
        // Check to see whether this is a file or directory.  Skip files.
        //

        if (!(dirInfo->FileAttributes&FILE_ATTRIBUTE_DIRECTORY)) {
            continue;
        }

        //
        // Create a unicode string for the returned file name.
        //

        fileName.Length = fileName.MaximumLength = (USHORT) dirInfo->FileNameLength;
        fileName.Buffer = dirInfo->FileName;

        //
        // Create a KWString from this unicode string.
        //

        fileNameString = fileName;

        //
        // Add this string to the array, unless it is "." or "..".
        //

        r = fileNameString.CompareTo(L".");
        if (!r) {
            continue;
        }

        r = fileNameString.CompareTo(L"..");
        if (!r) {
            continue;
        }

        status = _DirectoryNameArray->Append(fileNameString);

        if (!NT_SUCCESS(status)) {
            goto Finish;
        }
    }

Finish:

    //
    // Close the handle.
    //

    if (h) {
        KNt::Close(h);
    }

    //
    // Complete the request.
    //

    if (!NT_SUCCESS(status)) {
        KTraceFailedAsyncRequest(status, this, 0, 0);
    }

    Complete(status);
}

KVolumeNamespaceStandard::QueryFilesContext::~QueryFilesContext(
    )
{
    // Nothing.
}

KVolumeNamespaceStandard::QueryFilesContext::QueryFilesContext(
    __in KWString& FullyQualifiedDirectoryName,
    __inout NameArray& FileNameArray
    )
    :   _FullyQualifiedDirectoryName(GetThisAllocator())
{
    SetConstructorStatus(Initialize(FullyQualifiedDirectoryName, FileNameArray));
}

NTSTATUS
KVolumeNamespaceStandard::QueryFilesContext::Initialize(
    __in KWString& FullyQualifiedDirectoryName,
    __inout NameArray& FileNameArray
    )
{
    _FullyQualifiedDirectoryName = FullyQualifiedDirectoryName;
    _FileNameArray = &FileNameArray;

    return _FullyQualifiedDirectoryName.Status();
}

VOID
KVolumeNamespaceStandard::QueryFilesContext::OnStart(
    )
{
    GetThisKtlSystem().DefaultSystemThreadPool().QueueWorkItem(*this);
}

VOID
KVolumeNamespaceStandard::QueryFilesContext::Execute(
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE h = NULL;
    OBJECT_ATTRIBUTES oa;
    IO_STATUS_BLOCK ioStatus;
    KBuffer::SPtr dirInfoBuffer;
    FILE_DIRECTORY_INFORMATION* dirInfo;
    UNICODE_STRING fileName;
    KWString fileNameString(GetThisAllocator());

    //
    // Open the directory.
    //

    InitializeObjectAttributes(&oa, _FullyQualifiedDirectoryName, OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = KNt::OpenFile(&h, FILE_LIST_DIRECTORY | SYNCHRONIZE, &oa, &ioStatus,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT);

    if (status == STATUS_INVALID_PARAMETER || status == STATUS_NOT_A_DIRECTORY) {

        //
        // This might be the root of the device, so, add a backslash to denote the root directory, and try again.
        //

        _FullyQualifiedDirectoryName += KVolumeNamespace::PathSeparator;

        status = _FullyQualifiedDirectoryName.Status();

        if (!NT_SUCCESS(status)) {
            goto Finish;
        }

        InitializeObjectAttributes(&oa, _FullyQualifiedDirectoryName, OBJ_CASE_INSENSITIVE, NULL, NULL);

        status = KNt::OpenFile(&h, FILE_LIST_DIRECTORY | SYNCHRONIZE, &oa, &ioStatus,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT);
    }

    if (!NT_SUCCESS(status)) {
        h = NULL;
        goto Finish;
    }

    //
    // Create a large buffer to receive a single directory entry.
    //

    status = KBuffer::Create(0x1000, dirInfoBuffer, GetThisAllocator(), KTL_TAG_NAMESPACE);

    if (!NT_SUCCESS(status)) {
        goto Finish;
    }

    dirInfo = (FILE_DIRECTORY_INFORMATION*) dirInfoBuffer->GetBuffer();

    //
    // Enumerate the directory, one entry at a time.
    //

    _FileNameArray->Clear();

    for (;;) {

        status = KNt::QueryDirectoryFile(h, NULL, NULL, NULL, &ioStatus, dirInfoBuffer->GetBuffer(), dirInfoBuffer->QuerySize(),
                FileDirectoryInformation, TRUE, NULL, FALSE);

        if (!NT_SUCCESS(status)) {

            //
            // A failure here marks the end of the enumeration.
            //

            status = STATUS_SUCCESS;

            break;
        }

        //
        // Check to see whether this is a file or directory.  Skip files.
        //

        if (dirInfo->FileAttributes&FILE_ATTRIBUTE_DIRECTORY) {
            continue;
        }

        //
        // Create a unicode string for the returned file name.
        //

        fileName.Length = fileName.MaximumLength = (USHORT) dirInfo->FileNameLength;
        fileName.Buffer = dirInfo->FileName;

        //
        // Create a KWString from this unicode string.
        //

        fileNameString = fileName;

        //
        // Add this string to the array.
        //

        status = _FileNameArray->Append(fileNameString);

        if (!NT_SUCCESS(status)) {
            goto Finish;
        }
    }

Finish:

    //
    // Close the handle.
    //

    if (h) {
        KNt::Close(h);
    }

    //
    // Complete the request.
    //

    if (!NT_SUCCESS(status)) {
        KTraceFailedAsyncRequest(status, this, 0, 0);
    }

    Complete(status);
}

#if !defined(PLATFORM_UNIX)

KVolumeNamespaceStandard::DeviceNameNode::DeviceNameNode(KAllocator& Allocator)
    :   DeviceName(Allocator)
{
    RtlZeroMemory(&VolumeId, sizeof(GUID));
    DriveLetter = 0;
}
#endif

#if !defined(PLATFORM_UNIX)
//** HardLink Query and Set Support

//* HardLink Query
class KVolumeNamespaceStandard::QueryHardLinksOp : public KAsyncContextBase, public KThreadPool::WorkItem
{
    K_FORCE_SHARED(QueryHardLinksOp);

public:
    static
    NTSTATUS
    Create(__in KAllocator& Allocator, __out QueryHardLinksOp::SPtr& Result)
    {
        Result = _new(KTL_TAG_NAMESPACE, Allocator) QueryHardLinksOp();
        if (Result == nullptr)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        return Result->Status();
    }

    NTSTATUS
    SetQuery(
        __in KWString& FullyQualifiedLinkName,
        __out NameArray& FileNameArray)
    {
        _FullyQualifiedLinkName = FullyQualifiedLinkName;
        if (!NT_SUCCESS(_FullyQualifiedLinkName.Status()))
        {
            return _FullyQualifiedLinkName.Status();
        }

        FileNameArray.Clear();
        _Result = &FileNameArray;

        return STATUS_SUCCESS;
    }
    
    NTSTATUS
    StartQuery(
        __in KWString& FullyQualifiedLinkName,
        __out NameArray& FileNameArray,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync)
    {
        NTSTATUS status = SetQuery(FullyQualifiedLinkName, FileNameArray);

        if (! NT_SUCCESS(status))
        {
            return(status);
        }

        Start(ParentAsync, Completion);
        return STATUS_PENDING;
    }

private:
    void
    OnStart() override
    {
        // Simple schedule on worker thread
        GetThisKtlSystem().DefaultSystemThreadPool().QueueWorkItem(*this);

        // Continued @ Execute()
    }

    void
    Execute() override
    {
        HANDLE              linkHandle = 0;
        HANDLE              volHandle = 0;
        OBJECT_ATTRIBUTES   oa;
        IO_STATUS_BLOCK     ioStatus;
        GUID                volumeGuid;
        KWString            volumeName(GetThisAllocator());
        KWString            volumeRelPath(GetThisAllocator());

        NTSTATUS            status = STATUS_SUCCESS;
        KFinally([&]()
        {
            if (linkHandle != 0)
            {
                KNt::Close(linkHandle);
            }

            if (volHandle != 0)
            {
                KNt::Close(volHandle);
            }

            Complete(status);
        });

        status = KVolumeNamespaceStandard::SplitAndNormalizeObjectPath(_FullyQualifiedLinkName, volumeName, volumeRelPath);
        if (!NT_SUCCESS(status))
        {
            return;
        }

        KStringView         rawVolumeRelPath((WCHAR*)volumeRelPath);

        status = KVolumeNamespaceStandard::QueryVolumeGuid(GetThisAllocator(), volumeName, volumeGuid);
        if (!NT_SUCCESS(status))
        {
            return;
        }

        status = KVolumeNamespaceStandard::CreateFullyQualifiedRootDirectoryName(volumeGuid, volumeName);
        if (!NT_SUCCESS(status))
        {
            return;
        }

        // Open the reference HardLinks
        InitializeObjectAttributes(&oa, _FullyQualifiedLinkName, OBJ_CASE_INSENSITIVE, NULL, NULL);
        status = KNt::OpenFile(&linkHandle, FILE_GENERIC_READ, &oa, &ioStatus, 0, 0);
        if (!NT_SUCCESS(status))
        {
            return;
        }
        KInvariant(linkHandle != 0);

        // Open the volume that conatins the reference hard links
        InitializeObjectAttributes(&oa, volumeName, OBJ_CASE_INSENSITIVE, NULL, NULL);
        status = KNt::OpenFile(
                &volHandle,
                FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                &oa, &ioStatus,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, FILE_SYNCHRONOUS_IO_NONALERT);

        if (!NT_SUCCESS(status))
        {
            return;
        }
        KInvariant(volHandle != 0);

        // Compute needed space and allocate buffer for hard link info
        FILE_LINKS_INFORMATION      info;
        status = KNt::QueryInformationFile(
            linkHandle,
            &ioStatus,
            &info,
            sizeof(info),
            FILE_INFORMATION_CLASS::FileHardLinkInformation);

        KInvariant(status == STATUS_BUFFER_OVERFLOW);

        KUniquePtr<FILE_LINKS_INFORMATION> fli =
            (FILE_LINKS_INFORMATION*)_new(KTL_TAG_NAMESPACE, GetThisAllocator()) UCHAR[info.BytesNeeded];

        if (!fli)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            return;
        }

        status = KNt::QueryInformationFile(
            linkHandle,
            &ioStatus,
            &(*fli),
            info.BytesNeeded,
            FILE_INFORMATION_CLASS::FileHardLinkInformation);

        if (!NT_SUCCESS(status))
        {
            return;
        }

        NameArray       results(GetThisAllocator(), fli->EntriesReturned - 1);
        status = results.Status();
        if (!NT_SUCCESS(status))
        {
            return;
        }

        FILE_LINK_ENTRY_INFORMATION*    entry = &fli->Entry;
        KBuffer::SPtr                   fileInfoBuffer;
        status = KBuffer::Create(sizeof(FILE_NAME_INFORMATION), fileInfoBuffer, GetThisAllocator());
        if (!NT_SUCCESS(status))
        {
            return;
        }

        for (ULONG ix = 0; ix < fli->EntriesReturned; ix++)
        {
            // Open parent directory by its FileId and get its volume-relative path
            HANDLE              parentDir = 0;
            KFinally([&]()
            {
                if (parentDir != 0)
                {
                    KNt::Close(parentDir);
                    parentDir = 0;
                }
            });

            UNICODE_STRING      dirFileId;
            OBJECT_ATTRIBUTES   oa1;

            dirFileId.Buffer = (PWCH)&entry->ParentFileId;
            dirFileId.Length = dirFileId.MaximumLength = (USHORT)sizeof(PWCH);
            InitializeObjectAttributes(&oa1, &dirFileId, OBJ_CASE_INSENSITIVE, volHandle, NULL);

            status = KNt::OpenFile(
                &parentDir,
                FILE_LIST_DIRECTORY | SYNCHRONIZE,
                &oa1,
                &ioStatus,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_OPEN_BY_FILE_ID| FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT);

            if (!NT_SUCCESS(status))
            {
                return;
            }
            KInvariant(parentDir != 0);

            memset(fileInfoBuffer->GetBuffer(), 0, fileInfoBuffer->QuerySize());
            status = KNt::QueryInformationFile(
                parentDir,
                &ioStatus,
                fileInfoBuffer->GetBuffer(),
                fileInfoBuffer->QuerySize(),
                FILE_INFORMATION_CLASS::FileNameInformation);

            if (status == STATUS_BUFFER_OVERFLOW)
            {
                FILE_NAME_INFORMATION&  fni = *(FILE_NAME_INFORMATION*)fileInfoBuffer->GetBuffer();
                status = fileInfoBuffer->SetSize(sizeof(FILE_NAME_INFORMATION) + fni.FileNameLength);

                if (NT_SUCCESS(status))
                {
                    status = KNt::QueryInformationFile(
                        parentDir,
                        &ioStatus,
                        fileInfoBuffer->GetBuffer(),
                        fileInfoBuffer->QuerySize(),
                        FILE_INFORMATION_CLASS::FileNameInformation);
                }
            }

            FILE_NAME_INFORMATION&  fni = *(FILE_NAME_INFORMATION*)fileInfoBuffer->GetBuffer();

            // Build up the volume-relative path to current hardlink
            KWString                fniVolRelativePath(GetThisAllocator());

            UNICODE_STRING  ucs;
            ucs.Buffer = fni.FileName;
            if (entry->FileNameLength > (MAXUSHORT / 2))
            {
                status = STATUS_BUFFER_OVERFLOW;
                return;
            }
            ucs.Length = ucs.MaximumLength = (USHORT)(fni.FileNameLength);

            fniVolRelativePath += ucs;
            status = fniVolRelativePath.Status();
            if (!NT_SUCCESS(status))
            {
                return;
            }

            WCHAR* p = fniVolRelativePath;
            ULONG lastChar = fniVolRelativePath.Length() / sizeof(WCHAR);

            if ((lastChar == 0) || (p[lastChar-1] != L'\\'))
            {
                fniVolRelativePath += L"\\";
                status = fniVolRelativePath.Status();
                if (!NT_SUCCESS(status))
                {
                    return;
                }
            }

            ucs.Buffer = entry->FileName;
            if (entry->FileNameLength > (MAXUSHORT / 2))
            {
                status = STATUS_BUFFER_OVERFLOW;
                return;
            }
            ucs.Length = ucs.MaximumLength = (USHORT)(entry->FileNameLength * 2);

            fniVolRelativePath += ucs;
            status = fniVolRelativePath.Status();
            if (!NT_SUCCESS(status))
            {
                return;
            }

            // Drop reference hardlink from result set
            if (rawVolumeRelPath.CompareNoCase(KStringView((UNICODE_STRING &)fniVolRelativePath)) != 0)
            {
                // build up a full volume path the
                KWString        linkStr(volumeName);
                status = linkStr.Status();
                if (!NT_SUCCESS(status))
                {
                    return;
                }

                linkStr += fniVolRelativePath;
                status = linkStr.Status();
                if (!NT_SUCCESS(status))
                {
                    return;
                }

                status = results.Append(Ktl::Move(linkStr));
                if (!NT_SUCCESS(status))
                {
                    return;
                }
            }

            entry = (FILE_LINK_ENTRY_INFORMATION*)(((UCHAR*)entry) + entry->NextEntryOffset);
        }

        *_Result = Ktl::Move(results);
        status = _Result->Status();
    }

private:
    // Op state
    KWString        _FullyQualifiedLinkName;
    NameArray*      _Result;
};

KVolumeNamespaceStandard::QueryHardLinksOp::QueryHardLinksOp()
    :   _FullyQualifiedLinkName(GetThisAllocator()),
        _Result(nullptr)
{
}

KVolumeNamespaceStandard::QueryHardLinksOp::~QueryHardLinksOp()
{
}
#endif // !PLATFORM_UNIX

#if !defined(PLATFORM_UNIX)
NTSTATUS
KVolumeNamespace::QueryHardLinks(
    __in KWString& FullyQualifiedLinkName,
    __in KAllocator& Allocator,
    __out NameArray& FileNameArray,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync)
{
    KVolumeNamespaceStandard::QueryHardLinksOp::SPtr    op;
    NTSTATUS    status = KVolumeNamespaceStandard::QueryHardLinksOp::Create(Allocator, op);

    if (NT_SUCCESS(status))
    {
        status = op->StartQuery(FullyQualifiedLinkName, FileNameArray, Completion, ParentAsync);
    }

    return status;
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KVolumeNamespace::QueryHardLinksAsync(
    __in KWString& FullyQualifiedLinkName,
    __in KAllocator& Allocator,
    __out NameArray& FileNameArray,
    __in_opt KAsyncContextBase* const ParentAsync)
{
    KVolumeNamespaceStandard::QueryHardLinksOp::SPtr    op;
    NTSTATUS    status = KVolumeNamespaceStandard::QueryHardLinksOp::Create(Allocator, op);

    status = op->SetQuery(FullyQualifiedLinkName, FileNameArray);
    
    if (NT_SUCCESS(status))
    {
        ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)op.RawPtr()), ParentAsync, NULL);
        status = co_await awaiter;      
    }

    co_return status;   
}
#endif

//* HardLink Set
class KVolumeNamespaceStandard::SetHardLinksOp : public KAsyncContextBase, public KThreadPool::WorkItem
{
    K_FORCE_SHARED(SetHardLinksOp);

public:
    static
    NTSTATUS
    Create(__in KAllocator& Allocator, __out SetHardLinksOp::SPtr& Result)
    {
        Result = _new(KTL_TAG_NAMESPACE, Allocator) SetHardLinksOp();
        if (Result == nullptr)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        return Result->Status();
    }

    NTSTATUS
    SetSet(
        __in KWString& FullyQualifiedExistingLinkName,
        __in KWString& FullyQualifiedNewLinkName)
    {
        _FullyQualifiedExistingLinkName = FullyQualifiedExistingLinkName;
        if (!NT_SUCCESS(_FullyQualifiedExistingLinkName.Status()))
        {
            return _FullyQualifiedExistingLinkName.Status();
        }

        _FullyQualifiedNewLinkName = FullyQualifiedNewLinkName;
        if (!NT_SUCCESS(_FullyQualifiedNewLinkName.Status()))
        {
            return _FullyQualifiedNewLinkName.Status();
        }

        return STATUS_SUCCESS;
    }

    NTSTATUS
    StartSet(
        __in KWString& FullyQualifiedExistingLinkName,
        __in KWString& FullyQualifiedNewLinkName,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync)
    {
        NTSTATUS status = SetSet(FullyQualifiedExistingLinkName, FullyQualifiedNewLinkName);
        if (! NT_SUCCESS(status))
        {
            return(status);
        }

        Start(ParentAsync, Completion);
        return STATUS_PENDING;
    }
        
    NTSTATUS
    SetSet(
        __in HANDLE LinkHandle,
        __in KWString& FullyQualifiedNewLinkName)
    {
        _LinkHandle = LinkHandle;
        _FullyQualifiedNewLinkName = FullyQualifiedNewLinkName;
        if (!NT_SUCCESS(_FullyQualifiedNewLinkName.Status()))
        {
            return _FullyQualifiedNewLinkName.Status();
        }

        return STATUS_SUCCESS;
    }

    NTSTATUS
    StartSet(
        __in HANDLE LinkHandle,
        __in KWString& FullyQualifiedNewLinkName,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync)
    {
        _LinkHandle = LinkHandle;
        _FullyQualifiedNewLinkName = FullyQualifiedNewLinkName;
        if (!NT_SUCCESS(_FullyQualifiedNewLinkName.Status()))
        {
            return _FullyQualifiedNewLinkName.Status();
        }

        NTSTATUS status = SetSet(LinkHandle, FullyQualifiedNewLinkName);
        if (! NT_SUCCESS(status))
        {
            return(status);
        }

        Start(ParentAsync, Completion);
        return STATUS_PENDING;
    }
    
private:
    void
    OnStart() override
    {
        // Simple schedule on worker thread
        GetThisKtlSystem().DefaultSystemThreadPool().QueueWorkItem(*this);

        // Continued @ Execute()
    }

    void
    Execute() override
    {
        HANDLE              linkHandle = _LinkHandle;
        OBJECT_ATTRIBUTES   oa;
        IO_STATUS_BLOCK     ioStatus;
        NTSTATUS            status = STATUS_SUCCESS;
        BOOLEAN             useTempLinkHandle = FALSE;

        if (linkHandle == 0)
        {
            InitializeObjectAttributes(&oa, _FullyQualifiedExistingLinkName, OBJ_CASE_INSENSITIVE, NULL, NULL);

            status = KNt::OpenFile(&linkHandle, FILE_GENERIC_READ, &oa, &ioStatus, 0, 0);
            if (!NT_SUCCESS(status))
            {
                Complete(status);
                return;
            }

            useTempLinkHandle = TRUE;
        }

        KFinally([&]()
        {
            if (useTempLinkHandle)
            {
                KNt::Close(linkHandle);
            }
        });

        HRESULT hr;
        ULONG   sizeofNewLinkInfo;
        hr = ULongAdd(sizeof(FILE_LINK_INFORMATION), _FullyQualifiedNewLinkName.Length(), &sizeofNewLinkInfo);
        KInvariant(SUCCEEDED(hr));
        KUniquePtr<FILE_LINK_INFORMATION>   newLinkInfo =
            (FILE_LINK_INFORMATION*)_new(KTL_TAG_NAMESPACE, GetThisAllocator()) UCHAR[sizeofNewLinkInfo];

        if (!newLinkInfo)
        {
            Complete(STATUS_INSUFFICIENT_RESOURCES);
            return;
        }

        newLinkInfo->ReplaceIfExists = FALSE;
        newLinkInfo->RootDirectory = NULL;
        newLinkInfo->FileNameLength = _FullyQualifiedNewLinkName.Length();
        KMemCpySafe(newLinkInfo->FileName, newLinkInfo->FileNameLength, (WCHAR*)_FullyQualifiedNewLinkName, newLinkInfo->FileNameLength);

        status = KNt::SetInformationFile(
            linkHandle,
            &ioStatus,
            &(*newLinkInfo),
            sizeofNewLinkInfo,
            FILE_INFORMATION_CLASS::FileLinkInformation);

        Complete(status);
    }

private:
    // Op state
    HANDLE          _LinkHandle;
    KWString        _FullyQualifiedExistingLinkName;
    KWString        _FullyQualifiedNewLinkName;
};

KVolumeNamespaceStandard::SetHardLinksOp::SetHardLinksOp()
    :   _FullyQualifiedExistingLinkName(GetThisAllocator()),
        _FullyQualifiedNewLinkName(GetThisAllocator()),
        _LinkHandle(0)
{
}

KVolumeNamespaceStandard::SetHardLinksOp::~SetHardLinksOp()
{
}
#endif // !PLATFORM_UNIX

#if !defined(PLATFORM_UNIX)
NTSTATUS
KVolumeNamespace::SetHardLink(
    __in KWString& FullyQualifiedExistingLinkName,
    __in KWString& FullyQualifiedNewLinkName,
    __in KAllocator& Allocator,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync)
{
    KVolumeNamespaceStandard::SetHardLinksOp::SPtr    op;
    NTSTATUS    status = KVolumeNamespaceStandard::SetHardLinksOp::Create(Allocator, op);

    if (NT_SUCCESS(status))
    {
        status = op->StartSet(
            FullyQualifiedExistingLinkName,
            FullyQualifiedNewLinkName,
            Completion,
            ParentAsync);
    }

    return status;
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KVolumeNamespace::SetHardLinkAsync(
    __in KWString& FullyQualifiedExistingLinkName,
    __in KWString& FullyQualifiedNewLinkName,
    __in KAllocator& Allocator,
    __in_opt KAsyncContextBase* const ParentAsync)
{
    KVolumeNamespaceStandard::SetHardLinksOp::SPtr    op;
    NTSTATUS    status = KVolumeNamespaceStandard::SetHardLinksOp::Create(Allocator, op);

    if (NT_SUCCESS(status))
    {
        status = op->SetSet(FullyQualifiedExistingLinkName, FullyQualifiedNewLinkName);

        if (NT_SUCCESS(status))
        {
            ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)op.RawPtr()), ParentAsync, NULL);
            status = co_await awaiter;
        }
    }

    co_return status;
}
#endif
#endif

#if !defined(PLATFORM_UNIX)
NTSTATUS
KVolumeNamespace::SetHardLink(
    __in HANDLE LinkHandle,
    __in KWString& FullyQualifiedNewLinkName,
    __in KAllocator& Allocator,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync)
{
    KVolumeNamespaceStandard::SetHardLinksOp::SPtr    op;
    NTSTATUS    status = KVolumeNamespaceStandard::SetHardLinksOp::Create(Allocator, op);

    if (NT_SUCCESS(status))
    {
        status = op->StartSet(
            LinkHandle,
            FullyQualifiedNewLinkName,
            Completion,
            ParentAsync);
    }

    return status;
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KVolumeNamespace::SetHardLinkAsync(
    __in HANDLE LinkHandle,
    __in KWString& FullyQualifiedNewLinkName,
    __in KAllocator& Allocator,
    __in_opt KAsyncContextBase* const ParentAsync)
{
    KVolumeNamespaceStandard::SetHardLinksOp::SPtr    op;
    NTSTATUS    status = KVolumeNamespaceStandard::SetHardLinksOp::Create(Allocator, op);

    if (NT_SUCCESS(status))
    {
        status = op->SetSet(LinkHandle, FullyQualifiedNewLinkName);
        if (NT_SUCCESS(status))
        {
            ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)op.RawPtr()), ParentAsync, NULL);
            status = co_await awaiter;
        }       
    }

    co_return status;
}
#endif

//** Volume name normalization helpers
class KVolumeNamespaceStandard::QueryGlobalVolumeDirectoryNameOp : public KAsyncContextBase, public KThreadPool::WorkItem
{
    K_FORCE_SHARED(QueryGlobalVolumeDirectoryNameOp);

public:
    static
    NTSTATUS
    Create(__in KAllocator& Allocator, __out QueryGlobalVolumeDirectoryNameOp::SPtr& Result)
    {
        Result = _new(KTL_TAG_NAMESPACE, Allocator) QueryGlobalVolumeDirectoryNameOp();
        if (Result == nullptr)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        return Result->Status();
    }

    NTSTATUS
    SetQuery(
        __in KWString& FullyQualifiedVolumeDirectoryName,
        __out KWString& GlobalDirectoryName)
    {
        GlobalDirectoryName.Reset();

        _FullyQualifiedVolumeDirectoryName = FullyQualifiedVolumeDirectoryName;
        if (!NT_SUCCESS(_FullyQualifiedVolumeDirectoryName.Status()))
        {
            return _FullyQualifiedVolumeDirectoryName.Status();
        }

        _ResultingGlobalDirectoryName = &GlobalDirectoryName;

        return STATUS_SUCCESS;
    }

    NTSTATUS
    StartQuery(
        __in KWString& FullyQualifiedVolumeDirectoryName,
        __out KWString& GlobalDirectoryName,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync)
    {
        NTSTATUS status = SetQuery(FullyQualifiedVolumeDirectoryName, GlobalDirectoryName);
        if (! NT_SUCCESS(status))
        {
            return status;
        }
        
        Start(ParentAsync, Completion);
        return STATUS_PENDING;
    }

private:
    void
    OnStart() override
    {
        // Simple schedule on worker thread
        GetThisKtlSystem().DefaultSystemThreadPool().QueueWorkItem(*this);

        // Continued @ Execute()
    }

    void
    Execute() override
    {
        NTSTATUS        status = STATUS_SUCCESS;
        GUID            volGuid;

        status = KVolumeNamespaceStandard::QueryVolumeGuid(GetThisAllocator(), _FullyQualifiedVolumeDirectoryName, volGuid);
        if (!NT_SUCCESS(status))
        {
            Complete(status);
            return;
        }

        status = KVolumeNamespace::CreateFullyQualifiedRootDirectoryName(volGuid, *_ResultingGlobalDirectoryName);
        Complete(status);
    }

private:
    // Op state
    KWString        _FullyQualifiedVolumeDirectoryName;
    KWString*       _ResultingGlobalDirectoryName;
};

KVolumeNamespaceStandard::QueryGlobalVolumeDirectoryNameOp::QueryGlobalVolumeDirectoryNameOp()
    :   _FullyQualifiedVolumeDirectoryName(GetThisAllocator()),
        _ResultingGlobalDirectoryName(nullptr)
{
}

KVolumeNamespaceStandard::QueryGlobalVolumeDirectoryNameOp::~QueryGlobalVolumeDirectoryNameOp()
{
}

//** Convert a fully qualified volume name to corresponding "\GLOBAL??\volume{guid}" form
//
//  Parameters:
//      FullyQualifiedVolumeDirectoryName - FQ Name of the volume (e.g. "\??\C:" or "\global??\c:", ...)
//      GlobalDirectoryName - Resulting normalized form of the volume directory name
//
NTSTATUS
KVolumeNamespace::QueryGlobalVolumeDirectoryName(
    __in KWString& FullyQualifiedVolumeDirectoryName,
    __in KAllocator& Allocator,
    __out KWString& GlobalDirectoryName,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync)
{
    KVolumeNamespaceStandard::QueryGlobalVolumeDirectoryNameOp::SPtr    op;

    NTSTATUS status = KVolumeNamespaceStandard::QueryGlobalVolumeDirectoryNameOp::Create(
        Allocator,
        op);

    if (!NT_SUCCESS(status))
    {
        GlobalDirectoryName.Reset();
        return status;
    }

    return op->StartQuery(FullyQualifiedVolumeDirectoryName, GlobalDirectoryName, Completion, ParentAsync);
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KVolumeNamespace::QueryGlobalVolumeDirectoryNameAsync(
    __in KWString& FullyQualifiedVolumeDirectoryName,
    __in KAllocator& Allocator,
    __out KWString& GlobalDirectoryName,
    __in_opt KAsyncContextBase* const ParentAsync)
{
    KVolumeNamespaceStandard::QueryGlobalVolumeDirectoryNameOp::SPtr    op;

    NTSTATUS status = KVolumeNamespaceStandard::QueryGlobalVolumeDirectoryNameOp::Create(
        Allocator,
        op);

    status = op->SetQuery(FullyQualifiedVolumeDirectoryName, GlobalDirectoryName);
    if (!NT_SUCCESS(status))
    {
        co_return status;
    }
    
    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)op.RawPtr()), ParentAsync, NULL);
    status = co_await awaiter;
    
    co_return status;
}
#endif

//** Retrieve the stable volume id (GUID) for a given volume identfied by it FQ name
class KVolumeNamespaceStandard::GetVolumeIdOp : public KAsyncContextBase, public KThreadPool::WorkItem
{
    K_FORCE_SHARED(GetVolumeIdOp);

private:
    KWString        _FullyQualifiedRootDirectoryName;
    GUID&           _VolumeId;

public:
    GetVolumeIdOp(__in KWString& FullyQualifiedRootDirectoryName, __out GUID& VolumeId);

private:
    void
    OnStart() override
    {
        // Simple schedule on worker thread
        GetThisKtlSystem().DefaultSystemThreadPool().QueueWorkItem(*this);

        // Continued @ Execute()
    }

    void
    Execute() override
    {
        Complete(KVolumeNamespaceStandard::QueryVolumeGuid(GetThisAllocator(), _FullyQualifiedRootDirectoryName, _VolumeId));
    }

public:
    static
    NTSTATUS
    StartGet(
        __in KWString& FullyQualifiedRootDirectoryName,
        __in KAllocator& Allocator,
        __out GUID& VolumeId,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync)
    {
        // Fire off the internal op
        GetVolumeIdOp::SPtr   getOp;

        getOp = _new(KTL_TAG_NAMESPACE,Allocator) GetVolumeIdOp(FullyQualifiedRootDirectoryName, VolumeId);

        if (getOp == nullptr)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        if (!NT_SUCCESS(getOp->Status()))
        {
            return getOp->Status();
        }

        getOp->Start(ParentAsync, Completion);
        return STATUS_PENDING;
    }
};

KVolumeNamespaceStandard::GetVolumeIdOp::GetVolumeIdOp(__in KWString& FullyQualifiedRootDirectoryName, __out GUID& VolumeId)
    :   _FullyQualifiedRootDirectoryName(FullyQualifiedRootDirectoryName),
        _VolumeId(VolumeId)
{
    SetConstructorStatus(_FullyQualifiedRootDirectoryName.Status());
}

KVolumeNamespaceStandard::GetVolumeIdOp::~GetVolumeIdOp()
{
}

NTSTATUS
KVolumeNamespace::QueryVolumeIdFromRootDirectoryName(
    __in KWString& FullyQualifiedRootDirectoryName,
    __in KAllocator& Allocator,
    __out GUID& VolumeId,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync)
{
    // Fire off the internal op
    return KVolumeNamespaceStandard::GetVolumeIdOp::StartGet(
        FullyQualifiedRootDirectoryName,
        Allocator,
        VolumeId,
        Completion,
        ParentAsync);
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KVolumeNamespace::QueryVolumeIdFromRootDirectoryNameAsync(
    __in KWString& FullyQualifiedRootDirectoryName,
    __in KAllocator& Allocator,
    __out GUID& VolumeId,
    __in_opt KAsyncContextBase* const ParentAsync)
{
    NTSTATUS status;
    
    // Fire off the internal op
    KVolumeNamespaceStandard::GetVolumeIdOp::SPtr   getOp;

    getOp = _new(KTL_TAG_NAMESPACE,Allocator) KVolumeNamespaceStandard::GetVolumeIdOp(FullyQualifiedRootDirectoryName, VolumeId);

    if (getOp == nullptr)
    {
        co_return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(getOp->Status()))
    {
        co_return getOp->Status();
    }

    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)getOp.RawPtr()), ParentAsync, NULL);

    status = co_await awaiter;
    co_return status;
}
#endif
#endif


//* QueryFullFileAttributes imp
class KVolumeNamespaceStandard::QueryFullFileAttributesOp : public KAsyncContextBase, public KThreadPool::WorkItem
{
    K_FORCE_SHARED(QueryFullFileAttributesOp);

private:
    KWString                            _FullyQualifiedPath;
    FILE_NETWORK_OPEN_INFORMATION&      _Result;

public:
    QueryFullFileAttributesOp(__in KWString& FullyQualifiedPath, __out FILE_NETWORK_OPEN_INFORMATION& Result);

private:
    void
    OnStart() override
    {
        // Simple schedule on worker thread
        GetThisKtlSystem().DefaultSystemThreadPool().QueueWorkItem(*this);

        // Continued @ Execute()
    }

    void
    Execute() override
    {
        OBJECT_ATTRIBUTES   oa;

        InitializeObjectAttributes(&oa, _FullyQualifiedPath, OBJ_CASE_INSENSITIVE, NULL, NULL);
        Complete(KNt::QueryFullAttributesFile(oa, _Result));
    }

public:
    static
    NTSTATUS
    StartQuery(
        __in KWString& FullyQualifiedPath,
        __in KAllocator& Allocator,
        __out FILE_NETWORK_OPEN_INFORMATION& Result,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync)
    {
        // Fire off the internal op
        QueryFullFileAttributesOp::SPtr   op;

        op = _new(KTL_TAG_NAMESPACE, Allocator) QueryFullFileAttributesOp(FullyQualifiedPath, Result);

        if (op == nullptr)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        if (!NT_SUCCESS(op->Status()))
        {
            return op->Status();
        }

        op->Start(ParentAsync, Completion);
        return STATUS_PENDING;
    }
};

KVolumeNamespaceStandard::QueryFullFileAttributesOp::QueryFullFileAttributesOp(
    __in KWString& FullyQualifiedPath,
    __out FILE_NETWORK_OPEN_INFORMATION& Result)
    :   _FullyQualifiedPath(FullyQualifiedPath),
        _Result(Result)
{
    SetConstructorStatus(FullyQualifiedPath.Status());
}

KVolumeNamespaceStandard::QueryFullFileAttributesOp::~QueryFullFileAttributesOp()
{
}


NTSTATUS
KVolumeNamespace::QueryFullFileAttributes(
    __in KWString& FullyQualifiedPath,
    __in KAllocator& Allocator,
    __out FILE_NETWORK_OPEN_INFORMATION& Result,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync)
{
    return KVolumeNamespaceStandard::QueryFullFileAttributesOp::StartQuery(
        FullyQualifiedPath,
        Allocator,
        Result,
        Completion,
        ParentAsync);
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KVolumeNamespace::QueryFullFileAttributesAsync(
    __in KWString& FullyQualifiedPath,
    __in KAllocator& Allocator,
    __out FILE_NETWORK_OPEN_INFORMATION& Result,
    __in_opt KAsyncContextBase* const ParentAsync)
{
    NTSTATUS status;
    // Fire off the internal op
    KVolumeNamespaceStandard::QueryFullFileAttributesOp::SPtr   op;

    op = _new(KTL_TAG_NAMESPACE, Allocator) KVolumeNamespaceStandard::QueryFullFileAttributesOp(FullyQualifiedPath, Result);

    if (op == nullptr)
    {
        co_return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(op->Status()))
    {
        co_return op->Status();
    }

    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)op.RawPtr()), ParentAsync, NULL);

    status = co_await awaiter;
    co_return status;   
}

ktl::Awaitable<NTSTATUS>
KVolumeNamespace::QueryFullFileAttributesAsync(
    __in LPCWSTR FullyQualifiedPath,
    __in KAllocator& Allocator,
    __out FILE_NETWORK_OPEN_INFORMATION& Result,
    __in_opt KAsyncContextBase* const ParentAsync)
{
	KWString s(Allocator);
	s = FullyQualifiedPath;
	if (! NT_SUCCESS(s.Status()))
	{
		co_return s.Status();
	}

	co_return co_await KVolumeNamespace::QueryFullFileAttributesAsync(s, Allocator, Result, ParentAsync);
}
#endif

//* Rename imp
class KVolumeNamespaceStandard::RenameFileOp : public KAsyncContextBase, public KThreadPool::WorkItem
{
    K_FORCE_SHARED(RenameFileOp);

private:
    KWString                            _FromPathName;
    KWString                            _ToPathName;
    BOOLEAN                             _OverwriteIfExists;

public:
    RenameFileOp(__in KWString const & FromPathName, __in KWString const & ToPathName, __in BOOLEAN OverwriteIfExists);
    RenameFileOp(__in LPCWSTR FromPathName, __in LPCWSTR ToPathName, __in BOOLEAN OverwriteIfExists);

private:
    void
    OnStart() override
    {
        // Simple schedule on worker thread
        GetThisKtlSystem().DefaultSystemThreadPool().QueueWorkItem(*this);

        // Continued @ Execute()
    }

    void
    Execute() override
    {
        NTSTATUS status;
        status = HLPalFunctions::RenameFile((WCHAR*)_FromPathName,
                                            (WCHAR*)_ToPathName,
                                            _ToPathName.Length(),
                                            _OverwriteIfExists,
                                            GetThisAllocator());
        Complete(status);
    }

public:
    static
    NTSTATUS
    StartRename(
        __in KWString const & FromPathName,
        __in KWString const & ToPathName,
        __in BOOLEAN OverwriteIfExists,
        __in KAllocator& Allocator,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync)
    {
        // Fire off the internal op
        RenameFileOp::SPtr   op;

        op = _new(KTL_TAG_NAMESPACE, Allocator) RenameFileOp(FromPathName, ToPathName, OverwriteIfExists);

        if (op == nullptr)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        if (!NT_SUCCESS(op->Status()))
        {
            return op->Status();
        }

        op->Start(ParentAsync, Completion);
        return STATUS_PENDING;
    }

    static
    NTSTATUS
    StartRename(
        __in LPCWSTR FromPathName,
        __in LPCWSTR ToPathName,
        __in BOOLEAN OverwriteIfExists,
        __in KAllocator& Allocator,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync)
    {
        // Fire off the internal op
        RenameFileOp::SPtr   op;

        op = _new(KTL_TAG_NAMESPACE, Allocator) RenameFileOp(FromPathName, ToPathName, OverwriteIfExists);

        if (op == nullptr)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        if (!NT_SUCCESS(op->Status()))
        {
            return op->Status();
        }

        op->Start(ParentAsync, Completion);
        return STATUS_PENDING;
    }
    
    
};

KVolumeNamespaceStandard::RenameFileOp::RenameFileOp(
    __in KWString const & FromPathName,
    __in KWString const & ToPathName,
    __in BOOLEAN OverwriteIfExists)                                                                  
    :   _FromPathName(FromPathName),
        _ToPathName(ToPathName),
        _OverwriteIfExists(OverwriteIfExists)
{
    NTSTATUS status;

    _FromPathName += L"\0";
    status = _FromPathName.Status();
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }

    _ToPathName += L"\0";
    status = _ToPathName.Status();
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }   
}

KVolumeNamespaceStandard::RenameFileOp::RenameFileOp(
    __in LPCWSTR FromPathName,
    __in LPCWSTR ToPathName,
    __in BOOLEAN OverwriteIfExists)                                                                  
    :   _FromPathName(GetThisAllocator(), FromPathName),
        _ToPathName(GetThisAllocator(), ToPathName),
        _OverwriteIfExists(OverwriteIfExists)
{
    NTSTATUS status;

    _FromPathName += L"\0";
    status = _FromPathName.Status();
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }

    _ToPathName += L"\0";
    status = _ToPathName.Status();
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }   
}

KVolumeNamespaceStandard::RenameFileOp::~RenameFileOp()
{
}


NTSTATUS
KVolumeNamespace::RenameFile(
    __in KWString const & FromPathName,
    __in KWString const & ToPathName,
    __in BOOLEAN OverwriteIfExists,
    __in KAllocator& Allocator,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync)
{
    return KVolumeNamespaceStandard::RenameFileOp::StartRename(
        FromPathName,
        ToPathName,
        OverwriteIfExists,
        Allocator,
        Completion,
        ParentAsync);
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KVolumeNamespace::RenameFileAsync(
    __in KWString const & FromPathName,
    __in KWString const & ToPathName,
    __in BOOLEAN OverwriteIfExists,
    __in KAllocator& Allocator,
    __in_opt KAsyncContextBase* const ParentAsync)
{
    NTSTATUS status;
    
    // Fire off the internal op
    KVolumeNamespaceStandard::RenameFileOp::SPtr   op;

    op = _new(KTL_TAG_NAMESPACE, Allocator) KVolumeNamespaceStandard::RenameFileOp(FromPathName, ToPathName, OverwriteIfExists);

    if (op == nullptr)
    {
        co_return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(op->Status()))
    {
        co_return op->Status();
    }

    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)op.RawPtr()), ParentAsync, NULL);

    status = co_await awaiter;
    co_return status;       
}
#endif

NTSTATUS
KVolumeNamespace::RenameFile(
    __in LPCWSTR FromPathName,
    __in LPCWSTR ToPathName,
    __in BOOLEAN OverwriteIfExists,
    __in KAllocator& Allocator,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync)
{
    return KVolumeNamespaceStandard::RenameFileOp::StartRename(
        FromPathName,
        ToPathName,
        OverwriteIfExists,
        Allocator,
        Completion,
        ParentAsync);
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KVolumeNamespace::RenameFileAsync(
    __in LPCWSTR FromPathName,
    __in LPCWSTR ToPathName,
    __in BOOLEAN OverwriteIfExists,
    __in KAllocator& Allocator,
    __in_opt KAsyncContextBase* const ParentAsync)
{
    NTSTATUS status;
    
    // Fire off the internal op
    KVolumeNamespaceStandard::RenameFileOp::SPtr   op;

    op = _new(KTL_TAG_NAMESPACE, Allocator) KVolumeNamespaceStandard::RenameFileOp(FromPathName, ToPathName, OverwriteIfExists);

    if (op == nullptr)
    {
        co_return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(op->Status()))
    {
        co_return op->Status();
    }
    
    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)op.RawPtr()), ParentAsync, NULL);

    status = co_await awaiter;
    co_return status;   
}
#endif
