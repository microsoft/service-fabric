/*++

Copyright (c) Microsoft Corporation

Module Name:

    RvdLoggerTests.cpp

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

NTSTATUS LastStaticTestCallbackStatus;
const LONGLONG DefaultTestLogFileSize = (LONGLONG)1024*1024*1024;

#ifdef _WIN64
VOID
StaticTestCallback(
    PVOID EventPtr,
    KAsyncContextBase* const Parent,
    KAsyncContextBase& CompletedContext)
{
    UNREFERENCED_PARAMETER(Parent);

    KEvent* eventPtr = static_cast<KEvent*>(EventPtr);
    LastStaticTestCallbackStatus = CompletedContext.Status();
    eventPtr->SetEvent();
}
#endif

NTSTATUS
NullActivateDeactiveTest()
{
    NTSTATUS status;

    for (int i = 0; i < 10000; i++)
    {
        RvdLogManager::SPtr logManager;

        status = RvdLogManager::Create(KTL_TAG_LOGGER, KtlSystem::GlobalNonPagedAllocator(), logManager);
        if (NT_SUCCESS(status))
        {
            // Activate/Deactivate Test
            KEvent testSyncEvent(FALSE, FALSE);
            KAsyncContextBase::CompletionCallback callback;
#ifdef _WIN64
            callback.Bind((PVOID)(&testSyncEvent), &StaticTestCallback);
#endif
            status = logManager->Activate(nullptr, callback);
            if (!K_ASYNC_SUCCESS(status))
            {
                KDbgPrintf("NullActivateDeactiveTest: Activate() Failed: %i\n", __LINE__);
                KInvariant(FALSE);
                return status;
            }
            logManager->Deactivate();
            testSyncEvent.WaitUntilSet();

            logManager.Reset();
        }
        else
        {
            KDbgPrintf("NullActivateDeactiveTest: RvdLogManager::Create Failed: %i\n", __LINE__);
            KInvariant(FALSE);
            return status;
        }
    }

    {   // Prove Reuse works
        RvdLogManager::SPtr logManager;

        status = RvdLogManager::Create(KTL_TAG_LOGGER, KtlSystem::GlobalNonPagedAllocator(), logManager);
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("NullActivateDeactiveTest: RvdLogManager::Create Failed: %i\n", __LINE__);
            KInvariant(FALSE);
            return status;
        }

        KEvent testSyncEvent(FALSE, FALSE);
        KAsyncContextBase::CompletionCallback callback;
#ifdef _WIN64
        callback.Bind((PVOID)(&testSyncEvent), &StaticTestCallback);
#endif

        for (int i = 0; i < 10000; i++)
        {
            // Activate/Deactivate Test
            status = logManager->Activate(nullptr, callback);
            if (!K_ASYNC_SUCCESS(status))
            {
                KDbgPrintf("NullActivateDeactiveTest: Activate() Failed: %i\n", __LINE__);
                KInvariant(FALSE);
                return status;
            }
            logManager->Deactivate();
            testSyncEvent.WaitUntilSet();

            logManager->Reuse();
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
CleanAndPrepLog(WCHAR const DriveLetter, BOOLEAN CreateDir)
{
    // Make the directory if needed
    NTSTATUS status;
    WCHAR       driveLetterStr[2];
    driveLetterStr[0] = DriveLetter;
    driveLetterStr[1] = 0;

    KWString    rootPath(KtlSystem::GlobalNonPagedAllocator());
#if ! defined(PLATFORM_UNIX)
    rootPath = L"\\??\\";
    rootPath += driveLetterStr;
    rootPath += L":\\";
#else
    rootPath = L"";
#endif

    rootPath += &RvdDiskLogConstants::DirectoryName();
    if (!NT_SUCCESS(rootPath.Status()))
    {
        return rootPath.Status();
    }

    KEvent compEvent(FALSE, FALSE);
    KAsyncContextBase::CompletionCallback opDoneCallback;
#ifdef _WIN64
    opDoneCallback.Bind((PVOID)(&compEvent), &StaticTestCallback);
#endif

    // Try and enum and delete any existing log files
    KVolumeNamespace::NameArray files(KtlSystem::GlobalNonPagedAllocator());
    status = KVolumeNamespace::QueryFiles(rootPath, files, KtlSystem::GlobalNonPagedAllocator(), opDoneCallback);
    if (!K_ASYNC_SUCCESS(status))
    {
        KDbgPrintf("CleanAndPrepLog: QueryFiles() start failed: %i\n", __LINE__);
        return status;
    }

    compEvent.WaitUntilSet();
    status = LastStaticTestCallbackStatus;
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("CleanAndPrepLog: QueryFiles() failed: %i; with: 0x%0x\n", __LINE__, status);

        // OK to continue - maybe the directory does not exist
        if (CreateDir)
        {
            status = KVolumeNamespace::CreateDirectory(rootPath, KtlSystem::GlobalNonPagedAllocator(), opDoneCallback);
            if (!K_ASYNC_SUCCESS(status))
            {
                KDbgPrintf("CleanAndPrepLog: CreateDirectory() start failed: %i\n", __LINE__);
                return status;
            }

            compEvent.WaitUntilSet();
            status = LastStaticTestCallbackStatus;
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("CleanAndPrepLog: CreateDirectory() failed: %i; with: 0x%0x\n", __LINE__, status);
                return status;
            }
        }
    }
    else
    {
        //
        // Sleep to allow time for files to be close
        //
        KNt::Sleep(30 * 1000);

        // We have a list of files - try and delete them all
        for (ULONG ix = 0; ix < files.Count(); ix++)
        {
            KWString filePath(rootPath);
            filePath += files[ix];

            status = KVolumeNamespace::DeleteFileOrDirectory(filePath, KtlSystem::GlobalNonPagedAllocator(), opDoneCallback);
            if (!K_ASYNC_SUCCESS(status))
            {
                KDbgPrintf("CleanAndPrepLog: DeleteFileOrDirectory() start failed: %i\n", __LINE__);
                return status;
            }

            compEvent.WaitUntilSet();
            status = LastStaticTestCallbackStatus;
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("CleanAndPrepLog: DeleteFileOrDirectory() failed: %i; with: 0x%0x\n", __LINE__, status);
                return status;
            }
        }

        // Directory clean;
        if (!CreateDir)
        {
            status = KVolumeNamespace::DeleteFileOrDirectory(rootPath, KtlSystem::GlobalNonPagedAllocator(), opDoneCallback);
            if (!K_ASYNC_SUCCESS(status))
            {
                KDbgPrintf("CleanAndPrepLog: DeleteFileOrDirectory() start failed: %i\n", __LINE__);
                return status;
            }

            compEvent.WaitUntilSet();
            status = LastStaticTestCallbackStatus;
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("CleanAndPrepLog: DeleteFileOrDirectory() failed: %i; with: 0x%0x\n", __LINE__, status);
                return status;
            }
        }
    }

    return STATUS_SUCCESS;
}


NTSTATUS
BasicLogCreateTest()
{
    NTSTATUS status;
    UCHAR const testDriveLetter = 'C';

    // Clean the unit test drive environment
    status = CleanAndPrepLog((WCHAR)testDriveLetter, FALSE);        // Prove auto directory create works
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("BasicLogCreateTest: CleanAndPrepLog Failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return status;
    }

    RvdLogManager::SPtr logManager;
    KEvent activateEvent(FALSE, FALSE);

    status = RvdLogManager::Create(KTL_TAG_LOGGER, KtlSystem::GlobalNonPagedAllocator(), logManager);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("BasicLogCreateTest: RvdLogManager::Create Failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return status;
    }

    KAsyncContextBase::CompletionCallback activateDoneCallback;
#ifdef _WIN64
    activateDoneCallback.Bind((PVOID)(&activateEvent), &StaticTestCallback);
#endif

    status = logManager->Activate(nullptr, activateDoneCallback);
    if (!K_ASYNC_SUCCESS(status))
    {
        KDbgPrintf("BasicLogCreateTest: Activate() Failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return status;
    }

    RvdLogManager::AsyncCreateLog::SPtr createOp;
    status = logManager->CreateAsyncCreateLogContext(createOp);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("BasicLogCreateTest: CreateAsyncCreateLogContext Failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return status;
    }

    RvdLogManager::AsyncOpenLog::SPtr openOp;
    status = logManager->CreateAsyncOpenLogContext(openOp);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("BasicLogCreateTest: CreateAsyncOpenLogContext Failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return status;
    }

    RvdLogManager::AsyncDeleteLog::SPtr deleteOp;
    status = logManager->CreateAsyncDeleteLogContext(deleteOp);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("BasicLogCreateTest: CreateAsyncDeleteLogContext Failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return status;
    }

    RvdLogManager::AsyncEnumerateLogs::SPtr enumOp;
    status = logManager->CreateAsyncEnumerateLogsContext(enumOp);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("BasicLogCreateTest: CreateAsyncEnumerateLogsContext Failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return status;
    }

    GUID diskIdGuid = {};

#if ! defined(PLATFORM_UNIX)    
    KEvent getVolInfoCompleteEvent(FALSE, FALSE);
    KAsyncContextBase::CompletionCallback getVolInfoDoneCallback;
#ifdef _WIN64
    getVolInfoDoneCallback.Bind((PVOID)(&getVolInfoCompleteEvent), &StaticTestCallback);
#endif

    KVolumeNamespace::VolumeInformationArray volInfo(KtlSystem::GlobalNonPagedAllocator());
    status = KVolumeNamespace::QueryVolumeListEx(volInfo, KtlSystem::GlobalNonPagedAllocator(), getVolInfoDoneCallback);
    if (!K_ASYNC_SUCCESS(status))
    {
        KDbgPrintf("BasicLogCreateTest: KVolumeNamespace::QueryVolumeListEx Failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return status;
    }
    getVolInfoCompleteEvent.WaitUntilSet();
    if (!NT_SUCCESS(LastStaticTestCallbackStatus))
    {
        KDbgPrintf("BasicLogCreateTest: KVolumeNamespace::QueryVolumeListEx Failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return LastStaticTestCallbackStatus;
    }

    // Find Drive C: volume GUID
    ULONG i;
    for (i = 0; i < volInfo.Count(); i++)
    {
        if (volInfo[i].DriveLetter == testDriveLetter)
        {
            break;
        }
    }

    if (i == volInfo.Count())
    {
        KDbgPrintf("BasicLogCreateTest: KVolumeNamespace::QueryVolumeListEx did not return volume guid for drive C: %i\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }
    diskIdGuid = volInfo[i].VolumeId;
#else
    diskIdGuid = RvdDiskLogConstants::HardCodedVolumeGuid();
#endif
    
    RvdLog::SPtr log1;
    KEvent doneEvent(FALSE, FALSE);
    KAsyncContextBase::CompletionCallback doneCallback;
#ifdef _WIN64
    doneCallback.Bind((PVOID)(&doneEvent), &StaticTestCallback);
#endif

    // Prove Open of non-existing log fails
    openOp->StartOpenLog(
        KGuid(diskIdGuid),
        RvdLogId(KGuid(TestLogIdGuid)),
        log1,
        nullptr,
        doneCallback);

    doneEvent.WaitUntilSet();
    if (NT_SUCCESS(openOp->Status()))
    {
        KDbgPrintf("BasicLogCreateTest: StartOpenLog succeeded - unexpected: %i\n", __LINE__);
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    // Prove Delete of non-existing log fails
    deleteOp->StartDeleteLog(
        KGuid(diskIdGuid),
        RvdLogId(KGuid(TestLogIdGuid)),
        nullptr,
        doneCallback);

    doneEvent.WaitUntilSet();
    if (NT_SUCCESS(deleteOp->Status()))
    {
        KDbgPrintf("BasicLogCreateTest: StartDeleteLog succeeded - unexpected: %i\n", __LINE__);
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    // Prove Create with bad geometry params fails correctly
    KWString logType(KtlSystem::GlobalNonPagedAllocator(), L"RvdLog");
    KInvariant(NT_SUCCESS(logType.Status()));

    createOp->StartCreateLog(
        KGuid(diskIdGuid),
        RvdLogId(KGuid(TestLogIdGuid)),
        logType,
        DefaultTestLogFileSize,
        1,          // Must be > 1
        0,          // Use default
        log1,
        nullptr,
        doneCallback);

    doneEvent.WaitUntilSet();
    KInvariant(createOp->Status() == STATUS_INVALID_PARAMETER);
    createOp->Reuse();

    createOp->StartCreateLog(
        KGuid(diskIdGuid),
        RvdLogId(KGuid(TestLogIdGuid)),
        logType,
        DefaultTestLogFileSize,
        0,          // Use default
        2045,       // < min and not 0 % 4K
        log1,
        nullptr,
        doneCallback);

    doneEvent.WaitUntilSet();
    KInvariant(createOp->Status() == STATUS_INVALID_PARAMETER);
    createOp->Reuse();

    // Create a log file
    createOp->StartCreateLog(
        KGuid(diskIdGuid),
        RvdLogId(KGuid(TestLogIdGuid)),
        logType,
        DefaultTestLogFileSize,
        0,          // Use default max streams
        0,          // Use default max record size
        log1,
        nullptr,
        doneCallback);

    KWString diskId(KtlSystem::GlobalNonPagedAllocator());
    diskId = diskIdGuid;
    KWString logId(KtlSystem::GlobalNonPagedAllocator());
    logId = KGuid(TestLogIdGuid);

    doneEvent.WaitUntilSet();
    if (!NT_SUCCESS(createOp->Status()))
    {
        KDbgPrintf("BasicLogCreateTest: StartCreateLog(%ws, %ws, %I64d) Failed: 0x%x at line  %i\n", (LPCWSTR)diskId, (LPCWSTR)logId, DefaultTestLogFileSize, createOp->Status(), __LINE__);
        KInvariant(FALSE);
        return createOp->Status();
    }

    // Prove default config values used to create the log
    ULONG   maxStreams = log1->QueryMaxAllowedStreams();
    KInvariant(maxStreams == RvdLogConfig::DefaultMaxNumberOfStreams);

    ULONG   maxRecordSize = log1->QueryMaxRecordSize();
    KInvariant(maxRecordSize == RvdLogConfig::DefaultMaxRecordSize);

    // Test closure of the created log1
    if (!log1.Reset())
    {
        KDbgPrintf("BasicLogCreateTest: expected destruction did not occur: %i\n", __LINE__);
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    // Prove open of existing log works
    openOp->Reuse();
    openOp->StartOpenLog(
        KGuid(diskIdGuid),
        RvdLogId(KGuid(TestLogIdGuid)),
        log1,
        nullptr,
        doneCallback);

    doneEvent.WaitUntilSet();
    if (!NT_SUCCESS(openOp->Status()))
    {
        KDbgPrintf("BasicLogCreateTest: StartOpenLog failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return openOp->Status();
    }

    // Prove a Create on an opened log fails
    createOp->Reuse();
    RvdLog::SPtr log2;
    createOp->StartCreateLog(
        KGuid(diskIdGuid),
        RvdLogId(KGuid(TestLogIdGuid)),
        logType,
        DefaultTestLogFileSize,
        log2,
        nullptr,
        doneCallback);

    doneEvent.WaitUntilSet();
    if (NT_SUCCESS(createOp->Status()))
    {
        KDbgPrintf("BasicLogCreateTest: StartCreateLog succeeded - unexpected: %i\n", __LINE__);
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }
    if (log2)
    {
        KDbgPrintf("BasicLogCreateTest: StartCreateLog returned unexpected SPtr: %i\n", __LINE__);
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    // Prove second open on an already open RvdLog works and returns the same instance ref
    openOp->Reuse();
    openOp->StartOpenLog(
        KGuid(diskIdGuid),
        RvdLogId(KGuid(TestLogIdGuid)),
        log2,
        nullptr,
        doneCallback);

    doneEvent.WaitUntilSet();
    if (!NT_SUCCESS(openOp->Status()))
    {
        KDbgPrintf("BasicLogCreateTest: StartOpenLog failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return openOp->Status();
    }
    if (!log2)
    {
        KDbgPrintf("BasicLogCreateTest: StartOpenLog did not return expected SPtr: %i\n", __LINE__);
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    if (log1.RawPtr() != log2.RawPtr())
    {
        KDbgPrintf("BasicLogCreateTest: StartOpenLog did not return expected SPtr value: %i\n", __LINE__);
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    // Prove that a delete of an already open log file fails
    deleteOp->Reuse();
    deleteOp->StartDeleteLog(
        KGuid(diskIdGuid),
        RvdLogId(KGuid(TestLogIdGuid)),
        nullptr,
        doneCallback);

    doneEvent.WaitUntilSet();
    if (NT_SUCCESS(deleteOp->Status()))
    {
        KDbgPrintf("BasicLogCreateTest: StartDeleteLog succeeded - unexpected: %i\n", __LINE__);
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    // Prove create of a stream on an opened empty log works
    {
        RvdLog::AsyncCreateLogStreamContext::SPtr createStreamOp;
        static GUID const streamIdGuid1     = {0x7997d8f7, 0x58f9, 0x4968, {0x89, 0x4a, 0x2a, 0xe4, 0xd2, 0xc1, 0xe9, 0xb9}};
        static GUID const streamTypeIdGuid1  = {0x13aa67d6, 0x21cf, 0x4681, {0xb6, 0x73, 0x8f, 0x07, 0xb5, 0x19, 0xf4, 0x9a}};

        // Make stream related op contexts
        status = log1->CreateAsyncCreateLogStreamContext(createStreamOp);
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("BasicLogCreateTest: CreateAsyncCreateLogStreamContext Failed: %i\n", __LINE__);
            KInvariant(FALSE);
            return status;
        }

        //** Create a log stream
        RvdLogStream::SPtr      stream;
        createStreamOp->StartCreateLogStream(
            RvdLogStreamId(KGuid(streamIdGuid1)),
            RvdLogStreamType(KGuid(streamTypeIdGuid1)),
            stream,
            nullptr,
            doneCallback);

        doneEvent.WaitUntilSet();
        if (!NT_SUCCESS(createStreamOp->Status()))
        {
            KDbgPrintf("BasicLogCreateTest: StartCreateLogStream Failed: %i\n", __LINE__);
            KInvariant(FALSE);
            return createStreamOp->Status();
        }
    }

    //
    // Wait for objects to be destructed
    //
    KNt::Sleep(1000);

    // Test closure of the opened log2
    if (log2.Reset())
    {
        KDbgPrintf("BasicLogCreateTest: unexpected destruction occurred: %i\n", __LINE__);
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    // Test closure of the opened log1
    if (!log1.Reset())
    {
        KDbgPrintf("BasicLogCreateTest: expected destruction did not occur: %i\n", __LINE__);
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

#if ! defined(PLATFORM_UNIX)    
    // Prove that one (1) log file can be enum'd
    KArray<RvdLogId>    logIds(KtlSystem::GlobalNonPagedAllocator());
    enumOp->StartEnumerateLogs(
        KGuid(diskIdGuid),
        logIds,
        nullptr,
        doneCallback);

    doneEvent.WaitUntilSet();
    if (!NT_SUCCESS(enumOp->Status()))
    {
        KDbgPrintf("BasicLogCreateTest: StartEnumerateLogs failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return enumOp->Status();
    }

    if (logIds.Count() != 1)
    {
        KDbgPrintf("BasicLogCreateTest: StartEnumerateLogs returned wrong number of log file ids: %i\n", __LINE__);
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    if (logIds[0].Get() != TestLogIdGuid)
    {
        KDbgPrintf("BasicLogCreateTest: StartEnumerateLogs returned wrong log file id: %i\n", __LINE__);
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }
#endif    

    // Prove an existing Log file can be deleted
    deleteOp->Reuse();
    deleteOp->StartDeleteLog(
        KGuid(diskIdGuid),
        RvdLogId(KGuid(TestLogIdGuid)),
        nullptr,
        doneCallback);

    doneEvent.WaitUntilSet();
    if (!NT_SUCCESS(deleteOp->Status()))
    {
        KDbgPrintf("BasicLogCreateTest: StartDeleteLog failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    // Prove we can create, close, and open logs with different geometry
    createOp->Reuse();
    createOp->StartCreateLog(
        KGuid(diskIdGuid),
        RvdLogId(KGuid(TestLogIdGuid)),
        logType,
        DefaultTestLogFileSize,
        RvdLogConfig::DefaultMaxNumberOfStreams / 2,
        RvdLogConfig::DefaultMaxRecordSize / 2,
        log1,
        nullptr,
        doneCallback);

    doneEvent.WaitUntilSet();
    KInvariant(NT_SUCCESS(createOp->Status()));

    KInvariant(log1->QueryMaxAllowedStreams() == (RvdLogConfig::DefaultMaxNumberOfStreams / 2));
    KInvariant(log1->QueryMaxRecordSize() == (RvdLogConfig::DefaultMaxRecordSize / 2));

    // Test closure of the opened log1
    KInvariant(log1.Reset());

    openOp->Reuse();
    openOp->StartOpenLog(
        KGuid(diskIdGuid),
        RvdLogId(KGuid(TestLogIdGuid)),
        log1,
        nullptr,
        doneCallback);

    doneEvent.WaitUntilSet();
    KInvariant(NT_SUCCESS(openOp->Status()));

    KInvariant(log1->QueryMaxAllowedStreams() == (RvdLogConfig::DefaultMaxNumberOfStreams / 2));
    KInvariant(log1->QueryMaxRecordSize() == (RvdLogConfig::DefaultMaxRecordSize / 2));

    // Test closure of the opened log1
    KInvariant(log1.Reset());

    deleteOp->Reuse();
    deleteOp->StartDeleteLog(
        KGuid(diskIdGuid),
        RvdLogId(KGuid(TestLogIdGuid)),
        nullptr,
        doneCallback);

    doneEvent.WaitUntilSet();
    KInvariant(NT_SUCCESS(deleteOp->Status()));

    createOp->Reuse();
    createOp->StartCreateLog(
        KGuid(diskIdGuid),
        RvdLogId(KGuid(TestLogIdGuid)),
        logType,
        DefaultTestLogFileSize,
        RvdLogConfig::DefaultMaxNumberOfStreams * 2,
        RvdLogConfig::DefaultMaxRecordSize * 2,
        log1,
        nullptr,
        doneCallback);

    doneEvent.WaitUntilSet();
    KInvariant(NT_SUCCESS(createOp->Status()));

    KInvariant(log1->QueryMaxAllowedStreams() == (RvdLogConfig::DefaultMaxNumberOfStreams * 2));
    KInvariant(log1->QueryMaxRecordSize() == (RvdLogConfig::DefaultMaxRecordSize * 2));

    // Test closure of the opened log1
    KInvariant(log1.Reset());

    openOp->Reuse();
    openOp->StartOpenLog(
        KGuid(diskIdGuid),
        RvdLogId(KGuid(TestLogIdGuid)),
        log1,
        nullptr,
        doneCallback);

    doneEvent.WaitUntilSet();
    KInvariant(NT_SUCCESS(openOp->Status()));

    KInvariant(log1->QueryMaxAllowedStreams() == (RvdLogConfig::DefaultMaxNumberOfStreams * 2));
    KInvariant(log1->QueryMaxRecordSize() == (RvdLogConfig::DefaultMaxRecordSize * 2));

    // Test closure of the opened log1
    KInvariant(log1.Reset());

    deleteOp->Reuse();
    deleteOp->StartDeleteLog(
        KGuid(diskIdGuid),
        RvdLogId(KGuid(TestLogIdGuid)),
        nullptr,
        doneCallback);

    doneEvent.WaitUntilSet();
    KInvariant(NT_SUCCESS(deleteOp->Status()));

    logManager->Deactivate();
    activateEvent.WaitUntilSet();
    return STATUS_SUCCESS;
}

VOID
GenerateRandomRecord(ULONG seedFactor, TestMetadata& Metadata, TestIoBufferRecord& Record)
{
    ULONG seed;
    Metadata.DataRandomSeed = (seed = ((ULONG)KNt::GetTickCount64() + seedFactor));
    for (ULONG ix = 0; ix < TestIoBufferRecordArraySize; ix++)
    {
        Record.RecordItems[ix] = RtlRandomEx(&seed);
    }

    Metadata.DataChecksum = KChecksum::Crc64(&Record, TestIoBufferRecordArraySize * sizeof(ULONG), 0);
}

VOID
GenerateBigRandomRecord(ULONG seedFactor, TestMetadata& Metadata, TestIoBufferRecord& Record)
{
    ULONG seed;
    Metadata.DataRandomSeed = (seed = ((ULONG)KNt::GetTickCount64() + seedFactor));
    for (ULONG ix = 0; ix < (TestIoBufferRecordArraySize * 2); ix++)
    {
        Record.RecordItems[ix] = RtlRandomEx(&seed);
    }

    Metadata.DataChecksum = KChecksum::Crc64(&Record, TestIoBufferRecordArraySize * sizeof(ULONG) * 2, 0);
}

BOOLEAN
ValidateRandomRecord(TestMetadata& Metadata, KIoBuffer& Record)
{
    ULONGLONG           partialChecksum = 0;
    KIoBufferElement*   currentElement = Record.First();

    while (currentElement != nullptr)
    {
        partialChecksum = KChecksum::Crc64(currentElement->GetBuffer(), currentElement->QuerySize(), partialChecksum);
        currentElement = Record.Next(*currentElement);
    }

    return (partialChecksum == Metadata.DataChecksum);
}

const ULONG TestRecordSize = sizeof(TestIoBufferRecord) + RvdDiskLogConstants::BlockSize;
const ULONG BigTestRecordSize = (sizeof(TestIoBufferRecord) * 2) + RvdDiskLogConstants::BlockSize;

NTSTATUS
SimpleLogFullAndWrapTest(RvdLog::SPtr Log, RvdLogStream::SPtr Stream)
{
#ifdef _WIN64
    KEvent doneEvent(FALSE, FALSE);
    KAsyncContextBase::CompletionCallback doneCallback((PVOID)(&doneEvent), &StaticTestCallback);

    LONGLONG lowestLsnOfLog;
    LONGLONG HighestLsnPlusOneOfLog;
    LONGLONG LsnOfEOF = DefaultTestLogFileSize - (sizeof(RvdDiskLogConstants::BlockSize) * 2);

    ((RvdLogManagerImp::RvdOnDiskLog*)(Log.RawPtr()))->GetPhysicalExtent(lowestLsnOfLog, HighestLsnPlusOneOfLog);
    KInvariant(HighestLsnPlusOneOfLog <= LsnOfEOF);

    RvdLogStream::AsyncWriteContext::SPtr streamWriteOp;
    NTSTATUS status = Stream->CreateAsyncWriteContext(streamWriteOp);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("SimpleLogFullAndWrapTest: CreateAsyncWriteContext failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return status;
    }

    KBuffer::SPtr       metaData;
    status = KBuffer::Create(sizeof(TestMetadata), metaData, KtlSystem::GlobalNonPagedAllocator());
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("SimpleLogFullAndWrapTest: KBuffer::Create failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return status;
    }

    TestMetadata*       testMetadataPtr = (TestMetadata*)metaData->GetBuffer();
    KIoBuffer::SPtr     ioBuffer;
    TestIoBufferRecord* recordPtr;
    KIoBuffer::SPtr     bigIoBuffer;
    TestIoBufferRecord* bigRecordPtr;
    RvdLogAsn           asnLimit(1000);

    status = KIoBuffer::CreateSimple(sizeof(TestIoBufferRecord), ioBuffer, (void*&)recordPtr,
            KtlSystem::GlobalNonPagedAllocator());
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("SimpleLogFullAndWrapTest: KIoBuffer::CreateSimple failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return status;
    }

    status = KIoBuffer::CreateSimple(sizeof(TestIoBufferRecord) * 2, bigIoBuffer, (void*&)bigRecordPtr,
            KtlSystem::GlobalNonPagedAllocator());
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("SimpleLogFullAndWrapTest: KIoBuffer::CreateSimple failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return status;
    }

    RvdLogAsn lowestAsn;
    RvdLogAsn highestAsn;
    RvdLogAsn streamTruncationAsn;

    status = Stream->QueryRecordRange(&lowestAsn, &highestAsn, &streamTruncationAsn);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("SimpleLogFullAndWrapTest: QueryRecordRange failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return status;
    }

    //** First fill the log until we get an LOG FULL condition; checking each write to see if it wrapped;

    lowestAsn = highestAsn.Get() + 1;
    BOOLEAN     splitWriteOccured = FALSE;
    ULONGLONG    totalLogSpace;
    Log->QuerySpaceInformation(&totalLogSpace, nullptr);

    // Limit test to one log's worth of writes
    // BUG, richhas, xxxxxx, find a better way to guarentee a split write occurs
    while (totalLogSpace > 0)
    {
        ULONG totalSizeWritten = 0;
        ULONGLONG roomLeftTillEof = ((RvdLogManagerImp::RvdOnDiskLog*)(Log.RawPtr()))->RemainingLsnSpaceToEOF();
        if ((roomLeftTillEof > 0) && (roomLeftTillEof < (TestRecordSize * 2)))
        {
            totalSizeWritten = BigTestRecordSize;
            GenerateBigRandomRecord((ULONG)lowestAsn.Get(), *testMetadataPtr, *bigRecordPtr);
            streamWriteOp->StartWrite(lowestAsn, 0, metaData, bigIoBuffer, nullptr, doneCallback);
        }
        else
        {
            totalSizeWritten = TestRecordSize;
            GenerateRandomRecord((ULONG)lowestAsn.Get(), *testMetadataPtr, *recordPtr);
            streamWriteOp->StartWrite(lowestAsn, 0, metaData, ioBuffer, nullptr, doneCallback);
        }
        doneEvent.WaitUntilSet();
        status = streamWriteOp->Status();
        if (!NT_SUCCESS(status))
        {
            totalSizeWritten = 0;
            if (status == STATUS_LOG_FULL)
            {
                // Truncate the tail record of the stream to create space
                Stream->Truncate(highestAsn, highestAsn);
                while (!Log->IsLogFlushed())
                {
                    KNt::Sleep(0);
                }
                highestAsn = highestAsn.Get() + 2;
            }
            else
            {
                KDbgPrintf("SimpleLogFullAndWrapTest: StartWrite() failed: %i\n", __LINE__);
                KInvariant(FALSE);
                return status;
            }
        }
        else
        {
            if (((RvdLogStreamImp::AsyncWriteStream*)(streamWriteOp.RawPtr()))->IoWrappedEOF())
            {
                splitWriteOccured = TRUE;
                break;
            }

            lowestAsn = lowestAsn.Get() + 1;
        }
        streamWriteOp->Reuse();
        totalLogSpace -= totalSizeWritten;
    }

    if (!splitWriteOccured)
    {
        KDbgPrintf("SimpleLogFullAndWrapTest: Test failed to write a split record: %i\n", __LINE__);
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    RvdLogStream::AsyncReadContext::SPtr streamReadOp;
    status = Stream->CreateAsyncReadContext(streamReadOp);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("SimpleLogFullAndWrapTest: CreateAsyncReadContext failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return status;
    }

    // Make sure all truncates have flushed
    while (!Log->IsLogFlushed())
    {
        KNt::Sleep(0);
    }

    //** Read and verify all records written
    {
        RvdLogAsn           lowAsn;
        RvdLogAsn           topAsn;

        status = Stream->QueryRecordRange(&lowAsn, &topAsn, nullptr);
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("SimpleLogFullAndWrapTest: QueryRecordRange failed: %i\n", __LINE__);
            KInvariant(FALSE);
            return status;
        }

        while (lowAsn <= topAsn)
        {
            // Test read each record in the log
            KBuffer::SPtr       readMetaData;
            KIoBuffer::SPtr     readIoBuffer;
            ULONGLONG           versionRead;

            streamReadOp->StartRead(lowAsn, &versionRead, readMetaData, readIoBuffer, nullptr, doneCallback);
            doneEvent.WaitUntilSet();
            if (!NT_SUCCESS(streamReadOp->Status()))
            {
                KDbgPrintf("SimpleLogFullAndWrapTest: StartRead returned unexpected status: %i\n", __LINE__);
                KInvariant(FALSE);
                return streamReadOp->Status();
            }
            streamReadOp->Reuse();

            if (!ValidateRandomRecord(
                (TestMetadata&)*((UCHAR*)readMetaData->GetBuffer()),
                *readIoBuffer.RawPtr()))
            {
                KDbgPrintf("SimpleLogFullAndWrapTest: ValidateRandomRecord failed: %i\n", __LINE__);
                KInvariant(FALSE);
                return STATUS_UNSUCCESSFUL;
            }

            lowAsn = lowAsn.Get() + 1;
        }
    }
#else
    UNREFERENCED_PARAMETER(Log);
    UNREFERENCED_PARAMETER(Stream);
#endif

    return STATUS_SUCCESS;
}


NTSTATUS
BasicLogStreamTest(
    WCHAR const TestDriveLetter,
    BOOLEAN DoFinalStreamDeleteTest,
    LogState::SPtr *const ResultingState)
{
#ifdef _WIN64
    if (ResultingState != nullptr)
    {
        *ResultingState = nullptr;
    }

    KInvariant((TestDriveLetter < 256) && (TestDriveLetter >= 0));

    NTSTATUS status;

    // Clean the unit test drive environment
    status = CleanAndPrepLog(TestDriveLetter);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("BasicLogStreamTest: CleanAndPrepLog Failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return status;
    }

    RvdLogManager::SPtr logManager;
    KEvent activateEvent(FALSE, FALSE);

    status = RvdLogManager::Create(KTL_TAG_LOGGER, KtlSystem::GlobalNonPagedAllocator(), logManager);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("BasicLogStreamTest: RvdLogManager::Create Failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return status;
    }

    KAsyncContextBase::CompletionCallback activateDoneCallback((PVOID)(&activateEvent), &StaticTestCallback);

    status = logManager->Activate(nullptr, activateDoneCallback);
    if (!K_ASYNC_SUCCESS(status))
    {
        KDbgPrintf("BasicLogStreamTest: Activate() Failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return status;
    }

    RvdLogManager::AsyncCreateLog::SPtr createOp;
    status = logManager->CreateAsyncCreateLogContext(createOp);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("BasicLogStreamTest: CreateAsyncCreateLogContext Failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return status;
    }

    GUID diskIdGuid = {};

#if !defined(PLATFORM_UNIX)
    KEvent getVolInfoCompleteEvent(FALSE, FALSE);
    KAsyncContextBase::CompletionCallback getVolInfoDoneCallback;
    getVolInfoDoneCallback.Bind((PVOID)(&getVolInfoCompleteEvent), &StaticTestCallback);

    KVolumeNamespace::VolumeInformationArray volInfo(KtlSystem::GlobalNonPagedAllocator());
    status = KVolumeNamespace::QueryVolumeListEx(volInfo, KtlSystem::GlobalNonPagedAllocator(), getVolInfoDoneCallback);
    if (!K_ASYNC_SUCCESS(status))
    {
        KDbgPrintf("BasicLogStreamTest: KVolumeNamespace::QueryVolumeListEx Failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return status;
    }
    getVolInfoCompleteEvent.WaitUntilSet();
    if (!NT_SUCCESS(LastStaticTestCallbackStatus))
    {
        KDbgPrintf("BasicLogStreamTest: KVolumeNamespace::QueryVolumeListEx Failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return LastStaticTestCallbackStatus;
    }

    // Find Drive C: volume GUID
    ULONG i;
    for (i = 0; i < volInfo.Count(); i++)
    {
        if (volInfo[i].DriveLetter == (UCHAR)TestDriveLetter)
        {
            break;
        }
    }

    if (i == volInfo.Count())
    {
        KDbgPrintf("BasicLogStreamTest: KVolumeNamespace::QueryVolumeListEx did not return volume guid for drive C: %i\n", __LINE__);
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }
    diskIdGuid = volInfo[i].VolumeId;
#else
    diskIdGuid = RvdDiskLogConstants::HardCodedVolumeGuid();
#endif
    
    RvdLog::SPtr log1;
    KEvent doneEvent(FALSE, FALSE);
    KAsyncContextBase::CompletionCallback doneCallback((PVOID)(&doneEvent), &StaticTestCallback);

    KEvent deleteStreamDoneEvent(FALSE, FALSE);
    KAsyncContextBase::CompletionCallback streamDeleteDoneCallback((PVOID)(&deleteStreamDoneEvent), &StaticTestCallback);

    // Create a log file; with altered geometry
    KWString logType(KtlSystem::GlobalNonPagedAllocator(), L"RvdLog");
    KInvariant(NT_SUCCESS(logType.Status()));

    createOp->StartCreateLog(
        KGuid(diskIdGuid),
        RvdLogId(KGuid(TestLogIdGuid)),
        logType,
        DefaultTestLogFileSize,
        RvdLogConfig::DefaultMaxNumberOfStreams / 2,
        RvdLogConfig::DefaultMaxRecordSize / 2,
        log1,
        nullptr,
        doneCallback);

    doneEvent.WaitUntilSet();
    if (!NT_SUCCESS(createOp->Status()))
    {
        KDbgPrintf("BasicLogStreamTest: StartCreateLog Failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return createOp->Status();
    }

    KInvariant(log1->QueryMaxAllowedStreams() == (RvdLogConfig::DefaultMaxNumberOfStreams / 2));
    KInvariant(log1->QueryMaxRecordSize() == (RvdLogConfig::DefaultMaxRecordSize / 2));

    RvdLog::AsyncCreateLogStreamContext::SPtr createStreamOp;
    RvdLog::AsyncOpenLogStreamContext::SPtr openStreamOp1;
    RvdLog::AsyncOpenLogStreamContext::SPtr openStreamOp2;
    RvdLog::AsyncDeleteLogStreamContext::SPtr deleteStreamOp;
    static GUID const streamIdGuid1     = {0x7997d8f7, 0x58f9, 0x4968, {0x89, 0x4a, 0x2a, 0xe4, 0xd2, 0xc1, 0xe9, 0xb9}};
    static GUID const streamTypeIdGuid1  = {0x13aa67d6, 0x21cf, 0x4681, {0xb6, 0x73, 0x8f, 0x07, 0xb5, 0x19, 0xf4, 0x9a}};
    BOOLEAN streamClosed;
    BOOLEAN streamOpened;
    BOOLEAN streamDeleted;

    // Make stream related op contexts
    status = log1->CreateAsyncCreateLogStreamContext(createStreamOp);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("BasicLogStreamTest: CreateAsyncCreateLogStreamContext Failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return status;
    }

    status = log1->CreateAsyncOpenLogStreamContext(openStreamOp1);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("BasicLogStreamTest: CreateAsyncOpenLogStreamContext Failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return status;
    }

    status = log1->CreateAsyncOpenLogStreamContext(openStreamOp2);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("BasicLogStreamTest: CreateAsyncOpenLogStreamContext Failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return status;
    }

    status = log1->CreateAsyncDeleteLogStreamContext(deleteStreamOp);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("BasicLogStreamTest: CreateAsyncOpenLogStreamContext Failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return status;
    }

    RvdLogStream::SPtr stream1;

    //** Prove we can't open a non-existing stream
    openStreamOp1->StartOpenLogStream(
        RvdLogStreamId(KGuid(streamIdGuid1)),
        stream1,
        nullptr,
        doneCallback);

    doneEvent.WaitUntilSet();
    if (NT_SUCCESS(openStreamOp1->Status()))
    {
        KDbgPrintf("BasicLogStreamTest: StartOpenLogStream unexpected success: %i\n", __LINE__);
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }
    if (stream1)
    {
        KDbgPrintf("BasicLogStreamTest: StartOpenLogStream unexpected return of SPtr: %i\n", __LINE__);
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    //** Prove we can't delete a non-existing stream
    deleteStreamOp->StartDeleteLogStream(
        RvdLogStreamId(KGuid(streamIdGuid1)),
        nullptr,
        streamDeleteDoneCallback);

    deleteStreamDoneEvent.WaitUntilSet();
    if (NT_SUCCESS(deleteStreamOp->Status()))
    {
        KDbgPrintf("BasicLogStreamTest: StartDeleteLogStream unexpected success: %i\n", __LINE__);
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    //** Create a log stream
    createStreamOp->StartCreateLogStream(
        RvdLogStreamId(KGuid(streamIdGuid1)),
        RvdLogStreamType(KGuid(streamTypeIdGuid1)),
        stream1,
        nullptr,
        doneCallback);

    doneEvent.WaitUntilSet();
    if (!NT_SUCCESS(createStreamOp->Status()))
    {
        KDbgPrintf("BasicLogStreamTest: StartCreateLogStream Failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return createStreamOp->Status();
    }

    RvdLogStream::SPtr stream2;

    //** Prove a second create fails
    createStreamOp->Reuse();
    createStreamOp->StartCreateLogStream(
        RvdLogStreamId(KGuid(streamIdGuid1)),
        RvdLogStreamType(KGuid(streamTypeIdGuid1)),
        stream2,
        nullptr,
        doneCallback);

    doneEvent.WaitUntilSet();
    if (NT_SUCCESS(createStreamOp->Status()))
    {
        KDbgPrintf("BasicLogStreamTest: StartCreateLogStream unexpected success: %i\n", __LINE__);
        KInvariant(FALSE);
        return createStreamOp->Status();
    }
    if (stream2)
    {
        KDbgPrintf("BasicLogStreamTest: StartCreateLogStream unexpected return of SPtr: %i\n", __LINE__);
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    //** Prove a secondary open on an already opened stream succeeds
    openStreamOp1->Reuse();
    openStreamOp1->StartOpenLogStream(
        RvdLogStreamId(KGuid(streamIdGuid1)),
        stream2,
        nullptr,
        doneCallback);

    doneEvent.WaitUntilSet();
    if (!NT_SUCCESS(openStreamOp1->Status()))
    {
        KDbgPrintf("BasicLogStreamTest: StartOpenLogStream failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return openStreamOp1->Status();
    }
    if (stream1.RawPtr() != stream2.RawPtr())
    {
        KDbgPrintf("BasicLogStreamTest: StartOpenLogStream returned mismatched stream SPtrs: %i\n", __LINE__);
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    //** Prove we can close access to the opened (created) log stream
    //** and that it still exists as a non-open stream

    //
    // give a chance for async to finish
    //
    KNt::Sleep(200);

    // Test closure of  stream1
    if (stream1.Reset())
    {
        KDbgPrintf("BasicLogCreateTest: unexpected destruction occurred: %i\n", __LINE__);
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    // Test closure of  stream2
    if (!stream2.Reset())
    {
        KDbgPrintf("BasicLogCreateTest: expected destruction did not occur: %i\n", __LINE__);
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    if (!log1->IsStreamIdValid(RvdLogStreamId(KGuid(streamIdGuid1))))
    {
        KDbgPrintf("BasicLogCreateTest: IsStreamIdValid failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    if (!log1->GetStreamState(RvdLogStreamId(KGuid(streamIdGuid1)), &streamOpened, &streamClosed, &streamDeleted))
    {
        KDbgPrintf("BasicLogCreateTest: GetStreamState failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    if (!(!streamOpened && streamClosed && !streamDeleted))
    {
        KDbgPrintf("BasicLogCreateTest: GetStreamState failed with unexpected returned state: %i\n", __LINE__);
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    //** Prove a delete on an already opened stream occurs when
    //   interest in that stream has gone away - delayed delete case
    deleteStreamOp->Reuse();
    deleteStreamOp->StartDeleteLogStream(
        RvdLogStreamId(KGuid(streamIdGuid1)),
        nullptr,
        streamDeleteDoneCallback);

    deleteStreamDoneEvent.WaitUntilSet();
    if (!NT_SUCCESS(deleteStreamOp->Status()))
    {
        KDbgPrintf("BasicLogStreamTest: StartDeleteLogStream completion reported a failure: %i\n", __LINE__);
        KInvariant(FALSE);
        return deleteStreamOp->Status();
    }

    if (log1->GetStreamState(RvdLogStreamId(KGuid(streamIdGuid1)), &streamOpened, &streamClosed, &streamDeleted))
    {
        if (!(!streamOpened && !streamClosed && streamDeleted))
        {
            KDbgPrintf("BasicLogCreateTest: GetStreamState failed with unexpected returned state: %i\n", __LINE__);
            KInvariant(FALSE);
            return STATUS_UNSUCCESSFUL;
        }
    }

    //** Prove that a create on a non-opened but existing stream fails

    // First make a stream
    createStreamOp->Reuse();
    createStreamOp->StartCreateLogStream(
        RvdLogStreamId(KGuid(streamIdGuid1)),
        RvdLogStreamType(KGuid(streamTypeIdGuid1)),
        stream1,
        nullptr,
        doneCallback);

    doneEvent.WaitUntilSet();
    if (!NT_SUCCESS(createStreamOp->Status()))
    {
        KDbgPrintf("BasicLogStreamTest: StartCreateLogStream failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return createStreamOp->Status();
    }

    //
    // give a chance for async to finish
    //
    KNt::Sleep(200);
    if (!stream1.Reset())           // Close it - but it still exists
    {
        KDbgPrintf("BasicLogCreateTest: expected destruction did not occur: %i\n", __LINE__);
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    createStreamOp->Reuse();
    createStreamOp->StartCreateLogStream(
        RvdLogStreamId(KGuid(streamIdGuid1)),
        RvdLogStreamType(KGuid(streamTypeIdGuid1)),
        stream2,
        nullptr,
        doneCallback);

    doneEvent.WaitUntilSet();
    if (NT_SUCCESS(createStreamOp->Status()))
    {
        KDbgPrintf("BasicLogStreamTest: StartCreateLogStream unexpected success: %i\n", __LINE__);
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }
    if (stream2)
    {
        KDbgPrintf("BasicLogStreamTest: StartCreateLogStream unexpected return of SPtr: %i\n", __LINE__);
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    //** Prove that an open of a non-opened but existing stream works
    openStreamOp1->Reuse();
    openStreamOp1->StartOpenLogStream(
        RvdLogStreamId(KGuid(streamIdGuid1)),
        stream2,
        nullptr,
        doneCallback);

    doneEvent.WaitUntilSet();
    if (!NT_SUCCESS(openStreamOp1->Status()))
    {
        KDbgPrintf("BasicLogStreamTest: StartOpenLogStream failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return openStreamOp1->Status();
    }

    if (!log1->GetStreamState(RvdLogStreamId(KGuid(streamIdGuid1)), &streamOpened, &streamClosed, &streamDeleted))
    {
        KDbgPrintf("BasicLogCreateTest: GetStreamState failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    if (!(streamOpened && !streamClosed && !streamDeleted))
    {
        KDbgPrintf("BasicLogCreateTest: GetStreamState failed with unexpected returned state: %i\n", __LINE__);
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    //** Prove simple stream write - but out of (ASN) order
    RvdLogStream::AsyncWriteContext::SPtr streamWriteOp;

    status = stream2->CreateAsyncWriteContext(streamWriteOp);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("BasicLogCreateTest: CreateAsyncWriteContext failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return status;
    }

    KBuffer::SPtr       metaData;
    status = KBuffer::Create(sizeof(TestMetadata), metaData, KtlSystem::GlobalNonPagedAllocator());
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("BasicLogCreateTest: KBuffer::Create failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return status;
    }

    TestMetadata*       testMetadataPtr = (TestMetadata*)metaData->GetBuffer();
    KIoBuffer::SPtr     ioBuffer;
    TestIoBufferRecord* recordPtr;
    RvdLogAsn           currentAsn = RvdLogAsn::Min();
    RvdLogAsn           asnLimit(1000);

    status = KIoBuffer::CreateSimple(sizeof(TestIoBufferRecord), ioBuffer, (void*&)recordPtr,
            KtlSystem::GlobalNonPagedAllocator());
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("BasicLogCreateTest: KIoBuffer::CreateSimple failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return status;
    }

    ULONGLONG            totalAvailSpace;
    ULONGLONG            initialFreeSpace;
    log1->QuerySpaceInformation(&totalAvailSpace, &initialFreeSpace);

    KDbgPrintf("BasicLogCreateTest: KIoBuffer size: %d\n", ioBuffer->QuerySize());
    ULONG               totalTestRecordSize =  ioBuffer->QuerySize() +
                                               (((metaData->QuerySize() + RvdDiskLogConstants::BlockSize - 1)
                                                    / RvdDiskLogConstants::BlockSize)
                                                * RvdDiskLogConstants::BlockSize);

    // Prove QueryRecordRange indicates an empty and never written stream
    RvdLogAsn lowestAsn;
    RvdLogAsn highestAsn;
    RvdLogAsn streamTruncationAsn;

    status = stream2->QueryRecordRange(&lowestAsn, &highestAsn, &streamTruncationAsn);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("BasicLogCreateTest: QueryRecordRange failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return status;
    }

    if (!lowestAsn.IsNull() || (highestAsn != lowestAsn) || (streamTruncationAsn > lowestAsn))
    {
        KDbgPrintf("BasicLogCreateTest: Range returned from QueryRecordRange is incorrect: %i\n", __LINE__);
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    while (currentAsn <= asnLimit)
    {
        // Cause out of order writes
        if (currentAsn < asnLimit)
        {
            GenerateRandomRecord((ULONG)currentAsn.Get() + 1, *testMetadataPtr, *recordPtr);
            streamWriteOp->StartWrite(RvdLogAsn(currentAsn.Get() + 1), 0, metaData, ioBuffer, nullptr, doneCallback);
            doneEvent.WaitUntilSet();
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("BasicLogCreateTest:StartWrite() failed: %i\n", __LINE__);
                KInvariant(FALSE);
                return status;
            }
        }
        streamWriteOp->Reuse();

        GenerateRandomRecord((ULONG)currentAsn.Get(), *testMetadataPtr, *recordPtr);
        streamWriteOp->StartWrite(currentAsn, 0, metaData, ioBuffer, nullptr, doneCallback);
        doneEvent.WaitUntilSet();
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("BasicLogCreateTest:StartWrite() failed: %i\n", __LINE__);
            KInvariant(FALSE);
            return status;
        }
        streamWriteOp->Reuse();

        currentAsn = currentAsn.Get() + 2;
    }

    // Prove written range is correct
    status = stream2->QueryRecordRange(&lowestAsn, &highestAsn, &streamTruncationAsn);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("BasicLogCreateTest: QueryRecordRange failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return status;
    }

    if ((lowestAsn != RvdLogAsn::Min()) || (highestAsn != asnLimit) || (streamTruncationAsn >= lowestAsn))
    {
        KDbgPrintf("BasicLogCreateTest: Range returned from QueryRecordRange is incorrect: %i\n", __LINE__);
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    // Prove that the QueryRecords API works
    {
        KArray<RvdLogStream::RecordMetadata> resultsArray(KtlSystem::GlobalNonPagedAllocator());

        status = stream2->QueryRecords(RvdLogAsn::Null(), RvdLogAsn::Max(), resultsArray);
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("BasicLogCreateTest: QueryRecords() failed: %i\n", __LINE__);
            KInvariant(FALSE);
            return status;
        }

        if ((resultsArray[0].Asn != lowestAsn) || (resultsArray[resultsArray.Count() - 1].Asn != highestAsn))
        {
            KDbgPrintf("BasicLogCreateTest: QueryRecords() failed to returned expected range: %i\n", __LINE__);
            KInvariant(FALSE);
            return STATUS_UNSUCCESSFUL;
        }

        // Validate order
        RvdLogAsn   nextAsn(lowestAsn);
        for (ULONG ix = 0; ix < resultsArray.Count(); ix++)
        {
            if (resultsArray[ix].Asn != nextAsn)
            {
                KDbgPrintf("BasicLogCreateTest: QueryRecords() failed to returned expected order: %i\n", __LINE__);
                KInvariant(FALSE);
                return STATUS_UNSUCCESSFUL;
            }

            nextAsn = nextAsn.Get() + 1;
        }
    }

    // Prove each record was recorded (on disk)
    RvdLogStream::AsyncReadContext::SPtr streamReadOp;

    status = stream2->CreateAsyncReadContext(streamReadOp);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("BasicLogCreateTest: CreateAsyncReadContext failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return status;
    }

    currentAsn = lowestAsn;
    LONGLONG  lastLsn = RvdLogLsn::Null().Get();
    while (currentAsn <= highestAsn)
    {
        ULONGLONG v;
        RvdLogStream::RecordDisposition d;
        ULONG ioSize;
        LONGLONG lsn;

        status = stream2->QueryRecord(currentAsn, &v, &d, &ioSize, (ULONGLONG*)&lsn);
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("BasicLogCreateTest: QueryRecord failed: %i\n", __LINE__);
            KInvariant(FALSE);
            return status;
        }

        if ((currentAsn.Get() == 999) || (currentAsn.Get() == 1000))
        {
            KDbgPrintf("BasicLogCreateTest: Asn: %I64d is @ LSN: %I64d\n", currentAsn.Get(), lsn);
        }

        if (lastLsn != RvdLogLsn::Null().Get())
        {
            if (((currentAsn.Get() - RvdLogAsn::Min().Get()) & 1) == 0)
            {
                // Even ASNs were written after their odd pairs - meaning their LSN should be > their odd pair
                if (lsn < lastLsn)
                {
                    KDbgPrintf("BasicLogCreateTest: Incorrect LSN returned from QueryRecord: %i\n", __LINE__);
                    KInvariant(FALSE);
                    return STATUS_UNSUCCESSFUL;
                }
            }
            else
            {
                if (lsn > lastLsn)
                {
                    KDbgPrintf("BasicLogCreateTest: Incorrect LSN returned from QueryRecord: %i\n", __LINE__);
                    KInvariant(FALSE);
                    return STATUS_UNSUCCESSFUL;
                }
            }
        }

        if ((v != 0) ||
            (d != RvdLogStream::RecordDisposition::eDispositionPersisted) ||
            (ioSize != totalTestRecordSize))
        {
            KDbgPrintf("BasicLogCreateTest: Incorrect Value(s) returned from QueryRecord: %i\n", __LINE__);
            KInvariant(FALSE);
            return STATUS_UNSUCCESSFUL;
        }

        // Test read each record in the log
        KBuffer::SPtr       readMetaData;
        KIoBuffer::SPtr     readIoBuffer;
        ULONGLONG           versionRead;

        streamReadOp->StartRead(RvdLogAsn::Min(), &versionRead, readMetaData, readIoBuffer, nullptr, doneCallback);
        doneEvent.WaitUntilSet();
        if (!NT_SUCCESS(streamReadOp->Status()))
        {
            KDbgPrintf("BasicLogStreamTest: StartRead returned unexpected status: %i\n", __LINE__);
            KInvariant(FALSE);
            return streamReadOp->Status();
        }
        streamReadOp->Reuse();

        if ((readMetaData->QuerySize() < sizeof(TestMetadata))
            || (readIoBuffer->QuerySize() < sizeof(TestIoBufferRecord)
            || (readIoBuffer->QueryNumberOfIoBufferElements() != 1)))
        {
            KDbgPrintf("BasicLogStreamTest: StartRead returned unexpected buffer configuration: %i\n", __LINE__);
            KInvariant(FALSE);
            return STATUS_UNSUCCESSFUL;
        }

        if (!ValidateRandomRecord(
            (TestMetadata&)*((UCHAR*)readMetaData->GetBuffer()),
            *readIoBuffer.RawPtr()))
        {
            KDbgPrintf("BasicLogStreamTest: ValidateRandomRecord failed: %i\n", __LINE__);
            KInvariant(FALSE);
            return STATUS_UNSUCCESSFUL;
        }

        lastLsn = lsn;
        currentAsn = currentAsn.Get() + 1;
    }

    //** Prove Truncate behavior
    for (ULONGLONG asnValue = lowestAsn.Get(); asnValue <= highestAsn.Get(); asnValue++)
    {
        RvdLogAsn asn(asnValue);
        stream2->Truncate(asn, asn);
        while (!log1->IsLogFlushed())
        {
            KNt::Sleep(0);
        }
    }

    status = stream2->QueryRecordRange(&lowestAsn, &highestAsn, &streamTruncationAsn);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("BasicLogCreateTest: QueryRecordRange failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return status;
    }

    if ((lowestAsn != highestAsn) || (lowestAsn != streamTruncationAsn))
    {
        KDbgPrintf("BasicLogCreateTest: unexpected QueryRecordRange values after truncation test: %i\n", __LINE__);
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    ULONGLONG spaceAfterTruncations;
    log1->QuerySpaceInformation(nullptr, &spaceAfterTruncations);
    if (spaceAfterTruncations != (totalAvailSpace - RvdLogPhysicalCheckpointRecord::ComputeSizeOnDisk(1)))
    {
        KDbgPrintf("BasicLogCreateTest: unexpected QuerySpaceInformation results after truncation test: %i\n", __LINE__);
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    // Write a test record into the log to test the read below
    // Also proves that a record can be written AT the current truncation point
    RvdLogAsn testReadRecordAsn(highestAsn.Get() + 1);
    streamWriteOp->StartWrite(testReadRecordAsn, 0, metaData, ioBuffer, nullptr, doneCallback);
    doneEvent.WaitUntilSet();
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("BasicLogCreateTest:StartWrite() failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return status;
    }
    streamWriteOp->Reuse();

    RvdLogStream::RecordDisposition rd;
    status = stream2->QueryRecord(testReadRecordAsn, nullptr, &rd);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("BasicLogCreateTest:QueryRecord() failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return status;
    }
    if (rd != RvdLogStream::RecordDisposition::eDispositionPersisted)
    {
        KDbgPrintf("BasicLogCreateTest:QueryRecord() returned unexpected results: %i\n", __LINE__);
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    //
    // give a chance for async to finish
    //
    KNt::Sleep(200);

    // Prove streamWriteOp destructs
    if (!streamWriteOp.Reset())
    {
        KDbgPrintf("BasicLogCreateTest: expected destruction did not occur: %i\n", __LINE__);
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    //** Prove that we get a not-implemented error from a stream read
    KBuffer::SPtr       readMetaData;
    KIoBuffer::SPtr     readIoBuffer;
    ULONGLONG           versionRead;

    streamReadOp->StartRead(RvdLogAsn::Min(), &versionRead, readMetaData, readIoBuffer, nullptr, doneCallback);
    doneEvent.WaitUntilSet();
    if (streamReadOp->Status() != STATUS_NOT_FOUND)
    {
        KDbgPrintf("BasicLogStreamTest: StartRead returned unexpected status: %i\n", __LINE__);
        KInvariant(FALSE);
        return streamReadOp->Status();
    }

    streamReadOp->Reuse();
    streamReadOp->StartRead(testReadRecordAsn, &versionRead, readMetaData, readIoBuffer, nullptr, doneCallback);
    doneEvent.WaitUntilSet();
    if (!NT_SUCCESS(streamReadOp->Status()))
    {
        KDbgPrintf("BasicLogStreamTest: StartRead returned unexpected status: %i\n", __LINE__);
        KInvariant(FALSE);
        return streamReadOp->Status();
    }

    status = SimpleLogFullAndWrapTest(log1, stream2);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("BasicLogStreamTest: LogFullAndWrapTest returned unexpected status: %i\n", __LINE__);
        KInvariant(FALSE);
        return status;
    }

    if (ResultingState != nullptr)
    {
        while (!log1->IsLogFlushed())
        {
            KNt::Sleep(0);
        }

        RvdLogManagerImp::RvdOnDiskLog* logImpPtr = (RvdLogManagerImp::RvdOnDiskLog*)log1.RawPtr();
        status = logImpPtr->UnsafeSnapLogState(*ResultingState, KTL_TAG_LOGGER, KtlSystem::GlobalNonPagedAllocator());
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("BasicLogCreateTest: UnsafeSnapLogState failed: %i\n", __LINE__);
            KInvariant(FALSE);
            return status;
        }
    }

    if (DoFinalStreamDeleteTest)
    {
        //** Prove that a delete on an "open" stream is delayed until after any existing interest in that
        //   stream is removed...

        // First prove the stream is valid and in an opened state
        if (!log1->GetStreamState(RvdLogStreamId(KGuid(streamIdGuid1)), &streamOpened, &streamClosed, &streamDeleted))
        {
            KDbgPrintf("BasicLogCreateTest: GetStreamState failed: %i\n", __LINE__);
            KInvariant(FALSE);
            return STATUS_UNSUCCESSFUL;
        }

        if (!(streamOpened && !streamClosed && !streamDeleted))     // must show opened
        {
            KDbgPrintf("BasicLogCreateTest: GetStreamState failed with unexpected returned state: %i\n", __LINE__);
            KInvariant(FALSE);
            return STATUS_UNSUCCESSFUL;
        }

        // Next issue the delete - and wait some time for for the op to be started
        deleteStreamOp->Reuse();
        deleteStreamOp->StartDeleteLogStream(
            RvdLogStreamId(KGuid(streamIdGuid1)),
            nullptr,
            streamDeleteDoneCallback);

        KNt::Sleep(1000);
        if (!log1->GetStreamState(RvdLogStreamId(KGuid(streamIdGuid1)), &streamOpened, &streamClosed, &streamDeleted))
        {
            KDbgPrintf("BasicLogCreateTest: GetStreamState failed: %i\n", __LINE__);
            KInvariant(FALSE);
            return STATUS_UNSUCCESSFUL;
        }

        if (!(!streamOpened && !streamClosed && streamDeleted))     // must show deleted
        {
            KDbgPrintf("BasicLogCreateTest: GetStreamState failed with unexpected returned state: %i\n", __LINE__);
            KInvariant(FALSE);
            return STATUS_UNSUCCESSFUL;
        }

        // Prove streamReadOp destructs
        if (!streamReadOp.Reset())
        {
            KDbgPrintf("BasicLogCreateTest: expected destruction did not occur: %i\n", __LINE__);
            KInvariant(FALSE);
            return STATUS_UNSUCCESSFUL;
        }

        // Test closure of stream2 - should cause last interest in stream2 to drop and the
        // started delete to be done - stream should not exists
        if (!stream2.Reset())
        {
            KDbgPrintf("BasicLogCreateTest: expected destruction did not occur: %i\n", __LINE__);
            KInvariant(FALSE);
            return STATUS_UNSUCCESSFUL;
        }
        if (log1->GetStreamState(RvdLogStreamId(KGuid(streamIdGuid1)), &streamOpened, &streamClosed, &streamDeleted))
        {
            KDbgPrintf("BasicLogCreateTest: GetStreamState success unexpected: %i\n", __LINE__);
            KInvariant(FALSE);
            return STATUS_UNSUCCESSFUL;
        }

        // Wait for completion of the delayed delete
        deleteStreamDoneEvent.WaitUntilSet();
        if (!NT_SUCCESS(deleteStreamOp->Status()))
        {
            KDbgPrintf("BasicLogStreamTest: StartDeleteLogStream completion reported a failure: %i\n", __LINE__);
            KInvariant(FALSE);
            return deleteStreamOp->Status();
        }

        //** Prove we can delete an existing but not opened stream
        // First make a stream
        createStreamOp->Reuse();
        createStreamOp->StartCreateLogStream(
            RvdLogStreamId(KGuid(streamIdGuid1)),
            RvdLogStreamType(KGuid(streamTypeIdGuid1)),
            stream1,
            nullptr,
            doneCallback);

        doneEvent.WaitUntilSet();
        if (!NT_SUCCESS(createStreamOp->Status()))
        {
            KDbgPrintf("BasicLogStreamTest: StartCreateLogStream failed: %i\n", __LINE__);
			KInvariant(FALSE);
            return createStreamOp->Status();
        }

        //
        // give a chance for async to finish
        //
        KNt::Sleep(200);
        if (!stream1.Reset())           // Close it - but it still exists
        {
            KDbgPrintf("BasicLogCreateTest: expected destruction did not occur: %i\n", __LINE__);
			KInvariant(FALSE);
            return STATUS_UNSUCCESSFUL;
        }
        if (!log1->IsStreamIdValid(RvdLogStreamId(KGuid(streamIdGuid1))))
        {
            KDbgPrintf("BasicLogCreateTest: GetStreamState failed: %i\n", __LINE__);
			KInvariant(FALSE);
            return STATUS_UNSUCCESSFUL;
        }

        deleteStreamOp->Reuse();
        deleteStreamOp->StartDeleteLogStream(
            RvdLogStreamId(KGuid(streamIdGuid1)),
            nullptr,
            doneCallback);

        doneEvent.WaitUntilSet();
        status = deleteStreamOp->Status();
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("BasicLogStreamTest: StartDeleteLogStream failed: %i\n", __LINE__);
			KInvariant(FALSE);
            return status;
        }

        if (log1->IsStreamIdValid(RvdLogStreamId(KGuid(streamIdGuid1))))
        {
            KDbgPrintf("BasicLogCreateTest: IsStreamIdValid succeeded - unexpected: %i\n", __LINE__);
			KInvariant(FALSE);
            return STATUS_UNSUCCESSFUL;
        }
    }
    else
    {
        //
        // give a chance for async to finish
        //
        KNt::Sleep(200);
        // Prove streamReadOp destructs
        if (!streamReadOp.Reset())
        {
            KDbgPrintf("BasicLogCreateTest: expected destruction did not occur: %i\n", __LINE__);
			KInvariant(FALSE);
            return STATUS_UNSUCCESSFUL;
        }

        // Test closure of stream2 - should cause last interest in stream2 to drop
        if (!stream2.Reset())
        {
            KDbgPrintf("BasicLogCreateTest: expected destruction did not occur: %i\n", __LINE__);
			KInvariant(FALSE);
            return STATUS_UNSUCCESSFUL;
        }
    }

    //
    // Give time for the asyncs to finish and free their refs
    //
    KNt::Sleep(500);

    // Test closure of the created async op contexts
    if (!createStreamOp.Reset())
    {
        KDbgPrintf("BasicLogCreateTest: expected destruction did not occur: %i\n", __LINE__);
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    if (!openStreamOp1.Reset())
    {
        KDbgPrintf("BasicLogCreateTest: expected destruction did not occur: %i\n", __LINE__);
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    if (!openStreamOp2.Reset())
    {
        KDbgPrintf("BasicLogCreateTest: expected destruction did not occur: %i\n", __LINE__);
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    if (!deleteStreamOp.Reset())
    {
        KDbgPrintf("BasicLogCreateTest: expected destruction did not occur: %i\n", __LINE__);
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    // Test closure of the created log1
    if (!log1.Reset())
    {
        KDbgPrintf("BasicLogCreateTest: expected destruction did not occur: %i\n", __LINE__);
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    logManager->Deactivate();
    activateEvent.WaitUntilSet();
#else
    UNREFERENCED_PARAMETER(TestDriveLetter);
    UNREFERENCED_PARAMETER(DoFinalStreamDeleteTest);
    UNREFERENCED_PARAMETER(ResultingState);
#endif
    return STATUS_SUCCESS;
}


//** SimpleParallelStreamTest
namespace SimpleParallelStreamTest
{
    struct TestContext
    {
        NTSTATUS            CompletionStatus;
        KEvent              CompletionEvent;
        RvdLogStream::SPtr  LogStream;
        RvdLog::SPtr        Log;
        KGuid               StreamGuid;
        KGuid               StreamType;

        TestContext(RvdLog::SPtr Log, KGuid StreamGuid, KGuid StreamType)
            :   CompletionStatus(STATUS_SUCCESS),
                CompletionEvent(FALSE, FALSE)
        {
            this->Log = Log;
            this->StreamGuid = StreamGuid;
            this->StreamType = StreamType;
        }
    };

    struct TestDataMarker
    {
        RvdLogAsn       Asn;
        ULONGLONG       MarkerOffset;
    };

    ULONG const         metaDataSize = 100;
    ULONG const         pagedAlignedDataSize = 1024 * 1024;
    ULONG const         numberOfMetaDataMarkers = metaDataSize / sizeof(TestDataMarker);
    ULONG const         numberOfPagedAlignedDataMarkers = pagedAlignedDataSize / sizeof(TestDataMarker);

    VOID
    ThreadEntry(PVOID Context)
    {
        TestContext&        testContext = *((TestContext*)Context);
        KAllocator&         allocator = KAllocatorSupport::GetAllocator(testContext.Log.RawPtr());
        Synchronizer        synchronizer;

        {
            RvdLog::AsyncCreateLogStreamContext::SPtr createStreamOp;
            testContext.CompletionStatus = testContext.Log->CreateAsyncCreateLogStreamContext(createStreamOp);
            if (!NT_SUCCESS(testContext.CompletionStatus))
            {
                KDbgPrintf("SimpleParallelStreamTest: CreateAsyncCreateLogStreamContext Failed: %i\n", __LINE__);
                testContext.CompletionEvent.SetEvent();
                return;
            }

            createStreamOp->StartCreateLogStream(
                testContext.StreamGuid,
                testContext.StreamType,
                testContext.LogStream,
                nullptr,
                synchronizer.AsyncCompletionCallback());

            testContext.CompletionStatus = synchronizer.WaitForCompletion();
            if (!NT_SUCCESS(testContext.CompletionStatus))
            {
                KDbgPrintf("SimpleParallelStreamTest: StartCreateLogStream(0) Failed: %i\n", __LINE__);
                testContext.CompletionEvent.SetEvent();
                return;
            }

            createStreamOp.Reset();
        }


        KBuffer::SPtr       metadata;
        KIoBuffer::SPtr     pagedAlignedData;
        ULONG const         recordCount = 1000;
        ULONG const         trimSizeInAsns = 250;

        union
        {
            TestDataMarker* pagedAlignedDataMarkers;
            void*           pagedAlignedDataBuffer;
        };

        testContext.CompletionStatus = KBuffer::Create(metaDataSize, metadata, allocator, KTL_TAG_TEST);
        if (!NT_SUCCESS(testContext.CompletionStatus))
        {
            testContext.CompletionEvent.SetEvent();
            return;
        }

        TestDataMarker*     metaDataMarkers = (TestDataMarker*)metadata->GetBuffer();

        testContext.CompletionStatus = KIoBuffer::CreateSimple(
            pagedAlignedDataSize,
            pagedAlignedData,
            pagedAlignedDataBuffer,
            allocator,
            KTL_TAG_TEST);

        if (!NT_SUCCESS(testContext.CompletionStatus))
        {
            testContext.CompletionEvent.SetEvent();
            return;
        }

        {
            ULONGLONG                               currentAsn = 1;
            ULONGLONG                               trimmedAsn = 0;
            RvdLogStream::AsyncWriteContext::SPtr   writeOp;

            testContext.CompletionStatus = testContext.LogStream->CreateAsyncWriteContext(writeOp);
            if (!NT_SUCCESS(testContext.CompletionStatus))
            {
                testContext.CompletionEvent.SetEvent();
                return;
            }

            while (currentAsn <= recordCount)
            {
                // Build a record
                RvdLogAsn       asn(currentAsn);
                for (ULONG ix = 0; ix < numberOfMetaDataMarkers; ix++)
                {
                    metaDataMarkers[ix].Asn = asn;
                    metaDataMarkers[ix].MarkerOffset = ix;
                }

                for (ULONG ix = 0; ix < numberOfPagedAlignedDataMarkers; ix++)
                {
                    pagedAlignedDataMarkers[ix].Asn = asn;
                    pagedAlignedDataMarkers[ix].MarkerOffset = ix;
                }

                writeOp->StartWrite(
                    asn,
                    0,
                    metadata,
                    pagedAlignedData,
                    nullptr,
                    synchronizer.AsyncCompletionCallback());

                testContext.CompletionStatus = synchronizer.WaitForCompletion();
                if (!NT_SUCCESS(testContext.CompletionStatus))
                {
                    KDbgPrintf("SimpleParallelStreamTest: Stream Write Failed with: 0x%08X; at line: %i\n",
                        testContext.CompletionStatus,
                        __LINE__);

                    testContext.CompletionEvent.SetEvent();
                    return;
                }

                if ((currentAsn - trimmedAsn) >= trimSizeInAsns)
                {
                    trimmedAsn += (trimSizeInAsns / 2);
                    RvdLogAsn       truncationPoint(trimmedAsn);
                    testContext.LogStream->Truncate(truncationPoint, truncationPoint);
                }

                currentAsn++;
                writeOp->Reuse();
            }
        }

        testContext.CompletionStatus = STATUS_SUCCESS;
        testContext.CompletionEvent.SetEvent();
    }

    NTSTATUS
    VerifyStreamRecords(RvdLogStream::SPtr LogStream)
    {
        NTSTATUS            status;
        RvdLogAsn           lowestAsn;
        RvdLogAsn           highestAsn;

        status = LogStream->QueryRecordRange(&lowestAsn, &highestAsn, nullptr);
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("SimpleParallelStreamTest: QueryRecordRange Failed with: 0x%08X; at line: %i\n",
                status,
                __LINE__);

            return status;
        }

        RvdLogStream::AsyncReadContext::SPtr    readOp;
        status = LogStream->CreateAsyncReadContext(readOp);
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("SimpleParallelStreamTest: CreateAsyncReadContext Failed with: 0x%08X; at line: %i\n",
                status,
                __LINE__);

            return status;
        }

        Synchronizer        synchronizer;
        while (lowestAsn <= highestAsn)
        {
            ULONGLONG       version;
            KBuffer::SPtr   metadata;
            KIoBuffer::SPtr pageAlignedData;

            readOp->StartRead(lowestAsn, &version, metadata, pageAlignedData, nullptr, synchronizer.AsyncCompletionCallback());
            status = synchronizer.WaitForCompletion();
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("SimpleParallelStreamTest: StartRead Failed with: 0x%08X; at line: %i\n",
                    status,
                    __LINE__);

                return status;
            }

            // Validate contents
            TestDataMarker*     testMarkers = (TestDataMarker*)(metadata->GetBuffer());

            for (ULONG ix = 0; ix < numberOfMetaDataMarkers; ix++)
            {
                if ((testMarkers[ix].Asn != lowestAsn) || (testMarkers[ix].MarkerOffset != ix))
                {
                    KDbgPrintf("SimpleParallelStreamTest: Metadata validation failed: at line: %i\n",
                        __LINE__);

                    return STATUS_UNSUCCESSFUL;
                }
            }

            KInvariant(pageAlignedData->QueryNumberOfIoBufferElements() == 1);
            testMarkers = (TestDataMarker*)(pageAlignedData->First()->GetBuffer());

            for (ULONG ix = 0; ix < numberOfPagedAlignedDataMarkers; ix++)
            {
                if ((testMarkers[ix].Asn != lowestAsn) || (testMarkers[ix].MarkerOffset != ix))
                {
                    KDbgPrintf("SimpleParallelStreamTest: Page Aligned Data validation failed: at line: %i\n",
                        __LINE__);

                    return STATUS_UNSUCCESSFUL;
                }
            }

            lowestAsn = lowestAsn.Get() + 1;
            readOp->Reuse();
        }

        return STATUS_SUCCESS;
    }

    NTSTATUS
    Execute(WCHAR TestDrive, KtlSystem& KtlSys)
    {
        NTSTATUS                        status;
        Synchronizer                    activeLogSynchronizer;
        Synchronizer                    synchronizer;

        // Clean the unit test drive environment
        status = CleanAndPrepLog(TestDrive);
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("SimpleParallelStreamTest: CleanAndPrepLog Failed: %i\n", __LINE__);
            return status;
        }

        RvdLogManager::SPtr logManager;
        KEvent activateEvent(FALSE, FALSE);

        status = RvdLogManager::Create(KTL_TAG_TEST, KtlSys.NonPagedAllocator(), logManager);
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("SimpleParallelStreamTest: RvdLogManager::Create Failed: %i\n", __LINE__);
            return status;
        }

        status = logManager->Activate(nullptr, activeLogSynchronizer.AsyncCompletionCallback());
        if (!K_ASYNC_SUCCESS(status))
        {
            KDbgPrintf("SimpleParallelStreamTest: Activate() Failed: %i\n", __LINE__);
            return status;
        }

        RvdLogManager::AsyncCreateLog::SPtr createOp;
        status = logManager->CreateAsyncCreateLogContext(createOp);
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("SimpleParallelStreamTest: CreateAsyncCreateLogContext Failed: %i\n", __LINE__);
            return status;
        }

        GUID                                        diskIdGuid = {};
#if !defined(PLATFORM_UNIX)     
        KVolumeNamespace::VolumeInformationArray    volInfo(KtlSystem::GlobalNonPagedAllocator());

        status = KVolumeNamespace::QueryVolumeListEx(volInfo, KtlSys.NonPagedAllocator(), synchronizer.AsyncCompletionCallback());
        if (!K_ASYNC_SUCCESS(status))
        {
            KDbgPrintf("SimpleParallelStreamTest: KVolumeNamespace::QueryVolumeListEx Failed: %i\n", __LINE__);
            return status;
        }

        status = synchronizer.WaitForCompletion();
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("SimpleParallelStreamTest: KVolumeNamespace::QueryVolumeListEx Failed: %i\n", __LINE__);
            return status;
        }

        // Find Test Drive's volume GUID
        ULONG i;
        for (i = 0; i < volInfo.Count(); i++)
        {
            if (volInfo[i].DriveLetter == (UCHAR)TestDrive)
            {
                break;
            }
        }

        if (i == volInfo.Count())
        {
            KDbgPrintf("SimpleParallelStreamTest: KVolumeNamespace::QueryVolumeListEx did not return volume guid for drive %C: %i\n",
                TestDrive,
                __LINE__);

            return STATUS_UNSUCCESSFUL;
        }
        diskIdGuid = volInfo[i].VolumeId;
#else
        diskIdGuid = RvdDiskLogConstants::HardCodedVolumeGuid();        
#endif

        RvdLog::SPtr log1;

        KWString logType(KtlSystem::GlobalNonPagedAllocator(), L"RvdLog");
        KInvariant(NT_SUCCESS(logType.Status()));

        // Create a log file
        createOp->StartCreateLog(
            KGuid(diskIdGuid),
            RvdLogId(KGuid(TestLogIdGuid)),
            logType,
            DefaultTestLogFileSize,
            log1,
            nullptr,
            synchronizer.AsyncCompletionCallback());

        status = synchronizer.WaitForCompletion();
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("SimpleParallelStreamTest: StartCreateLog Failed: %i\n", __LINE__);
            return status;
        }

        createOp.Reset();

        // Clean log has been created - now create two test streams
        KGuid                           stream0Guid;
        KGuid                           stream1Guid;
        KGuid                           streamType;

        stream0Guid.CreateNew();
        stream1Guid.CreateNew();
        streamType.CreateNew();

        RvdLogStream::SPtr  stream0;
        RvdLogStream::SPtr  stream1;

        {
            TestContext         threadContext0(log1, stream0Guid, streamType);
            TestContext         threadContext1(log1, stream1Guid, streamType);
            KThread::SPtr       thread0;
            KThread::SPtr       thread1;

            status = KThread::Create(ThreadEntry, &threadContext0, thread0, KtlSys.NonPagedAllocator(), KTL_TAG_TEST);
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("SimpleParallelStreamTest: KThread::Create failed: %i\n", __LINE__);
                return status;
            }

            status = KThread::Create(ThreadEntry, &threadContext1, thread1, KtlSys.NonPagedAllocator(), KTL_TAG_TEST);
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("SimpleParallelStreamTest: KThread::Create failed: %i\n", __LINE__);
                return status;
            }

            // Wait for each stream write thread to finish their write/truncate exercise
            threadContext0.CompletionEvent.WaitUntilSet();
            threadContext1.CompletionEvent.WaitUntilSet();

            if (!NT_SUCCESS(threadContext0.CompletionStatus))
            {
                KDbgPrintf("SimpleParallelStreamTest: Thread 0 failed: %i\n", __LINE__);
                return threadContext0.CompletionStatus;
            }

            if (!NT_SUCCESS(threadContext1.CompletionStatus))
            {
                KDbgPrintf("SimpleParallelStreamTest: Thread 1 failed: %i\n", __LINE__);
                return threadContext1.CompletionStatus;
            }

            stream0 = threadContext0.LogStream;
            stream1 = threadContext1.LogStream;
        }

        // Verify each streams contents after all I/O on the log is done
        while (!log1->IsLogFlushed())
        {
            KNt::Sleep(0);
        }

        status = VerifyStreamRecords(stream0);
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("SimpleParallelStreamTest: VerifyStreamRecords(0) Failed with: 0x%08X; at line: %i\n",
                status,
                __LINE__);

            return status;
        }

        status = VerifyStreamRecords(stream1);
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("SimpleParallelStreamTest: VerifyStreamRecords(1) Failed with: 0x%08X; at line: %i\n",
                status,
                __LINE__);

            return status;
        }

        stream0.Reset();
        stream1.Reset();
        log1.Reset();
        logManager->Deactivate();
        status = activeLogSynchronizer.WaitForCompletion();
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("SimpleParallelStreamTest: LogManager Deactivation Failed with: 0x%08X; at line: %i\n",
                status,
                __LINE__);

            return status;
        }

        return STATUS_SUCCESS;
    }
}


//** RvdLogLsnEntryTracker related tests
NTSTATUS
ValidateRvdLogLsnEntryTracker(
    RvdLogLsnEntryTracker& TestTracker,
    ULONG const LowestLsn,
    ULONG const HighestLsn,
    LONGLONG const TotalSize)
{
    NTSTATUS        status;
    ULONG           numberOfRecs;
    RvdLogConfig    config;
        KAssert(NT_SUCCESS(config.Status()));

    if ((numberOfRecs = TestTracker.QueryNumberOfRecords()) != (HighestLsn - LowestLsn + 1))
    {
        KDbgPrintf("ValidateRvdLogLsnEntryTracker: QueryNumberOfRecords returned unexpected result: %i\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    if (TestTracker.QueryTotalSize() != TotalSize)
    {
        KDbgPrintf("ValidateRvdLogLsnEntryTracker: QueryTotalSize returned unexpected result: %i\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    for (ULONG ix = 0; ix < numberOfRecs; ix++)
    {
        RvdLogLsn       lsn;
        ULONG           metadataSize;
        ULONG           ioBufSize;

        status = TestTracker.QueryRecord(ix, lsn, &metadataSize, &ioBufSize);
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("ValidateRvdLogLsnEntryTracker: QueryRecord failed: %i\n", __LINE__);
            return status;
        }

        if (lsn.Get() != (ix + LowestLsn))
        {
            KDbgPrintf("ValidateRvdLogLsnEntryTracker: QueryRecord returned unexpected result: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        ULONG           expectedSize = (ULONG)((ix + LowestLsn) % 1010);
        if (metadataSize != expectedSize)
        {
            KDbgPrintf("ValidateRvdLogLsnEntryTracker: QueryRecord returned unexpected result: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        ULONG           expectedIoSize = (ULONG)((ix + LowestLsn) / 1000 * 4096);
        if (ioBufSize != expectedIoSize)
        {
            KDbgPrintf("ValidateRvdLogLsnEntryTracker: QueryRecord returned unexpected result: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }
    }

    ULONG numberOfEntriesToGet = 0;
    if (NT_SUCCESS(TestTracker.GetAllRecordLsns(&numberOfEntriesToGet, nullptr)))
    {
        KDbgPrintf("ValidateRvdLogLsnEntryTracker: GetAllRecordLsns returned unexpected success: %i\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    RvdLogLsnEntry*     resultArray = _newArray<RvdLogLsnEntry>(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator(), numberOfEntriesToGet);
    status = TestTracker.GetAllRecordLsns(&numberOfEntriesToGet, resultArray);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("ValidateRvdLogLsnEntryTracker: GetAllRecordLsns failed: %i\n", __LINE__);
        return status;
    }

    for (ULONG ix = 0; ix < numberOfEntriesToGet; ix++)
    {
        RvdLogLsnEntry& entry = resultArray[ix];

        if (entry.Lsn.Get() != (ix + LowestLsn))
        {
            KDbgPrintf("ValidateRvdLogLsnEntryTracker: GetAllRecordLsns returned unexpected result: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        ULONG           expectedSize = (ULONG)((ix + LowestLsn) % 1010);
        if (entry.HeaderAndMetaDataSize != expectedSize)
        {
            KDbgPrintf("ValidateRvdLogLsnEntryTracker: GetAllRecordLsns returned unexpected result: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        ULONG           expectedIoSize = (ULONG)((ix + LowestLsn) / 1000 * 4096);
        if (entry.IoBufferSize != expectedIoSize)
        {
            KDbgPrintf("ValidateRvdLogLsnEntryTracker: GetAllRecordLsns returned unexpected result: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }
    }

    {
        KIoBuffer::SPtr ioBuffer;
        void* t;
        status = KIoBuffer::CreateSimple(4096, ioBuffer, t, KtlSystem::GlobalNonPagedAllocator(), KTL_TAG_TEST);
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("ValidateRvdLogLsnEntryTracker: CreateSimple failed: %i\n", __LINE__);
            return status;
        }

        ULONG           startingOffset = sizeof(RvdLogUserStreamCheckPointRecordHeader);
        ULONG           numberOfIoBufferEntries;
        ULONG           entriesIn1stSeg;
        ULONG           numberOfSegments;

        status = TestTracker.GetAllRecordLsnsIntoIoBuffer(
            (config.GetMaxMetadataSize() / 2),
            sizeof(RvdLogUserStreamCheckPointRecordHeader),
            *ioBuffer,
            startingOffset,
            numberOfIoBufferEntries,
            entriesIn1stSeg,
            numberOfSegments);

        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("ValidateRvdLogLsnEntryTracker: GetAllRecordLsnsIntoIoBuffer failed: %i\n", __LINE__);
            return status;
        }

        KIoBufferStream     stream(*ioBuffer, sizeof(RvdLogUserStreamCheckPointRecordHeader));
        ULONG const         entriesPerSegment = ((config.GetMaxMetadataSize() / 2) - sizeof(RvdLogUserStreamCheckPointRecordHeader)) / sizeof(RvdLogLsnEntry);
        ULONG               segmentEntries = entriesPerSegment;
        ULONG               ix = 0;
        RvdLogLsnEntry      tEntry;

        while (ix < numberOfEntriesToGet)
        {
            if (!stream.Pull(tEntry))
            {
                KDbgPrintf("ValidateRvdLogLsnEntryTracker: Failure during Pull: %i\n", __LINE__);
                return STATUS_UNSUCCESSFUL;
            }

            if (RtlCompareMemory(&tEntry, &resultArray[ix], sizeof(RvdLogLsnEntry)) != sizeof(RvdLogLsnEntry))
            {
                KDbgPrintf("ValidateRvdLogLsnEntryTracker: RtlCompareMemory Failure during Pull test: %i\n", __LINE__);
                return STATUS_UNSUCCESSFUL;
            }

            ix++;
            segmentEntries--;

            if (segmentEntries == 0)
            {
                // At end of segment - move the iobuffer stream ahead to the start of data in the next segment
                segmentEntries = entriesPerSegment;

                ULONG       pos = stream.GetPosition();
                KInvariant(stream.Skip((RvdDiskLogConstants::RoundUpToBlockBoundary(pos) - pos) + sizeof(RvdLogUserStreamCheckPointRecordHeader)));
            }
        }
    }

    {
        KIoBuffer::SPtr ioBuffer;
        void* t;
        status = KIoBuffer::CreateSimple(4096, ioBuffer, t, KtlSystem::GlobalNonPagedAllocator(), KTL_TAG_TEST);
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("ValidateRvdLogLsnEntryTracker: CreateSimple failed: %i\n", __LINE__);
            return status;
        }

        ULONG           startingOffset = 2000;
        ULONG           numberOfIoBufferEntries;
        ULONG           entriesIn1stSeg;
        ULONG           numberOfSegments;

        status = TestTracker.GetAllRecordLsnsIntoIoBuffer(
            MAXULONG,
            0,
            *ioBuffer,
            startingOffset,
            numberOfIoBufferEntries,
            entriesIn1stSeg,
            numberOfSegments);

        KInvariant(numberOfSegments == 1);
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("ValidateRvdLogLsnEntryTracker: GetAllRecordLsnsIntoIoBuffer failed: %i\n", __LINE__);
            return status;
        }

        KIoBufferStream  stream(*ioBuffer);
        if (!stream.Skip(2000))
        {
            KDbgPrintf("ValidateRvdLogLsnEntryTracker: Skip failed: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        ULONG           ix = 0;
        RvdLogLsnEntry  tEntry;
        while (ix < numberOfEntriesToGet)
        {
            if (!stream.Pull(tEntry))
            {
                KDbgPrintf("ValidateRvdLogLsnEntryTracker: Failure during Pull: %i\n", __LINE__);
                return STATUS_UNSUCCESSFUL;
            }

            if (RtlCompareMemory(&tEntry, &resultArray[ix], sizeof(RvdLogLsnEntry)) != sizeof(RvdLogLsnEntry))
            {
                KDbgPrintf("ValidateRvdLogLsnEntryTracker: RtlCompareMemory Failure during Pull test: %i\n", __LINE__);
                return STATUS_UNSUCCESSFUL;
            }
            ix++;
        }
    }

    _deleteArray(resultArray);
    resultArray = nullptr;

    for (ULONGLONG ix = LowestLsn; ix <= HighestLsn; ix += 2)
    {
        RvdLogLsn       lsn(ix);

        TestTracker.Truncate(lsn);
        if (TestTracker.QueryNumberOfRecords() != numberOfEntriesToGet)
        {
            KDbgPrintf("ValidateRvdLogLsnEntryTracker: Truncate caused unexpected result: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }
        numberOfEntriesToGet -= 2;
    }

    if (TestTracker.QueryNumberOfRecords() != 1)
    {
        KDbgPrintf("ValidateRvdLogLsnEntryTracker: Truncate caused unexpected result: %i\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
RvdLogLsnEntryTrackerTests()
{
    RvdLogLsnEntryTracker testTracker(KtlSystem::GlobalNonPagedAllocator());
    const ULONG LowestLsn = 1000;
    const ULONG HighestLsn = 350000;

    LONGLONG    totalSize = 0;
    NTSTATUS    status;

    for (ULONGLONG ix = LowestLsn; ix <= HighestLsn; ix++)
    {
        RvdLogLsn       lsn(ix);
        ULONG           size = (ULONG)(ix % 1010);
        ULONG           ioSize = (ULONG)(ix / 1000 * 4096);

        status = testTracker.AddHigherLsnRecord(lsn, size, ioSize);
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("RvdLogLsnEntryTrackerTest: AddHigherLsnRecord failed: %i\n", __LINE__);
            return status;
        }

        size = (((size + (RvdDiskLogConstants::BlockSize - 1)) / RvdDiskLogConstants::BlockSize) * RvdDiskLogConstants::BlockSize);
        totalSize += (size + ioSize);
    }

    status = ValidateRvdLogLsnEntryTracker(testTracker, LowestLsn, HighestLsn, totalSize);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("RvdLogLsnEntryTrackerTest: ValidateRvdLogLsnEntryTracker failed: %i\n", __LINE__);
        return status;
    }

    RvdLogLsnEntryTracker testTracker1(KtlSystem::GlobalNonPagedAllocator());
    totalSize = 0;

    for (ULONGLONG ix = HighestLsn; ix >= LowestLsn; ix--)
    {
        RvdLogLsn       lsn(ix);
        ULONG           size = (ULONG)(ix % 1010);
        ULONG           ioSize = (ULONG)(ix / 1000 * 4096);

        status = testTracker1.AddLowerLsnRecord(lsn, size, ioSize);
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("RvdLogLsnEntryTrackerTest: AddLowerLsnRecord failed: %i\n", __LINE__);
            return status;
        }

        size = (((size + (RvdDiskLogConstants::BlockSize - 1)) / RvdDiskLogConstants::BlockSize) * RvdDiskLogConstants::BlockSize);
        totalSize += (size + ioSize);
    }

    status = ValidateRvdLogLsnEntryTracker(testTracker1, LowestLsn, HighestLsn, totalSize);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("RvdLogLsnEntryTrackerTest: ValidateRvdLogLsnEntryTracker failed: %i\n", __LINE__);
        return status;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
RvdAsnIndexTests()
{
    {   //** Prove basic AsnIndex behavior and KIoBufferStream access to AsnIndex contents
        RvdAsnIndex testIndex(KtlSystem::GlobalNonPagedAllocator());
        ULONGLONG LowestAsn = RvdLogAsn::Min().Get();
        ULONGLONG HighestAsn = RvdLogAsn::Min().Get() + 150000;

        NTSTATUS    status;
        RvdLogConfig    config;
            KAssert(NT_SUCCESS(config.Status()));

        for (ULONGLONG ix = LowestAsn; ix <= HighestAsn; ix++)
        {
            RvdLogAsn                       asn(ix);
            RvdAsnIndexEntry::SPtr          indexEntry = _new(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator())
                RvdAsnIndexEntry(RvdLogAsn(asn), 0, 0, RvdLogStream::RecordDisposition::eDispositionPersisted, RvdLogLsn::Min());

            RvdAsnIndexEntry::SPtr          resultingIndexEntry;
            RvdAsnIndexEntry::SavedState    savedState;
            BOOLEAN                         prevStateStored;

            if (ix == (LowestAsn + 1))
            {
                // Cause one record to not be marked as eDispositionPersisted
                indexEntry =  _new(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator()) RvdAsnIndexEntry(
                    RvdLogAsn(asn),
                    0,
                    0,
                    RvdLogStream::RecordDisposition::eDispositionNone,
                    RvdLogLsn::Min());
            }

            ULONGLONG dontCare;
            status = testIndex.AddOrUpdate(indexEntry, resultingIndexEntry, savedState, prevStateStored, dontCare);
            if (!NT_SUCCESS(status))
            {
                KDbgPrintf("RvdAsnIndexTests: AddOrUpdate failed: %i\n", __LINE__);
                return status;
            }
        }

        KIoBuffer::SPtr     ioBuffer;
        void*               bPtr;

        status = KIoBuffer::CreateSimple(
            config.GetMaxMetadataSize() / 2,
            ioBuffer, bPtr,
            KtlSystem::GlobalNonPagedAllocator(),
            KTL_TAG_TEST);

        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("RvdAsnIndexTests: CreateSimple failed: %i\n", __LINE__);
            return status;
        }

        ULONG               ioBufferOffset = sizeof(RvdLogUserStreamCheckPointRecordHeader);
        ULONG               numberOfEntries;
        ULONG               numberOfSegments;

        status = testIndex.GetAllEntriesIntoIoBuffer(
            KtlSystem::GlobalNonPagedAllocator(),
            config.GetMaxMetadataSize() / 2,
            sizeof(RvdLogUserStreamCheckPointRecordHeader),
            *ioBuffer,
            ioBufferOffset,
            numberOfEntries,
            numberOfSegments);

        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("RvdAsnIndexTests: GetAllEntriesIntoIoBuffer failed: %i\n", __LINE__);
            return status;
        }

        if (numberOfEntries != (HighestAsn - LowestAsn))
        {
            KDbgPrintf("RvdAsnIndexTests: GetAllEntriesIntoIoBuffer return wrong numberOfEntries: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        KIoBufferStream     stream(*ioBuffer, sizeof(RvdLogUserStreamCheckPointRecordHeader));
        ULONG               entriesPerSegment =
            ((config.GetMaxMetadataSize() / 2) - sizeof(RvdLogUserStreamCheckPointRecordHeader)) / sizeof(AsnLsnMappingEntry);

        RvdLogAsn           nextExpectedAsn = RvdLogAsn::Min();
        AsnLsnMappingEntry  entry(RvdLogAsn::Null(), 0, RvdLogLsn::Null(), 0);

        ioBufferOffset = sizeof(RvdLogUserStreamCheckPointRecordHeader);

        for (ULONG segment = 0; segment < numberOfSegments; segment++)
        {
            for (ULONG ix = 0; (ix < entriesPerSegment) && (numberOfEntries > 0); ix++)
            {
                if (!stream.Pull(entry))
                {
                    KDbgPrintf("RvdAsnIndexTests: GetAllEntriesIntoIoBuffer Pull failed: %i\n", __LINE__);
                    return STATUS_UNSUCCESSFUL;
                }

                if (nextExpectedAsn.Get() == (RvdLogAsn::Min().Get() + 1))
                {
                    // Special case for one record that's marked as not-interesting (see above)
                    nextExpectedAsn = nextExpectedAsn.Get() + 1;
                }

                // Validate entry
                if (entry.RecordAsn != nextExpectedAsn)
                {
                    KDbgPrintf("RvdAsnIndexTests: GetAllEntriesIntoIoBuffer: failed Unexpected ASN: %i\n", __LINE__);
                    return STATUS_UNSUCCESSFUL;
                }

                numberOfEntries--;
                nextExpectedAsn = nextExpectedAsn.Get() + 1;
                ioBufferOffset += sizeof(AsnLsnMappingEntry);
            }

            // Bump offset to next segment boundary
            if (numberOfEntries > 0)
            {
                ULONG       currentPos = stream.GetPosition();
                KAssert(currentPos == ioBufferOffset);
                stream.Skip((RvdDiskLogConstants::RoundUpToBlockBoundary(currentPos) - currentPos) +
                            sizeof(RvdLogUserStreamCheckPointRecordHeader));

                ioBufferOffset += (RvdDiskLogConstants::RoundUpToBlockBoundary(currentPos) - currentPos) +
                                    sizeof(RvdLogUserStreamCheckPointRecordHeader);

                currentPos = stream.GetPosition();
                KAssert(currentPos == ioBufferOffset);
            }
        }
    }

    {   //** Prove AsnIndex order maintainance behaviors - e.g. maintain correct state supporting truncation in light of
        //   out-of-order ASNs wrt LSN space.

        //               Asn      Ver        Lsn(order)  LowestLsnOfHigherAsns    Disp      Size
        //
        //              0x253c   0x4302 0n107997499392(2)   0n107976638464      Persisted   0x41000
        //              0x253c   0x4303               (3)                       None
        //              0x253c   0x4302 0n107997499392(6)   0n107976638464      Persisted   0x41000     (Restored - simulates a failure and backout)
        //              0x253a   0x4303 0n108004302848(7)   0n107976638464      Pending     0x33000     (Update)
        //              0x253a   0x4303 0n107997499392(4)                       None                    (Start of write)
        //              0x253a   0x4302 0n107997290496(1)   0n107976638464      Persisted   0x33000
        //              0x2538   0x4303 0n108004249600(5)   0n108004249600      Pending     0x0d000

        // After operations:
        //              0x253c   0x4302 0n107997499392      0n107976638464      Persisted   0x41000
        //              0x253a   0x4303 0n108004302848      0n107976638464      Pending     0x33000
        //              0x2538   0x4303 0n108004249600      0n107976638464      Pending     0x0d000


        RvdAsnIndex testIndex(KtlSystem::GlobalNonPagedAllocator());

        //* Step 0: Set up highest ASN for test
        RvdAsnIndexEntry::SPtr      indexEntry = _new(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator())
            RvdAsnIndexEntry(RvdLogAsn(0x253d), 0x4302, 0x133000, RvdLogStream::RecordDisposition::eDispositionPersisted, RvdLogLsn(107976638464));

        RvdAsnIndexEntry::SPtr          resultIndexEntry;
        RvdAsnIndexEntry::SavedState    savedState;
        BOOLEAN                         savedStatePresent;
        ULONGLONG                       dontCare;
        NTSTATUS                        status = testIndex.AddOrUpdate(indexEntry, resultIndexEntry, savedState, savedStatePresent, dontCare);

        KInvariant(NT_SUCCESS(status));
        KInvariant(!savedStatePresent);
        KInvariant(resultIndexEntry->GetLowestLsnOfHigherASNs().Get() == 107976638464);

        //* Step 1: Write 0x253a/0x4302
        indexEntry = _new(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator())
            RvdAsnIndexEntry(RvdLogAsn(0x253a), 0x4302, 0x33000, RvdLogStream::RecordDisposition::eDispositionNone, RvdLogLsn::Null());

        status = testIndex.AddOrUpdate(indexEntry, resultIndexEntry, savedState, savedStatePresent, dontCare);
        KInvariant(NT_SUCCESS(status));
        KInvariant(!savedStatePresent);
        KInvariant(testIndex.UpdateLsnAndDisposition(indexEntry, 0x4302, RvdLogStream::RecordDisposition::eDispositionPending, RvdLogLsn(107997290496)));
        KInvariant(resultIndexEntry->GetLowestLsnOfHigherASNs().Get() == 107976638464);
        KInvariant(testIndex.UpdateDisposition(indexEntry, 0x4302, RvdLogStream::RecordDisposition::eDispositionPersisted));
        KInvariant(resultIndexEntry->GetLowestLsnOfHigherASNs().Get() == 107976638464);

        //* Step 2: Write 0x253c/0x4302
        indexEntry = _new(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator())
            RvdAsnIndexEntry(RvdLogAsn(0x253c), 0x4302, 0x41000, RvdLogStream::RecordDisposition::eDispositionNone, RvdLogLsn::Null());

        status = testIndex.AddOrUpdate(indexEntry, resultIndexEntry, savedState, savedStatePresent, dontCare);
        KInvariant(NT_SUCCESS(status));
        KInvariant(!savedStatePresent);
        KInvariant(testIndex.UpdateLsnAndDisposition(indexEntry, 0x4302, RvdLogStream::RecordDisposition::eDispositionPending, RvdLogLsn(107997499392)));
        KInvariant(resultIndexEntry->GetLowestLsnOfHigherASNs().Get() == 107976638464);
        KInvariant(testIndex.UpdateDisposition(indexEntry, 0x4302, RvdLogStream::RecordDisposition::eDispositionPersisted));
        KInvariant(resultIndexEntry->GetLowestLsnOfHigherASNs().Get() == 107976638464);

        //* Step 3: Start update of: 0x253c/0x4303
        indexEntry = _new(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator())
            RvdAsnIndexEntry(RvdLogAsn(0x253c), 0x4303, 0, RvdLogStream::RecordDisposition::eDispositionNone, RvdLogLsn::Null());

        status = testIndex.AddOrUpdate(indexEntry, resultIndexEntry, savedState, savedStatePresent, dontCare);
        KInvariant(NT_SUCCESS(status));
        KInvariant(savedStatePresent);
        KInvariant(savedState.GetMappingEntry().RecordAsnVersion == 0x4302);
        RvdAsnIndexEntry::SavedState    savedState0x253c = savedState;
        RvdAsnIndexEntry::SPtr          indexEntry0x253c = resultIndexEntry;

        //* Step 4: Start update of: 0x253a/0x4303
        indexEntry = _new(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator())
            RvdAsnIndexEntry(RvdLogAsn(0x253a), 0x4303, 0x33000, RvdLogStream::RecordDisposition::eDispositionNone, RvdLogLsn::Null());

        status = testIndex.AddOrUpdate(indexEntry, resultIndexEntry, savedState, savedStatePresent, dontCare);
        KInvariant(NT_SUCCESS(status));
        KInvariant(savedStatePresent);
        KInvariant(savedState.GetMappingEntry().RecordAsnVersion == 0x4302);
        RvdAsnIndexEntry::SPtr          indexEntry0x253a = resultIndexEntry;

        // Step 5: Write (in-flight) 0x2538/0x4303
        indexEntry = _new(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator())
            RvdAsnIndexEntry(RvdLogAsn(0x2538), 0x4303, 0x0d000, RvdLogStream::RecordDisposition::eDispositionNone, RvdLogLsn::Null());

        status = testIndex.AddOrUpdate(indexEntry, resultIndexEntry, savedState, savedStatePresent, dontCare);
        KInvariant(NT_SUCCESS(status));
        KInvariant(!savedStatePresent);
        KInvariant(testIndex.UpdateLsnAndDisposition(indexEntry, 0x4303, RvdLogStream::RecordDisposition::eDispositionPending, RvdLogLsn(108004249600)));
        KInvariant(resultIndexEntry->GetLowestLsnOfHigherASNs().Get() == 107976638464);
        RvdAsnIndexEntry::SPtr          indexEntry0x2538 = resultIndexEntry;

        // Step 6: Undo write of 0x253c/0x4303
        testIndex.Restore(indexEntry0x253c, 0x4303, savedState0x253c);
        KInvariant(indexEntry0x253c->GetLowestLsnOfHigherASNs().Get() == 107976638464);

        // Step 7: Finish update of: 0x253a/0x4303
        KInvariant(testIndex.UpdateLsnAndDisposition(indexEntry0x253a, 0x4303, RvdLogStream::RecordDisposition::eDispositionPending, RvdLogLsn(108004302848)));
        KInvariant(indexEntry0x2538->GetLowestLsnOfHigherASNs().Get() == 107976638464);
        KInvariant(indexEntry0x253c->GetLowestLsnOfHigherASNs().Get() == 107976638464);
        KInvariant(indexEntry0x253a->GetLowestLsnOfHigherASNs().Get() == 107976638464);

        KInvariant(testIndex.Validate());
    }

    {
        //* Generate random out-of-order ASN (wrt LSN) test - simulates seq LSN writes with OOO ASNs
        ULONG const     maxId = 10000;
        KArray<ULONG>   asnWriteOrder(KtlSystem::GlobalNonPagedAllocator(), maxId + 1);
        ULONG const     sizeofBitmapStore = ((maxId * 4) + 1) / sizeof(UCHAR);
        UCHAR*          bitmapStore = _newArray<UCHAR>(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator(), sizeofBitmapStore);
        KFinally([bitmapStore]()
        {
            _deleteArray(bitmapStore);
        });

        KBitmap         assignedAsns(bitmapStore, sizeofBitmapStore, ((maxId * 4) + 1));

        assignedAsns.ClearAllBits();

        for (ULONG lsn = 0; lsn <= maxId; lsn++)
        {
            ULONG       asn;
            do
            {
                asn = RandomGenerator::Get(maxId * 4, 1);
            } while (assignedAsns.CheckBit(asn));

            assignedAsns.SetBits(asn, 1);
            asnWriteOrder.Append(asn);
        }

        // Update an ASN table in LSN write order
        RvdAsnIndex testIndex(KtlSystem::GlobalNonPagedAllocator());

        for (ULONG lsn = 0; lsn <= maxId; lsn++)
        {
            RvdAsnIndexEntry::SPtr      indexEntry = _new(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator())
                RvdAsnIndexEntry(RvdLogAsn(asnWriteOrder[lsn]), 0x4302, 0x1, RvdLogStream::RecordDisposition::eDispositionPersisted, RvdLogLsn(lsn));

            RvdAsnIndexEntry::SPtr          resultIndexEntry;
            RvdAsnIndexEntry::SavedState    savedState;
            BOOLEAN                         savedStatePresent;
            ULONGLONG                       dontCare;
            NTSTATUS                        status = testIndex.AddOrUpdate(indexEntry, resultIndexEntry, savedState, savedStatePresent, dontCare);

            KInvariant(NT_SUCCESS(status));
            KInvariant(!savedStatePresent);
            KInvariant(resultIndexEntry->GetLowestLsnOfHigherASNs().Get() <= lsn);
        }

        KInvariant(testIndex.Validate());
    }

    return STATUS_SUCCESS;
}

void
RvdLogOnDiskConfigTests()
{
    RvdLogConfig            testObj;
    KInvariant(NT_SUCCESS(testObj.Status()));

    RvdLogOnDiskConfig      testObj1(testObj);

    RvdLogConfig            testObj2(testObj1);
    KInvariant(NT_SUCCESS(testObj2.Status()));

    RvdLogConfig            testObj3(4096, 1024, 512);
    KInvariant(testObj3.Status() == STATUS_INVALID_PARAMETER);

    RvdLogConfig            testObj4((RvdLogOnDiskConfig&)testObj3);
    KInvariant(testObj3.Status() == STATUS_INVALID_PARAMETER);
}

//** Tests underlying internal data structures used in the Logger
NTSTATUS
InternalStructuresTest()
{
    NTSTATUS status;

    RvdLogOnDiskConfigTests();

    status = RvdLogLsnEntryTrackerTests();
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("InternalStructuresTest: RvdLogLsnEntryTrackerTests failed: %i\n", __LINE__);
        return status;
    }

    status = RvdAsnIndexTests();
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("InternalStructuresTest: RvdAsnIndexTests failed: %i\n", __LINE__);
        return status;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
InternalBasicDiskLoggerTest(__in int argc, __in_ecount(argc) WCHAR* args[])
{
    KDbgPrintf("RvdLogger Unit Test: Start: BasicDiskLoggerTest\n");
    KInvariant(argc >= 1);

    NTSTATUS        status = STATUS_SUCCESS;

    if (!NT_SUCCESS(InternalStructuresTest()))
    {
        KDbgPrintf("RvdLogger Unit Test: InternalStructuresTest Failed\n");
        return STATUS_UNSUCCESSFUL;
    }

    if (!NT_SUCCESS(NullActivateDeactiveTest()))
    {
        KDbgPrintf("RvdLogger Unit Test: NullActivateDeactiveTest Failed\n");
        return STATUS_UNSUCCESSFUL;
    }

    if (!NT_SUCCESS(BasicLogCreateTest()))
    {
        KDbgPrintf("RvdLogger Unit Test: BasicLogCreateTest Failed\n");
        return STATUS_UNSUCCESSFUL;
    }

    if (!NT_SUCCESS(BasicLogStreamTest(*(args[0]))))
    {
        KDbgPrintf("RvdLogger Unit Test: BasicLogStreamTest Failed\n");
        return STATUS_UNSUCCESSFUL;
    }

    if (!NT_SUCCESS(status = SimpleParallelStreamTest::Execute(*(args[0]), KtlSystem::GetDefaultKtlSystem())))
    {
        KDbgPrintf("RvdLogger Unit Test: SimpleParallelStreamTest Failed: 0x%08X\n", status);
        return status;
    }

    if (!NT_SUCCESS(ParallelLogTest()))
    {
        KDbgPrintf("RvdLogger Unit Test: ParallelLogTest Failed\n");
        return STATUS_UNSUCCESSFUL;
    }

    KDbgPrintf("RvdLogger Unit Test: Complete: BasicDiskLoggerTest\n");
    return STATUS_SUCCESS;
}


//** Main Test Entry Point: BasicDiskLoggerTest
NTSTATUS
BasicDiskLoggerTest(__in int argc, __in_ecount(argc) WCHAR* args[])
{
    NTSTATUS result;
    KtlSystem* underlyingSystem;

    EventRegisterMicrosoft_Windows_KTL();
    KtlSystem::Initialize(FALSE,              // Do not enable VNetworking
                         &underlyingSystem);

    if ((result = InternalBasicDiskLoggerTest(argc, args)) != STATUS_SUCCESS)
    {
        KDbgPrintf("RvdLogger Unit Test: FAILED: 0X%X\n", result);
    }

    KtlSystem::Shutdown();
    EventUnregisterMicrosoft_Windows_KTL();

    return result;
}

#if CONSOLE_TEST
int __cdecl
#if !defined(PLATFORM_UNIX)
wmain(__in int argc, __in_ecount(argc) WCHAR* args[])
{
#else
main(int argc, char* cargs[])
{
    std::vector<WCHAR*> args_vec(argc);
    WCHAR** args = (WCHAR**)args_vec.data();
    std::vector<std::wstring> wargs(argc);
    for (int iter = 0; iter < argc; iter++)
    {
        wargs[iter] = Utf8To16(cargs[iter]);
        args[iter] = (WCHAR*)(wargs[iter].data());
    }
#endif
    NTSTATUS result = STATUS_SUCCESS;

    KDbgPrintf("Usage:\n");
    KDbgPrintf("    RvdLoggerTests [Basic | RecoveryTest | StructVerify | StreamTest | AliasTest] C:\n\n");

    if (argc <= 2)
    {
        result = BasicDiskLoggerTest(argc - 1, &args[1]);
    }
    else
    {
        argc--;
        args++;
        if (_wcsicmp(args[0], L"Basic") == 0)
        {
            result = BasicDiskLoggerTest(argc - 1, args + 1);
        }
        else if (_wcsicmp(args[0], L"AliasTest") == 0)
        {
            result = RvdLoggerAliasTests(argc - 1, args + 1);
        }
        else if (_wcsicmp(args[0], L"RecoveryTest") == 0)
        {
            result = RvdLoggerRecoveryTests(argc - 1, args + 1);
        }
        else if (_wcsicmp(args[0], L"StructVerify") == 0)
        {
            result = DiskLoggerStructureVerifyTests(argc - 1, args + 1);
        }
        else if ((_wcsicmp(args[0], L"StreamTest") == 0) && (argc >= 2))
        {
            result = LogStreamAsyncIoTests(argc - 1, args + 1);
        }
        else
        {
            result = STATUS_INVALID_PARAMETER_1;
        }
    }

    return RtlNtStatusToDosError(result);
}
#endif
