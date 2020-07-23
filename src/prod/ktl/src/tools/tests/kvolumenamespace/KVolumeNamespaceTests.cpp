/*++

Copyright (c) Microsoft Corporation

Module Name:

    KVolumeNamespaceTests.cpp

Abstract:

    This file contains test case implementations.

    To add a new unit test case:
    1. Declare your test case routine in KVolumeNamespaceTests.h.
    2. Add an entry to the gs_KuTestCases table in KVolumeNamespaceTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.

--*/
#pragma once
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>
#include "KVolumeNamespaceTests.h"
#include <CommandLineParser.h>

#if KTL_USER_MODE
#include <ktlevents.um.h>
#else
#include <ktlevents.km.h>
#endif

class TestContext : public KAsyncContextBase {

    K_FORCE_SHARED(TestContext);

    public:

        static
        NTSTATUS
        Create(
            __out TestContext::SPtr& Context
            );

        VOID
        Completion(
            __in KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );

        NTSTATUS
        Wait(
            );

    private:

        KEvent _Event;
        NTSTATUS _Status;

};

TestContext::~TestContext(
    )
{
}

NOFAIL
TestContext::TestContext(
    )
{
    _Status = STATUS_SUCCESS;
}

NTSTATUS
TestContext::Create(
    __out TestContext::SPtr& Context
    )
{
    TestContext* context;

    context = _new(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator()) TestContext;
    if (!context) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Context = context;

    return STATUS_SUCCESS;
}

VOID
TestContext::Completion(
    __in KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);

    _Status = Async.Status();
    _Event.SetEvent();
}

NTSTATUS
TestContext::Wait(
    )
{
    _Event.WaitUntilSet();
    _Event.ResetEvent();
    return _Status;
}

//* Sub-tree deletion helper
void
DeleteDir(WCHAR* DirPath, KAllocator& Allocator, BOOLEAN OkToFail = FALSE)
{
    KSynchronizer       compSync;
    NTSTATUS            status = STATUS_SUCCESS;
    KWString            toDelete(Allocator, DirPath);
    KInvariant(NT_SUCCESS(toDelete.Status()));

    // Recurse for each sub-dir
    KVolumeNamespace::NameArray namesToDelete(Allocator);
    status = KVolumeNamespace::QueryDirectories(toDelete, namesToDelete, Allocator, compSync);
    KInvariant(OkToFail || NT_SUCCESS(status) || (status == STATUS_OBJECT_NAME_NOT_FOUND));
    status = compSync.WaitForCompletion();
    KInvariant(OkToFail || NT_SUCCESS(status) || (status == STATUS_OBJECT_NAME_NOT_FOUND));

    if (status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        // if dir doesn't exist then we are ok
        return;
    }

    for (ULONG ix = 0; ix < namesToDelete.Count(); ix++)
    {
        KWString        delPath(toDelete);
        KInvariant(NT_SUCCESS(delPath.Status()));
        delPath += KVolumeNamespace::PathSeparator;
        KInvariant(NT_SUCCESS(delPath.Status()));
        delPath += namesToDelete[ix];
        KInvariant(NT_SUCCESS(delPath.Status()));

        DeleteDir((WCHAR*)delPath, Allocator);
    }

    // Waste any files
    status = KVolumeNamespace::QueryFiles(toDelete, namesToDelete, Allocator, compSync);
    KInvariant(NT_SUCCESS(status));
    status = compSync.WaitForCompletion();
    KInvariant(OkToFail || NT_SUCCESS(status));

    for (ULONG ix = 0; ix < namesToDelete.Count(); ix++)
    {
        KWString        delPath(toDelete);
        KInvariant(NT_SUCCESS(delPath.Status()));
        delPath += KVolumeNamespace::PathSeparator;
        KInvariant(NT_SUCCESS(delPath.Status()));
        delPath += namesToDelete[ix];
        KInvariant(NT_SUCCESS(delPath.Status()));

        status = KVolumeNamespace::DeleteFileOrDirectory(delPath, Allocator, compSync);
        KInvariant(NT_SUCCESS(status));
        status = compSync.WaitForCompletion();
        KInvariant(NT_SUCCESS(status));
    }

    status = KVolumeNamespace::DeleteFileOrDirectory(toDelete, Allocator, compSync);
    KInvariant(NT_SUCCESS(status));
    status = compSync.WaitForCompletion();
    KInvariant(OkToFail || NT_SUCCESS(status));
};


#if defined(K_UseResumable)
ktl::Awaitable<void>
DeleteDirAsync(WCHAR* DirPath, KAllocator& Allocator, BOOLEAN OkToFail = FALSE)
{
    NTSTATUS            status = STATUS_SUCCESS;
    KWString            toDelete(Allocator, DirPath);
    KInvariant(NT_SUCCESS(toDelete.Status()));

    // Recurse for each sub-dir
    KVolumeNamespace::NameArray namesToDelete(Allocator);
    status = co_await KVolumeNamespace::QueryDirectoriesAsync((LPCWSTR)toDelete, namesToDelete, Allocator);
    KInvariant(OkToFail || NT_SUCCESS(status) || (status == STATUS_OBJECT_NAME_NOT_FOUND));

    if (status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        // if dir doesn't exist then we are ok
        co_return;
    }

    for (ULONG ix = 0; ix < namesToDelete.Count(); ix++)
    {
        KWString        delPath(toDelete);
        KInvariant(NT_SUCCESS(delPath.Status()));
        delPath += KVolumeNamespace::PathSeparator;
        KInvariant(NT_SUCCESS(delPath.Status()));
        delPath += namesToDelete[ix];
        KInvariant(NT_SUCCESS(delPath.Status()));

        co_await DeleteDirAsync((WCHAR*)delPath, Allocator);
    }

    // Waste any files
    status = co_await KVolumeNamespace::QueryFilesAsync((LPCWSTR)toDelete, namesToDelete, Allocator);
    KInvariant(OkToFail || NT_SUCCESS(status));

    for (ULONG ix = 0; ix < namesToDelete.Count(); ix++)
    {
        KWString        delPath(toDelete);
        KInvariant(NT_SUCCESS(delPath.Status()));
        delPath += KVolumeNamespace::PathSeparator;
        KInvariant(NT_SUCCESS(delPath.Status()));
        delPath += namesToDelete[ix];
        KInvariant(NT_SUCCESS(delPath.Status()));

        status = co_await KVolumeNamespace::DeleteFileOrDirectoryAsync(delPath, Allocator);
        KInvariant(NT_SUCCESS(status));
    }

    status = co_await KVolumeNamespace::DeleteFileOrDirectoryAsync(toDelete, Allocator);
    KInvariant(OkToFail || NT_SUCCESS(status));

    co_return;
};
#endif

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
CreateAFileAsync(
    __in KWString& FileName,
    __in KAllocator& Allocator
    )
{
    NTSTATUS status;
    KBlockFile::SPtr file;

    status = co_await KBlockFile::CreateAsync(FileName,
                                FALSE,
                                KBlockFile::eCreateAlways,
                                file,
                                NULL,
                                Allocator);
    KInvariant(NT_SUCCESS(status));
    
    co_return(status);
}

ktl::Awaitable<NTSTATUS>
VerifyAFileAsync(
    __in KWString& FileName,
    __in KAllocator& Allocator
    )
{
    NTSTATUS status;
    KBlockFile::SPtr file;

    status = co_await KBlockFile::CreateAsync(FileName,
                                FALSE,
                                KBlockFile::eOpenExisting,
                                file,
                                NULL,
                                Allocator);
    
    co_return(status);
}

ktl::Awaitable<NTSTATUS>
DeleteAFileAsync(
    __in KWString& FileName,
    __in KAllocator& Allocator
    )
{
    NTSTATUS status;
    
    status = co_await KVolumeNamespace::DeleteFileOrDirectoryAsync(FileName, Allocator);
    
    co_return(status);
}
#endif

#if !defined(PLATFORM_UNIX)
/*   Test QueryGlobalVolumeDirectoryName(), 
          QueryVolumeIdFromRootDirectoryName, 
          SetHardLink(), 
          QueryFullFileAttributes(), 
          and QueryHardLinks()
*/

NTSTATUS
PerformHardLinkTest(
    KWString convertedVolName,
    BOOLEAN rootDir
    )
{
    NTSTATUS status;
    KSynchronizer       compSync;
    KAllocator&         allocator = KtlSystem::GlobalNonPagedAllocator();

    static WCHAR*       subDirs[] =
    {
        L"",
        L"TestHLinks1",
        L"TestHLinks1\\TestHLinks2",
        nullptr
    };

    WCHAR**             subDirNamePtr = &subDirs[0];
    while (*subDirNamePtr != nullptr)
    {
        if (rootDir && **subDirNamePtr == 0)
        {
            subDirNamePtr++;
            continue;
        }

        KWString        subDirName(convertedVolName);  
            KInvariant(NT_SUCCESS(subDirName.Status()));
        
        subDirName += L"\\";
            KInvariant(NT_SUCCESS(subDirName.Status()));

        subDirName += *subDirNamePtr;
            KInvariant(NT_SUCCESS(subDirName.Status()));


        DeleteDir(subDirName, allocator, rootDir);


        status = KVolumeNamespace::CreateDirectory(subDirName, allocator, compSync);
        KInvariant(NT_SUCCESS(status));
        status = compSync.WaitForCompletion();
        KInvariant(NT_SUCCESS(status));

        subDirNamePtr++;
    }

    // Build up FQN for root HardLink
    KWString                hl0(convertedVolName);
        KInvariant(NT_SUCCESS(hl0.Status()));
    hl0 += L"\\real.txt";
        KInvariant(NT_SUCCESS(hl0.Status()));

    HANDLE                  realFileHandle; 
    OBJECT_ATTRIBUTES       oa;
    IO_STATUS_BLOCK         ioStatus;
            
    InitializeObjectAttributes(&oa, hl0, OBJ_CASE_INSENSITIVE, NULL, NULL);
    status = KNt::CreateFile(
        &realFileHandle, 
        GENERIC_ALL, 
        &oa, 
        &ioStatus, 
        NULL, 
        FILE_ATTRIBUTE_NORMAL, 
        0, 
        FILE_OVERWRITE_IF, 
        FILE_NON_DIRECTORY_FILE, 
        NULL, 
        0);

    KInvariant(NT_SUCCESS(status));

    //*****
    union
    {
        FILE_FS_VOLUME_INFORMATION    info;
        UCHAR           filler1[1024];
    };
    
    // Get the volume's guid
    status = KNt::QueryVolumeInformationFile(
        realFileHandle,
        &ioStatus,
        &info,
        sizeof(filler1),
        FS_INFORMATION_CLASS::FileFsVolumeInformation);


    KInvariant(NT_SUCCESS(status));
    KNt::Close(realFileHandle);

    // Prove it exists - testing QueryFullFileAttributes()
    FILE_NETWORK_OPEN_INFORMATION   fnoi;

    status = KVolumeNamespace::QueryFullFileAttributes(hl0, allocator, fnoi, compSync);
    KInvariant(NT_SUCCESS(status));
    status = compSync.WaitForCompletion();
    KInvariant(NT_SUCCESS(status));

    KArray<KWString>        fqHardLinkNames(allocator);
    WCHAR*                  fqHardLinkNamesPtrs[] =
    {
        L"\\hl2.txt",
        L"\\hl1.txt",
        L"\\TestHLinks1\\hl3.txt",
        L"\\TestHLinks1\\TestHLinks2\\hl4.txt",
    };

    for (ULONG ix = 0; ix < sizeof(fqHardLinkNamesPtrs) / sizeof(WCHAR*); ix++)
    {
        KWString        hlPath(convertedVolName);
            KInvariant(NT_SUCCESS(hlPath.Status()));

        hlPath += fqHardLinkNamesPtrs[ix];
            KInvariant(NT_SUCCESS(hlPath.Status()));

        status = KVolumeNamespace::SetHardLink(hl0, hlPath, allocator, compSync, nullptr);
            KInvariant(NT_SUCCESS(hlPath.Status()));
        status = compSync.WaitForCompletion();
            KInvariant(NT_SUCCESS(hlPath.Status()));
    
        status = fqHardLinkNames.Append(Ktl::Move(hlPath));
            KInvariant(NT_SUCCESS(hlPath.Status()));

    }

    KVolumeNamespace::NameArray     links(allocator);
    status = KVolumeNamespace::QueryHardLinks(hl0, allocator, links, compSync, nullptr);
    KInvariant(NT_SUCCESS(status));
    status = compSync.WaitForCompletion();
    KInvariant(NT_SUCCESS(status));

    // Prove all (and only) HardLinks created above are returned from QueryHardLinks()
    for (ULONG ix = 0; ix < links.Count(); ix++)
    {
        KTestPrintf("KVolumeNamespaceTest: Link(%u): %S\n", ix, (PWCHAR)(links[ix]));

        KStringView             currentHardLink((UNICODE_STRING&)links[ix]);
        BOOLEAN                 foundLink = FALSE;
        for (ULONG ix1 = 0; ix1 < fqHardLinkNames.Count(); ix1++)
        {
            if (currentHardLink.CompareNoCase(KStringView((UNICODE_STRING&)fqHardLinkNames[ix1])) == 0)
            {
                KInvariant(fqHardLinkNames.Remove(ix1));
                foundLink = TRUE;
                break;
            }
        }

        KInvariant(foundLink);
    }
    KInvariant(fqHardLinkNames.Count() == 0);

    return(STATUS_SUCCESS);
}


#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
PerformHardLinkTestAsync(
    KWString convertedVolName,
    BOOLEAN rootDir
    )
{
    NTSTATUS status;
    KAllocator&         allocator = KtlSystem::GlobalNonPagedAllocator();

    static WCHAR*       subDirs[] =
    {
        L"",
        L"TestHLinks1",
        L"TestHLinks1\\TestHLinks2",
        nullptr
    };

    WCHAR**             subDirNamePtr = &subDirs[0];
    while (*subDirNamePtr != nullptr)
    {
        if (rootDir && **subDirNamePtr == 0)
        {
            subDirNamePtr++;
            continue;
        }

        KWString        subDirName(convertedVolName);  
            KInvariant(NT_SUCCESS(subDirName.Status()));
        
        subDirName += L"\\";
            KInvariant(NT_SUCCESS(subDirName.Status()));

        subDirName += *subDirNamePtr;
            KInvariant(NT_SUCCESS(subDirName.Status()));


        co_await DeleteDirAsync(subDirName, allocator, rootDir);


        status = co_await KVolumeNamespace::CreateDirectoryAsync(subDirName, allocator);
        KInvariant(NT_SUCCESS(status));

        subDirNamePtr++;
    }

    // Build up FQN for root HardLink
    KWString                hl0(convertedVolName);
        KInvariant(NT_SUCCESS(hl0.Status()));
    hl0 += L"\\real.txt";
        KInvariant(NT_SUCCESS(hl0.Status()));

    status = co_await CreateAFileAsync(hl0, allocator);
    KInvariant(NT_SUCCESS(status));
    
    
    // Prove it exists - testing QueryFullFileAttributes()
    FILE_NETWORK_OPEN_INFORMATION   fnoi;

    status = co_await KVolumeNamespace::QueryFullFileAttributesAsync((LPCWSTR)hl0, allocator, fnoi);
    KInvariant(NT_SUCCESS(status));

    KArray<KWString>        fqHardLinkNames(allocator);
    WCHAR*                  fqHardLinkNamesPtrs[] =
    {
        L"\\hl2.txt",
        L"\\hl1.txt",
        L"\\TestHLinks1\\hl3.txt",
        L"\\TestHLinks1\\TestHLinks2\\hl4.txt",
    };

    for (ULONG ix = 0; ix < sizeof(fqHardLinkNamesPtrs) / sizeof(WCHAR*); ix++)
    {
        KWString        hlPath(convertedVolName);
            KInvariant(NT_SUCCESS(hlPath.Status()));

        hlPath += fqHardLinkNamesPtrs[ix];
            KInvariant(NT_SUCCESS(hlPath.Status()));

        status = co_await KVolumeNamespace::SetHardLinkAsync(hl0, hlPath, allocator, nullptr);
            KInvariant(NT_SUCCESS(hlPath.Status()));
    
        status = fqHardLinkNames.Append(Ktl::Move(hlPath));
            KInvariant(NT_SUCCESS(hlPath.Status()));

    }

    KVolumeNamespace::NameArray     links(allocator);
    status = co_await KVolumeNamespace::QueryHardLinksAsync(hl0, allocator, links, nullptr);
    KInvariant(NT_SUCCESS(status));

    // Prove all (and only) HardLinks created above are returned from QueryHardLinks()
    for (ULONG ix = 0; ix < links.Count(); ix++)
    {
        KTestPrintf("KVolumeNamespaceTest: Link(%u): %S\n", ix, (PWCHAR)(links[ix]));

        KStringView             currentHardLink((UNICODE_STRING&)links[ix]);
        BOOLEAN                 foundLink = FALSE;
        for (ULONG ix1 = 0; ix1 < fqHardLinkNames.Count(); ix1++)
        {
            if (currentHardLink.CompareNoCase(KStringView((UNICODE_STRING&)fqHardLinkNames[ix1])) == 0)
            {
                KInvariant(fqHardLinkNames.Remove(ix1));
                foundLink = TRUE;
                break;
            }
        }

        KInvariant(foundLink);
    }
    KInvariant(fqHardLinkNames.Count() == 0);

    co_return(STATUS_SUCCESS);
}
#endif

void
HardLinksSupportTest()
{
    KSynchronizer       compSync;
    KAllocator&         allocator = KtlSystem::GlobalNonPagedAllocator();
    NTSTATUS            status;
    GUID                volId;
    GUID                volId1;
    KWString            volumeName(allocator);

    KWString dirName(allocator);
    //** Prove QueryVolumeIdFromRootDirectoryName() works

    dirName = KWString(allocator, L"C:");
    status = KVolumeNamespace::QueryVolumeIdFromRootDirectoryName(dirName, allocator, volId, compSync);
    KInvariant(NT_SUCCESS(status));
    status = compSync.WaitForCompletion();
    KInvariant(NT_SUCCESS(status));

    dirName = KWString(allocator, L"c:");
    status = KVolumeNamespace::QueryVolumeIdFromRootDirectoryName(dirName, allocator, volId, compSync);
    KInvariant(NT_SUCCESS(status));
    status = compSync.WaitForCompletion();
    KInvariant(NT_SUCCESS(status));

    dirName = KWString(allocator, L"C:\\temp");
    status = KVolumeNamespace::QueryVolumeIdFromRootDirectoryName(dirName, allocator, volId, compSync);
    KInvariant(NT_SUCCESS(status));
    status = compSync.WaitForCompletion();
    KInvariant(NT_SUCCESS(status));

    dirName = KWString(allocator, L"\\??\\C:");
    status = KVolumeNamespace::QueryVolumeIdFromRootDirectoryName(dirName, allocator, volId, compSync);
    KInvariant(NT_SUCCESS(status));
    status = compSync.WaitForCompletion();
    KInvariant(NT_SUCCESS(status));

    dirName = KWString(allocator, L"\\global??\\C:\\Windows\\System32");
    status = KVolumeNamespace::QueryVolumeIdFromRootDirectoryName(dirName, allocator, volId, compSync);
    KInvariant(NT_SUCCESS(status));
    status = compSync.WaitForCompletion();
    KInvariant(NT_SUCCESS(status));

    // prove that a resolved volume name path still works with QueryVolumeIdFromRootDirectoryName
    status = KVolumeNamespace::CreateFullyQualifiedRootDirectoryName(volId, volumeName);
    KInvariant(NT_SUCCESS(status));

    status = KVolumeNamespace::QueryVolumeIdFromRootDirectoryName(volumeName, allocator, volId1, compSync);
    KInvariant(NT_SUCCESS(status));
    status = compSync.WaitForCompletion();
    KInvariant(NT_SUCCESS(status));
    KInvariant(volId == volId1);

    // Prove proper error returns
    dirName = KWString(allocator, L"1:");
    status = KVolumeNamespace::QueryVolumeIdFromRootDirectoryName(dirName, allocator, volId, compSync);
    KInvariant(NT_SUCCESS(status));
    status = compSync.WaitForCompletion();
    KInvariant(!NT_SUCCESS(status));
    KInvariant(status == STATUS_OBJECT_NAME_NOT_FOUND);

    dirName = KWString(allocator, L"\\Device\\HarddiskVolume1");
    status = KVolumeNamespace::QueryVolumeIdFromRootDirectoryName(dirName, allocator, volId1, compSync);
    KInvariant(NT_SUCCESS(status));
    status = compSync.WaitForCompletion();
    KInvariant(!NT_SUCCESS(status));
    KInvariant(status == STATUS_OBJECT_TYPE_MISMATCH);

    //** Prove QueryGlobalVolumeDirectoryName() works
    KWString                        convertedVolName(allocator);
    dirName = KWString(allocator, L"\\??\\C:");
    status = KVolumeNamespace::QueryGlobalVolumeDirectoryName(dirName, allocator, convertedVolName, compSync);
    //status = KVolumeNamespace::QueryGlobalVolumeDirectoryName(KWString(allocator, L"\\??\\F:"), allocator, convertedVolName, compSync);
    KInvariant(NT_SUCCESS(status));
    status = compSync.WaitForCompletion();
    KInvariant(NT_SUCCESS(status));

    // Validate a result in the following form:
    //      \GLOBAL??\Volume{d498b863-8a6d-44f9-b040-48eab3b42b02}
    //      012345678901234567890123456789012345678901234567890123
    //                1         2         3         4         5

    KInvariant(convertedVolName.Length() == (sizeof(WCHAR) * 54));
    KInvariant(memcmp((WCHAR*)convertedVolName, L"\\GLOBAL??\\Volume{", 17) == 0);
    KInvariant(*(((WCHAR*)convertedVolName) + 25) == '-');
    KInvariant(*(((WCHAR*)convertedVolName) + 30) == '-');
    KInvariant(*(((WCHAR*)convertedVolName) + 35) == '-');
    KInvariant(*(((WCHAR*)convertedVolName) + 40) == '-');
    KInvariant(*(((WCHAR*)convertedVolName) + 53) == '}');

    //* Prove hardlink creation

    // First try in the root directory
    status = PerformHardLinkTest(convertedVolName, TRUE);
    
    // Next try in a subdirectory
    //      Create some sub-dirs
    convertedVolName += L"\\KtlCITDir";
    KInvariant(NT_SUCCESS(convertedVolName.Status()));

    // Just an easy way to make sure DeleteDir works
    status = KVolumeNamespace::CreateDirectory(convertedVolName, allocator, compSync);
    KInvariant(NT_SUCCESS(status));
    status = compSync.WaitForCompletion();
    KInvariant(NT_SUCCESS(status) || (status == STATUS_OBJECT_NAME_COLLISION));

    DeleteDir(convertedVolName, allocator);

    status = PerformHardLinkTest(convertedVolName, FALSE);

    //* Cleanup temp dir
    DeleteDir(convertedVolName, allocator);
}


#if defined(K_UseResumable)
ktl::Awaitable<void>
HardLinksSupportTestAsync()
{
    KAllocator&         allocator = KtlSystem::GlobalNonPagedAllocator();
    NTSTATUS            status;
    GUID                volId;
    GUID                volId1;
    KWString            volumeName(allocator);

    KWString dirName(allocator);
    //** Prove QueryVolumeIdFromRootDirectoryName() works

    dirName = KWString(allocator, L"C:");
    status = co_await KVolumeNamespace::QueryVolumeIdFromRootDirectoryNameAsync(dirName, allocator, volId);
    KInvariant(NT_SUCCESS(status));

    dirName = KWString(allocator, L"c:");
    status = co_await KVolumeNamespace::QueryVolumeIdFromRootDirectoryNameAsync(dirName, allocator, volId);
    KInvariant(NT_SUCCESS(status));

    dirName = KWString(allocator, L"C:\\temp");
    status = co_await KVolumeNamespace::QueryVolumeIdFromRootDirectoryNameAsync(dirName, allocator, volId);
    KInvariant(NT_SUCCESS(status));

    dirName = KWString(allocator, L"\\??\\C:");
    status = co_await KVolumeNamespace::QueryVolumeIdFromRootDirectoryNameAsync(dirName, allocator, volId);
    KInvariant(NT_SUCCESS(status));

    dirName = KWString(allocator, L"\\global??\\C:\\Windows\\System32");
    status = co_await KVolumeNamespace::QueryVolumeIdFromRootDirectoryNameAsync(dirName, allocator, volId);
    KInvariant(NT_SUCCESS(status));

    // prove that a resolved volume name path still works with QueryVolumeIdFromRootDirectoryName
    status = KVolumeNamespace::CreateFullyQualifiedRootDirectoryName(volId, volumeName);
    KInvariant(NT_SUCCESS(status));

    status = co_await KVolumeNamespace::QueryVolumeIdFromRootDirectoryNameAsync(volumeName, allocator, volId1);
    KInvariant(NT_SUCCESS(status));
    KInvariant(volId == volId1);

    // Prove proper error returns
    dirName = KWString(allocator, L"1:");
    status = co_await KVolumeNamespace::QueryVolumeIdFromRootDirectoryNameAsync(dirName, allocator, volId);
    KInvariant(!NT_SUCCESS(status));
    KInvariant(status == STATUS_OBJECT_NAME_NOT_FOUND);

    dirName = KWString(allocator, L"\\Device\\HarddiskVolume1");
    status = co_await KVolumeNamespace::QueryVolumeIdFromRootDirectoryNameAsync(dirName, allocator, volId1);
    KInvariant(!NT_SUCCESS(status));
    KInvariant(status == STATUS_OBJECT_TYPE_MISMATCH);

    //** Prove QueryGlobalVolumeDirectoryName() works
    KWString                        convertedVolName(allocator);
    dirName = KWString(allocator, L"\\??\\C:");
    status = co_await KVolumeNamespace::QueryGlobalVolumeDirectoryNameAsync(dirName, allocator, convertedVolName);
    //status = KVolumeNamespace::QueryGlobalVolumeDirectoryName(KWString(allocator, L"\\??\\F:"), allocator, convertedVolName, compSync);
    KInvariant(NT_SUCCESS(status));

    // Validate a result in the following form:
    //      \GLOBAL??\Volume{d498b863-8a6d-44f9-b040-48eab3b42b02}
    //      012345678901234567890123456789012345678901234567890123
    //                1         2         3         4         5

    KInvariant(convertedVolName.Length() == (sizeof(WCHAR) * 54));
    KInvariant(memcmp((WCHAR*)convertedVolName, L"\\GLOBAL??\\Volume{", 17) == 0);
    KInvariant(*(((WCHAR*)convertedVolName) + 25) == '-');
    KInvariant(*(((WCHAR*)convertedVolName) + 30) == '-');
    KInvariant(*(((WCHAR*)convertedVolName) + 35) == '-');
    KInvariant(*(((WCHAR*)convertedVolName) + 40) == '-');
    KInvariant(*(((WCHAR*)convertedVolName) + 53) == '}');

    //* Prove hardlink creation

    // First try in the root directory
    status = co_await PerformHardLinkTestAsync(convertedVolName, TRUE);
    
    // Next try in a subdirectory
    //      Create some sub-dirs
    convertedVolName += L"\\KtlCITDir";
    KInvariant(NT_SUCCESS(convertedVolName.Status()));

    // Just an easy way to make sure DeleteDir works
    status = co_await KVolumeNamespace::CreateDirectoryAsync(convertedVolName, allocator);
    KInvariant(NT_SUCCESS(status) || (status == STATUS_OBJECT_NAME_COLLISION));

    co_await DeleteDirAsync(convertedVolName, allocator);

    status = co_await PerformHardLinkTestAsync(convertedVolName, FALSE);

    //* Cleanup temp dir
    co_await DeleteDirAsync(convertedVolName, allocator);

    co_return;
}
#endif
#endif

typedef struct
{
    KStringView ObjectPath;
    NTSTATUS ExpectedStatus;
    KStringView ExpectedPath;
    KStringView ExpectedFilename;
} TESTCASE;

NTSTATUS TestSplitObjectPathInPathAndFilename(
    )
{
    NTSTATUS status;
#if !defined(PLATFORM_UNIX)
    const ULONG testCaseCount = 10;
#else
    const ULONG testCaseCount = 5;
#endif
    TESTCASE testCase[testCaseCount] = {

#if !defined(PLATFORM_UNIX)
        // Positive test cases
        { L"\\", STATUS_SUCCESS, L"\\", L"" },
        { L"\\Filename", STATUS_SUCCESS, L"\\", L"Filename" },
        { L"\\Path1\\Filename", STATUS_SUCCESS, L"\\Path1\\", L"Filename" },
        { L"\\Path1\\Path2\\Filename", STATUS_SUCCESS, L"\\Path1\\Path2\\", L"Filename" },
        { L"\\??\\c:\\", STATUS_SUCCESS, L"\\??\\c:\\", L"" },
        { L"\\??\\c:\\onelevel", STATUS_SUCCESS, L"\\??\\c:\\", L"onelevel" },
        { L"\\??\\c:\\onelevel\\twolevel", STATUS_SUCCESS, L"\\??\\c:\\onelevel\\", L"twolevel"  },
        { L"\\??\\c:\\onelevel\\twolevel\\", STATUS_SUCCESS, L"\\??\\c:\\onelevel\\twolevel\\", L""  },
        { L"\\??\\c:\\a\\b\\c\\d\\e\\f\\g", STATUS_SUCCESS, L"\\??\\c:\\a\\b\\c\\d\\e\\f\\", L"g"  },

        // Negative test cases
        { L"abcd", STATUS_INVALID_PARAMETER, L"", L""  },
#else
        // Positive test cases
        { L"/", STATUS_SUCCESS, L"/", L"" },
        { L"/Filename", STATUS_SUCCESS, L"/", L"Filename" },
        { L"/Path1/Filename", STATUS_SUCCESS, L"/Path1/", L"Filename" },
        { L"/Path1/Path2/Filename", STATUS_SUCCESS, L"/Path1/Path2/", L"Filename" },

        // Negative test cases
        { L"abcd", STATUS_INVALID_PARAMETER, L"", L""  },       
#endif
    };

    for (ULONG i = 0; i < testCaseCount; i++)
    {
        KWString path(KtlSystem::GlobalNonPagedAllocator());    
        KWString filename(KtlSystem::GlobalNonPagedAllocator());

        status = KVolumeNamespace::SplitObjectPathInPathAndFilename(KtlSystem::GlobalNonPagedAllocator(),
                                                                    testCase[i].ObjectPath,
                                                                    path,
                                                                    filename);
        if (status != testCase[i].ExpectedStatus)
        {
#if defined(PLATFORM_UNIX)
            KTestPrintf("TestSplitObjectPathInPathAndFilename: %s expected status %x, actual status %x\n",
                         Utf16To8((PWCHAR)testCase[i].ObjectPath).c_str(), testCase[i].ExpectedStatus, status);
#else
            KTestPrintf("TestSplitObjectPathInPathAndFilename: %ws expected status %x, actual status %x\n",
                        (PWCHAR)testCase[i].ObjectPath, testCase[i].ExpectedStatus, status);
#endif
            KInvariant(FALSE);
        }

        if (NT_SUCCESS(status))
        {
            KWString expectedPath(KtlSystem::GlobalNonPagedAllocator(), (PWCHAR)(testCase[i].ExpectedPath));
            if (path != expectedPath)
            {
#if defined(PLATFORM_UNIX)
                KTestPrintf("TestSplitObjectPathInPathAndFilename: %s expected path %s, actual path %s\n",
                            Utf16To8((PWCHAR)testCase[i].ObjectPath).c_str(),
                            Utf16To8((PWCHAR)testCase[i].ExpectedPath).c_str(),
                            Utf16To8((PWCHAR)path).c_str());
#else
                KTestPrintf("TestSplitObjectPathInPathAndFilename: %ws expected path %ws, actual path %ws\n",
                            (PWCHAR)testCase[i].ObjectPath, (PWCHAR)testCase[i].ExpectedPath, (PWCHAR)path);
#endif
                KInvariant(FALSE);
            }

            KWString expectedFilename(KtlSystem::GlobalNonPagedAllocator(), (PWCHAR)(testCase[i].ExpectedFilename));
            if (filename != expectedFilename)
            {
#if defined(PLATFORM_UNIX)
                KTestPrintf("TestSplitObjectPathInPathAndFilename: %s expected filename %s, actual filename %s\n",
                            Utf16To8((PWCHAR)testCase[i].ObjectPath).c_str(),
                            Utf16To8((PWCHAR)testCase[i].ExpectedFilename).c_str(),
                            Utf16To8((PWCHAR)filename).c_str());
#else
                KTestPrintf("TestSplitObjectPathInPathAndFilename: %ws expected filename %ws, actual filename %ws\n",
                            (PWCHAR)testCase[i].ObjectPath, (PWCHAR)testCase[i].ExpectedFilename, (PWCHAR)filename);
#endif
                KInvariant(FALSE);
            }
        }
        
    }
    return(STATUS_SUCCESS);
}

NTSTATUS
CreateAFile(
    __in KWString& FileName,
    __in KAllocator& Allocator
    )
{
    NTSTATUS status;
    KSynchronizer sync;
    KBlockFile::SPtr file;

    status = KBlockFile::Create(FileName,
                                FALSE,
                                KBlockFile::eCreateAlways,
                                file,
                                sync,
                                NULL,
                                Allocator);
    KInvariant(NT_SUCCESS(status));
    status = sync.WaitForCompletion();
    KInvariant(NT_SUCCESS(status));
    
    return(status);
}

NTSTATUS
VerifyAFile(
    __in KWString& FileName,
    __in KAllocator& Allocator
    )
{
    NTSTATUS status;
    KSynchronizer sync;
    KBlockFile::SPtr file;

    status = KBlockFile::Create(FileName,
                                FALSE,
                                KBlockFile::eOpenExisting,
                                file,
                                sync,
                                NULL,
                                Allocator);
    KInvariant(NT_SUCCESS(status));
    status = sync.WaitForCompletion();
    
    return(status);
}

NTSTATUS
DeleteAFile(
    __in KWString& FileName,
    __in KAllocator& Allocator
    )
{
    NTSTATUS status;
    KSynchronizer sync;
    
    status = KVolumeNamespace::DeleteFileOrDirectory(FileName, Allocator, sync);
    KInvariant(NT_SUCCESS(status));
    status = sync.WaitForCompletion();
    
    return(status);
}



#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
TestRenameFilesAsync(
    __in KAllocator& Allocator,
    __in WCHAR* DirPath
    )
{
    //
    // Test 1: Successful rename from A to B, B does not exist, no
    //         overwrite flag set
    //
    {
        NTSTATUS status;
        KWString fromFile(Allocator, DirPath);
        KWString toFile(Allocator, DirPath);
        
        fromFile += KVolumeNamespace::PathSeparator;
        fromFile += L"FromFile";
        KInvariant(NT_SUCCESS(fromFile.Status()));
        
        toFile += KVolumeNamespace::PathSeparator;
        toFile += L"ToFile";
        KInvariant(NT_SUCCESS(toFile.Status()));

        status = co_await DeleteAFileAsync(fromFile, Allocator);        
        status = co_await DeleteAFileAsync(toFile, Allocator);
        
        status = co_await CreateAFileAsync(fromFile, Allocator);
        KInvariant(NT_SUCCESS(status));

        status = co_await KVolumeNamespace::RenameFileAsync(fromFile, toFile, FALSE, Allocator, NULL);
        KInvariant(NT_SUCCESS(status));

        status = co_await VerifyAFileAsync(toFile, Allocator);
        KInvariant(NT_SUCCESS(status));

        status = co_await VerifyAFileAsync(fromFile, Allocator);
        KInvariant(status == STATUS_OBJECT_NAME_NOT_FOUND);

        status = co_await DeleteAFileAsync(toFile, Allocator);
        KInvariant(NT_SUCCESS(status));


        //
        // Now try with the WCHAR version of the api
        //
        status = co_await CreateAFileAsync(fromFile, Allocator);
        KInvariant(NT_SUCCESS(status));

        status = co_await KVolumeNamespace::RenameFileAsync((PWCHAR)fromFile, (PWCHAR)toFile, FALSE, Allocator, NULL);
        KInvariant(NT_SUCCESS(status));

        status = co_await VerifyAFileAsync(toFile, Allocator);
        KInvariant(NT_SUCCESS(status));

        status = co_await VerifyAFileAsync(fromFile, Allocator);
        KInvariant(status == STATUS_OBJECT_NAME_NOT_FOUND);

        status = co_await DeleteAFileAsync(toFile, Allocator);
        KInvariant(NT_SUCCESS(status));     
    }

    //
    // Test 2: Successful rename from A to B, B does exist, 
    //         overwrite flag set
    //
    {
        NTSTATUS status;
        KWString fromFile(Allocator, DirPath);
        KWString toFile(Allocator, DirPath);

        fromFile += KVolumeNamespace::PathSeparator;
        fromFile += L"FromFile";
        KInvariant(NT_SUCCESS(fromFile.Status()));
        
        toFile += KVolumeNamespace::PathSeparator;
        toFile += L"ToFile";
        KInvariant(NT_SUCCESS(toFile.Status()));

        status = co_await CreateAFileAsync(fromFile, Allocator);
        KInvariant(NT_SUCCESS(status));

        status = co_await CreateAFileAsync(toFile, Allocator);
        KInvariant(NT_SUCCESS(status));

        status = co_await KVolumeNamespace::RenameFileAsync(fromFile, toFile, TRUE, Allocator, NULL);
        KInvariant(NT_SUCCESS(status));

        status = co_await VerifyAFileAsync(toFile, Allocator);
        KInvariant(NT_SUCCESS(status));

        status = co_await VerifyAFileAsync(fromFile, Allocator);
        KInvariant(status == STATUS_OBJECT_NAME_NOT_FOUND);

        status = co_await DeleteAFileAsync(toFile, Allocator);
        KInvariant(NT_SUCCESS(status));
    }

    //
    // Test 3: Successful rename from A to B, B does not exist, 
    //         overwrite flag set
    //
    {
        NTSTATUS status;
        KWString fromFile(Allocator, DirPath);
        KWString toFile(Allocator, DirPath);

        fromFile += KVolumeNamespace::PathSeparator;
        fromFile += L"FromFile";
        KInvariant(NT_SUCCESS(fromFile.Status()));
        
        toFile += KVolumeNamespace::PathSeparator;
        toFile += L"ToFile";
        KInvariant(NT_SUCCESS(toFile.Status()));

        status = co_await CreateAFileAsync(fromFile, Allocator);
        KInvariant(NT_SUCCESS(status));

        status = co_await KVolumeNamespace::RenameFileAsync(fromFile, toFile, TRUE, Allocator, NULL);
        KInvariant(NT_SUCCESS(status));

        status = co_await VerifyAFileAsync(toFile, Allocator);
        KInvariant(NT_SUCCESS(status));

        status = co_await VerifyAFileAsync(fromFile, Allocator);
        KInvariant(status == STATUS_OBJECT_NAME_NOT_FOUND);

        status = co_await DeleteAFileAsync(toFile, Allocator);
        KInvariant(NT_SUCCESS(status));
    }

    //
    // Test 4: Failed rename from A to B, B does exist, 
    //         no overwrite flag set
    //
    {
        NTSTATUS status;
        KWString fromFile(Allocator, DirPath);
        KWString toFile(Allocator, DirPath);

        fromFile += KVolumeNamespace::PathSeparator;
        fromFile += L"FromFile";
        KInvariant(NT_SUCCESS(fromFile.Status()));
        
        toFile += KVolumeNamespace::PathSeparator;
        toFile += L"ToFile";
        KInvariant(NT_SUCCESS(toFile.Status()));

        status = co_await CreateAFileAsync(fromFile, Allocator);
        KInvariant(NT_SUCCESS(status));

        status = co_await CreateAFileAsync(toFile, Allocator);
        KInvariant(NT_SUCCESS(status));

        status = co_await KVolumeNamespace::RenameFileAsync(fromFile, toFile, FALSE, Allocator, NULL);
        KInvariant(status == STATUS_OBJECT_NAME_COLLISION);

        status = co_await VerifyAFileAsync(toFile, Allocator);
        KInvariant(NT_SUCCESS(status));

        status = co_await VerifyAFileAsync(fromFile, Allocator);
        KInvariant(NT_SUCCESS(status));

        status = co_await DeleteAFileAsync(toFile, Allocator);
        KInvariant(NT_SUCCESS(status));
        
        status = co_await DeleteAFileAsync(fromFile, Allocator);
        KInvariant(NT_SUCCESS(status));
    }

    //
    // Test 5: Failed rename from A to B, A does not exist.
    //
    {
        NTSTATUS status;
        KWString fromFile(Allocator, DirPath);
        KWString toFile(Allocator, DirPath);

        fromFile += KVolumeNamespace::PathSeparator;
        fromFile += L"FromFile";
        KInvariant(NT_SUCCESS(fromFile.Status()));
        
        toFile += KVolumeNamespace::PathSeparator;
        toFile += L"ToFile";
        KInvariant(NT_SUCCESS(toFile.Status()));

        status = co_await KVolumeNamespace::RenameFileAsync(fromFile, toFile, FALSE, Allocator, NULL);
        KInvariant(status == STATUS_OBJECT_NAME_NOT_FOUND);
    }
    
    co_return(STATUS_SUCCESS);
}
#endif

NTSTATUS
TestRenameFiles(
    __in KAllocator& Allocator,
    __in WCHAR* DirPath
    )
{
    //
    // Test 1: Successful rename from A to B, B does not exist, no
    //         overwrite flag set
    //
    {
        NTSTATUS status;
        KSynchronizer sync;     
        KWString fromFile(Allocator, DirPath);
        KWString toFile(Allocator, DirPath);
        
        fromFile += KVolumeNamespace::PathSeparator;
        fromFile += L"FromFile";
        KInvariant(NT_SUCCESS(fromFile.Status()));
        
        toFile += KVolumeNamespace::PathSeparator;
        toFile += L"ToFile";
        KInvariant(NT_SUCCESS(toFile.Status()));

        status = DeleteAFile(fromFile, Allocator);      
        status = DeleteAFile(toFile, Allocator);
        
        status = CreateAFile(fromFile, Allocator);
        KInvariant(NT_SUCCESS(status));

        status = KVolumeNamespace::RenameFile(fromFile, toFile, FALSE, Allocator, sync, NULL);
        KInvariant(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        KInvariant(NT_SUCCESS(status));

        status = VerifyAFile(toFile, Allocator);
        KInvariant(NT_SUCCESS(status));

        status = VerifyAFile(fromFile, Allocator);
        KInvariant(status == STATUS_OBJECT_NAME_NOT_FOUND);

        status = DeleteAFile(toFile, Allocator);
        KInvariant(NT_SUCCESS(status));


        //
        // Now try with the WCHAR version of the api
        //
        status = CreateAFile(fromFile, Allocator);
        KInvariant(NT_SUCCESS(status));

        status = KVolumeNamespace::RenameFile((PWCHAR)fromFile, (PWCHAR)toFile, FALSE, Allocator, sync, NULL);
        KInvariant(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        KInvariant(NT_SUCCESS(status));

        status = VerifyAFile(toFile, Allocator);
        KInvariant(NT_SUCCESS(status));

        status = VerifyAFile(fromFile, Allocator);
        KInvariant(status == STATUS_OBJECT_NAME_NOT_FOUND);

        status = DeleteAFile(toFile, Allocator);
        KInvariant(NT_SUCCESS(status));
        
    }

    //
    // Test 2: Successful rename from A to B, B does exist, 
    //         overwrite flag set
    //
    {
        NTSTATUS status;
        KSynchronizer sync;     
        KWString fromFile(Allocator, DirPath);
        KWString toFile(Allocator, DirPath);

        fromFile += KVolumeNamespace::PathSeparator;
        fromFile += L"FromFile";
        KInvariant(NT_SUCCESS(fromFile.Status()));
        
        toFile += KVolumeNamespace::PathSeparator;
        toFile += L"ToFile";
        KInvariant(NT_SUCCESS(toFile.Status()));

        status = CreateAFile(fromFile, Allocator);
        KInvariant(NT_SUCCESS(status));

        status = CreateAFile(toFile, Allocator);
        KInvariant(NT_SUCCESS(status));

        status = KVolumeNamespace::RenameFile(fromFile, toFile, TRUE, Allocator, sync, NULL);
        KInvariant(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        KInvariant(NT_SUCCESS(status));

        status = VerifyAFile(toFile, Allocator);
        KInvariant(NT_SUCCESS(status));

        status = VerifyAFile(fromFile, Allocator);
        KInvariant(status == STATUS_OBJECT_NAME_NOT_FOUND);

        status = DeleteAFile(toFile, Allocator);
        KInvariant(NT_SUCCESS(status));
    }

    //
    // Test 3: Successful rename from A to B, B does not exist, 
    //         overwrite flag set
    //
    {
        NTSTATUS status;
        KSynchronizer sync;     
        KWString fromFile(Allocator, DirPath);
        KWString toFile(Allocator, DirPath);

        fromFile += KVolumeNamespace::PathSeparator;
        fromFile += L"FromFile";
        KInvariant(NT_SUCCESS(fromFile.Status()));
        
        toFile += KVolumeNamespace::PathSeparator;
        toFile += L"ToFile";
        KInvariant(NT_SUCCESS(toFile.Status()));

        status = CreateAFile(fromFile, Allocator);
        KInvariant(NT_SUCCESS(status));

        status = KVolumeNamespace::RenameFile(fromFile, toFile, TRUE, Allocator, sync, NULL);
        KInvariant(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        KInvariant(NT_SUCCESS(status));

        status = VerifyAFile(toFile, Allocator);
        KInvariant(NT_SUCCESS(status));

        status = VerifyAFile(fromFile, Allocator);
        KInvariant(status == STATUS_OBJECT_NAME_NOT_FOUND);

        status = DeleteAFile(toFile, Allocator);
        KInvariant(NT_SUCCESS(status));
    }

    //
    // Test 4: Failed rename from A to B, B does exist, 
    //         no overwrite flag set
    //
    {
        NTSTATUS status;
        KSynchronizer sync;     
        KWString fromFile(Allocator, DirPath);
        KWString toFile(Allocator, DirPath);

        fromFile += KVolumeNamespace::PathSeparator;
        fromFile += L"FromFile";
        KInvariant(NT_SUCCESS(fromFile.Status()));
        
        toFile += KVolumeNamespace::PathSeparator;
        toFile += L"ToFile";
        KInvariant(NT_SUCCESS(toFile.Status()));

        status = CreateAFile(fromFile, Allocator);
        KInvariant(NT_SUCCESS(status));

        status = CreateAFile(toFile, Allocator);
        KInvariant(NT_SUCCESS(status));

        status = KVolumeNamespace::RenameFile(fromFile, toFile, FALSE, Allocator, sync, NULL);
        KInvariant(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        KInvariant(status == STATUS_OBJECT_NAME_COLLISION);

        status = VerifyAFile(toFile, Allocator);
        KInvariant(NT_SUCCESS(status));

        status = VerifyAFile(fromFile, Allocator);
        KInvariant(NT_SUCCESS(status));

        status = DeleteAFile(toFile, Allocator);
        KInvariant(NT_SUCCESS(status));
        
        status = DeleteAFile(fromFile, Allocator);
        KInvariant(NT_SUCCESS(status));
    }

    //
    // Test 5: Failed rename from A to B, A does not exist.
    //
    {
        NTSTATUS status;
        KSynchronizer sync;     
        KWString fromFile(Allocator, DirPath);
        KWString toFile(Allocator, DirPath);

        fromFile += KVolumeNamespace::PathSeparator;
        fromFile += L"FromFile";
        KInvariant(NT_SUCCESS(fromFile.Status()));
        
        toFile += KVolumeNamespace::PathSeparator;
        toFile += L"ToFile";
        KInvariant(NT_SUCCESS(toFile.Status()));

        status = KVolumeNamespace::RenameFile(fromFile, toFile, FALSE, Allocator, sync, NULL);
        KInvariant(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        KInvariant(status == STATUS_OBJECT_NAME_NOT_FOUND);
    }
    
    return(STATUS_SUCCESS);
}


NTSTATUS
KVolumeNamespaceTestX(
    int argc, WCHAR* args[]
    )
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

    NTSTATUS status = STATUS_SUCCESS;
    TestContext::SPtr context;
    KAsyncContextBase::CompletionCallback completion;
#if !defined(PLATFORM_UNIX) 
    KVolumeNamespace::VolumeIdArray idArray(KtlSystem::GlobalNonPagedAllocator());
    KVolumeNamespace::VolumeInformationArray informationArray(KtlSystem::GlobalNonPagedAllocator());
#endif
    KWString volumeName(KtlSystem::GlobalNonPagedAllocator());
    KWString testDirName(KtlSystem::GlobalNonPagedAllocator());
    KWString dirName(KtlSystem::GlobalNonPagedAllocator());
    ULONG i;
    KGuid guid;
    KWString guidName(KtlSystem::GlobalNonPagedAllocator());
    KWString subDirName(KtlSystem::GlobalNonPagedAllocator());
    KVolumeNamespace::NameArray nameArray(KtlSystem::GlobalNonPagedAllocator());
    ULONG j;
    LONG r;
    KBlockFile::SPtr file;

    EventRegisterMicrosoft_Windows_KTL();

    KTestPrintf("Starting KVolumeNamespaceTest test\n");

    //
    // Test SplitObjectPathInPathAndFilename
    //
    status = TestSplitObjectPathInPathAndFilename();
    KInvariant(NT_SUCCESS(status));

#if defined(PLATFORM_UNIX)  
    //
    // Test CreateFullyQualifiedName for / on Linux
    //
    {        
        KWString name(KtlSystem::GlobalNonPagedAllocator());
        KWString actualName(KtlSystem::GlobalNonPagedAllocator());
        KWString dir1(KtlSystem::GlobalNonPagedAllocator());
        KWString expectedName1(KtlSystem::GlobalNonPagedAllocator());
        KWString dir2(KtlSystem::GlobalNonPagedAllocator());
        KWString expectedName2(KtlSystem::GlobalNonPagedAllocator());
        name = L"filename";

    status = name.Status();
    if (!NT_SUCCESS(status)) {
        KTestPrintf("could not assign name %x\n", status);
        goto Finish;
    }

    expectedName1 = L"/filename";
    status = expectedName1.Status();
    if (!NT_SUCCESS(status)) {
        KTestPrintf("could not assign expectedname1 %x\n", status);
        goto Finish;
    }

    dir1 = L"/";
    status = dir1.Status();
    if (!NT_SUCCESS(status)) {
        KTestPrintf("could not assign dir1 %x\n", status);
        goto Finish;
    }
        status = KVolumeNamespace::CreateFullyQualifiedName(dir1,
                                name,
                                actualName);
    if (!NT_SUCCESS(status)) {
        KTestPrintf("CreateFullyQualifiedName 1 failed %x\n", status);
        goto Finish;
    }
    
    if (actualName.CompareTo(expectedName1) != 0)
    {
        KTestPrintf("Compare1 failed %x\n", status);
        goto Finish;
    }
    
    expectedName2 = L"/dir/filename";
    status = expectedName2.Status();
    if (!NT_SUCCESS(status)) {
        KTestPrintf("could not assign expectedname2 %x\n", status);
        goto Finish;
    }
    
    dir2 = L"/dir";
    status = dir2.Status();
    if (!NT_SUCCESS(status)) {
        KTestPrintf("could not assign dir2 %x\n", status);
        goto Finish;
    }
        status = KVolumeNamespace::CreateFullyQualifiedName(dir2,
                                name,
                                actualName);
    if (!NT_SUCCESS(status)) {
        KTestPrintf("CreateFullyQualifiedName 2 failed %x\n", status);
        goto Finish;
    }
    
    if (actualName.CompareTo(expectedName2) != 0)
    {
        KTestPrintf("Compare2 failed %x", status);
        goto Finish;
    }
    
    }
#endif    
    //
    // Create the test context that will allow this test to just run on this thread.
    //

    status = TestContext::Create(context);

    if (!NT_SUCCESS(status)) {
        KTestPrintf("could not create test context %x\n", status);
        goto Finish;
    }

    //
    // Bind the completion delegate.
    //

    completion.Bind(context.RawPtr(), &TestContext::Completion);

#if !defined(PLATFORM_UNIX)
    //
    // Enumerate all of the volumes.
    //

    status = KVolumeNamespace::QueryVolumeList(idArray, KtlSystem::GlobalNonPagedAllocator(), completion);

    if (status == STATUS_PENDING) {
        status = context->Wait();
    }

    if (!NT_SUCCESS(status)) {
        KTestPrintf("could not query the volume list %x\n", status);
        goto Finish;
    }

    //
    // Enumerate all of the unstable volumes.
    //

    status = KVolumeNamespace::QueryVolumeListEx(informationArray, KtlSystem::GlobalNonPagedAllocator(), completion);

    if (status == STATUS_PENDING) {
        status = context->Wait();
    }

    if (!NT_SUCCESS(status)) {
        KTestPrintf("could not query the unstable volume list %x\n", status);
        goto Finish;
    }

    for (i = 0; i < informationArray.Count(); i++) {
        if (!informationArray[i].IsStableId) {
            idArray.Append(informationArray[i].VolumeId);
        }
    }

    //
    // Create the union of the stable and unstable lists.
    //

    //
    // If the list of volumes is empty, then just finish now.
    //

    if (!idArray.Count()) {
        KTestPrintf("There are no volumes to experiment on.  Probably there are no GPT volumes, only MBR volumes.\n");
        goto Finish;
    }

    GUID gguid;

    //
    // Pick out c:. If C: is not present them pick first one from the stable list
    //
    if (! (KVolumeNamespace::QueryVolumeIdFromDriveLetter(informationArray, 'C', gguid))) {
        gguid = idArray[0];
    }

    //
    // Make the volume name from the guid.
    //

    status = KVolumeNamespace::CreateFullyQualifiedRootDirectoryName(gguid, volumeName);

    if (!NT_SUCCESS(status)) {
        KTestPrintf("could not create volume name %x\n", status);
        goto Finish;
    }
#else
    volumeName = L"/tmp";
    status = volumeName.Status();
    if (!NT_SUCCESS(status)) {
        KTestPrintf("could not create volume name %x\n", status);
        goto Finish;
    }   
#endif

    testDirName = L"KVolumeNamespaceTestDirectory";

    status = testDirName.Status();

    if (!NT_SUCCESS(status)) {
        KTestPrintf("could not create test dir name %x\n", status);
        goto Finish;
    }
    
    //
    // Create a new name for a test directory.
    //

    status = KVolumeNamespace::CreateFullyQualifiedName(volumeName, testDirName, dirName);

    if (!NT_SUCCESS(status)) {
        KTestPrintf("could not create dir name %x\n", status);
        goto Finish;
    }

    //
    // Create the test directory.
    //

    status = KVolumeNamespace::CreateDirectory(dirName, KtlSystem::GlobalNonPagedAllocator(), completion);

    if (status == STATUS_PENDING) {
        status = context->Wait();
    }

    //
    // Ignore the error, the directory might already be there.
    //


    //
    // Rename files tests
    //
    status = TestRenameFiles(KtlSystem::GlobalNonPagedAllocator(), (WCHAR*)dirName);
    KInvariant(NT_SUCCESS(status));

        
    //
    // Create some sub-directories and some files.
    //

    for (i = 0; i < 10; i++) {

        //
        // Create sub-directory with guid-name.
        //

        guid.CreateNew();

        guidName = guid;

        status = guidName.Status();

        if (!NT_SUCCESS(status)) {
            KTestPrintf("could not create guid name %x\n", status);
            goto Finish;
        }

        status = KVolumeNamespace::CreateFullyQualifiedName(dirName, guidName, subDirName);

        if (!NT_SUCCESS(status)) {
            KTestPrintf("could not create sub dir name %x\n", status);
            goto Finish;
        }

        status = KVolumeNamespace::CreateDirectory(subDirName, KtlSystem::GlobalNonPagedAllocator(), completion);

        if (status == STATUS_PENDING) {
            status = context->Wait();
        }

        if (!NT_SUCCESS(status)) {
            KTestPrintf("could not create directory %x\n", status);
            goto Finish;
        }

        //
        // Enumerate directory, and make sure that the created sub-directory is there.
        //

        status = KVolumeNamespace::QueryDirectories(dirName, nameArray, KtlSystem::GlobalNonPagedAllocator(), completion);

        if (status == STATUS_PENDING) {
            status = context->Wait();
        }

        if (!NT_SUCCESS(status)) {
            KTestPrintf("query directories failed with %x\n", status);
            goto Finish;
        }

        for (j = 0; j < nameArray.Count(); j++) {
            r = nameArray[j].CompareTo(guidName);
            if (!r) {
                break;
            }
        }

        if (j == nameArray.Count()) {
            KTestPrintf("created directory not found\n", status);
            status = STATUS_INVALID_PARAMETER;
            goto Finish;
        }

        //
        // Create file with guid name.
        //

        guid.CreateNew();

        guidName = guid;

        status = guidName.Status();

        if (!NT_SUCCESS(status)) {
            KTestPrintf("could not create guid name %x\n", status);
            goto Finish;
        }

        status = KVolumeNamespace::CreateFullyQualifiedName(dirName, guidName, subDirName);

        if (!NT_SUCCESS(status)) {
            KTestPrintf("could not create sub dir name %x\n", status);
            goto Finish;
        }

        status = KBlockFile::Create(subDirName, FALSE, KBlockFile::eCreateNew, file, completion, NULL,
                KtlSystem::GlobalNonPagedAllocator());

        if (status == STATUS_PENDING) {
            status = context->Wait();
        }

        if (!NT_SUCCESS(status)) {
            KTestPrintf("create file failed with %x\n", status);
            goto Finish;
        }

        file = NULL;

        //
        // Enumerate directory, and make sure that the created file is there.
        //

        status = KVolumeNamespace::QueryFiles(dirName, nameArray, KtlSystem::GlobalNonPagedAllocator(), completion);

        if (status == STATUS_PENDING) {
            status = context->Wait();
        }

        if (!NT_SUCCESS(status)) {
            KTestPrintf("query directories failed with %x\n", status);
            goto Finish;
        }

        for (j = 0; j < nameArray.Count(); j++) {
            r = nameArray[j].CompareTo(guidName);
            if (!r) {
                break;
            }
        }

        if (j == nameArray.Count()) {
            KTestPrintf("created directory not found\n", status);
            status = STATUS_INVALID_PARAMETER;
            goto Finish;
        }
    }

    //
    // Clean up, delete everything.
    //

    status = KVolumeNamespace::QueryFiles(dirName, nameArray, KtlSystem::GlobalNonPagedAllocator(), completion);

    if (status == STATUS_PENDING) {
        status = context->Wait();
    }

    if (!NT_SUCCESS(status)) {
        KTestPrintf("query files failed with %x\n", status);
        goto Finish;
    }

    for (i = 0; i < nameArray.Count(); i++) {

        status = KVolumeNamespace::CreateFullyQualifiedName(dirName, nameArray[i], subDirName);

        if (!NT_SUCCESS(status)) {
            KTestPrintf("could not create sub dir name %x\n", status);
            goto Finish;
        }

        status = KVolumeNamespace::DeleteFileOrDirectory(subDirName, KtlSystem::GlobalNonPagedAllocator(), completion);

        if (status == STATUS_PENDING) {
            status = context->Wait();
        }

        if (!NT_SUCCESS(status)) {
            KTestPrintf("delete file failed with %x\n", status);
            goto Finish;
        }
    }

    status = KVolumeNamespace::QueryDirectories(dirName, nameArray, KtlSystem::GlobalNonPagedAllocator(), completion);

    if (status == STATUS_PENDING) {
        status = context->Wait();
    }

    if (!NT_SUCCESS(status)) {
        KTestPrintf("query directories failed with %x\n", status);
        goto Finish;
    }

    for (i = 0; i < nameArray.Count(); i++) {

        status = KVolumeNamespace::CreateFullyQualifiedName(dirName, nameArray[i], subDirName);

        if (!NT_SUCCESS(status)) {
            KTestPrintf("could not create sub dir name %x\n", status);
            goto Finish;
        }

        status = KVolumeNamespace::DeleteFileOrDirectory(subDirName, KtlSystem::GlobalNonPagedAllocator(), completion);

        if (status == STATUS_PENDING) {
            status = context->Wait();
        }

        if (!NT_SUCCESS(status)) {
            KTestPrintf("delete file failed with %x\n", status);
            goto Finish;
        }
    }

    //
    // Finally delete the test directory.
    //

    status = KVolumeNamespace::DeleteFileOrDirectory(dirName, KtlSystem::GlobalNonPagedAllocator(), completion);

    if (status == STATUS_PENDING) {
        status = context->Wait();
    }

    if (!NT_SUCCESS(status)) {
        KTestPrintf("delete test directory failed with %x\n", status);
        goto Finish;
    }

#if !defined(PLATFORM_UNIX)
    HardLinksSupportTest();
#endif

Finish:

    EventUnregisterMicrosoft_Windows_KTL();

    return status;
}

#if defined(PLATFORM_UNIX)
static std::string utf16to8X(const wchar_t *wstr)
{
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> conv;
    return conv.to_bytes((const char16_t *) wstr);
}
#endif

#if defined(K_UseResumable)
ktl::Awaitable<void>
LongPathnameTestAsync(
    __in KAllocator& Allocator,
    __in WCHAR* DirPath
    )
{
    NTSTATUS status;
    PWCHAR longPath128 = L"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
    KWString dirName(Allocator, DirPath);

    //
    // Test 1: Create a directory with a very long path
    //
    {    
        for (ULONG i = 0; i < 8; i++)
        {
            dirName += KVolumeNamespace::PathSeparator;
            dirName += longPath128;     
            KInvariant(NT_SUCCESS(dirName.Status()));

            status = co_await KVolumeNamespace::CreateDirectoryAsync((LPCWSTR)dirName, Allocator);

#if defined(PLATFORM_UNIX)
            if (! NT_SUCCESS(status))
            {
                std::string fmtA = utf16to8X((LPCWSTR)dirName);
                printf("\nStatus %x %s %llx \n", status, fmtA.c_str(), (ULONGLONG)((LPCWSTR)dirName));
            }
#endif
            
            KInvariant(NT_SUCCESS(status));     
        }
    }
    
    KWString fileName(dirName);
    fileName += KVolumeNamespace::PathSeparator;
    fileName += L"filename";
    KInvariant(NT_SUCCESS(fileName.Status()));

    KWString fileName2(dirName);
    fileName2 += KVolumeNamespace::PathSeparator;
    fileName2 += L"filename2";
    KInvariant(NT_SUCCESS(fileName2.Status()));    

    //
    // Test 2: Create a file within that directory
    //
    {
        status = co_await CreateAFileAsync(fileName, Allocator);
        KInvariant(NT_SUCCESS(status));     
    }

    //
    // Test 3: Rename file within that long directory and verify new
    //         file
    //
    {
        
        status = co_await KVolumeNamespace::RenameFileAsync((LPCWSTR)fileName, (LPCWSTR)fileName2, FALSE, Allocator, NULL);
        KInvariant(NT_SUCCESS(status));

        status = co_await VerifyAFileAsync(fileName2, Allocator);
        KInvariant(NT_SUCCESS(status));
    }

    //
    // Test 4: Delete file and directory with long names
    //
    {
        status = co_await KVolumeNamespace::DeleteFileOrDirectoryAsync((LPCWSTR)fileName2, Allocator);
        KInvariant(NT_SUCCESS(status));

        status = co_await KVolumeNamespace::DeleteFileOrDirectoryAsync(dirName, Allocator);
        KInvariant(NT_SUCCESS(status));     
    }

    co_await DeleteDirAsync(DirPath, Allocator);
    co_return;
}

ktl::Awaitable<NTSTATUS>
KVolumeNamespaceTestXAsync(
    int argc, WCHAR* args[]
    )
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

    NTSTATUS status = STATUS_SUCCESS;
    TestContext::SPtr context;
    KAsyncContextBase::CompletionCallback completion;
#if !defined(PLATFORM_UNIX) 
    KVolumeNamespace::VolumeIdArray idArray(KtlSystem::GlobalNonPagedAllocator());
    KVolumeNamespace::VolumeInformationArray informationArray(KtlSystem::GlobalNonPagedAllocator());
#endif
    KWString volumeName(KtlSystem::GlobalNonPagedAllocator());
    KWString testDirName(KtlSystem::GlobalNonPagedAllocator());
    KWString dirName(KtlSystem::GlobalNonPagedAllocator());
    ULONG i;
    KGuid guid;
    KWString guidName(KtlSystem::GlobalNonPagedAllocator());
    KWString subDirName(KtlSystem::GlobalNonPagedAllocator());
    KVolumeNamespace::NameArray nameArray(KtlSystem::GlobalNonPagedAllocator());
    ULONG j;
    LONG r;
    KBlockFile::SPtr file;

    EventRegisterMicrosoft_Windows_KTL();

    KTestPrintf("Starting KVolumeNamespaceTest Async test\n");

#if !defined(PLATFORM_UNIX)
    //
    // Enumerate all of the volumes.
    //

    status = co_await KVolumeNamespace::QueryVolumeListAsync(idArray, KtlSystem::GlobalNonPagedAllocator());

    if (!NT_SUCCESS(status)) {
        KTestPrintf("could not query the volume list %x\n", status);
        goto Finish;
    }

    //
    // Enumerate all of the unstable volumes.
    //

    status = co_await KVolumeNamespace::QueryVolumeListExAsync(informationArray, KtlSystem::GlobalNonPagedAllocator());
    
    if (!NT_SUCCESS(status)) {
        KTestPrintf("could not query the unstable volume list %x\n", status);
        goto Finish;
    }

    for (i = 0; i < informationArray.Count(); i++) {
        if (!informationArray[i].IsStableId) {
            idArray.Append(informationArray[i].VolumeId);
        }
    }

    //
    // Create the union of the stable and unstable lists.
    //

    //
    // If the list of volumes is empty, then just finish now.
    //

    if (!idArray.Count()) {
        KTestPrintf("There are no volumes to experiment on.  Probably there are no GPT volumes, only MBR volumes.\n");
        goto Finish;
    }

    GUID gguid;

    //
    // Pick out c:. If C: is not present them pick first one from the stable list
    //
    if (! (KVolumeNamespace::QueryVolumeIdFromDriveLetter(informationArray, 'C', gguid))) {
        gguid = idArray[0];
    }

    //
    // Make the volume name from the guid.
    //

    status = KVolumeNamespace::CreateFullyQualifiedRootDirectoryName(gguid, volumeName);

    if (!NT_SUCCESS(status)) {
        KTestPrintf("could not create volume name %x\n", status);
        goto Finish;
    }
#else
    volumeName = L"/tmp";
    status = volumeName.Status();
    if (!NT_SUCCESS(status)) {
        KTestPrintf("could not create volume name %x\n", status);
        goto Finish;
    }   
#endif

    testDirName = L"KVolumeNamespaceTestDirectory";

    status = testDirName.Status();

    if (!NT_SUCCESS(status)) {
        KTestPrintf("could not create test dir name %x\n", status);
        goto Finish;
    }
    
    //
    // Create a new name for a test directory.
    //

    status = KVolumeNamespace::CreateFullyQualifiedName(volumeName, testDirName, dirName);

    if (!NT_SUCCESS(status)) {
        KTestPrintf("could not create dir name %x\n", status);
        goto Finish;
    }

    //
    // Create the test directory.
    //

    status = co_await KVolumeNamespace::CreateDirectoryAsync(dirName, KtlSystem::GlobalNonPagedAllocator());

    //
    // Ignore the error, the directory might already be there.
    //

    //
    // Rename files tests
    //
    status = co_await TestRenameFilesAsync(KtlSystem::GlobalNonPagedAllocator(), (WCHAR*)dirName);
    KInvariant(NT_SUCCESS(status));

    //
    // Create some sub-directories and some files.
    //

    for (i = 0; i < 10; i++) {

        //
        // Create sub-directory with guid-name.
        //

        guid.CreateNew();

        guidName = guid;

        status = guidName.Status();

        if (!NT_SUCCESS(status)) {
            KTestPrintf("could not create guid name %x\n", status);
            goto Finish;
        }

        status = KVolumeNamespace::CreateFullyQualifiedName(dirName, guidName, subDirName);

        if (!NT_SUCCESS(status)) {
            KTestPrintf("could not create sub dir name %x\n", status);
            goto Finish;
        }

        status = co_await KVolumeNamespace::CreateDirectoryAsync(subDirName, KtlSystem::GlobalNonPagedAllocator());

        if (!NT_SUCCESS(status)) {
            KTestPrintf("could not create directory %x\n", status);
            goto Finish;
        }

        //
        // Enumerate directory, and make sure that the created sub-directory is there.
        //

        status = co_await KVolumeNamespace::QueryDirectoriesAsync(dirName, nameArray, KtlSystem::GlobalNonPagedAllocator());
        
        if (!NT_SUCCESS(status)) {
            KTestPrintf("query directories failed with %x\n", status);
            goto Finish;
        }

        for (j = 0; j < nameArray.Count(); j++) {
            r = nameArray[j].CompareTo(guidName);
            if (!r) {
                break;
            }
        }

        if (j == nameArray.Count()) {
            KTestPrintf("created directory not found\n", status);
            status = STATUS_INVALID_PARAMETER;
            goto Finish;
        }

        //
        // Create file with guid name.
        //

        guid.CreateNew();

        guidName = guid;

        status = guidName.Status();

        if (!NT_SUCCESS(status)) {
            KTestPrintf("could not create guid name %x\n", status);
            goto Finish;
        }

        status = KVolumeNamespace::CreateFullyQualifiedName(dirName, guidName, subDirName);

        if (!NT_SUCCESS(status)) {
            KTestPrintf("could not create sub dir name %x\n", status);
            goto Finish;
        }

        status = co_await KBlockFile::CreateAsync(subDirName, FALSE, KBlockFile::eCreateNew, file, NULL,
                KtlSystem::GlobalNonPagedAllocator());

        if (!NT_SUCCESS(status)) {
            KTestPrintf("create file failed with %x\n", status);
            goto Finish;
        }

        file = NULL;

        //
        // Enumerate directory, and make sure that the created file is there.
        //

        status = co_await KVolumeNamespace::QueryFilesAsync(dirName, nameArray, KtlSystem::GlobalNonPagedAllocator());

        if (status == STATUS_PENDING) {
            status = context->Wait();
        }

        if (!NT_SUCCESS(status)) {
            KTestPrintf("query directories failed with %x\n", status);
            goto Finish;
        }

        for (j = 0; j < nameArray.Count(); j++) {
            r = nameArray[j].CompareTo(guidName);
            if (!r) {
                break;
            }
        }

        if (j == nameArray.Count()) {
            KTestPrintf("created directory not found\n", status);
            status = STATUS_INVALID_PARAMETER;
            goto Finish;
        }
    }

    //
    // Clean up, delete everything.
    //

    status = co_await KVolumeNamespace::QueryFilesAsync(dirName, nameArray, KtlSystem::GlobalNonPagedAllocator());

    if (!NT_SUCCESS(status)) {
        KTestPrintf("query files failed with %x\n", status);
        goto Finish;
    }

    for (i = 0; i < nameArray.Count(); i++) {

        status = KVolumeNamespace::CreateFullyQualifiedName(dirName, nameArray[i], subDirName);

        if (!NT_SUCCESS(status)) {
            KTestPrintf("could not create sub dir name %x\n", status);
            goto Finish;
        }

        status = co_await KVolumeNamespace::DeleteFileOrDirectoryAsync(subDirName, KtlSystem::GlobalNonPagedAllocator());

        if (!NT_SUCCESS(status)) {
            KTestPrintf("delete file failed with %x\n", status);
            goto Finish;
        }
    }

    status = co_await KVolumeNamespace::QueryDirectoriesAsync(dirName, nameArray, KtlSystem::GlobalNonPagedAllocator());

    if (!NT_SUCCESS(status)) {
        KTestPrintf("query directories failed with %x\n", status);
        goto Finish;
    }

    for (i = 0; i < nameArray.Count(); i++) {

        status = KVolumeNamespace::CreateFullyQualifiedName(dirName, nameArray[i], subDirName);

        if (!NT_SUCCESS(status)) {
            KTestPrintf("could not create sub dir name %x\n", status);
            goto Finish;
        }

        status = co_await KVolumeNamespace::DeleteFileOrDirectoryAsync(subDirName, KtlSystem::GlobalNonPagedAllocator());
        if (!NT_SUCCESS(status)) {
            KTestPrintf("delete file failed with %x\n", status);
            goto Finish;
        }
    }

    co_await LongPathnameTestAsync(KtlSystem::GlobalNonPagedAllocator(), dirName);
    
#if !defined(PLATFORM_UNIX)
    co_await HardLinksSupportTestAsync();
#endif

Finish:

    EventUnregisterMicrosoft_Windows_KTL();

    co_return status;
}
#endif


NTSTATUS
KVolumeNamespaceTest(
    int argc, WCHAR* args[]
    )
{
    NTSTATUS status;

#if defined(PLATFORM_UNIX)
    status = KtlTraceRegister();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Failed to KtlTraceRegister\n");
        return(status);
    }
#endif
    
    KTestPrintf("KVolumeNamespaceTest: STARTED\n");

    status = KtlSystem::Initialize();

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = KVolumeNamespaceTestX(argc, args);
    if (! NT_SUCCESS(status))
    {
        KInvariant(FALSE);
    }
    
#if defined(K_UseResumable)
    status = SyncAwait(KVolumeNamespaceTestXAsync(argc, args));
    if (! NT_SUCCESS(status))
    {
        KInvariant(FALSE);
    }
#endif

    KtlSystem::Shutdown();

    KTestPrintf("KVolumeNamespaceTest: COMPLETED\n");

#if defined(PLATFORM_UNIX)
    KtlTraceUnregister();
#endif  
    
    return status;
}

#if CONSOLE_TEST
int
main(int argc, char* args[])
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

    return RtlNtStatusToDosError(KVolumeNamespaceTest(0, NULL));
}
#endif
