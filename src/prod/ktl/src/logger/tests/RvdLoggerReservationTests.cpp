/*++

Copyright (c) Microsoft Corporation

Module Name:

    RvdLoggerVerifyTests.cpp

Abstract:

--*/
#pragma once
#include "RvdLoggerTests.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>
#include <InternalKtlLogger.h>
#include "RvdLogStreamAsyncIOTests.h"
#include <KtlLogRecovery.h>

#if KTL_USER_MODE
#include <ktlevents.um.h>
#else
#include <ktlevents.km.h>
#endif


#if (CONSOLE_TEST) || (DISPLAY_ON_CONSOLE)
#undef KDbgPrintf
#define KDbgPrintf(...)     printf(__VA_ARGS__)
#endif


NTSTATUS
BasicReservationTests(WCHAR OnDrive, KAllocator& Allocator)
{
    static GUID const   testLogIdGuid = {3, 0, 0, {0, 0, 0, 0, 0, 0, 0, 3}};
    static GUID const   testStreamTypeGuid = {4, 0, 0, {0, 0, 0, 0, 0, 0, 0, 2}};
    KWString            fullyQualifiedDiskFileName(Allocator);
    KGuid               driveGuid;
    RvdLogId            logId(testLogIdGuid);
    RvdLogStreamType    testStreamType(testStreamTypeGuid);
    NTSTATUS            status;
    LONGLONG            logSize = 1024 * 1024 * 1024;
    Synchronizer        shutdownCompEvent;
    RvdLogManager::SPtr logManager;

    {
        status = CleanAndPrepLog(OnDrive);
        KInvariant(NT_SUCCESS(status));

        status = GetDriveAndPathOfLogFile(OnDrive, logId, driveGuid, fullyQualifiedDiskFileName);
        KInvariant(NT_SUCCESS(status));

        status = RvdLogManager::Create(KTL_TAG_TEST, Allocator, logManager);
        KInvariant(NT_SUCCESS(status));

        status = logManager->Activate(nullptr, shutdownCompEvent.AsyncCompletionCallback());
        KInvariant(NT_SUCCESS(status));

        RvdLogManager::AsyncCreateLog::SPtr         createLogOp;
        RvdLog::AsyncCreateLogStreamContext::SPtr   createStreamOp;
        Synchronizer                                compEvent;

        status = logManager->CreateAsyncCreateLogContext(createLogOp);
        KInvariant(NT_SUCCESS(status));

        RvdLog::SPtr                            log;
        RvdLogManagerImp::RvdOnDiskLog::SPtr    logImp;

        KWString logType(KtlSystem::GlobalNonPagedAllocator(), L"RvdLog");
        KInvariant(NT_SUCCESS(logType.Status()));

        createLogOp->StartCreateLog(
            driveGuid,
            logId,
            logType,
            logSize,
            log,
            nullptr,
            compEvent.AsyncCompletionCallback());

        status = compEvent.WaitForCompletion();
        KInvariant(NT_SUCCESS(status));

        logImp = log.DownCast<RvdLogManagerImp::RvdOnDiskLog>();

        KInvariant(log->QueryCurrentReservation() == 0);

        GUID const                  testStreamIdGuid = {1, 0, 0, {0, 0, 0, 0, 0, 0, 0, 1}};
        RvdLogStreamId              testStreamId(testStreamIdGuid);

        status = log->CreateAsyncCreateLogStreamContext(createStreamOp);
        KInvariant(NT_SUCCESS(status));

        // Prove basic stream reservations
        {
            RvdLogStream::SPtr          stream;
            createStreamOp->StartCreateLogStream(
                testStreamId,
                RvdLogStreamType(testStreamTypeGuid),
                stream,
                nullptr,
                compEvent.AsyncCompletionCallback());

            status = compEvent.WaitForCompletion();
            KInvariant(NT_SUCCESS(status));

            KInvariant(stream->QueryCurrentReservation() == 0);

            RvdLogStream::AsyncReservationContext::SPtr  reserveOp;
            status = stream->CreateUpdateReservationContext(reserveOp);
            KInvariant(NT_SUCCESS(status));

            // Prove main stream increase of reserve
            reserveOp->StartUpdateReservation(1000, nullptr, compEvent.AsyncCompletionCallback());
            status = compEvent.WaitForCompletion();
            KInvariant(NT_SUCCESS(status));
            reserveOp->Reuse();
        
            KInvariant(stream->QueryCurrentReservation() == 1000);
            KInvariant(log->QueryCurrentReservation() == 1000);

            // Prove main stream reduction of reserve
            reserveOp->StartUpdateReservation(-500, nullptr, compEvent.AsyncCompletionCallback());
            status = compEvent.WaitForCompletion();
            KInvariant(NT_SUCCESS(status));
            reserveOp->Reuse();
        
            KInvariant(stream->QueryCurrentReservation() == 500);
            KInvariant(log->QueryCurrentReservation() == 500);

            // Prove proper behavior of over-relieve write case
            KIoBuffer::SPtr         dummyBuffer;
            void*                   dummyBufferPtr;
            status = KIoBuffer::CreateSimple(4096, dummyBuffer, dummyBufferPtr, Allocator);
            KInvariant(NT_SUCCESS(status));

            KBuffer::SPtr           dummyMetadata;
            status = KBuffer::Create(10, dummyMetadata, Allocator);
            KInvariant(NT_SUCCESS(status));

            RvdLogStream::AsyncWriteContext::SPtr   writeOp;

            status = stream->CreateAsyncWriteContext(writeOp);
            KInvariant(NT_SUCCESS(status));

            writeOp->StartReservedWrite(1000, RvdLogAsn(1), 0, dummyMetadata, dummyBuffer, nullptr, compEvent.AsyncCompletionCallback());
            status = compEvent.WaitForCompletion();
            KInvariant(status == K_STATUS_LOG_RESERVE_TOO_SMALL);
            writeOp->Reuse();

            // Prove proper behavior of relieve write case
            writeOp->StartReservedWrite(250, RvdLogAsn(1), 0, dummyMetadata, dummyBuffer, nullptr, compEvent.AsyncCompletionCallback());
            status = compEvent.WaitForCompletion();
            KInvariant(NT_SUCCESS(status));
            writeOp->Reuse();

            KInvariant(stream->QueryCurrentReservation() == 250);
            KInvariant(log->QueryCurrentReservation() == 250);

            // Prove over reserve checked
            reserveOp->StartUpdateReservation(logSize, nullptr, compEvent.AsyncCompletionCallback());
            status = compEvent.WaitForCompletion();
            KInvariant(status == STATUS_LOG_FULL);
            reserveOp->Reuse();

            KInvariant(stream->QueryCurrentReservation() == 250);
            KInvariant(log->QueryCurrentReservation() == 250);

            // Prove over relieve checked
            reserveOp->StartUpdateReservation(-logSize, nullptr, compEvent.AsyncCompletionCallback());
            status = compEvent.WaitForCompletion();
            KInvariant(status == K_STATUS_LOG_RESERVE_TOO_SMALL);
            reserveOp->Reuse();

            KInvariant(stream->QueryCurrentReservation() == 250);
            KInvariant(log->QueryCurrentReservation() == 250);

            // Prove log full failure because of reserved space and that a reserved write succeeds
            ULONGLONG       freeSpace = 0;
            log->QuerySpaceInformation(nullptr, &freeSpace);

            freeSpace -= logImp->GetConfig().GetMinFreeSpace();
            reserveOp->StartUpdateReservation(freeSpace, nullptr, compEvent.AsyncCompletionCallback());
            status = compEvent.WaitForCompletion();
            KInvariant(NT_SUCCESS(status));
            reserveOp->Reuse();

            KInvariant(stream->QueryCurrentReservation() == (250 + freeSpace));
            KInvariant(log->QueryCurrentReservation() == (250 + freeSpace));

            writeOp->StartWrite(RvdLogAsn(2), 0, dummyMetadata, dummyBuffer, nullptr, compEvent.AsyncCompletionCallback());
            status = compEvent.WaitForCompletion();
            KInvariant(status == STATUS_LOG_FULL);
            writeOp->Reuse();

            KInvariant(stream->QueryCurrentReservation() == (250 + freeSpace));
            KInvariant(log->QueryCurrentReservation() == (250 + freeSpace));

            writeOp->StartReservedWrite(freeSpace, RvdLogAsn(3), 0, dummyMetadata, dummyBuffer, nullptr, compEvent.AsyncCompletionCallback());
            status = compEvent.WaitForCompletion();
            KInvariant(NT_SUCCESS(status));
            writeOp->Reuse();

            KInvariant(stream->QueryCurrentReservation() == 250);
            KInvariant(log->QueryCurrentReservation() == 250);

            // Now prove that a normal write will work after the relieving write
            writeOp->StartWrite(RvdLogAsn(4), 0, dummyMetadata, dummyBuffer, nullptr, compEvent.AsyncCompletionCallback());
            status = compEvent.WaitForCompletion();
            KInvariant(NT_SUCCESS(status));
            writeOp->Reuse();

            KInvariant(stream->QueryCurrentReservation() == 250);
            KInvariant(log->QueryCurrentReservation() == 250);
        }

        KNt::Sleep(250);

        // Stream closed - log reserve should go to 0
        KInvariant(log->QueryCurrentReservation() == 0);
    }

    logManager->Deactivate();
    status = shutdownCompEvent.WaitForCompletion();
    KInvariant(NT_SUCCESS(status));
    return STATUS_SUCCESS;
}


//** Main Test Entry Point: RvdLoggerReservationTests
NTSTATUS
RvdLoggerReservationTests(__in int argc, __in_ecount(argc) WCHAR* args[])
{
    KDbgPrintf("RvdLoggerReservationTests: Starting\n");

    EventRegisterMicrosoft_Windows_KTL();
    KFinally([]()
    {
        EventUnregisterMicrosoft_Windows_KTL();
    });

    NTSTATUS status = KtlSystem::Initialize(FALSE, nullptr);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("RvdLoggerReservationTests: KtlSystem::Initialize Failed: 0x%08X: Line %i\n", status, __LINE__);
        KInvariant(FALSE);
        return status;
    }

    KFinally([]()
    {
        KtlSystem::Shutdown();
    });


    // Syntax: <drive> 
    if (argc < 1)
    {
        KDbgPrintf("RvdLoggerReservationTests: Drive Letter Missing: %i\n", __LINE__);
        return STATUS_INVALID_PARAMETER_2;
    }

    LONGLONG baselineAllocations = KAllocatorSupport::gs_AllocsRemaining;

    KDbgPrintf("RvdLoggerReservationTests: BasicReservationTests Starting\n");
    status = BasicReservationTests(*args[0], KtlSystem::GlobalNonPagedAllocator());
    KDbgPrintf("RvdLoggerReservationTests: BasicReservationTests Completed\n");
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("RvdLoggerReservationTests: BasicReservationTests failed: 0x%08X: Line %i\n", status, __LINE__);
        KInvariant(FALSE);
        return status;
    }

    KNt::Sleep(250);       // Allow async object releases
    if (KAllocatorSupport::gs_AllocsRemaining != baselineAllocations)
    {
        KDbgPrintf("RvdLoggerReservationTests: BasicReservationTests Memory Leak Detected: %i\n", __LINE__);
        KtlSystemBase::DebugOutputKtlSystemBases();
        KInvariant(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    KDbgPrintf("RvdLoggerReservationTests: Completed");
    return status;
}
