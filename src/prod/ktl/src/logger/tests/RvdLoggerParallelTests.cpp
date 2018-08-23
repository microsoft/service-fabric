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
#if defined(PLATFORM_UNIX)
#include "RvdLogTestHelpers.h"
#endif
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>
#include <InternalKtlLogger.h>

#if (CONSOLE_TEST) || (DISPLAY_ON_CONSOLE)
#undef KDbgPrintf
#define KDbgPrintf(...)     printf(__VA_ARGS__)
#endif

NTSTATUS
ParallelLogTest()
{
    NTSTATUS status;
    UCHAR const testDriveLetter = 'C';

    // Clean the unit test drive environment
    status = CleanAndPrepLog((WCHAR)testDriveLetter);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("ParallelLogTest: CleanAndPrepLog Failed: %i\n", __LINE__);
        return status;
    }

    RvdLogManager::SPtr logManager;
    KEvent activateEvent(FALSE, FALSE);

    status = RvdLogManager::Create(KTL_TAG_LOGGER, KtlSystem::GlobalNonPagedAllocator(), logManager);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("ParallelLogTest: RvdLogManager::Create Failed: %i\n", __LINE__);
        return status;
    }

    KAsyncContextBase::CompletionCallback activateDoneCallback;
#ifdef _WIN64
    activateDoneCallback.Bind((PVOID)(&activateEvent), &StaticTestCallback);
#endif

    status = logManager->Activate(nullptr, activateDoneCallback);
    if (!K_ASYNC_SUCCESS(status))
    {
        KDbgPrintf("ParallelLogTest: Activate() Failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return status;
    }

    const int noOfParallelOps = 10;
    RvdLogManager::AsyncCreateLog::SPtr createOpArray[noOfParallelOps];
    for (int i = 0; i < noOfParallelOps; i++)
    {
        status = logManager->CreateAsyncCreateLogContext(createOpArray[i]);
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("ParallelLogTest: CreateAsyncCreateLogContext[%d] Failed: %i\n", i, __LINE__);
            return status;
        }
    }

    RvdLogManager::AsyncEnumerateLogs::SPtr enumOp;
    status = logManager->CreateAsyncEnumerateLogsContext(enumOp);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("ParallelLogTest: CreateAsyncEnumerateLogsContext Failed: %i\n", __LINE__);
        return status;
    }

    RvdLogManager::AsyncOpenLog::SPtr openOpArray[noOfParallelOps];
    for (int i = 0; i < noOfParallelOps; i++)
    {
        status = logManager->CreateAsyncOpenLogContext(openOpArray[i]);
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("ParallelLogTest: CreateAsyncOpenLogContext[%d] Failed: %i\n", i, __LINE__);
            return status;
        }
    }

    RvdLogManager::AsyncDeleteLog::SPtr deleteOpArray[noOfParallelOps];
    for (int i = 0; i < noOfParallelOps; i++)
    {
        status = logManager->CreateAsyncDeleteLogContext(deleteOpArray[i]);
        if (!NT_SUCCESS(status))
        {
            KDbgPrintf("ParallelLogTest: CreateAsyncDeleteLogContext[%d] Failed: %i\n", i, __LINE__);
            return status;
        }
    }

    GUID diskIdGuid;
    // static GUID const logIdGuid     = {0x14575ca2, 0x1f3f, 0x42a8, {0x96, 0x71, 0xC2, 0xFc, 0xA7, 0xC4, 0x75, 0xa6}};

    KEvent getVolInfoCompleteEvent(FALSE, FALSE);
    KAsyncContextBase::CompletionCallback getVolInfoDoneCallback;
#ifdef _WIN64
    getVolInfoDoneCallback.Bind((PVOID)(&getVolInfoCompleteEvent), &StaticTestCallback);
#endif

    
#if !defined(PLATFORM_UNIX)
    KVolumeNamespace::VolumeInformationArray volInfo(KtlSystem::GlobalNonPagedAllocator());
    status = KVolumeNamespace::QueryVolumeListEx(volInfo, KtlSystem::GlobalNonPagedAllocator(), getVolInfoDoneCallback);
    if (!K_ASYNC_SUCCESS(status))
    {
        KDbgPrintf("ParallelLogTest: KVolumeNamespace::QueryVolumeListEx Failed: %i\n", __LINE__);
        return status;
    }
    getVolInfoCompleteEvent.WaitUntilSet();
    if (!NT_SUCCESS(LastStaticTestCallbackStatus))
    {
        KDbgPrintf("ParallelLogTest: KVolumeNamespace::QueryVolumeListEx Failed: %i\n", __LINE__);
        return LastStaticTestCallbackStatus;
    }

    // Find Drive C: volume GUID
    ULONG i1;
    for (i1 = 0; i1 < volInfo.Count(); i1++)
    {
        if (volInfo[i1].DriveLetter == testDriveLetter)
        {
            break;
        }
    }

    if (i1 == volInfo.Count())
    {
        KDbgPrintf("ParallelLogTest: KVolumeNamespace::QueryVolumeListEx did not return volume guid for drive C: %i\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }
    diskIdGuid = volInfo[i1].VolumeId;
#else
    diskIdGuid = RvdDiskLogConstants::HardCodedVolumeGuid();
#endif
    
    // Create many logs asynchronously
    RvdLog::SPtr logArray[noOfParallelOps];
    KEvent doneEventArray[noOfParallelOps];
    KGuid logGuidArray[noOfParallelOps];
    KAsyncContextBase::CompletionCallback doneCallbackArray[noOfParallelOps];

    for (int i = 0; i < noOfParallelOps; i++)
    {
        logGuidArray[i].CreateNew();
#ifdef _WIN64
        doneCallbackArray[i].Bind((PVOID)(&doneEventArray[i]), &StaticTestCallback);
#endif
    }

    KWString logType(KtlSystem::GlobalNonPagedAllocator(), L"RvdLog");
    KInvariant(NT_SUCCESS(logType.Status()));

    for (int i = 0; i < noOfParallelOps; i++)
    {
        doneEventArray[i].ResetEvent();
        createOpArray[i]->StartCreateLog(
            KGuid(diskIdGuid),
            RvdLogId(logGuidArray[i]),
            logType,
            (LONGLONG)1024*1024*256, // Create a small log
            logArray[i],
            nullptr,
            doneCallbackArray[i]);
    }

    for (int i = 0; i < noOfParallelOps; i++)
    {
        doneEventArray[i].WaitUntilSet();
        if (!NT_SUCCESS(createOpArray[i]->Status()))
        {
            KDbgPrintf(
                "ParallelLogTest: StartCreateLog Failed for log %ws: 0x%x at %i\n",
                (WCHAR*)KWString(KtlSystem::GlobalNonPagedAllocator(), logGuidArray[i]),
                createOpArray[i]->Status(),
                __LINE__);

            KInvariant(FALSE);
            return createOpArray[i]->Status();
        }
    }

    // Enumerate logs and verify that all created logs actually exist
    KArray<RvdLogId>    logIds(KtlSystem::GlobalNonPagedAllocator());
    doneEventArray[0].ResetEvent();
    enumOp->StartEnumerateLogs(
        KGuid(diskIdGuid),
        logIds,
        nullptr,
        doneCallbackArray[0]);

    doneEventArray[0].WaitUntilSet();
    if (!NT_SUCCESS(enumOp->Status()))
    {
        KDbgPrintf("ParallelLogTest: StartEnumerateLogs failed: %i\n", __LINE__);
        KInvariant(FALSE);
        return enumOp->Status();
    }

    if (logIds.Count() != noOfParallelOps)
    {
        KDbgPrintf("ParallelLogTest: StartEnumerateLogs returned wrong number of log file ids: %i\n", __LINE__);
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    BOOLEAN logFound;
    for (int i = 0; i < noOfParallelOps; i++)
    {
        logFound = FALSE;
        for (int j = 0; j < noOfParallelOps; j++)
        {
            if (logIds[j] == logGuidArray[i])
            {
                logFound = TRUE;
            }
        }
        if(logFound == FALSE)
        {
            KDbgPrintf(
                "ParallelLogTest: StartEnumerateLogs could not find log file %ws: %i\n",
                (WCHAR*)KWString(KtlSystem::GlobalNonPagedAllocator(), logGuidArray[i]), __LINE__);

            KInvariant(FALSE);
            return STATUS_UNSUCCESSFUL;
        }
    }
    
    // Test closure of all logs
    for (int i = 0; i < noOfParallelOps; i++)
    {
        if(!logArray[i].Reset())
        {
            KDbgPrintf(
                "ParallelLogTest: expected destruction did not occur for log %ws: %i\n",
                (WCHAR*)KWString(KtlSystem::GlobalNonPagedAllocator(), logGuidArray[i]), __LINE__);

            KInvariant(FALSE);
            return STATUS_UNSUCCESSFUL;
        }
    }

    // Asynchronous open of all logs - one open per log
    for (int i = 0; i < noOfParallelOps; i++)
    {
        doneEventArray[i].ResetEvent();
        openOpArray[i]->StartOpenLog(
            KGuid(diskIdGuid),
            RvdLogId(logGuidArray[i]),
            logArray[i],
            nullptr,
            doneCallbackArray[i]);
    }

    for (int i = 0; i < noOfParallelOps; i++)
    {
        doneEventArray[i].WaitUntilSet();
        if (!NT_SUCCESS(openOpArray[i]->Status()))
        {
            KDbgPrintf(
                "ParallelLogTest: StartOpenLog failed for log %ws: 0x%X: %i\n",
                (WCHAR*)KWString(KtlSystem::GlobalNonPagedAllocator(), logGuidArray[i]),
                openOpArray[i]->Status(),
                __LINE__);

            KInvariant(FALSE);
            return openOpArray[i]->Status();
        }
    }

    // Close all logs
    for (int i = 0; i < noOfParallelOps; i++)
    {
        if(!logArray[i].Reset())
        {
            KDbgPrintf(
                "ParallelLogTest: expected destruction did not occur for log %ws: %i\n",
                (WCHAR*)KWString(KtlSystem::GlobalNonPagedAllocator(), logGuidArray[i]), __LINE__);

            KInvariant(FALSE);
            return STATUS_UNSUCCESSFUL;
        }
    }

    // Asynchronous open of one log - many opens of one log
    for (int i = 0; i < noOfParallelOps; i++)
    {
        doneEventArray[i].ResetEvent();
        openOpArray[i]->Reuse();
        openOpArray[i]->StartOpenLog(
            KGuid(diskIdGuid),
            RvdLogId(logGuidArray[0]),
            logArray[i],
            nullptr,
            doneCallbackArray[i]);
    }

    for (int i = 0; i < noOfParallelOps; i++)
    {
        doneEventArray[i].WaitUntilSet();
        if (!NT_SUCCESS(openOpArray[i]->Status()))
        {
            KDbgPrintf(
                "ParallelLogTest: StartOpenLog failed for log %ws: %i %x\n",
                (WCHAR*)KWString(KtlSystem::GlobalNonPagedAllocator(), logGuidArray[0]), __LINE__, openOpArray[i]->Status());

            KInvariant(FALSE);
            return openOpArray[i]->Status();
        }
    }

    // Release all references to the opened log
    for (int i = 0; i < noOfParallelOps; i++)
    {
        logArray[i] = nullptr;
    }

    // Delete all created logs asynchronously
    for (int i = 0; i < noOfParallelOps; i++)
    {
        doneEventArray[i].ResetEvent();
        deleteOpArray[i]->StartDeleteLog(
            KGuid(diskIdGuid),
            RvdLogId(logGuidArray[i]),
            nullptr,
            doneCallbackArray[i]);
    }

    for (int i = 0; i < noOfParallelOps; i++)
    {
        doneEventArray[i].WaitUntilSet();
        if (!NT_SUCCESS(deleteOpArray[i]->Status()))
        {
            KDbgPrintf(
                "ParallelLogTest: StartDeleteLog failed for log %ws: %i\n",
                (WCHAR*)KWString(KtlSystem::GlobalNonPagedAllocator(), logGuidArray[i]), __LINE__);

            KInvariant(FALSE);
            return deleteOpArray[i]->Status();
        }
    }

    logManager->Deactivate();
    activateEvent.WaitUntilSet();
    return STATUS_SUCCESS;
}
