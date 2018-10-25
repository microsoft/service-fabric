/*++

Copyright (c) Microsoft Corporation

Module Name:

    RvdLoggerAliasTests.cpp

Abstract:

    This file contains test case implementations.

    To add a new unit test case:
    1. Declare your test case routine in RvdLoggerTests.h.
    2. Add an entry to the gs_KuTestCases table in RvdLoggerTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
        this file or any other file.

--*/
#pragma once
#include "RvdLoggerTests.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>
#include <InternalKtlLogger.h>
#include "RvdLogTestHelpers.h"

#if KTL_USER_MODE
#include <ktlevents.um.h>
#else
#include <ktlevents.km.h>
#endif

#if (CONSOLE_TEST) || (DISPLAY_ON_CONSOLE)
#undef KDbgPrintf
#define KDbgPrintf(...)     printf(__VA_ARGS__)
#endif



//* Sub-tree deletion helper
void
DeleteDir(WCHAR* DirPath, KAllocator& Allocator)
{
    KSynchronizer       compSync;
    NTSTATUS            status = STATUS_SUCCESS;
    KWString            toDelete(Allocator, DirPath);
    KInvariant(NT_SUCCESS(toDelete.Status()));

    // Recurse for each sub-dir
    KVolumeNamespace::NameArray namesToDelete(Allocator);
    status = KVolumeNamespace::QueryDirectories(toDelete, namesToDelete, Allocator, compSync);
    KInvariant(NT_SUCCESS(status));
    status = compSync.WaitForCompletion();
    if ((status == STATUS_OBJECT_NAME_NOT_FOUND) || (status == STATUS_NOT_A_DIRECTORY))
    {
        return;
    }
    KInvariant(NT_SUCCESS(status));

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
    KInvariant(NT_SUCCESS(status));

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
    KInvariant(NT_SUCCESS(status));
};

NTSTATUS
FileExists(const KStringView& FullFileNamePath)
{
    OBJECT_ATTRIBUTES   oa;
    FILE_NETWORK_OPEN_INFORMATION   info;
    UNICODE_STRING ucs = FullFileNamePath.ToUNICODE_STRING();

    InitializeObjectAttributes(&oa, &ucs, OBJ_CASE_INSENSITIVE, NULL, NULL);
    return KNt::QueryFullAttributesFile(oa, info);
}

NTSTATUS
InternalLoggerAliasTest(__in int argc, __in_ecount(argc) WCHAR* args[])
{
    KInvariant(argc == 1);

    NTSTATUS        status = STATUS_SUCCESS;
    KAllocator&     allocator = KtlSystem::GlobalNonPagedAllocator();
    GUID            driveGuid;
    KSynchronizer   sync;

    KWString        dedicatedLogFilesDirRoot(allocator);
    KWString        aliasLogFilesDirRoot(allocator);
	
#if !defined(PLATFORM_UNIX)
    KWString        driveRootStore(allocator, args[0]);
    KInvariant(NT_SUCCESS(driveRootStore.Status()));
    KStringView     driveRoot((WCHAR*)driveRootStore);
	
    if (driveRoot.Length() == 1)
    {
        driveRootStore += L":";
        KInvariant(NT_SUCCESS(driveRootStore.Status()));
        driveRoot.KStringView::KStringView((UNICODE_STRING)driveRootStore);
    }

    status = KVolumeNamespace::QueryVolumeIdFromRootDirectoryName(driveRootStore, allocator, driveGuid, sync);
    KInvariant(NT_SUCCESS(status));
    status = sync.WaitForCompletion();
    KInvariant(NT_SUCCESS(status));

    status = KVolumeNamespace::CreateFullyQualifiedRootDirectoryName(driveGuid, driveRootStore);
    KInvariant(NT_SUCCESS(status));
    driveRoot.KStringView::KStringView((UNICODE_STRING)driveRootStore);

    dedicatedLogFilesDirRoot = driveRootStore;
    aliasLogFilesDirRoot = driveRootStore;
#else
    dedicatedLogFilesDirRoot = L"";
    aliasLogFilesDirRoot = L"";
#endif
	
    dedicatedLogFilesDirRoot += KVolumeNamespace::PathSeparator;
    KInvariant(NT_SUCCESS(dedicatedLogFilesDirRoot.Status()));
    dedicatedLogFilesDirRoot += (WCHAR*)&RvdDiskLogConstants::RawDirectoryNameStr();
    KInvariant(NT_SUCCESS(dedicatedLogFilesDirRoot.Status()));
    aliasLogFilesDirRoot += KVolumeNamespace::PathSeparator;
    aliasLogFilesDirRoot += L"RvdLogAliasTest";
    KInvariant(NT_SUCCESS(aliasLogFilesDirRoot.Status()));
	
    DeleteDir(dedicatedLogFilesDirRoot, allocator);
    DeleteDir(aliasLogFilesDirRoot, allocator);

    status = KVolumeNamespace::CreateDirectory(aliasLogFilesDirRoot, allocator, sync);
    KInvariant(NT_SUCCESS(status));
    status = sync.WaitForCompletion();
    KInvariant(NT_SUCCESS(status));
    KFinally
    (
        [&] () -> void
        {
            DeleteDir(aliasLogFilesDirRoot, allocator);
        }
    );

    RvdLogManager::SPtr     logManager;
    KSynchronizer           logActiveSync;
    status = RvdLogManager::Create(KTL_TAG_TEST, allocator, logManager);
    KInvariant(NT_SUCCESS(status));

    status = logManager->Activate(nullptr, logActiveSync);
    KInvariant(NT_SUCCESS(status));

    //** Prove basic log file creation via an alias
    RvdLogManager::AsyncCreateLog::SPtr    createLogOp;
    status = logManager->CreateAsyncCreateLogContext(createLogOp);
    KInvariant(NT_SUCCESS(status));

    RvdLog::SPtr    log1;
    KWString        log1PathBuffer(allocator);

#if !defined(PLATFORM_UNIX)
    log1PathBuffer = driveRoot;
    status = log1PathBuffer.Status();
    KInvariant(NT_SUCCESS(status));
    log1PathBuffer += L"\\RvdLogAliasTest\\TestLog1.Alias.log";
#else
    log1PathBuffer = L"/tmp/TestLog1.Alias.log";
#endif
    status = log1PathBuffer.Status();
    KInvariant(NT_SUCCESS(status));

    KStringView     log1Path((WCHAR*)log1PathBuffer);

    KWString logType(KtlSystem::GlobalNonPagedAllocator(), L"RvdLog");
    KInvariant(NT_SUCCESS(logType.Status()));

    createLogOp->StartCreateLog(
        log1Path,
        RvdLogId(KGuid(TestLogIdGuid)),
        logType,
        DefaultTestLogFileSize,
        0,          // Use default
        0,          //      "
        log1,
        nullptr,
        sync);

    status = sync.WaitForCompletion();
    KInvariant(NT_SUCCESS(status));

    // Allow the log file to close; free up manager context
    createLogOp = nullptr;
    KNt::Sleep(250);
    KInvariant(log1.Reset());

    logManager->Deactivate();
    status = logActiveSync.WaitForCompletion();
    KInvariant(NT_SUCCESS(status));

    //  Sleep to give logManager a chance to be freed from the async
    KNt::Sleep(250);
    KInvariant(logManager.Reset());

    //** Prove the right files were created in the right place
    KWString            fullAliasFilePath(allocator);

    // Prove Alias exists
    fullAliasFilePath = log1Path;
    KInvariant(NT_SUCCESS(fullAliasFilePath.Status()));

    status = FileExists(KStringView((UNICODE_STRING)fullAliasFilePath));
    KInvariant(NT_SUCCESS(status));

    //** Prove normal Disk Recovery and log open via an Alias works
    status = RvdLogManager::Create(KTL_TAG_TEST, allocator, logManager);
    KInvariant(NT_SUCCESS(status));

    status = logManager->Activate(nullptr, logActiveSync);
    KInvariant(NT_SUCCESS(status));

    RvdLogManager::AsyncOpenLog::SPtr    openLogOp;
    status = logManager->CreateAsyncOpenLogContext(openLogOp);
    KInvariant(NT_SUCCESS(status));

    openLogOp->StartOpenLog(KStringView((UNICODE_STRING)fullAliasFilePath), log1, nullptr, sync);
    status = sync.WaitForCompletion();
    KInvariant(NT_SUCCESS(status));

    // Allow the log file to close; free up manager context
    openLogOp = nullptr;
    KInvariant(log1.Reset());

    logManager->Deactivate();
    status = logActiveSync.WaitForCompletion();
    KInvariant(NT_SUCCESS(status));
    KNt::Sleep(250);
    KInvariant(logManager.Reset());

    //** Prove Deletion via alias name works
    status = RvdLogManager::Create(KTL_TAG_TEST, allocator, logManager);
    KInvariant(NT_SUCCESS(status));

    status = logManager->Activate(nullptr, logActiveSync);
    KInvariant(NT_SUCCESS(status));

    status = FileExists(KStringView((UNICODE_STRING)fullAliasFilePath));
    KInvariant(NT_SUCCESS(status));

    RvdLogManager::AsyncDeleteLog::SPtr     deleteOp;
    status = logManager->CreateAsyncDeleteLogContext(deleteOp);
    KInvariant(NT_SUCCESS(status));

    deleteOp->StartDeleteLog(KStringView((UNICODE_STRING)fullAliasFilePath), nullptr, sync);
    status = sync.WaitForCompletion();
    KInvariant(NT_SUCCESS(status));

    deleteOp = nullptr;
    logManager->Deactivate();
    status = logActiveSync.WaitForCompletion();
    KInvariant(NT_SUCCESS(status));
    KNt::Sleep(250);
    KInvariant(logManager.Reset());

    status = FileExists(KStringView((UNICODE_STRING)fullAliasFilePath));
    KInvariant(status == STATUS_OBJECT_NAME_NOT_FOUND);

    //* Prove that Open/Create/Delete into the reserved directory fails
    KWString            fullDedicatedFilePath(allocator);
    status = RvdDiskLogConstants::BuildFullyQualifiedLogName(KGuid(driveGuid), RvdLogId(KGuid(TestLogIdGuid)), fullDedicatedFilePath);
    KInvariant(NT_SUCCESS(status));

    status = RvdLogManager::Create(KTL_TAG_TEST, allocator, logManager);
    KInvariant(NT_SUCCESS(status));

    status = logManager->Activate(nullptr, logActiveSync);
    KInvariant(NT_SUCCESS(status));

    status = logManager->CreateAsyncCreateLogContext(createLogOp);
    KInvariant(NT_SUCCESS(status));

    createLogOp->StartCreateLog(
        KStringView((UNICODE_STRING)fullDedicatedFilePath),
        RvdLogId(KGuid(TestLogIdGuid)),
        logType,
        DefaultTestLogFileSize,
        0,          // Use default
        0,          //      "
        log1,
        nullptr,
        sync);

    status = sync.WaitForCompletion();
    KInvariant(status == STATUS_ACCESS_DENIED);

    status = logManager->CreateAsyncDeleteLogContext(deleteOp);
    KInvariant(NT_SUCCESS(status));

    deleteOp->StartDeleteLog(KStringView((UNICODE_STRING)fullDedicatedFilePath), nullptr, sync);
    status = sync.WaitForCompletion();
    KInvariant(status == STATUS_ACCESS_DENIED);

    status = logManager->CreateAsyncOpenLogContext(openLogOp);
    KInvariant(NT_SUCCESS(status));

    openLogOp->StartOpenLog(KStringView((UNICODE_STRING)fullDedicatedFilePath), log1, nullptr, sync);
    status = sync.WaitForCompletion();
    KInvariant(status == STATUS_ACCESS_DENIED);
	
    deleteOp = nullptr;
    openLogOp = nullptr;
    createLogOp = nullptr;
    log1 = nullptr;
    logManager->Deactivate();
    status = logActiveSync.WaitForCompletion();
    KInvariant(NT_SUCCESS(status));
    KNt::Sleep(250);
    KInvariant(logManager.Reset());

    DeleteDir(dedicatedLogFilesDirRoot, allocator);
    DeleteDir(aliasLogFilesDirRoot, allocator);
    return STATUS_SUCCESS;
}

//** Test Entry Point: AliasTest
NTSTATUS
RvdLoggerAliasTests(__in int argc, __in_ecount(argc) WCHAR* args[])
{
    NTSTATUS result;
    KtlSystem* underlyingSystem;

    EventRegisterMicrosoft_Windows_KTL();
    KtlSystem::Initialize(FALSE,              // Do not enable VNetworking
                         &underlyingSystem);

    underlyingSystem->SetStrictAllocationChecks(TRUE);
    if ((result = InternalLoggerAliasTest(argc, args)) != STATUS_SUCCESS)
    {
        KDbgPrintf("RvdLogger: InternalLoggerAliasTest: Unit Test: FAILED: 0X%X\n", result);
    }

    KtlSystem::Shutdown();
    EventUnregisterMicrosoft_Windows_KTL();

    return result;
}

