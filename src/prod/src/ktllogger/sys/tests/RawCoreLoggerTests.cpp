// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#if defined(PLATFORM_UNIX)
#include <boost/test/unit_test.hpp>
#include "boost-taef.h"
#endif
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>

#include <ktllogger.h>
#include <InternalKtlLogger.h>

#include "KtlLoggerTests.h"

#if !defined(PLATFORM_UNIX)
#include "WexTestClass.h"
#endif

#include "CloseSync.h"

#if !defined(PLATFORM_UNIX)
using namespace WEX::Logging;
using namespace WEX::Common;
using namespace WEX::TestExecution;
#endif

#include "VerifyQuiet.h"

#if KTL_USER_MODE
 #define _printf(...)   printf(__VA_ARGS__)
// #define _printf(...)   KDbgPrintf(__VA_ARGS__)

extern volatile LONGLONG gs_AllocsRemaining;
#else
 #define _printf(...)   KDbgPrintf(__VA_ARGS__)
 #define wprintf(...)    KDbgPrintf(__VA_ARGS__)
#endif

#define ALLOCATION_TAG 'LLKT'
 
extern KAllocator* g_Allocator;

NTSTATUS CoreLoggerRead(
    RvdLogStream::SPtr LogStream,
    RvdLogStream::AsyncReadContext::ReadType Type,
    RvdLogAsn& Asn,
    ULONGLONG& Version,
    KBuffer::SPtr& ReadMetadata,
    KIoBuffer::SPtr& ReadIoBuffer
    )
{
    NTSTATUS status;
    KSynchronizer sync;
    RvdLogStream::AsyncReadContext::SPtr readOp;

    status = LogStream->CreateAsyncReadContext(readOp);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    readOp->StartRead(Asn, Type, &Asn, &Version, ReadMetadata, ReadIoBuffer, NULL, sync);
    status = sync.WaitForCompletion();

    return(status);
}

NTSTATUS CoreLoggerWrite(
    PWCHAR Tag,
    RvdLogStream::SPtr LogStream,
    RvdLogAsn Asn,
    ULONG DataSize,
    ULONGLONG Version = 1,
    BOOLEAN ForceStreamCheckpoint = FALSE
    )
{
    NTSTATUS status;
    KSynchronizer sync;
    KBuffer::SPtr metadata;
    KIoBuffer::SPtr data;
    PVOID p;

    if (DataSize != 0)
    {
        status = KIoBuffer::CreateSimple(DataSize, data, p, *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    } else {
        data = nullptr;
    }

    status = KBuffer::Create(256, metadata, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    RvdLogStream::AsyncWriteContext::SPtr writeOp;
    RvdLogStreamImp::AsyncWriteStream::SPtr writeOpImp;

    status = LogStream->CreateAsyncWriteContext(writeOp);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    writeOpImp = up_cast<RvdLogStreamImp::AsyncWriteStream>(writeOp);
    writeOpImp->_ForceStreamCheckpoint = ForceStreamCheckpoint;

    writeOp->StartWrite(Asn, Version, metadata, data, nullptr, sync);
    status = sync.WaitForCompletion();

    if (Tag != NULL)
    {
        //printf("%ws: _DebugFileWriteOffset0 %ld, _DebugFileWriteOffset1 %ld\n", Tag, writeOpImp->_DebugFileWriteOffset0, writeOpImp->_DebugFileWriteOffset1);
    }
    return(status);
}
    
VOID TruncateToBadLogLowLsnTest(
    KGuid DiskId
    )
{

    //
    // This test case is to simulate a bug where stream recovery fails
    // because the Log low LSN is higher than the stream low LSN. The
    // scenario involves a container with two streams.

    // One stream has only a single record such that its stream lowLSN == highLSN but
    // the highLSN != nextLSN. For example:
    //
    // StreamId: {49fdfa2c-15aa-4ebe-9ba3-4d1c45facf6f}; LowestLsn: 368435200; HighestLsn:368435200; NextLsn: 368660480
    // 
    //
    // Another stream has a set of records where the stream lowLSN is
    // higher than the lowLSN of the first stream. For example:
    //
    // StreamId: {333dfb8a-2209-4290-a53b-71a20b115306}; LowestLsn: 368926720; HighestLsn:368926720; NextLsn: 368926720
    //
    //
    // In the failing scenario the second stream is truncated which
    // causes the log lowLSN to be increased to 368926720. This is the
    // bug - the log lowLSN should not have been changed. Next the
    // first stream is opened and fails.
    //

    NTSTATUS status;
    KSynchronizer activateSync;
    KSynchronizer sync;
    RvdLogManager::SPtr logManager;

    //
    // Get a log manager
    //
    status = RvdLogManager::Create(KTL_TAG_LOGGER, KtlSystem::GlobalNonPagedAllocator(), logManager);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    status = logManager->Activate(nullptr, activateSync);
    VERIFY_IS_TRUE(K_ASYNC_SUCCESS(status));

    logManager->RegisterVerificationCallback(DiskId, &VerifyRawRecordCallback);

    //
    // Create a container and then 2 streams
    //
    KGuid logContainerGuid;
    RvdLogId logContainerId;
    RvdLogManager::AsyncCreateLog::SPtr createLogOp;
    RvdLog::SPtr logContainer;

    logContainerGuid.CreateNew();
    logContainerId = static_cast<RvdLogId>(logContainerGuid);
    
    KGuid emptyStreamGuid;
    RvdLogStreamId emptyStreamId;
    RvdLogStream::SPtr emptyLogStream;
    RvdLogStreamImp::SPtr emptyLogStreamImp;


    emptyStreamGuid.CreateNew();
    emptyStreamId = static_cast<RvdLogStreamId>(emptyStreamGuid);

    KGuid otherStreamGuid;
    RvdLogStreamId otherStreamId;
    RvdLogStream::SPtr otherLogStream;
    RvdLogStreamImp::SPtr otherLogStreamImp;

    otherStreamGuid.CreateNew();
    otherStreamId = static_cast<RvdLogStreamId>(otherStreamGuid);

    RvdLogAsn highestAsn = 9*0x10000;
    ULONG lastOtherAsn;

    {
        KAsyncEvent closeEvent;
        KAsyncEvent::WaitContext::SPtr waitForClose;

        status = closeEvent.CreateWaitContext(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator(), waitForClose);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        
        RvdLog::AsyncCreateLogStreamContext::SPtr createStreamOp;
    
        status = logManager->CreateAsyncCreateLogContext(createLogOp);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KWString logType(KtlSystem::GlobalNonPagedAllocator(), L"TestLogType");
        KInvariant(NT_SUCCESS(logType.Status()));
        
        createLogOp->StartCreateLog(
            DiskId,
            logContainerId,
            logType,
            DefaultTestLogFileSize,
            0,          // Use default max streams
            0,          // Use default max record size
            logContainer,
            nullptr,
            sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = logContainer->CreateAsyncCreateLogStreamContext(createStreamOp);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        createStreamOp->StartCreateLogStream(otherStreamId, 
                                             DiskId,
                                             otherLogStream,
                                             nullptr,
                                             sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));


        //
        // Now write a bunch of stuff to the empty stream and then a bunch more stuff to the other stream. Next
        // truncate the empty stream. At this point the streams should be staged correctly. Close the streams.
        //
        RvdLogStream::AsyncWriteContext::SPtr emptyStreamWriteOp;
        RvdLogStream::AsyncWriteContext::SPtr otherStreamWriteOp;

        status = otherLogStream->CreateAsyncWriteContext(otherStreamWriteOp);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        ULONG otherAsn = 1;

        for (ULONG i = 0; i < 100; i++)
        {
            status = CoreLoggerWrite(NULL,
                                     otherLogStream,
                                        otherAsn,
                                        0x4000);              // 64K
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            otherAsn++;
        }

        lastOtherAsn = otherAsn;

        createStreamOp->Reuse();
        createStreamOp->StartCreateLogStream(emptyStreamId, 
                                             DiskId,
                                             emptyLogStream,
                                             nullptr,
                                             sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = emptyLogStream->CreateAsyncWriteContext(emptyStreamWriteOp);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = CoreLoggerWrite(L"empty 12",
                            emptyLogStream,
                            12,
                            0);              // 64K
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = CoreLoggerWrite(L"empty 14",
                            emptyLogStream,
                            14,
                            0);              // 64K
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        emptyLogStream->Truncate(15, 15);

        status = CoreLoggerWrite(L"empty 16",
                            emptyLogStream,
                            16,
                            0);              // 64K
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        for (ULONG i = 0; i < 10; i++)
        {
            status = CoreLoggerWrite(NULL,
                                     otherLogStream,
                                     otherAsn,
                                     0x1000);              // 64K
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            otherAsn++;
        }

        // 
        // Close the container and streams
        //
        createLogOp.Reset();
        createStreamOp.Reset();
        emptyStreamWriteOp.Reset();
        otherStreamWriteOp.Reset();
        emptyLogStream.Reset();
        otherLogStream.Reset();
        logContainer->SetShutdownEvent(&closeEvent);
        logContainer.Reset();

        waitForClose->StartWaitUntilSet(NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    KNt::Sleep(250);

    //
    // Now reopen the other stream and truncate it. This should cause the log lowLSN to go higher than
    // the empty stream LSN. Reopen the empty stream and verify that it opens correctly.
    //
    //
    {
        KAsyncEvent closeEvent;
        KAsyncEvent::WaitContext::SPtr waitForClose;

        status = closeEvent.CreateWaitContext(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator(), waitForClose);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        RvdLogManager::AsyncOpenLog::SPtr openLogOp;
        status = logManager->CreateAsyncOpenLogContext(openLogOp);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        openLogOp->StartOpenLog(DiskId, logContainerId, logContainer, nullptr, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        RvdLog::AsyncOpenLogStreamContext::SPtr openStreamOp;
        status = logContainer->CreateAsyncOpenLogStreamContext(openStreamOp);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        openStreamOp->StartOpenLogStream(otherStreamId,
                                            otherLogStream,
                                            nullptr,
                                            sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        otherLogStream->Truncate(lastOtherAsn+5, lastOtherAsn+5);

        openStreamOp->Reuse();
        openStreamOp->StartOpenLogStream(emptyStreamId,
                                            emptyLogStream,
                                            nullptr,
                                            sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // Now verify that we can read the empty stream record 16
        //
        RvdLogStream::AsyncReadContext::SPtr readOp;

        status = emptyLogStream->CreateAsyncReadContext(readOp);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        ULONGLONG version;
        KBuffer::SPtr readMetadata;
        KIoBuffer::SPtr readIoBuffer;

        readOp->StartRead(16, &version, readMetadata, readIoBuffer, NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // All done, cleanup
        //
        readOp.Reset();
        openLogOp.Reset();
        openStreamOp.Reset();
        emptyLogStream.Reset();
        otherLogStream.Reset();
        logContainer->SetShutdownEvent(&closeEvent);
        logContainer.Reset();

        waitForClose->StartWaitUntilSet(NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    // TODO: Include cases with a stranded stream checkpoint record

    KNt::Sleep(250);

    RvdLogManager::AsyncDeleteLog::SPtr deleteLogOp;

    status = logManager->CreateAsyncDeleteLogContext(deleteLogOp);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    deleteLogOp->StartDeleteLog(DiskId, logContainerId, nullptr, sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    logManager->Deactivate();
    activateSync.WaitForCompletion();   
    logManager.Reset();
}

VOID StreamCheckpointBoundaryTest(
    KGuid DiskId
    )
{

    //
    // This test case is to simulate a bug where stream checkpoint record is built
    // incorrectly and causes a bugcheck. The boundary condition is where all ASN entries exactly fit within
    // the first set of checkpoint record segments while the last ASN segment has zero LSN entries.
    //

    NTSTATUS status;
    KSynchronizer activateSync;
    KSynchronizer sync;
    RvdLogManager::SPtr logManager;

    //
    // Get a log manager
    //
    status = RvdLogManager::Create(KTL_TAG_LOGGER, KtlSystem::GlobalNonPagedAllocator(), logManager);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    status = logManager->Activate(nullptr, activateSync);
    VERIFY_IS_TRUE(K_ASYNC_SUCCESS(status));

    logManager->RegisterVerificationCallback(DiskId, &VerifyRawRecordCallback);

    //
    // Create a container and a stream
    //
    KGuid logContainerGuid;
    RvdLogId logContainerId;
    RvdLogManager::AsyncCreateLog::SPtr createLogOp;
    RvdLog::SPtr logContainer;

    logContainerGuid.CreateNew();
    logContainerId = static_cast<RvdLogId>(logContainerGuid);
    
    KGuid otherStreamGuid;
    RvdLogStreamId otherStreamId;
    RvdLogStream::SPtr otherLogStream;
    RvdLogStreamImp::SPtr otherLogStreamImp;

    otherStreamGuid.CreateNew();
    otherStreamId = static_cast<RvdLogStreamId>(otherStreamGuid);

    RvdLogAsn highestAsn = 9*0x10000;
    {
        RvdLog::AsyncCreateLogStreamContext::SPtr createStreamOp;
    
        status = logManager->CreateAsyncCreateLogContext(createLogOp);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KWString logType(KtlSystem::GlobalNonPagedAllocator(), L"TestLogType");
        createLogOp->StartCreateLog(
            DiskId,
            logContainerId,
            logType,
            DefaultTestLogFileSize,
            4,          // 4 streams
            0x100000,   // 1MB record size
            logContainer,
            nullptr,
            sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = logContainer->CreateAsyncCreateLogStreamContext(createStreamOp);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        createStreamOp->StartCreateLogStream(otherStreamId, 
                                             DiskId,
                                             otherLogStream,
                                             nullptr,
                                             sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));


        //
        // Configure the ASN and LSN tables exactly correctly
        //
        RvdLogStreamImp::SPtr logStreamImp;
        logStreamImp = static_cast<RvdLogStreamImp*>(otherLogStream.RawPtr());
        LogStreamDescriptor* logStreamDesc = &logStreamImp->GetLogStreamDescriptor();
        ULONG segmentSize = 0x20000;

        //
        // Add exactly the right number of ASN and LSN entries
        //
        KIoBuffer::SPtr recordIoBuffer;
        ULONG recordSize;
        RvdLogAsn asn = 1;
        ULONGLONG version = 1;
        RvdLogLsn lsn = 0x1000000;

        for (ULONG i = 0; i < 0x2fEE; i++)
        {
            RvdAsnIndexEntry::SPtr          resultEntry;
            RvdAsnIndexEntry::SavedState    entryState;
            BOOLEAN                         previousIndexEntryWasStored;

            RvdAsnIndexEntry::SPtr          newEntry = _new(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator()) RvdAsnIndexEntry(
                asn,
                version,
                0x10000,
                RvdLogStream::RecordDisposition::eDispositionPersisted,
                lsn
                );
            VERIFY_IS_TRUE((newEntry != nullptr) ? TRUE : FALSE);

            ULONGLONG dontCare;
            logStreamDesc->AsnIndex.AddOrUpdate(newEntry, resultEntry, entryState, previousIndexEntryWasStored, dontCare);
            asn = asn.Get() + 1;
            lsn = lsn.Get() + 1;
        }

        for (ULONG i = 0; i < 0x2FF8; i++)
        {
            RvdLogLsnEntry lsnEntry;

            status = logStreamDesc->LsnIndex.AddHigherLsnRecord(lsn, 0x1000, 0x10000);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            lsn = lsn.Get() + 0x10;
        }

        //
        // Now go and prepare a stream checkpoint record
        //
        RvdLogConfig rvdlogConfig(4, 0x100000);

        status = KIoBuffer::CreateEmpty(recordIoBuffer, *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        logStreamDesc->BuildStreamCheckPointRecordTables(*g_Allocator, *recordIoBuffer, rvdlogConfig, recordSize);

        //
        // Finally validate that all segments are formed correctly
        //
        VERIFY_IS_TRUE(recordIoBuffer->QuerySize() == 0x91000);

        ULONG numberOfSegments = recordSize / segmentSize;
        if (numberOfSegments * segmentSize < recordSize)
        {
            numberOfSegments++;
        }
        VERIFY_IS_TRUE(numberOfSegments == 5);

        KIoBufferStream         stream(*recordIoBuffer, 0);

        for (ULONG segment = 0; segment < numberOfSegments; segment++)
        {
            KAssert(stream.GetBufferPointerRange() >= sizeof(RvdLogUserStreamCheckPointRecordHeader));
            RvdLogUserStreamCheckPointRecordHeader* const hdr = (RvdLogUserStreamCheckPointRecordHeader*)stream.GetBufferPointer();

            VERIFY_IS_TRUE(hdr->SegmentNumber == segment);
            VERIFY_IS_TRUE(hdr->NumberOfSegments == numberOfSegments);
            stream.Skip(segmentSize);
        }

        // 
        // Close the container and streams
        //
        otherLogStream.Reset();
        logContainer.Reset();
    }

    KNt::Sleep(250);

    RvdLogManager::AsyncDeleteLog::SPtr deleteLogOp;

    status = logManager->CreateAsyncDeleteLogContext(deleteLogOp);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    deleteLogOp->StartDeleteLog(DiskId, logContainerId, nullptr, sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    logManager->Deactivate();
    activateSync.WaitForCompletion();   
    logManager.Reset();
}


VOID QueryLogTypeTest(
    KGuid DiskId
    )
{
    NTSTATUS status;
    KSynchronizer activateSync;
    KSynchronizer sync;
    RvdLogManager::SPtr logManager;

    //
    // Get a log manager
    //
    status = RvdLogManager::Create(KTL_TAG_LOGGER, KtlSystem::GlobalNonPagedAllocator(), logManager);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = logManager->Activate(nullptr, activateSync);
    VERIFY_IS_TRUE(K_ASYNC_SUCCESS(status));

    logManager->RegisterVerificationCallback(DiskId, &VerifyRawRecordCallback);

    //
    // Create a container and a stream
    //
    KGuid logContainerGuid;
    RvdLogId logContainerId;
    KWString logType(KtlSystem::GlobalNonPagedAllocator(), L"TestLogType");
    KWString readCreatedLogType(KtlSystem::GlobalNonPagedAllocator());
    KWString readOpenedLogType(KtlSystem::GlobalNonPagedAllocator());

    logContainerGuid.CreateNew();
    logContainerId = static_cast<RvdLogId>(logContainerGuid);

    //
    // Test case 1: Create log with specific log type and verify QueryLogType
    //              returns correctly.
    //
    {
        RvdLog::SPtr logContainer;
        RvdLogManager::AsyncCreateLog::SPtr createLogOp;

        status = logManager->CreateAsyncCreateLogContext(createLogOp);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        createLogOp->StartCreateLog(
            DiskId,
            logContainerId,
            logType,
            DefaultTestLogFileSize,
            4,          // 4 streams
            0x100000,   // 1MB record size
            logContainer,
            nullptr,
            sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // Query log type
        //
        logContainer->QueryLogType(readCreatedLogType);
        VERIFY_IS_TRUE((readCreatedLogType == logType) ? TRUE : FALSE);

        logContainer.Reset();
    }

    KNt::Sleep(250);

    //
    // Test case 2: Reopen log with specific log type and verify QueryLogType
    //              returns correctly.
    //
    {
        RvdLog::SPtr logContainer;
        RvdLogManager::AsyncOpenLog::SPtr openLogOp;
        status = logManager->CreateAsyncOpenLogContext(openLogOp);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        openLogOp->StartOpenLog(DiskId,
            logContainerId,
            logContainer,
            nullptr,
            sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // Query log type
        //
        logContainer->QueryLogType(readOpenedLogType);
        VERIFY_IS_TRUE((readOpenedLogType == logType) ? TRUE : FALSE);
        logContainer.Reset();
    }

    KNt::Sleep(250);

    RvdLogManager::AsyncDeleteLog::SPtr deleteLogOp;

    status = logManager->CreateAsyncDeleteLogContext(deleteLogOp);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    deleteLogOp->StartDeleteLog(DiskId, logContainerId, nullptr, sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    // Max length is 16 chars - only 15 char plus nul saved
    KWString longLogType(KtlSystem::GlobalNonPagedAllocator(), L"12345678901234567890");
    KWString longCompareLogType(KtlSystem::GlobalNonPagedAllocator(), L"123456789012345");
    KWString longCreatedLogType(KtlSystem::GlobalNonPagedAllocator());
    KWString longOpenedLogType(KtlSystem::GlobalNonPagedAllocator());

    logContainerGuid.CreateNew();
    logContainerId = static_cast<RvdLogId>(logContainerGuid);

    //
    // Test case 3: Create log with long log type and verify QueryLogType
    //              returns correctly.
    //
    {
        RvdLog::SPtr logContainer;
        RvdLogManager::AsyncCreateLog::SPtr createLogOp;

        status = logManager->CreateAsyncCreateLogContext(createLogOp);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        createLogOp->StartCreateLog(
            DiskId,
            logContainerId,
            longLogType,
            DefaultTestLogFileSize,
            4,          // 4 streams
            0x100000,   // 1MB record size
            logContainer,
            nullptr,
            sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // Query log type
        //
        logContainer->QueryLogType(longCreatedLogType);
        VERIFY_IS_TRUE((longCreatedLogType == longCompareLogType) ? TRUE : FALSE);

        logContainer.Reset();
    }

    KNt::Sleep(250);

    //
    // Test case 4: Reopen log with long log type and verify QueryLogType
    //              returns correctly.
    //
    {
        RvdLog::SPtr logContainer;
        RvdLogManager::AsyncOpenLog::SPtr openLogOp;
        status = logManager->CreateAsyncOpenLogContext(openLogOp);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        openLogOp->StartOpenLog(DiskId,
            logContainerId,
            logContainer,
            nullptr,
            sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // Query log type
        //
        logContainer->QueryLogType(longOpenedLogType);
        VERIFY_IS_TRUE((longOpenedLogType == longCompareLogType) ? TRUE : FALSE);

        logContainer.Reset();
    }

    KNt::Sleep(250);

    deleteLogOp->Reuse();
    deleteLogOp->StartDeleteLog(DiskId, logContainerId, nullptr, sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));



    logManager->Deactivate();
    activateSync.WaitForCompletion();
    logManager.Reset();
}

VOID DeleteRecordsTest(
    KGuid DiskId
    )
{
    NTSTATUS status;
    KSynchronizer activateSync;
    KSynchronizer sync;
    RvdLogManager::SPtr logManager;

    //
    // Get a log manager
    //
    status = RvdLogManager::Create(KTL_TAG_LOGGER, KtlSystem::GlobalNonPagedAllocator(), logManager);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = logManager->Activate(nullptr, activateSync);
    VERIFY_IS_TRUE(K_ASYNC_SUCCESS(status));

    logManager->RegisterVerificationCallback(DiskId, &VerifyRawRecordCallback);

    //
    // Create a container and a stream
    //
    KGuid logContainerGuid;
    RvdLogId logContainerId;
    RvdLogManager::AsyncCreateLog::SPtr createLogOp;
    RvdLog::SPtr logContainer;

    logContainerGuid.CreateNew();
    logContainerId = static_cast<RvdLogId>(logContainerGuid);

    KGuid otherStreamGuid;
    RvdLogStreamId otherStreamId;
    RvdLogStream::SPtr otherLogStream;
    RvdLogStreamImp::SPtr otherLogStreamImp;

    otherStreamGuid.CreateNew();
    otherStreamId = static_cast<RvdLogStreamId>(otherStreamGuid);

    RvdLogAsn asn = 0x1000;
    {
        RvdLog::AsyncCreateLogStreamContext::SPtr createStreamOp;

        status = logManager->CreateAsyncCreateLogContext(createLogOp);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KWString logType(KtlSystem::GlobalNonPagedAllocator(), L"TestLogType");
        KInvariant(NT_SUCCESS(logType.Status()));
        
        createLogOp->StartCreateLog(
            DiskId,
            logContainerId,
            logType,
            DefaultTestLogFileSize,
            4,          // 4 streams
            0x100000,   // 1MB record size
            logContainer,
            nullptr,
            sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = logContainer->CreateAsyncCreateLogStreamContext(createStreamOp);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        createStreamOp->StartCreateLogStream(otherStreamId,
            DiskId,
            otherLogStream,
            nullptr,
            sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));


        //
        // Test 1: Write 1000 records and verify that all are in the QueryRecords list
        //
        const ULONG recordCount = 100;
        const ULONGLONG recordVersion = 2;
        {
            for (ULONG i = 0; i < recordCount; i++)
            {
                RvdLogAsn asn1 = (i * 5) + 5;

                status = CoreLoggerWrite(L"Setup 1000 records", otherLogStream, asn1, 0x4000, recordVersion);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }

            RvdLogAsn lowAsn;
            RvdLogAsn highAsn;
            RvdLogAsn truncationAsn;

            status = otherLogStream->QueryRecordRange(&lowAsn, &highAsn, &truncationAsn);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            VERIFY_IS_TRUE(lowAsn.Get() == 5);
            VERIFY_IS_TRUE(highAsn.Get() == (((recordCount - 1) * 5) + 5));
            VERIFY_IS_TRUE(truncationAsn.Get() == 0);

            KArray<RvdLogStream::RecordMetadata> records(*g_Allocator);
            VERIFY_IS_TRUE(NT_SUCCESS(records.Status()));

            status = otherLogStream->QueryRecords(lowAsn, highAsn, records);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            VERIFY_IS_TRUE(records.Count() == recordCount);
            for (ULONG i = 0; i < recordCount; i++)
            {
                VERIFY_IS_TRUE((records[i].Asn == (i * 5) + 5) ? true : false);
                VERIFY_IS_TRUE((records[i].Version == recordVersion) ? true : false);
            }
        }

        //
        // Test 2: Delete 500 alternating records from list and verify that only records not deleted
        //         are in the QueryRecords list
        //
        {
            for (ULONG i = 0; i < recordCount; i = i + 2)
            {
                RvdLogAsn asn1 = (i * 5) + 5;

                status = otherLogStream->DeleteRecord(asn1, recordVersion);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }

            RvdLogAsn lowAsn;
            RvdLogAsn highAsn;
            RvdLogAsn truncationAsn;

            status = otherLogStream->QueryRecordRange(&lowAsn, &highAsn, &truncationAsn);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            VERIFY_IS_TRUE(lowAsn.Get() == 10);
            VERIFY_IS_TRUE(highAsn.Get() == (((recordCount - 1) * 5) + 5));
            VERIFY_IS_TRUE(truncationAsn.Get() == 0);

            KArray<RvdLogStream::RecordMetadata> records(*g_Allocator);
            VERIFY_IS_TRUE(NT_SUCCESS(records.Status()));

            status = otherLogStream->QueryRecords(lowAsn, highAsn, records);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            VERIFY_IS_TRUE(records.Count() == recordCount / 2);
            for (ULONG i = 0; i < recordCount / 2; i++)
            {
                VERIFY_IS_TRUE((records[i].Asn == ((i + 1) * 10)) ? true : false);
                VERIFY_IS_TRUE(records[i].Version == recordVersion);
            }
        }

        //
        // Test 3: Try to delete a ASN that doesn't exist and an ASN with a bad version
        //
        {
            RvdLogAsn asn1 = 35;
            status = otherLogStream->DeleteRecord(asn1, recordVersion);
            VERIFY_IS_TRUE(status == STATUS_NOT_FOUND);

            asn1 = 50;
            status = otherLogStream->DeleteRecord(asn1, recordVersion+1);
            VERIFY_IS_TRUE(status == STATUS_NOT_FOUND);
        }

        //
        // Test 4: Try to read all records using ReadExact and verify that only records not deleted are
        //         read successfully.
        //
        {
            RvdLogAsn asn1;

            for (ULONG i = 0; i < recordCount; i++)
            {
                KBuffer::SPtr metadata;
                KIoBuffer::SPtr iodata;
                ULONGLONG version;
                NTSTATUS expectedStatus;

                asn1 = (i * 5) + 5;
                expectedStatus = ((asn1.Get() % 10) == 0) ? STATUS_SUCCESS : STATUS_NOT_FOUND;

                status = CoreLoggerRead(otherLogStream, RvdLogStream::AsyncReadContext::ReadExactRecord, asn1, version, metadata, iodata);
                VERIFY_IS_TRUE(status == expectedStatus);
            }
        }

        //
        // Test 5: Starting at first record, perform ReadNext and verify that only records
        //         that are not deleted are returned
        //
        {
            RvdLogAsn asn1 = 10;

            for (ULONG i = 0; i < (recordCount / 2)-1; i++)
            {
                KBuffer::SPtr metadata;
                KIoBuffer::SPtr iodata;
                ULONGLONG version;

                status = CoreLoggerRead(otherLogStream, RvdLogStream::AsyncReadContext::ReadNextRecord, asn1, version, metadata, iodata);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                VERIFY_IS_TRUE((asn1 == ((i + 2) * 10)) ? true : false);
                VERIFY_IS_TRUE(version == recordVersion);
            }
        }

        //
        // Test 6: Starting at first record, perform ReadContaining with the deleted ASNs and verify that only records
        //         that are not deleted are returned
        //
        {
            RvdLogAsn asn1 = 15;

            for (ULONG i = 0; i < recordCount / 2; i++)
            {
                KBuffer::SPtr metadata;
                KIoBuffer::SPtr iodata;
                ULONGLONG version;

                status = CoreLoggerRead(otherLogStream, RvdLogStream::AsyncReadContext::ReadContainingRecord, asn1, version, metadata, iodata);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                VERIFY_IS_TRUE((asn1 == ((i + 1) * 10)) ? true : false);
                VERIFY_IS_TRUE(version == recordVersion);

                asn1.Set(asn1.Get() + 15);
            }
        }

        //
        // Test 7: Delete all remaining records and verify that space in the log is getting freed.
        //
        {
            ULONGLONG totalSpace, freeSpace;
            ULONGLONG freeSpaceBefore;

            logContainer->QuerySpaceInformation(&totalSpace, &freeSpaceBefore);

            for (ULONG i = 0; i < recordCount / 2; i = i + 2)
            {
                RvdLogAsn asn1 = (i + 1) * 10;

                status = otherLogStream->DeleteRecord(asn1, recordVersion);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                
                //
                // Give implicit truncate a chance to run
                //
                ULONG loops = 0;
                do
                {
                    KNt::Sleep(100);
                    logContainer->QuerySpaceInformation(&totalSpace, &freeSpace);
                } while ((freeSpace <= freeSpaceBefore) && (++loops < 10));

                VERIFY_IS_TRUE(freeSpace > freeSpaceBefore);
                freeSpace = freeSpaceBefore;
            }
        }

        //
        // Test 8: Out of order ASNs in LSN space, does it clean up blocked space if records
        //         blocking space to be freed are deleted ?
        //
        {
            ULONGLONG totalSpace, freeSpace;
            ULONGLONG freeSpaceBefore;
            ULONGLONG freeSpaceInitially;
            RvdLogAsn asn1;

            logContainer->QuerySpaceInformation(&totalSpace, &freeSpaceInitially);

            asn1 = 10000;
            status = CoreLoggerWrite(L"Write 10000", otherLogStream, asn1, 0x4000, recordVersion);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            asn1 = 11000;
            status = CoreLoggerWrite(L"Write 11000", otherLogStream, asn1, 0x4000, recordVersion);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            asn1 = 12000;
            status = CoreLoggerWrite(L"Write 12000", otherLogStream, asn1, 0x4000, recordVersion);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            asn1 = 13000;
            status = CoreLoggerWrite(L"Write 13000", otherLogStream, asn1, 0x4000, recordVersion);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            asn1 = 11500;
            status = CoreLoggerWrite(L"Write 11500", otherLogStream, asn1, 0x4000, recordVersion);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            asn1 = 11501;
            status = CoreLoggerWrite(L"Write 11501", otherLogStream, asn1, 0x4000, recordVersion);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            asn1 = 14000;
            status = CoreLoggerWrite(L"Write 14000", otherLogStream, asn1, 0x4000, recordVersion);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            //
            // Truncating to 11600 will logically free 11500 and 11501 but their space
            // cannot be reclaimed since 12000 and 13000 are before it
            //
            logContainer->QuerySpaceInformation(&totalSpace, &freeSpaceBefore);

            otherLogStream->Truncate(11600, 11600);

            ULONG loops = 0;
            do
            {
                KNt::Sleep(100);
                logContainer->QuerySpaceInformation(&totalSpace, &freeSpace);
            } while ((freeSpace <= freeSpaceBefore) && (++loops < 10));

            VERIFY_IS_TRUE(freeSpace > freeSpaceBefore);
            freeSpaceBefore = freeSpace;

            //
            // Deleting 13000 still doesn't free up space since 12000 is in the way
            //
            status = otherLogStream->DeleteRecord(13000, recordVersion);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            loops = 0;
            do
            {
                KNt::Sleep(100);
                logContainer->QuerySpaceInformation(&totalSpace, &freeSpace);
            } while ((freeSpace != freeSpaceBefore) && (++loops < 10));

            VERIFY_IS_TRUE(freeSpace == freeSpaceBefore);

            //
            // Finally deleting 12000 will clear out old records and we should see the
            // free space go back to the initial free space
            //
            status = otherLogStream->DeleteRecord(12000, recordVersion);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            //
            // Give implicit truncate write a chance to run
            //
            loops = 0;
            do
            {
                KNt::Sleep(100);
                logContainer->QuerySpaceInformation(&totalSpace, &freeSpace);
            } while ((freeSpace <= freeSpaceBefore) && (++loops < 10));

            VERIFY_IS_TRUE(freeSpace > freeSpaceBefore);
        }

        // 
        // Close the container and streams
        //
        otherLogStream.Reset();
        logContainer.Reset();
    }

    KNt::Sleep(250);

    RvdLogManager::AsyncDeleteLog::SPtr deleteLogOp;

    status = logManager->CreateAsyncDeleteLogContext(deleteLogOp);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    deleteLogOp->StartDeleteLog(DiskId, logContainerId, nullptr, sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    logManager->Deactivate();
    activateSync.WaitForCompletion();
    logManager.Reset();
}


VOID StreamCheckpointAtEndOfLogTest(
    KGuid DiskId
    )
{

    //
    // This test case is to simulate a bug where multi-part stream checkpoint records are the last records
    // in the log. The bug was that the HighestCompletedLSN was not being set correctly in the stream checkpoint
    // record segemnts beyond the first segment.
    //
    // A second bug occured in the stream recovery. Log recovery will
    // recover the highest LSN as the last record in the log; in the
    // case below that is 60a1000. However stream recovery and stream
    // asyncwrite will only add the first LSN of the stream checkpoint
    // record, in this case 6061000. Stream recovery validates that the
    // correct highest LSN (60a1000) is recovered.
    //
    // See the first stream checkpoint segment has correct HighLSN but the last ones do not:
    // ---R     6055000 length         c000 expectnext : 6061000 highlsn : 6054000 chkptlsn : 5eb5000 prevlsn : 6054000
    // ---S     6061000 length        20000 expectnext : 6081000 highlsn : 6054000 chkptlsn : 5eb5000 prevlsn : 6055000
    // --LS     6081000 length        20000 expectnext : 60a1000 HIGHLSN : 6074000 chkptlsn : 5eb5000 prevlsn : 6061000
    // --LS     60a1000 length         9000 expectnext : 60aa000 HIGHLSN : 6094000 chkptlsn : 5eb5000 prevlsn : 6081000

    //
    // A key for the test is to use a small record size and a large
    // number of records in order to force the stream checkpoint record
    // to span multiple segments. There is also support for forcing a
    // stream checkpoint record after a log write
    //
    NTSTATUS status;
    KSynchronizer activateSync;
    KSynchronizer sync;
    RvdLogManager::SPtr logManager;

    //
    // Get a log manager
    //
    status = RvdLogManager::Create(KTL_TAG_LOGGER, KtlSystem::GlobalNonPagedAllocator(), logManager);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = logManager->Activate(nullptr, activateSync);
    VERIFY_IS_TRUE(K_ASYNC_SUCCESS(status));

    logManager->RegisterVerificationCallback(DiskId, &VerifyRawRecordCallback);

    //
    // Create a container and a stream
    //
    KGuid logContainerGuid;
    RvdLogId logContainerId;
    RvdLogManager::AsyncCreateLog::SPtr createLogOp;
    RvdLog::SPtr logContainer;

    logContainerGuid.CreateNew();
    logContainerId = static_cast<RvdLogId>(logContainerGuid);

    KGuid otherStreamGuid;
    RvdLogStreamId otherStreamId;
    RvdLogStream::SPtr otherLogStream;

    otherStreamGuid.CreateNew();
    otherStreamId = static_cast<RvdLogStreamId>(otherStreamGuid);

    {
        RvdLog::AsyncCreateLogStreamContext::SPtr createStreamOp;

        status = logManager->CreateAsyncCreateLogContext(createLogOp);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KWString logType(KtlSystem::GlobalNonPagedAllocator(), L"TestLogType");
        KInvariant(NT_SUCCESS(logType.Status()));
        
        createLogOp->StartCreateLog(
            DiskId,
            logContainerId,
            logType,
            DefaultTestLogFileSize,
            4,          // 4 streams
            64 * 1024,
            logContainer,
            nullptr,
            sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = logContainer->CreateAsyncCreateLogStreamContext(createStreamOp);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        createStreamOp->StartCreateLogStream(otherStreamId,
            DiskId,
            otherLogStream,
            nullptr,
            sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        RvdLogStream::AsyncWriteContext::SPtr otherStreamWriteOp;

        status = otherLogStream->CreateAsyncWriteContext(otherStreamWriteOp);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        ULONG otherAsn = 1;

        //
        // Write 512 records - this should be enough to force multisegment
        // stream checkpoint
        //
        for (ULONG i = 0; i < 512; i++)
        {
            status = CoreLoggerWrite(NULL,
                otherLogStream,
                otherAsn,
                0);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            otherAsn++;
        }

        //
        // Write a few larger records at the end of the file to ensure
        // segment offsets and LSNs don't match by accident
        for (ULONG i = 0; i < 3; i++)
        {
            status = CoreLoggerWrite(NULL,
                otherLogStream,
                otherAsn,
                16 * 1024);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            otherAsn++;
        }

        //
        // Force a stream checkpoint as the last records in the log
        //
        status = CoreLoggerWrite(NULL,
            otherLogStream,
            otherAsn,
            0x2000,
            2,
            TRUE);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        otherAsn++;

        // 
        // Close the container and streams
        //
        KAsyncEvent closeEvent;
        KAsyncEvent::WaitContext::SPtr waitForClose;

        status = closeEvent.CreateWaitContext(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator(), waitForClose);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        createLogOp.Reset();
        createStreamOp.Reset();
        otherStreamWriteOp.Reset();
        otherLogStream.Reset();
        logContainer->SetShutdownEvent(&closeEvent);
        logContainer.Reset();

        waitForClose->StartWaitUntilSet(NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    {
        //
        // Reopen the log container
        //
        RvdLogManager::AsyncOpenLog::SPtr openLogOp;
        status = logManager->CreateAsyncOpenLogContext(openLogOp);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        openLogOp->StartOpenLog(DiskId, logContainerId, logContainer, nullptr, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        RvdLog::AsyncOpenLogStreamContext::SPtr openStreamOp;
        status = logContainer->CreateAsyncOpenLogStreamContext(openStreamOp);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        openStreamOp->StartOpenLogStream(otherStreamId,
                                         otherLogStream,
                                         nullptr,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KAsyncEvent closeEvent;
        KAsyncEvent::WaitContext::SPtr waitForClose;

        status = closeEvent.CreateWaitContext(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator(), waitForClose);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        openLogOp.Reset();
        openStreamOp.Reset();
        otherLogStream.Reset();
        logContainer->SetShutdownEvent(&closeEvent);
        logContainer.Reset();

        waitForClose->StartWaitUntilSet(NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    RvdLogManager::AsyncDeleteLog::SPtr deleteLogOp;

    status = logManager->CreateAsyncDeleteLogContext(deleteLogOp);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    deleteLogOp->StartDeleteLog(DiskId, logContainerId, nullptr, sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    logManager->Deactivate();
    activateSync.WaitForCompletion();
    logManager.Reset();
}


VOID DuplicateRecordInLogTest(
    KGuid DiskId
    )
{
    //
    // This test case is to verify that a duplicate record in the same
    // stream, that is, two records that have the same ASN and Version
    // persisted in the same stream will cause the stream to fail to
    // open rather than silently failing the second record and opening
    // successfully. There was a case where a duplicate record was
    // written to a stream but this was not caught on stream recovery.
    //
    NTSTATUS status;
    KSynchronizer activateSync;
    KSynchronizer sync;
    RvdLogManager::SPtr logManager;

    //
    // Get a log manager
    //
    status = RvdLogManager::Create(KTL_TAG_LOGGER, KtlSystem::GlobalNonPagedAllocator(), logManager);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = logManager->Activate(nullptr, activateSync);
    VERIFY_IS_TRUE(K_ASYNC_SUCCESS(status));

    logManager->RegisterVerificationCallback(KtlLogInformation::KtlLogDefaultStreamType(), &VerifyRawRecordCallback);

    //
    // Create a container and a stream
    //
    KGuid logContainerGuid;
    RvdLogId logContainerId;
    RvdLogManager::AsyncCreateLog::SPtr createLogOp;
    RvdLog::SPtr logContainer;

    logContainerGuid.CreateNew();
    logContainerId = static_cast<RvdLogId>(logContainerGuid);

    KGuid otherStreamGuid;
    RvdLogStreamId otherStreamId;
    RvdLogStream::SPtr otherLogStream;

    otherStreamGuid.CreateNew();
    otherStreamId = static_cast<RvdLogStreamId>(otherStreamGuid);

    ULONG otherAsn = 1;
    
    {
        RvdLog::AsyncCreateLogStreamContext::SPtr createStreamOp;

        status = logManager->CreateAsyncCreateLogContext(createLogOp);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KWString logType(KtlSystem::GlobalNonPagedAllocator(), L"TestLogType");
        KInvariant(NT_SUCCESS(logType.Status()));
        
        createLogOp->StartCreateLog(
            DiskId,
            logContainerId,
            logType,
            DefaultTestLogFileSize,
            4,          // 4 streams
            64 * 1024,
            logContainer,
            nullptr,
            sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = logContainer->CreateAsyncCreateLogStreamContext(createStreamOp);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        createStreamOp->StartCreateLogStream(otherStreamId,
            KtlLogInformation::KtlLogDefaultStreamType(),
            otherLogStream,
            nullptr,
            sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        RvdLogStream::AsyncWriteContext::SPtr otherStreamWriteOp;

        status = otherLogStream->CreateAsyncWriteContext(otherStreamWriteOp);
        VERIFY_IS_TRUE(NT_SUCCESS(status));


        //
        // Write 512 records - this should be enough.
        //
        for (ULONG i = 0; i < 512; i++)
        {
            status = CoreLoggerWrite(NULL,
                otherLogStream,
                otherAsn,
                0);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            otherAsn++;
        }

        // 
        // Close the container and streams
        //
        KAsyncEvent closeEvent;
        KAsyncEvent::WaitContext::SPtr waitForClose;

        status = closeEvent.CreateWaitContext(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator(), waitForClose);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        createLogOp.Reset();
        createStreamOp.Reset();
        otherStreamWriteOp.Reset();
        otherLogStream.Reset();
        logContainer->SetShutdownEvent(&closeEvent);
        logContainer.Reset();

        waitForClose->StartWaitUntilSet(NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    //
    // Corrupt log by effectively truncating it at an earlier point and
    // then rewrite an existing record
    //
    {
        KBlockFile::SPtr logFile;
        KWString logPath(*g_Allocator);

#if defined(PLATFORM_UNIX)
        logPath = L"/RvdLog/Log";
#else
        //
        // Format is: \\??\Volume{a3316ef4-dfb4-40a3-9fae-427705f23d1f}\Log{3b4b7db6-ba84-4ab1-af83-ae0154a4d268}.log
        //
        logPath = L"\\??\\Volume";
        logPath += DiskId;
        logPath += L"\\rvdlog\\Log";
#endif
        logPath += logContainerId.Get();
        logPath += L".log";
        
        status = logPath.Status();

        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = KBlockFile::Create(logPath,
                                    TRUE,
                                    KBlockFile::CreateDisposition::eOpenExisting,
                                    logFile,
                                    sync,
                                    nullptr,
                                    *g_Allocator);
        
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        ULONG recordSize = 0x1000;
        ULONGLONG corruptionOffset = 0x218000;
        
        KIoBuffer::SPtr zeroBuffer;
        PVOID p;
        status = KIoBuffer::CreateSimple(recordSize, zeroBuffer, p, *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        memset(p, 0, recordSize);
        status = logFile->Transfer(KBlockFile::IoPriority::eForeground,
                                   KBlockFile::TransferType::eWrite,
                                   corruptionOffset,
                                   *zeroBuffer,
                                   sync,
                                   nullptr,
                                   nullptr);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        logFile->Close();
    }


    {
        //
        // Reopen the log container and stream, expect success at the
        // lower ASN and then write the duplicate record
        //
        RvdLogManager::AsyncOpenLog::SPtr openLogOp;
        status = logManager->CreateAsyncOpenLogContext(openLogOp);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        openLogOp->StartOpenLog(DiskId, logContainerId, logContainer, nullptr, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        RvdLog::AsyncOpenLogStreamContext::SPtr openStreamOp;
        status = logContainer->CreateAsyncOpenLogStreamContext(openStreamOp);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        openStreamOp->StartOpenLogStream(otherStreamId,
                                         otherLogStream,
                                         nullptr,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        RvdLogAsn lowAsn;
        RvdLogAsn highAsn;
        RvdLogAsn truncationAsn;
        otherLogStream->QueryRecordRange(&lowAsn, &highAsn, &truncationAsn);
        VERIFY_IS_TRUE(highAsn.Get() == 507);

        //
        // Write copy of the end record earlier
        //
        otherAsn = otherAsn - 2;
        status = CoreLoggerWrite(NULL,
            otherLogStream,
            otherAsn,
            0);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        otherAsn++;
        
        
        KAsyncEvent closeEvent;
        KAsyncEvent::WaitContext::SPtr waitForClose;

        status = closeEvent.CreateWaitContext(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator(), waitForClose);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        openLogOp.Reset();
        openStreamOp.Reset();
        otherLogStream.Reset();
        logContainer->SetShutdownEvent(&closeEvent);
        logContainer.Reset();

        waitForClose->StartWaitUntilSet(NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }
    
    
    {
        //
        // Reopen the log container, expect failure K_STATUS_LOG_STRUCTURE_FAULT
        //
        RvdLogManager::AsyncOpenLog::SPtr openLogOp;
        status = logManager->CreateAsyncOpenLogContext(openLogOp);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        openLogOp->StartOpenLog(DiskId, logContainerId, logContainer, nullptr, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        RvdLog::AsyncOpenLogStreamContext::SPtr openStreamOp;
        status = logContainer->CreateAsyncOpenLogStreamContext(openStreamOp);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        openStreamOp->StartOpenLogStream(otherStreamId,
                                         otherLogStream,
                                         nullptr,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == K_STATUS_LOG_STRUCTURE_FAULT);

        KAsyncEvent closeEvent;
        KAsyncEvent::WaitContext::SPtr waitForClose;

        status = closeEvent.CreateWaitContext(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator(), waitForClose);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        openLogOp.Reset();
        openStreamOp.Reset();
        otherLogStream.Reset();
        logContainer->SetShutdownEvent(&closeEvent);
        logContainer.Reset();

        waitForClose->StartWaitUntilSet(NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }
    

    RvdLogManager::AsyncDeleteLog::SPtr deleteLogOp;

    status = logManager->CreateAsyncDeleteLogContext(deleteLogOp);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    deleteLogOp->StartDeleteLog(DiskId, logContainerId, nullptr, sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    logManager->Deactivate();
    activateSync.WaitForCompletion();
    logManager.Reset();
}


VOID CorruptedRecordTest(
    KGuid DiskId
    )
{
    //
    // This test case is to verify that a corrupted record in the
    // middle of a log will cause the log to fail to open and not cause
    // the log to under recover
    //
    NTSTATUS status;
    KSynchronizer activateSync;
    KSynchronizer sync;
    RvdLogManager::SPtr logManager;

    //
    // Get a log manager
    //
    status = RvdLogManager::Create(KTL_TAG_LOGGER, KtlSystem::GlobalNonPagedAllocator(), logManager);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = logManager->Activate(nullptr, activateSync);
    VERIFY_IS_TRUE(K_ASYNC_SUCCESS(status));

    logManager->RegisterVerificationCallback(KtlLogInformation::KtlLogDefaultStreamType(), &VerifyRawRecordCallback);

    //
    // Create a container and a stream
    //
    KGuid logContainerGuid;
    RvdLogId logContainerId;
    RvdLogManager::AsyncCreateLog::SPtr createLogOp;
    RvdLog::SPtr logContainer;

    logContainerGuid.CreateNew();
    logContainerId = static_cast<RvdLogId>(logContainerGuid);

    KGuid otherStreamGuid;
    RvdLogStreamId otherStreamId;
    RvdLogStream::SPtr otherLogStream;

    otherStreamGuid.CreateNew();
    otherStreamId = static_cast<RvdLogStreamId>(otherStreamGuid);

    ULONG otherAsn = 1;
    
    {
        RvdLog::AsyncCreateLogStreamContext::SPtr createStreamOp;

        status = logManager->CreateAsyncCreateLogContext(createLogOp);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KWString logType(KtlSystem::GlobalNonPagedAllocator(), L"TestLogType");
        KInvariant(NT_SUCCESS(logType.Status()));
        
        createLogOp->StartCreateLog(
            DiskId,
            logContainerId,
            logType,
            DefaultTestLogFileSize,
            4,          // 4 streams
            64 * 1024,
            RvdLogManager::AsyncCreateLog::FlagSparseFile,
            logContainer,
            nullptr,
            sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = logContainer->CreateAsyncCreateLogStreamContext(createStreamOp);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        createStreamOp->StartCreateLogStream(otherStreamId,
            KtlLogInformation::KtlLogDefaultStreamType(),
            otherLogStream,
            nullptr,
            sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        RvdLogStream::AsyncWriteContext::SPtr otherStreamWriteOp;

        status = otherLogStream->CreateAsyncWriteContext(otherStreamWriteOp);
        VERIFY_IS_TRUE(NT_SUCCESS(status));


        //
        // Write 512 records - this should be enough.
        //
        for (ULONG i = 0; i < 512; i++)
        {
            status = CoreLoggerWrite(NULL,
                otherLogStream,
                otherAsn,
                0);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            otherAsn++;
        }

        // 
        // Close the container and streams
        //
        KAsyncEvent closeEvent;
        KAsyncEvent::WaitContext::SPtr waitForClose;

        status = closeEvent.CreateWaitContext(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator(), waitForClose);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        createLogOp.Reset();
        createStreamOp.Reset();
        otherStreamWriteOp.Reset();
        otherLogStream.Reset();
        logContainer->SetShutdownEvent(&closeEvent);
        logContainer.Reset();

        waitForClose->StartWaitUntilSet(NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    KBlockFile::SPtr logFile;
    KWString logPath(*g_Allocator);
    ULONG recordSize = 0x1000;
    ULONGLONG corruptionOffset = 0x218000;

#if defined(PLATFORM_UNIX)
    logPath = L"/RvdLog/Log";
#else
    //
    // Format is: \\??\Volume{a3316ef4-dfb4-40a3-9fae-427705f23d1f}\Log{3b4b7db6-ba84-4ab1-af83-ae0154a4d268}.log
    //
    logPath = L"\\??\\Volume";
    logPath += DiskId;
    logPath += L"\\rvdlog\\Log";
#endif
    logPath += logContainerId.Get();
    logPath += L".log";
    
    status = logPath.Status();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    //
    // Corrupt a single record in the log by flipping a bit
    //
    {
        status = KBlockFile::Create(logPath,
                                    TRUE,
                                    KBlockFile::CreateDisposition::eOpenExisting,
                                    logFile,
                                    sync,
                                    nullptr,
                                    *g_Allocator);
        
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        
        KIoBuffer::SPtr recordBuffer;
        PVOID p;
        status = KIoBuffer::CreateSimple(recordSize, recordBuffer, p, *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = logFile->Transfer(KBlockFile::IoPriority::eForeground,
                                   KBlockFile::TransferType::eRead,
                                   corruptionOffset,
                                   *recordBuffer,
                                   sync,
                                   nullptr,
                                   nullptr);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        PUCHAR pp = (PUCHAR)p;
        pp[0x30] = 0x99;

        status = logFile->Transfer(KBlockFile::IoPriority::eForeground,
                                   KBlockFile::TransferType::eWrite,
                                   corruptionOffset,
                                   *recordBuffer,
                                   sync,
                                   nullptr,
                                   nullptr);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
                
        logFile->Close();
    }


    {
        //
        // Reopen the log container and stream, expect error
        //
        RvdLogManager::AsyncOpenLog::SPtr openLogOp;
        status = logManager->CreateAsyncOpenLogContext(openLogOp);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        openLogOp->StartOpenLog(DiskId, logContainerId, logContainer, nullptr, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == K_STATUS_LOG_STRUCTURE_FAULT);
    }   

    
    //
    // Corrupt log by effectively truncating it at an earlier point by
    // zeroing out a record.
    //
    {
        status = KBlockFile::Create(logPath,
                                    TRUE,
                                    KBlockFile::CreateDisposition::eOpenExisting,
                                    logFile,
                                    sync,
                                    nullptr,
                                    *g_Allocator);
        
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KIoBuffer::SPtr zeroBuffer;
        PVOID p;
        status = KIoBuffer::CreateSimple(recordSize, zeroBuffer, p, *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        memset(p, 0, recordSize);
        status = logFile->Transfer(KBlockFile::IoPriority::eForeground,
                                   KBlockFile::TransferType::eWrite,
                                   corruptionOffset,
                                   *zeroBuffer,
                                   sync,
                                   nullptr,
                                   nullptr);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        logFile->Close();
    }


    {
        //
        // Reopen the log container and stream, expect success at the
        // lower ASN 
        //
        RvdLogManager::AsyncOpenLog::SPtr openLogOp;
        status = logManager->CreateAsyncOpenLogContext(openLogOp);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        openLogOp->StartOpenLog(DiskId, logContainerId, logContainer, nullptr, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        RvdLog::AsyncOpenLogStreamContext::SPtr openStreamOp;
        status = logContainer->CreateAsyncOpenLogStreamContext(openStreamOp);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        openStreamOp->StartOpenLogStream(otherStreamId,
                                         otherLogStream,
                                         nullptr,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        RvdLogAsn lowAsn;
        RvdLogAsn highAsn;
        RvdLogAsn truncationAsn;
        otherLogStream->QueryRecordRange(&lowAsn, &highAsn, &truncationAsn);
        VERIFY_IS_TRUE(highAsn.Get() == 507);

        
        KAsyncEvent closeEvent;
        KAsyncEvent::WaitContext::SPtr waitForClose;

        status = closeEvent.CreateWaitContext(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator(), waitForClose);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        openLogOp.Reset();
        openStreamOp.Reset();
        otherLogStream.Reset();
        logContainer->SetShutdownEvent(&closeEvent);
        logContainer.Reset();

        waitForClose->StartWaitUntilSet(NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }   

    RvdLogManager::AsyncDeleteLog::SPtr deleteLogOp;

    status = logManager->CreateAsyncDeleteLogContext(deleteLogOp);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    deleteLogOp->StartDeleteLog(DiskId, logContainerId, nullptr, sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    logManager->Deactivate();
    activateSync.WaitForCompletion();
    logManager.Reset();
}


VOID SetupRawCoreLoggerTests(
    KGuid& DiskId,
    ULONGLONG& StartingAllocs,
    KtlSystem*& System
    )
{
    NTSTATUS status;

    status = KtlSystem::Initialize(FALSE,     // Do not enable VNetwork, we don't need it
                                   &System);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    g_Allocator = &KtlSystem::GlobalNonPagedAllocator();

    KDbgMirrorToDebugger = TRUE;

    System->SetStrictAllocationChecks(TRUE);

    StartingAllocs = KAllocatorSupport::gs_AllocsRemaining;

    EventRegisterMicrosoft_Windows_KTL();

#if !defined(PLATFORM_UNIX)
    UCHAR driveLetter = 0;
    status = FindDiskIdForDriveLetter(driveLetter, DiskId);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
#else
    // {204F01E4-6DEC-48fe-8483-B2C6B79ABECB}
    GUID randomGuid = 
        { 0x204f01e4, 0x6dec, 0x48fe, { 0x84, 0x83, 0xb2, 0xc6, 0xb7, 0x9a, 0xbe, 0xcb } };
    
    DiskId = randomGuid;    
#endif
}

VOID CleanupRawCoreLoggerTests(
    KGuid& DiskId,
    ULONGLONG& StartingAllocs,
    KtlSystem*& System
    )
{
    UNREFERENCED_PARAMETER(DiskId);
    UNREFERENCED_PARAMETER(System); 
    UNREFERENCED_PARAMETER(StartingAllocs);
    
    EventUnregisterMicrosoft_Windows_KTL();

    KtlSystem::Shutdown();
}
