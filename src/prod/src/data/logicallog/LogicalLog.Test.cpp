// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestHeaders.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

#define TESTLOGICALLOG_TAG 'xxLL'

using namespace ktl;

namespace LogTests
{
    const int AbortSleepDurationMs = 5000;
    Common::StringLiteral const TraceComponent("LogicalLogTest");
    
    ktl::Awaitable<NTSTATUS> TryOpenFileAsync(KWString & path, KAllocator & allocator)
    {
        NTSTATUS status;
        KBlockFile::SPtr file;

        status = co_await KBlockFile::CreateAsync(path, TRUE, KBlockFile::CreateDisposition::eOpenExisting, file, nullptr, allocator);
        if (NT_SUCCESS(status))
        {
            file->Close();
        }

        co_return status;
    }

    class TracingInit
    {
    public:
        
        TracingInit()
        {
            { Common::Config config; } // trigger tracing config loading
            
            RegisterKtlTraceCallback(Common::ServiceFabricKtlTraceCallback);
        }
    };

    class LogicalLogTest : public LogTestBase
    {
        K_FORCE_SHARED(LogicalLogTest)

    public:

        static NTSTATUS Create(
            __in KAllocator& allocator,
            __in Data::Utilities::PartitionedReplicaId & prId,
            __in KtlLoggerMode ktlLoggerMode,
            __out LogicalLogTest::SPtr& logicalLogTest)
        {
            NTSTATUS status;

            logicalLogTest = _new(TESTLOGICALLOG_TAG, allocator) LogicalLogTest(prId, ktlLoggerMode);

            if (!logicalLogTest)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                KTraceOutOfMemory(0, status, nullptr, 0, 0);
                return status;
            }

            status = logicalLogTest->Status();
            if (!NT_SUCCESS(status))
            {
                return Ktl::Move(logicalLogTest)->Status();
            }

            return STATUS_SUCCESS;
        }

        NTSTATUS CreateAndOpenLogManager(__in Data::Utilities::PartitionedReplicaId const & prId, __out ILogManagerHandle::SPtr& logManager)
        {
            NTSTATUS status;
            ILogManagerHandle::SPtr manager;

            status = SyncAwait(logManager_->GetHandle(prId, GetTestDirectoryPath(), CancellationToken::None, manager));
            if (!NT_SUCCESS(status))
            {
                return status;
            }

            logManager = Ktl::Move(manager);
            return STATUS_SUCCESS;
        }

        NTSTATUS CreateAndOpenLogManager(__out ILogManagerHandle::SPtr& logManager) override
        {
            NTSTATUS status =  CreateAndOpenLogManager(*prId_, logManager);
            return status;
        }

        NTSTATUS CreateDefaultPhysicalLog(
            __in ILogManagerHandle& logManager,
            __out IPhysicalLogHandle::SPtr& physicalLog) override
        {
            NTSTATUS status;

            IPhysicalLogHandle::SPtr log;
            status = SyncAwait(logManager.CreateAndOpenPhysicalLogAsync(
                CancellationToken::None,
                log));
            if (!NT_SUCCESS(status))
            {
                return status;
            }

            physicalLog = Ktl::Move(log);

            return STATUS_SUCCESS;
        }

        NTSTATUS CreatePhysicalLog(
            __in ILogManagerHandle& logManager,
            __in KGuid const & id,
            __in KString const & path,
            __out IPhysicalLogHandle::SPtr& physicalLog) override
        {
            NTSTATUS status;

            IPhysicalLogHandle::SPtr log;
            status = SyncAwait(logManager.CreateAndOpenPhysicalLogAsync(
                path,
                id,
                1024 * 1024 * 1024,
                0,
                0,
                LogCreationFlags::UseSparseFile,
                CancellationToken::None,
                log));
            if (!NT_SUCCESS(status))
            {
                return status;
            }

            physicalLog = Ktl::Move(log);

            return STATUS_SUCCESS;
        }

        NTSTATUS OpenDefaultPhysicalLog(
            __in ILogManagerHandle& logManager,
            __out IPhysicalLogHandle::SPtr& physicalLog) override
        {
            return SyncAwait(logManager.OpenPhysicalLogAsync(CancellationToken::None, physicalLog));
        }

        NTSTATUS OpenPhysicalLog(
            __in ILogManagerHandle& logManager,
            __in KGuid const & id,
            __in KString const & path, 
            __out IPhysicalLogHandle::SPtr& physicalLog) override
        {
            return SyncAwait(logManager.OpenPhysicalLogAsync(path, id, CancellationToken::None, physicalLog));
        }

        NTSTATUS CreateLogicalLog(
            __in IPhysicalLogHandle& physicalLog,
            __in KGuid const & id,
            __in KString const & path,
            __out ILogicalLog::SPtr& logicalLog) override
        {
            NTSTATUS status;
            
            ILogicalLog::SPtr log;
            KString::CSPtr alias;
            status = SyncAwait(physicalLog.CreateAndOpenLogicalLogAsync(
                id, 
                alias,
                path,
                nullptr,
                1024 * 1024 * 100,
                128 * 1024,
                LogCreationFlags::UseSparseFile,
                CancellationToken::None,
                log));
            if (!NT_SUCCESS(status))
            {
                return status;
            }

            logicalLog = Ktl::Move(log);

            return STATUS_SUCCESS;
        }

        NTSTATUS OpenLogicalLog(
            __in IPhysicalLogHandle& physicalLog,
            __in KGuid const & id,
            __in KString const &,
            __out ILogicalLog::SPtr& logicalLog) override
        {
            return SyncAwait(physicalLog.OpenLogicalLogAsync(id, CancellationToken::None, logicalLog));
        }

        VOID VerifyState(
            __in_opt LogManager* logManager = nullptr,
            __in int logManagerHandlesCountExpected = -1,
            __in int logManagerLogsCountExpected = -1,
            __in BOOLEAN logManagerIsLoadedExpected = FALSE,
            __in_opt PhysicalLog* physicalLog0 = nullptr,
            __in int physicalLog0HandlesCountExpected = -1,
            __in int physicalLog0LogsCountExpected = -1,
            __in BOOLEAN physicalLog0IsOpenExpected = FALSE,
            __in_opt PhysicalLogHandle* physicalLogHandle0 = nullptr,
            __in BOOLEAN physicalLogHandle0IsFunctionalExpected = FALSE,
            __in_opt PhysicalLog* physicalLog1 = nullptr,
            __in int physicalLog1HandlesCountExpected = -1,
            __in int physicalLog1LogsCountExpected = -1,
            __in BOOLEAN physicalLog1IsOpenExpected = FALSE,
            __in_opt PhysicalLogHandle* physicalLogHandle1 = nullptr,
            __in BOOLEAN physicalLogHandle1IsFunctionalExpected = FALSE,
            __in BOOLEAN physicalLogHandlesOwnersEqualExpected = FALSE,
            __in_opt LogicalLog* logicalLog = nullptr,
            __in BOOLEAN logicalLogIsFunctionalExpected = FALSE) override
        {
            if (logManager != nullptr)
            {
                VERIFY_ARE_EQUAL(logManagerIsLoadedExpected, logManager->IsLoaded);

                if (logManagerIsLoadedExpected)
                {
                    VERIFY_IS_FALSE(logManagerHandlesCountExpected == 0 && logManagerLogsCountExpected == 0);
                }

                if (logManagerHandlesCountExpected != -1)
                {
                    VERIFY_ARE_EQUAL(logManagerHandlesCountExpected, logManager->HandlesCount);
                }
                if (logManagerLogsCountExpected != -1)
                {
                    VERIFY_ARE_EQUAL(logManagerLogsCountExpected, logManager->LogsCount);
                }
            }

            if (physicalLog0 != nullptr)
            {
                VERIFY_ARE_EQUAL(physicalLog0IsOpenExpected, physicalLog0->IsOpen());

                if (physicalLog0IsOpenExpected)
                {
                    VERIFY_IS_FALSE(physicalLog0HandlesCountExpected == 0 && physicalLog0LogsCountExpected == 0);
                }

                if (physicalLog0HandlesCountExpected != -1)
                {
                    VERIFY_ARE_EQUAL(physicalLog0HandlesCountExpected, physicalLog0->HandlesCount);
                }
                
                if (physicalLog0LogsCountExpected != -1)
                {
                    VERIFY_ARE_EQUAL(physicalLog0LogsCountExpected, physicalLog0->LogsCount);
                }
            }

            if (physicalLog1 != nullptr)
            {
                VERIFY_ARE_EQUAL(physicalLog1IsOpenExpected, physicalLog1->IsOpen());

                if (physicalLog1IsOpenExpected)
                {
                    VERIFY_IS_FALSE(physicalLog1HandlesCountExpected == 1 && physicalLog1LogsCountExpected == 1);
                }

                if (physicalLog1HandlesCountExpected != -1)
                {
                    VERIFY_ARE_EQUAL(physicalLog1HandlesCountExpected, physicalLog1->HandlesCount);
                }
                
                if (physicalLog1LogsCountExpected != -1)
                {
                    VERIFY_ARE_EQUAL(physicalLog1LogsCountExpected, physicalLog1->LogsCount);
                }
            }

            if (physicalLogHandle0 != nullptr)
            {
                VERIFY_ARE_EQUAL(physicalLogHandle0IsFunctionalExpected, physicalLogHandle0->IsFunctional);
            }

            if (physicalLogHandle1 != nullptr)
            {
                VERIFY_ARE_EQUAL(physicalLogHandle1IsFunctionalExpected, physicalLogHandle1->IsFunctional);
            }

            if (physicalLogHandle0 != nullptr && physicalLogHandle1 != nullptr)
            {
                VERIFY_ARE_EQUAL(physicalLogHandlesOwnersEqualExpected, &physicalLogHandle0->Owner == &physicalLogHandle1->Owner);
            }

            if (logicalLog != nullptr)
            {
                VERIFY_ARE_EQUAL(logicalLogIsFunctionalExpected, logicalLog->IsFunctional);
            }
        }

        VOID LogSizeSpaceRemainingTest()
        {
            NTSTATUS status;

            KString::CSPtr alias;

            ILogManagerHandle::SPtr logManager;
            status = CreateAndOpenLogManager(logManager);
            VERIFY_STATUS_SUCCESS("CreateAndOpenLogManager", status);

            KString::SPtr physicalLogName;
            GenerateUniqueFilename(physicalLogName);
            KWString physicalLogNameWString(GetThisAllocator());
            physicalLogNameWString = *physicalLogName;
            VERIFY_STATUS_SUCCESS("KWString assignment", status);

            KGuid physicalLogId;
            physicalLogId.CreateNew();

            // Don't care if this fails
            SyncAwait(logManager->DeletePhysicalLogAsync(*physicalLogName, physicalLogId, CancellationToken::None));

            IPhysicalLogHandle::SPtr physicalLog;
            status = CreatePhysicalLog(*logManager, physicalLogId, *physicalLogName, physicalLog);
            VERIFY_STATUS_SUCCESS("CreatePhysicalLog", status);

            KString::SPtr logicalLogName;
            GenerateUniqueFilename(logicalLogName);
            KGuid logicalLogId;
            logicalLogId.CreateNew();
            ILogicalLog::SPtr logicalLog;
            status = CreateLogicalLog(*physicalLog, logicalLogId, *logicalLogName, logicalLog);
            VERIFY_STATUS_SUCCESS("CreateLogicalLog", status);

            ULONGLONG size0 = logicalLog->Size;
            ULONGLONG spaceRemaining0 = logicalLog->SpaceRemaining;
            VERIFY_ARE_NOT_EQUAL(0, spaceRemaining0);

            KBuffer::SPtr buf;
            PUCHAR bufferPtr;
            AllocBuffer(10, buf, bufferPtr);
            status = SyncAwait(logicalLog->AppendAsync(*buf, 0, 10, CancellationToken::None));
            VERIFY_STATUS_SUCCESS("AppendAsync", status);

            status = SyncAwait(logicalLog->FlushWithMarkerAsync(CancellationToken::None));
            VERIFY_STATUS_SUCCESS("FlushAsync", status);

            ULONGLONG size1 = logicalLog->Size;
            ULONGLONG spaceRemaining1 = logicalLog->SpaceRemaining;
            VERIFY_ARE_NOT_EQUAL(0, size1);
            VERIFY_ARE_NOT_EQUAL(0, spaceRemaining1);
            VERIFY_IS_TRUE(size0 < size1);
            VERIFY_IS_TRUE(spaceRemaining0 > spaceRemaining1);


            // close, reopen, recheck
            status = SyncAwait(logicalLog->CloseAsync(CancellationToken::None));
            VERIFY_STATUS_SUCCESS("LogicalLog::CloseAsync", status);

            status = OpenLogicalLog(*physicalLog, logicalLogId, *logicalLogName, logicalLog);
            VERIFY_STATUS_SUCCESS("OpenLogicalLog", status);

            ULONGLONG size2 = logicalLog->Size;
            ULONGLONG spaceRemaining2 = logicalLog->SpaceRemaining;
            VERIFY_ARE_NOT_EQUAL(0, size2);
            VERIFY_ARE_NOT_EQUAL(0, spaceRemaining2);
            VERIFY_IS_TRUE(size2 <= size1);
            VERIFY_IS_TRUE(spaceRemaining2 >= spaceRemaining1);


            // truncate and confirm that size goes down and spaceRemaining goes up
            // Disabled pending a reliable way to test truncatehead
            /*status = SyncAwait(logicalLog->TruncateHead(logicalLog->WritePosition));
            VERIFY_STATUS_SUCCESS("TruncateHead", status);

            status = SyncAwait(logicalLog->FlushWithMarkerAsync(CancellationToken::None));
            VERIFY_STATUS_SUCCESS("FlushAsync", status);

            ULONGLONG start = KNt::GetTickCount64();
            while (KNt::GetTickCount64() - start <= 15000 && logicalLog->Size == size2)
            {
                KNt::Sleep(250);
            }
            ULONGLONG size3 = logicalLog->Size;
            ULONGLONG spaceRemaining3 = logicalLog->SpaceRemaining;
            VERIFY_IS_TRUE(size3 < size2);
            VERIFY_IS_TRUE(spaceRemaining3 > spaceRemaining2);*/


            // truncatetail does not reduce the size of the log so is not suitable to test

            // Cleanup
            status = SyncAwait(logicalLog->CloseAsync(CancellationToken::None));
            VERIFY_STATUS_SUCCESS("LogicalLog::CloseAsync", status);
            logicalLog = nullptr;

            status = SyncAwait(physicalLog->CloseAsync(CancellationToken::None));
            VERIFY_STATUS_SUCCESS("PhysicalLogHandle::CloseAsync", status);
            physicalLog = nullptr;

            status = SyncAwait(logManager->DeletePhysicalLogAsync(*physicalLogName, physicalLogId, CancellationToken::None));
            VERIFY_STATUS_SUCCESS("LogManagerHandle::DeletePhysicalLogAsync", status);

            status = SyncAwait(logManager->CloseAsync(CancellationToken::None));
            VERIFY_STATUS_SUCCESS("LogManagerHandle::CloseAsync", status);
        }

        VOID BasicTest()
        {
            NTSTATUS status;
            //LONG bytesRead;
            KBuffer::SPtr buf;

            ILogManagerHandle::SPtr logManager;
            status = CreateAndOpenLogManager(logManager);
            VERIFY_STATUS_SUCCESS("CreateAndOpenLogManager", status);

            LogManager::SPtr logManagerService = static_cast<LogManagerHandle&>(*logManager).owner_.RawPtr();

            VerifyState(
                logManagerService.RawPtr(),         // LogManager
                1,                                  // LogManager.HandlesCount
                0,                                  // LogManager.LogsCount
                TRUE);                              // LogManager.IsLoaded

            // Prove abort works
            logManager->Abort();
            logManager = nullptr;

            //
            // LogManager handle cleanup is an async operation and so we cannot guarantee
            // that it has been run at this point. TFS 6029884 was created as a result
            // of this occurring in this CIT. Waiting for 5 seconds should be sufficient.
            //
            KNt::Sleep(AbortSleepDurationMs);
            VerifyState(
                logManagerService.RawPtr(),         // LogManager
                0,                                  // LogManager.HandlesCount
                0,                                  // LogManager.LogsCount
                FALSE);                             // LogManager.IsLoaded

            status = CreateAndOpenLogManager(logManager);
            VERIFY_STATUS_SUCCESS("CreateAndOpenLogManager", status);
            VerifyState(
                logManagerService.RawPtr(),         // LogManager
                1,                                  // LogManager.HandlesCount
                0,                                  // LogManager.LogsCount
                TRUE);                              // LogManager.IsLoaded

            ILogManagerHandle::SPtr logManager1;
            status = CreateAndOpenLogManager(logManager1);
            VERIFY_STATUS_SUCCESS("CreateAndOpenLogManager", status);
            VerifyState(
                logManagerService.RawPtr(),         // LogManager
                2,                                  // LogManager.HandlesCount
                0,                                  // LogManager.LogsCount
                TRUE);                              // LogManager.IsLoaded

            KString::SPtr physicalLogName;
            GenerateUniqueFilename(physicalLogName);

            KGuid physicalLogId;
            physicalLogId.CreateNew();

            // Don't care if this fails
            SyncAwait(logManager->DeletePhysicalLogAsync(*physicalLogName, physicalLogId, CancellationToken::None));
            SyncAwait(logManager->DeletePhysicalLogAsync(CancellationToken::None));

            // Prove physical log can be created
            IPhysicalLogHandle::SPtr physicalLog;
            status = CreatePhysicalLog(*logManager, physicalLogId, *physicalLogName, physicalLog);
            VERIFY_STATUS_SUCCESS("CreatePhysicalLog", status);

            VerifyState(
                logManagerService.RawPtr(),                             // LogManager
                2,                                                      // LogManager.HandlesCount
                1,                                                      // LogManager.LogsCount
                TRUE,                                                   // LogManager.IsLoaded
                &static_cast<PhysicalLogHandle&>(*physicalLog).Owner,   // PhysicalLog0
                1,                                                      // PhysicalLog0.HandlesCount
                0,                                                      // PhysicalLog0.LogsCount
                TRUE,                                                   // PhysicalLog0.IsOpen
                &static_cast<PhysicalLogHandle&>(*physicalLog),         // PhysicalLogHandle0
                TRUE);                                                  // PhysicalLogHandle0.IsFunctional

            // Prove secondary attempt to create a physical log will fail
            {
                IPhysicalLogHandle::SPtr physicalLog1;
                status = CreatePhysicalLog(*logManager, physicalLogId, *physicalLogName, physicalLog1);
                VERIFY_IS_TRUE(status == STATUS_OBJECT_NAME_COLLISION);
            }
            VerifyState(
                logManagerService.RawPtr(),                             // LogManager
                2,                                                      // LogManager.HandlesCount
                1,                                                      // LogManager.LogsCount
                TRUE,                                                   // LogManager.IsLoaded
                &static_cast<PhysicalLogHandle&>(*physicalLog).Owner,   // PhysicalLog0
                1,                                                      // PhysicalLog0.HandlesCount
                0,                                                      // PhysicalLog0.LogsCount
                TRUE,                                                   // PhysicalLog0.IsOpen
                &static_cast<PhysicalLogHandle&>(*physicalLog),         // PhysicalLogHandle0
                TRUE);                                                  // PhysicalLogHandle0.IsFunctional

            // Prove base close after create works
            VERIFY_IS_TRUE(static_cast<PhysicalLogHandle&>(*physicalLog).IsFunctional);
            status = SyncAwait(physicalLog->CloseAsync(CancellationToken::None));
            VerifyState(
                logManagerService.RawPtr(),                             // LogManager
                2,                                                      // LogManager.HandlesCount
                0,                                                      // LogManager.LogsCount
                TRUE,                                                   // LogManager.IsLoaded
                &static_cast<PhysicalLogHandle&>(*physicalLog).Owner,   // PhysicalLog0
                0,                                                      // PhysicalLog0.HandlesCount
                0,                                                      // PhysicalLog0.LogsCount
                FALSE,                                                  // PhysicalLog0.IsOpen
                &static_cast<PhysicalLogHandle&>(*physicalLog),         // PhysicalLogHandle0
                FALSE);                                                 // PhysicalLogHandle0.IsFunctional
            physicalLog = nullptr;

            // Prove physical log delete works
            status = SyncAwait(logManager->DeletePhysicalLogAsync(*physicalLogName, physicalLogId, CancellationToken::None));
            VERIFY_STATUS_SUCCESS("DeletePhysicalLogAsync", status);

            VerifyState(
                logManagerService.RawPtr(),         // LogManager
                2,                                  // LogManager.HandlesCount
                0,                                  // LogManager.LogsCount
                TRUE);                              // LogManager.IsLoaded
  
            // additionally prove all the above works with default log (default meaning using what settings were passed to LogManager open) 
            {
                IPhysicalLogHandle::SPtr defaultPhysicalLog;
                status = CreateDefaultPhysicalLog(*logManager, defaultPhysicalLog);
                VERIFY_STATUS_SUCCESS("CreateDefaultPhysicalLog", status);

                VerifyState(
                    logManagerService.RawPtr(),                                     // LogManager
                    2,                                                              // LogManager.HandlesCount
                    1,                                                              // LogManager.LogsCount
                    TRUE,                                                           // LogManager.IsLoaded
                    &static_cast<PhysicalLogHandle&>(*defaultPhysicalLog).Owner,    // PhysicalLog0
                    1,                                                              // PhysicalLog0.HandlesCount
                    0,                                                              // PhysicalLog0.LogsCount
                    TRUE,                                                           // PhysicalLog0.IsOpen
                    &static_cast<PhysicalLogHandle&>(*defaultPhysicalLog),          // PhysicalLogHandle0
                    TRUE);                                                          // PhysicalLogHandle0.IsFunctional

                // Prove secondary attempt to create a physical log will fail
                {
                    IPhysicalLogHandle::SPtr defaultPhysicalLog1;
                    status = CreateDefaultPhysicalLog(*logManager, defaultPhysicalLog1);
                    VERIFY_IS_TRUE(status == STATUS_OBJECT_NAME_COLLISION);
                }

                VerifyState(
                    logManagerService.RawPtr(),                                     // LogManager
                    2,                                                              // LogManager.HandlesCount
                    1,                                                              // LogManager.LogsCount
                    TRUE,                                                           // LogManager.IsLoaded
                    &static_cast<PhysicalLogHandle&>(*defaultPhysicalLog).Owner,    // PhysicalLog0
                    1,                                                              // PhysicalLog0.HandlesCount
                    0,                                                              // PhysicalLog0.LogsCount
                    TRUE,                                                           // PhysicalLog0.IsOpen
                    &static_cast<PhysicalLogHandle&>(*defaultPhysicalLog),          // PhysicalLogHandle0
                    TRUE);                                                          // PhysicalLogHandle0.IsFunctional

                //* Prove base close after create works
                VERIFY_IS_TRUE(static_cast<PhysicalLogHandle&>(*defaultPhysicalLog).IsFunctional);
                status = SyncAwait(defaultPhysicalLog->CloseAsync(CancellationToken::None));
                VerifyState(
                    logManagerService.RawPtr(),                                     // LogManager
                    2,                                                              // LogManager.HandlesCount
                    0,                                                              // LogManager.LogsCount
                    TRUE,                                                           // LogManager.IsLoaded
                    &static_cast<PhysicalLogHandle&>(*defaultPhysicalLog).Owner,    // PhysicalLog0
                    0,                                                              // PhysicalLog0.HandlesCount
                    0,                                                              // PhysicalLog0.LogsCount
                    FALSE,                                                          // PhysicalLog0.IsOpen
                    &static_cast<PhysicalLogHandle&>(*defaultPhysicalLog),          // PhysicalLogHandle0
                    FALSE);                                                         // PhysicalLogHandle0.IsFunctional
                defaultPhysicalLog = nullptr;

                // Prove physical log delete works
                status = SyncAwait(logManager->DeletePhysicalLogAsync(CancellationToken::None));
                VERIFY_STATUS_SUCCESS("DeletePhysicalLogAsync", status);

                VerifyState(
                    logManagerService.RawPtr(),         // LogManager
                    2,                                  // LogManager.HandlesCount
                    0,                                  // LogManager.LogsCount
                    TRUE);
            }

            // prove with a staging log
            // "default" application guid has a special meaning when using the driver (mapping) and so this test isn't appropriate
            if (ktlLoggerMode_ == KtlLoggerMode::InProc)
            {
                IPhysicalLogHandle::SPtr stagingLog;
                KString::SPtr stagingLogPath;
                GenerateUniqueFilename(stagingLogPath);
                status = CreatePhysicalLog(*logManager, KGuid(KtlLogger::Constants::DefaultApplicationSharedLogId.AsGUID()), *stagingLogPath, stagingLog);
                VERIFY_STATUS_SUCCESS("CreatePhysicalLog", status);
                
                VerifyState(
                    logManagerService.RawPtr(),                                     // LogManager
                    2,                                                              // LogManager.HandlesCount
                    1,                                                              // LogManager.LogsCount
                    TRUE,                                                           // LogManager.IsLoaded
                    &static_cast<PhysicalLogHandle&>(*stagingLog).Owner,            // PhysicalLog0
                    1,                                                              // PhysicalLog0.HandlesCount
                    0,                                                              // PhysicalLog0.LogsCount
                    TRUE,                                                           // PhysicalLog0.IsOpen
                    &static_cast<PhysicalLogHandle&>(*stagingLog),                  // PhysicalLogHandle0
                    TRUE);
                // Prove secondary attempt to create a physical log will fail
#if 0           // re-enable when create-create case is understood
                {
                    IPhysicalLogHandle::SPtr stagingLog1;
                    status = CreatePhysicalLog(*logManager, KGuid(KtlLogger::Constants::DefaultApplicationSharedLogId.AsGUID()), *stagingLogPath, stagingLog1);
                    VERIFY_IS_TRUE(status == STATUS_OBJECT_NAME_COLLISION);
                }
#endif
                // Prove that opening on a different log manager handle fails (unique path from user, which is ignored).  Using the same logmanager handle is NOT SUPPORTED and buggy
                {
                    Data::Utilities::PartitionedReplicaId::SPtr secondReplicaId;
                    ILogManagerHandle::SPtr logManager2;
                    IPhysicalLogHandle::SPtr stagingLog1;
                    KString::SPtr stagingLogPath1;

                    secondReplicaId = Data::Utilities::PartitionedReplicaId::Create(prId_->PartitionId, prId_->ReplicaId + 1, GetThisAllocator());
                    GenerateUniqueFilename(stagingLogPath1);

                    // It is required to use a separate logmanager handle, as in the product a logmanager handle will only be used by one replica, so the 
                    // replicaid/partitionid/path are picked up when the logmanager handle is created
                    status = CreateAndOpenLogManager(*secondReplicaId, logManager2);
                    VERIFY_STATUS_SUCCESS("LogManager::GetHandle", status);
                    
                    // It actually doesn't matter what path is passed here for the staging log, since the path is chosen under the covers based on the prid
                    status = OpenPhysicalLog(*logManager2, KGuid(KtlLogger::Constants::DefaultApplicationSharedLogId.AsGUID()), *stagingLogPath1, stagingLog1);
                    VERIFY_IS_TRUE(status == STATUS_OBJECT_PATH_NOT_FOUND || status == STATUS_OBJECT_NAME_NOT_FOUND);

                    status = SyncAwait(logManager2->CloseAsync(CancellationToken::None));
                    VERIFY_STATUS_SUCCESS("LogManagerHandle::Close", status);
                }

                // Prove that opening on a different log manager handle fails (same path from user, which is ignored).  Using the same logmanager handle is NOT SUPPORTED and buggy
                {
                    Data::Utilities::PartitionedReplicaId::SPtr secondReplicaId;
                    ILogManagerHandle::SPtr logManager2;
                    IPhysicalLogHandle::SPtr stagingLog1;

                    secondReplicaId = Data::Utilities::PartitionedReplicaId::Create(prId_->PartitionId, prId_->ReplicaId + 1, GetThisAllocator());

                    // It is required to use a separate logmanager handle, as in the product a logmanager handle will only be used by one replica, so the 
                    // replicaid/partitionid/path are picked up when the logmanager handle is created
                    status = CreateAndOpenLogManager(*secondReplicaId, logManager2);
                    VERIFY_STATUS_SUCCESS("LogManager::GetHandle", status);

                    // It actually doesn't matter what path is passed here for the staging log, since the path is chosen under the covers based on the prid
                    status = OpenPhysicalLog(*logManager2, KGuid(KtlLogger::Constants::DefaultApplicationSharedLogId.AsGUID()), *stagingLogPath, stagingLog1);
                    VERIFY_IS_TRUE(status == STATUS_OBJECT_PATH_NOT_FOUND || status == STATUS_OBJECT_NAME_NOT_FOUND);

                    status = SyncAwait(logManager2->CloseAsync(CancellationToken::None));
                    VERIFY_STATUS_SUCCESS("LogManagerHandle::Close", status);
                }

                VerifyState(
                    logManagerService.RawPtr(),                                     // LogManager
                    2,                                                              // LogManager.HandlesCount
                    1,                                                              // LogManager.LogsCount
                    TRUE,                                                           // LogManager.IsLoaded
                    &static_cast<PhysicalLogHandle&>(*stagingLog).Owner,            // PhysicalLog0
                    1,                                                              // PhysicalLog0.HandlesCount
                    0,                                                              // PhysicalLog0.LogsCount
                    TRUE,                                                           // PhysicalLog0.IsOpen
                    &static_cast<PhysicalLogHandle&>(*stagingLog),                  // PhysicalLogHandle0
                    TRUE);                                                          // PhysicalLogHandle0.IsFunctional

                // Prove base close after create works
                VERIFY_IS_TRUE(static_cast<PhysicalLogHandle&>(*stagingLog).IsFunctional);
                status = SyncAwait(stagingLog->CloseAsync(CancellationToken::None));
                VerifyState(
                    logManagerService.RawPtr(),                                     // LogManager
                    2,                                                              // LogManager.HandlesCount
                    0,                                                              // LogManager.LogsCount
                    TRUE,                                                           // LogManager.IsLoaded
                    &static_cast<PhysicalLogHandle&>(*stagingLog).Owner,            // PhysicalLog0
                    0,                                                              // PhysicalLog0.HandlesCount
                    0,                                                              // PhysicalLog0.LogsCount
                    FALSE,                                                          // PhysicalLog0.IsOpen
                    &static_cast<PhysicalLogHandle&>(*stagingLog),                  // PhysicalLogHandle0
                    FALSE);                                                         // PhysicalLogHandle0.IsFunctional                
                stagingLog = nullptr;

                // Prove physical log delete works
                status = SyncAwait(logManager->DeletePhysicalLogAsync(*stagingLogPath, KGuid(KtlLogger::Constants::DefaultApplicationSharedLogId.AsGUID()), CancellationToken::None));
                VerifyState(
                    logManagerService.RawPtr(),         // LogManager
                    2,                                  // LogManager.HandlesCount
                    0,                                  // LogManager.LogsCount
                    TRUE);
            }


            // Prove we can re-create; open on a different handle; close the original - and all is well
            status = CreatePhysicalLog(*logManager, physicalLogId, *physicalLogName, physicalLog);
            VERIFY_STATUS_SUCCESS("CreatePhysicalLog", status);

            VerifyState(
                logManagerService.RawPtr(),                             // LogManager
                2,                                                      // LogManager.HandlesCount
                1,                                                      // LogManager.LogsCount
                TRUE,                                                   // LogManager.IsLoaded
                &static_cast<PhysicalLogHandle&>(*physicalLog).Owner,   // PhysicalLog0
                1,                                                      // PhysicalLog0.HandlesCount
                0,                                                      // PhysicalLog0.LogsCount
                TRUE,                                                   // PhysicalLog0.IsOpen
                &static_cast<PhysicalLogHandle&>(*physicalLog),         // PhysicalLogHandle0
                TRUE);                                                  // PhysicalLogHandle0.IsFunctional

            IPhysicalLogHandle::SPtr physicalLog2;
            status = OpenPhysicalLog(*logManager, physicalLogId, *physicalLogName, physicalLog2);
            VERIFY_STATUS_SUCCESS("OpenPhysicalLog", status);

            VerifyState(
                logManagerService.RawPtr(),                             // LogManager
                2,                                                      // LogManager.HandlesCount
                1,                                                      // LogManager.LogsCount
                TRUE,                                                   // LogManager.IsLoaded
                &static_cast<PhysicalLogHandle&>(*physicalLog).Owner,   // PhysicalLog0
                2,                                                      // PhysicalLog0.HandlesCount
                0,                                                      // PhysicalLog0.LogsCount
                TRUE,                                                   // PhysicalLog0.IsOpen
                &static_cast<PhysicalLogHandle&>(*physicalLog),         // PhysicalLogHandle0
                TRUE,                                                   // PhysicalLogHandle0.IsFunctional
                &static_cast<PhysicalLogHandle&>(*physicalLog2).Owner,  // PhysicalLog1
                2,                                                      // PhysicalLog1.HandlesCount
                0,                                                      // PhysicalLog1.LogsCount
                TRUE,                                                   // PhysicalLog1.IsOpen
                &static_cast<PhysicalLogHandle&>(*physicalLog2),        // PhysicalLogHandle1
                TRUE,                                                   // PhysicalLogHandle1.IsFunctional
                TRUE);                                                  // PhysicalLogHandle0.Owner == PhysicalLogHandle1.Owner

            status = SyncAwait(logManager->CloseAsync(CancellationToken::None));
            VERIFY_STATUS_SUCCESS("LogManagerHandle::CloseAsync", status);
            VerifyState(
                logManagerService.RawPtr(),                             // LogManager
                1,                                                      // LogManager.HandlesCount
                1,                                                      // LogManager.LogsCount
                TRUE,                                                   // LogManager.IsLoaded
                &static_cast<PhysicalLogHandle&>(*physicalLog).Owner,   // PhysicalLog0
                2,                                                      // PhysicalLog0.HandlesCount
                0,                                                      // PhysicalLog0.LogsCount
                TRUE,                                                   // PhysicalLog0.IsOpen
                &static_cast<PhysicalLogHandle&>(*physicalLog),         // PhysicalLogHandle0
                TRUE,                                                   // PhysicalLogHandle0.IsFunctional
                &static_cast<PhysicalLogHandle&>(*physicalLog2).Owner,  // PhysicalLog1
                2,                                                      // PhysicalLog1.HandlesCount
                0,                                                      // PhysicalLog1.LogsCount
                TRUE,                                                   // PhysicalLog1.IsOpen
                &static_cast<PhysicalLogHandle&>(*physicalLog2),        // PhysicalLogHandle1
                TRUE,                                                   // PhysicalLogHandle1.IsFunctional
                TRUE);                                                  // PhysicalLogHandle0.Owner == PhysicalLogHandle1.Owner

            status = SyncAwait(logManager1->CloseAsync(CancellationToken::None));
            VERIFY_STATUS_SUCCESS("LogManagerHandle::CloseAsync", status);
            VerifyState(
                logManagerService.RawPtr(),                             // LogManager
                0,                                                      // LogManager.HandlesCount
                1,                                                      // LogManager.LogsCount
                TRUE,                                                   // LogManager.IsLoaded
                &static_cast<PhysicalLogHandle&>(*physicalLog).Owner,   // PhysicalLog0
                2,                                                      // PhysicalLog0.HandlesCount
                0,                                                      // PhysicalLog0.LogsCount
                TRUE,                                                   // PhysicalLog0.IsOpen
                &static_cast<PhysicalLogHandle&>(*physicalLog),         // PhysicalLogHandle0
                TRUE,                                                   // PhysicalLogHandle0.IsFunctional
                &static_cast<PhysicalLogHandle&>(*physicalLog2).Owner,  // PhysicalLog1
                2,                                                      // PhysicalLog1.HandlesCount
                0,                                                      // PhysicalLog1.LogsCount
                TRUE,                                                   // PhysicalLog1.IsOpen
                &static_cast<PhysicalLogHandle&>(*physicalLog2),        // PhysicalLogHandle1
                TRUE,                                                   // PhysicalLogHandle1.IsFunctional
                TRUE);                                                  // PhysicalLogHandle0.Owner == PhysicalLogHandle1.Owner

            status = SyncAwait(physicalLog->CloseAsync(CancellationToken::None));
            VerifyState(
                logManagerService.RawPtr(),                             // LogManager
                0,                                                      // LogManager.HandlesCount
                1,                                                      // LogManager.LogsCount
                TRUE,                                                   // LogManager.IsLoaded
                &static_cast<PhysicalLogHandle&>(*physicalLog).Owner,   // PhysicalLog0
                1,                                                      // PhysicalLog0.HandlesCount
                0,                                                      // PhysicalLog0.LogsCount
                TRUE,                                                   // PhysicalLog0.IsOpen
                &static_cast<PhysicalLogHandle&>(*physicalLog),         // PhysicalLogHandle0
                FALSE,                                                  // PhysicalLogHandle0.IsFunctional
                &static_cast<PhysicalLogHandle&>(*physicalLog2).Owner,  // PhysicalLog1
                1,                                                      // PhysicalLog1.HandlesCount
                0,                                                      // PhysicalLog1.LogsCount
                TRUE,                                                   // PhysicalLog1.IsOpen
                &static_cast<PhysicalLogHandle&>(*physicalLog2),        // PhysicalLogHandle1
                TRUE,                                                   // PhysicalLogHandle1.IsFunctional
                TRUE);                                                  // PhysicalLogHandle0.Owner == PhysicalLogHandle1.Owner

            status = SyncAwait(physicalLog2->CloseAsync(CancellationToken::None));
            VerifyState(
                logManagerService.RawPtr(),                             // LogManager
                0,                                                      // LogManager.HandlesCount
                0,                                                      // LogManager.LogsCount
                FALSE,                                                  // LogManager.IsLoaded
                &static_cast<PhysicalLogHandle&>(*physicalLog).Owner,   // PhysicalLog0
                0,                                                      // PhysicalLog0.HandlesCount
                0,                                                      // PhysicalLog0.LogsCount
                FALSE,                                                  // PhysicalLog0.IsOpen
                &static_cast<PhysicalLogHandle&>(*physicalLog),         // PhysicalLogHandle0
                FALSE,                                                  // PhysicalLogHandle0.IsFunctional
                &static_cast<PhysicalLogHandle&>(*physicalLog2).Owner,  // PhysicalLog1
                0,                                                      // PhysicalLog1.HandlesCount
                0,                                                      // PhysicalLog1.LogsCount
                FALSE,                                                  // PhysicalLog1.IsOpen
                &static_cast<PhysicalLogHandle&>(*physicalLog2),        // PhysicalLogHandle1
                FALSE,                                                  // PhysicalLogHandle1.IsFunctional
                TRUE);                                                  // PhysicalLogHandle0.Owner == PhysicalLogHandle1.Owner

            physicalLog = nullptr;
            physicalLog2 = nullptr;

            // todo: use verify function for the rest of the test

            // prove physical log abort
            status = CreateAndOpenLogManager(logManager);
            VERIFY_STATUS_SUCCESS("CreateAndOpenLogManager", status);
            VerifyState(
                logManagerService.RawPtr(),         // LogManager
                1,                                  // LogManager.HandlesCount
                0,                                  // LogManager.LogsCount
                TRUE);                              // LogManager.IsLoaded

            // abort
            status = OpenPhysicalLog(*logManager, physicalLogId, *physicalLogName, physicalLog);
            VERIFY_STATUS_SUCCESS("OpenPhysicalLog", status);
            VERIFY_IS_TRUE(static_cast<PhysicalLogHandle&>(*physicalLog).IsFunctional);
            VERIFY_IS_TRUE(logManagerService->IsLoaded);
            VERIFY_ARE_EQUAL(1, logManagerService->HandlesCount);
            VERIFY_ARE_EQUAL(1, logManagerService->LogsCount);
            VERIFY_IS_TRUE(static_cast<PhysicalLogHandle&>(*physicalLog).Owner.IsOpen());
            VERIFY_ARE_EQUAL(1, static_cast<PhysicalLogHandle&>(*physicalLog).Owner.HandlesCount);
            VERIFY_ARE_EQUAL(0, static_cast<PhysicalLogHandle&>(*physicalLog).Owner.LogsCount);

            physicalLog->Abort();
            KNt::Sleep(AbortSleepDurationMs); // Allow background Abort-triggered clean up to happen
            VERIFY_IS_TRUE(logManagerService->IsLoaded);
            VERIFY_ARE_EQUAL(1, logManagerService->HandlesCount);
            VERIFY_ARE_EQUAL(0, logManagerService->LogsCount);
            VERIFY_IS_FALSE(static_cast<PhysicalLogHandle&>(*physicalLog).IsFunctional);
            VERIFY_IS_FALSE(static_cast<PhysicalLogHandle&>(*physicalLog).Owner.IsOpen());
            VERIFY_ARE_EQUAL(0, static_cast<PhysicalLogHandle&>(*physicalLog).Owner.HandlesCount);
            VERIFY_ARE_EQUAL(0, static_cast<PhysicalLogHandle&>(*physicalLog).Owner.LogsCount);
            physicalLog = nullptr;

            status = OpenPhysicalLog(*logManager, physicalLogId, *physicalLogName, physicalLog);
            VERIFY_STATUS_SUCCESS("OpenPhysicalLog", status);
            VERIFY_IS_TRUE(static_cast<PhysicalLogHandle&>(*physicalLog).IsFunctional);
            VERIFY_IS_TRUE(logManagerService->IsLoaded);
            VERIFY_ARE_EQUAL(1, logManagerService->HandlesCount);
            VERIFY_ARE_EQUAL(1, logManagerService->LogsCount);
            VERIFY_IS_TRUE(static_cast<PhysicalLogHandle&>(*physicalLog).Owner.IsOpen());
            VERIFY_ARE_EQUAL(1, static_cast<PhysicalLogHandle&>(*physicalLog).Owner.HandlesCount);
            VERIFY_ARE_EQUAL(0, static_cast<PhysicalLogHandle&>(*physicalLog).Owner.LogsCount);


            KString::SPtr logicalLogName;
            GenerateUniqueFilename(logicalLogName);
            KGuid logicalLogId;
            logicalLogId.CreateNew();
            ILogicalLog::SPtr logicalLog;
            status = CreateLogicalLog(*physicalLog, logicalLogId, *logicalLogName, logicalLog);
            VERIFY_STATUS_SUCCESS("CreateLogicalLog", status);
            VERIFY_IS_TRUE(static_cast<PhysicalLogHandle&>(*physicalLog).IsFunctional);
            VERIFY_IS_TRUE(logManagerService->IsLoaded);
            VERIFY_ARE_EQUAL(1, logManagerService->HandlesCount);
            VERIFY_ARE_EQUAL(1, logManagerService->LogsCount);
            VERIFY_IS_TRUE(static_cast<PhysicalLogHandle&>(*physicalLog).Owner.IsOpen());
            VERIFY_ARE_EQUAL(1, static_cast<PhysicalLogHandle&>(*physicalLog).Owner.HandlesCount);
            VERIFY_ARE_EQUAL(1, static_cast<PhysicalLogHandle&>(*physicalLog).Owner.LogsCount);
            VERIFY_IS_TRUE(static_cast<LogicalLog&>(*logicalLog).IsFunctional);
            
            status = SyncAwait(logicalLog->CloseAsync());
            VERIFY_STATUS_SUCCESS("LogicalLog::CloseAsync", status);
            VERIFY_IS_FALSE(static_cast<LogicalLog&>(*logicalLog).IsFunctional);
            VERIFY_IS_TRUE(logManagerService->IsLoaded);
            VERIFY_ARE_EQUAL(1, logManagerService->HandlesCount);
            VERIFY_ARE_EQUAL(1, logManagerService->LogsCount);
            VERIFY_IS_TRUE(static_cast<PhysicalLogHandle&>(*physicalLog).Owner.IsOpen());
            VERIFY_ARE_EQUAL(1, static_cast<PhysicalLogHandle&>(*physicalLog).Owner.HandlesCount);
            VERIFY_ARE_EQUAL(0, static_cast<PhysicalLogHandle&>(*physicalLog).Owner.LogsCount);
            logicalLog = nullptr;

            // Prove we can re-open a LLog
            status = OpenLogicalLog(*physicalLog, logicalLogId, *logicalLogName, logicalLog);
            VERIFY_STATUS_SUCCESS("OpenLogicalLog", status);
            VERIFY_IS_TRUE(static_cast<PhysicalLogHandle&>(*physicalLog).IsFunctional);
            VERIFY_IS_TRUE(logManagerService->IsLoaded);
            VERIFY_ARE_EQUAL(1, logManagerService->HandlesCount);
            VERIFY_ARE_EQUAL(1, logManagerService->LogsCount);
            VERIFY_IS_TRUE(static_cast<PhysicalLogHandle&>(*physicalLog).Owner.IsOpen());
            VERIFY_ARE_EQUAL(1, static_cast<PhysicalLogHandle&>(*physicalLog).Owner.HandlesCount);
            VERIFY_ARE_EQUAL(1, static_cast<PhysicalLogHandle&>(*physicalLog).Owner.LogsCount);
            VERIFY_IS_TRUE(static_cast<LogicalLog&>(*logicalLog).IsFunctional);

            // Prove LLog abort
            logicalLog->Abort();
            logicalLog = nullptr;
            KNt::Sleep(AbortSleepDurationMs);      // Allow background cleanup to happen
            VERIFY_IS_TRUE(static_cast<PhysicalLogHandle&>(*physicalLog).IsFunctional);
            VERIFY_IS_TRUE(logManagerService->IsLoaded);
            VERIFY_ARE_EQUAL(1, logManagerService->HandlesCount);
            VERIFY_ARE_EQUAL(1, logManagerService->LogsCount);
            VERIFY_IS_TRUE(static_cast<PhysicalLogHandle&>(*physicalLog).Owner.IsOpen());
            VERIFY_ARE_EQUAL(1, static_cast<PhysicalLogHandle&>(*physicalLog).Owner.HandlesCount);
            VERIFY_ARE_EQUAL(0, static_cast<PhysicalLogHandle&>(*physicalLog).Owner.LogsCount);

            // Prove an open LogicalLog is functional with all Physical and Manager handles closed
            status = OpenLogicalLog(*physicalLog, logicalLogId, *logicalLogName, logicalLog);
            VERIFY_STATUS_SUCCESS("OpenLogicalLog", status);
            VERIFY_IS_TRUE(static_cast<PhysicalLogHandle&>(*physicalLog).IsFunctional);
            VERIFY_IS_TRUE(logManagerService->IsLoaded);
            VERIFY_ARE_EQUAL(1, logManagerService->HandlesCount);
            VERIFY_ARE_EQUAL(1, logManagerService->LogsCount);
            VERIFY_IS_TRUE(static_cast<PhysicalLogHandle&>(*physicalLog).Owner.IsOpen());
            VERIFY_ARE_EQUAL(1, static_cast<PhysicalLogHandle&>(*physicalLog).Owner.HandlesCount);
            VERIFY_ARE_EQUAL(1, static_cast<PhysicalLogHandle&>(*physicalLog).Owner.LogsCount);
            VERIFY_IS_TRUE(static_cast<LogicalLog&>(*logicalLog).IsFunctional);

            status = SyncAwait(physicalLog->CloseAsync(CancellationToken::None));
            VERIFY_STATUS_SUCCESS("PhysicalLogHandle::CloseAsync", status);
            VERIFY_IS_FALSE(static_cast<PhysicalLogHandle&>(*physicalLog).IsFunctional);
            VERIFY_IS_TRUE(logManagerService->IsLoaded);
            VERIFY_ARE_EQUAL(1, logManagerService->HandlesCount);
            VERIFY_ARE_EQUAL(1, logManagerService->LogsCount);
            VERIFY_IS_TRUE(static_cast<PhysicalLogHandle&>(*physicalLog).Owner.IsOpen());
            VERIFY_ARE_EQUAL(0, static_cast<PhysicalLogHandle&>(*physicalLog).Owner.HandlesCount);
            VERIFY_ARE_EQUAL(1, static_cast<PhysicalLogHandle&>(*physicalLog).Owner.LogsCount);
            VERIFY_IS_TRUE(static_cast<LogicalLog&>(*logicalLog).IsFunctional);

            status = SyncAwait(logManager->CloseAsync(CancellationToken::None));
            VERIFY_STATUS_SUCCESS("LogManagerHandle::CloseAsync", status);
            VERIFY_IS_TRUE(logManagerService->IsLoaded);
            VERIFY_ARE_EQUAL(0, logManagerService->HandlesCount);
            VERIFY_ARE_EQUAL(1, logManagerService->LogsCount);
            VERIFY_IS_TRUE(static_cast<PhysicalLogHandle&>(*physicalLog).Owner.IsOpen());
            VERIFY_ARE_EQUAL(0, static_cast<PhysicalLogHandle&>(*physicalLog).Owner.HandlesCount);
            VERIFY_ARE_EQUAL(1, static_cast<PhysicalLogHandle&>(*physicalLog).Owner.LogsCount);
            VERIFY_IS_TRUE(static_cast<LogicalLog&>(*logicalLog).IsFunctional);

            status = SyncAwait(logicalLog->CloseAsync(CancellationToken::None));
            VERIFY_STATUS_SUCCESS("LogicalLog::CloseAsync", status);
            VERIFY_IS_FALSE(static_cast<PhysicalLogHandle&>(*physicalLog).IsFunctional);
            VERIFY_IS_FALSE(logManagerService->IsLoaded);
            VERIFY_ARE_EQUAL(0, logManagerService->HandlesCount);
            VERIFY_ARE_EQUAL(0, logManagerService->LogsCount);
            VERIFY_IS_FALSE(static_cast<PhysicalLogHandle&>(*physicalLog).Owner.IsOpen());
            VERIFY_ARE_EQUAL(0, static_cast<PhysicalLogHandle&>(*physicalLog).Owner.HandlesCount);
            VERIFY_ARE_EQUAL(0, static_cast<PhysicalLogHandle&>(*physicalLog).Owner.LogsCount);
            VERIFY_IS_FALSE(static_cast<LogicalLog&>(*logicalLog).IsFunctional);
            logicalLog = nullptr;

            //* Prove exclusive LLog open works
            status = CreateAndOpenLogManager(logManager);
            VERIFY_STATUS_SUCCESS("CreateAndOpenLogManager", status);
            status = OpenPhysicalLog(*logManager, physicalLogId, *physicalLogName, physicalLog);
            VERIFY_STATUS_SUCCESS("OpenPhysicalLog", status);
            status = OpenPhysicalLog(*logManager, physicalLogId, *physicalLogName, physicalLog2);
            VERIFY_STATUS_SUCCESS("OpenPhysicalLog", status);
            VERIFY_IS_TRUE(static_cast<PhysicalLogHandle&>(*physicalLog).IsFunctional);
            VERIFY_IS_TRUE(logManagerService->IsLoaded);
            VERIFY_ARE_EQUAL(1, logManagerService->HandlesCount);
            VERIFY_ARE_EQUAL(1, logManagerService->LogsCount);
            VERIFY_IS_TRUE(static_cast<PhysicalLogHandle&>(*physicalLog).Owner.IsOpen());
            VERIFY_ARE_EQUAL(2, static_cast<PhysicalLogHandle&>(*physicalLog).Owner.HandlesCount);
            VERIFY_ARE_EQUAL(0, static_cast<PhysicalLogHandle&>(*physicalLog).Owner.LogsCount);
            VERIFY_IS_TRUE(static_cast<PhysicalLogHandle&>(*physicalLog2).Owner.IsOpen());
            VERIFY_ARE_EQUAL(2, static_cast<PhysicalLogHandle&>(*physicalLog2).Owner.HandlesCount);
            VERIFY_ARE_EQUAL(0, static_cast<PhysicalLogHandle&>(*physicalLog2).Owner.LogsCount);
            VERIFY_ARE_EQUAL(
                &static_cast<PhysicalLogHandle&>(*physicalLog).Owner,
                &static_cast<PhysicalLogHandle&>(*physicalLog2).Owner);

            status = OpenLogicalLog(*physicalLog, logicalLogId, *logicalLogName, logicalLog);
            VERIFY_STATUS_SUCCESS("OpenLogicalLog", status);
            VERIFY_IS_TRUE(static_cast<LogicalLog&>(*logicalLog).IsFunctional);
            VERIFY_IS_TRUE(static_cast<PhysicalLogHandle&>(*physicalLog).Owner.IsOpen());
            VERIFY_ARE_EQUAL(2, static_cast<PhysicalLogHandle&>(*physicalLog).Owner.HandlesCount);
            VERIFY_ARE_EQUAL(1, static_cast<PhysicalLogHandle&>(*physicalLog).Owner.LogsCount);

            {
                ILogicalLog::SPtr logicalLog1;
                status = OpenLogicalLog(*physicalLog2, logicalLogId, *logicalLogName, logicalLog1);
                VERIFY_IS_TRUE(status == STATUS_OBJECT_NAME_COLLISION);
                VERIFY_IS_TRUE(logicalLog1 == nullptr);
            }
            VERIFY_IS_TRUE(static_cast<LogicalLog&>(*logicalLog).IsFunctional);
            VERIFY_IS_TRUE(static_cast<PhysicalLogHandle&>(*physicalLog).IsFunctional);
            VERIFY_IS_TRUE(logManagerService->IsLoaded);
            VERIFY_ARE_EQUAL(1, logManagerService->HandlesCount);
            VERIFY_ARE_EQUAL(1, logManagerService->LogsCount);
            VERIFY_IS_TRUE(static_cast<PhysicalLogHandle&>(*physicalLog).Owner.IsOpen());
            VERIFY_ARE_EQUAL(2, static_cast<PhysicalLogHandle&>(*physicalLog).Owner.HandlesCount);
            VERIFY_ARE_EQUAL(1, static_cast<PhysicalLogHandle&>(*physicalLog).Owner.LogsCount);
            VERIFY_IS_TRUE(static_cast<PhysicalLogHandle&>(*physicalLog2).Owner.IsOpen());
            VERIFY_ARE_EQUAL(2, static_cast<PhysicalLogHandle&>(*physicalLog2).Owner.HandlesCount);
            VERIFY_ARE_EQUAL(1, static_cast<PhysicalLogHandle&>(*physicalLog2).Owner.LogsCount);

            status = SyncAwait(logicalLog->CloseAsync(CancellationToken::None));
            VERIFY_STATUS_SUCCESS("LogicalLog::CloseAsync", status);
            VERIFY_IS_FALSE(static_cast<LogicalLog&>(*logicalLog).IsFunctional);
            VERIFY_IS_TRUE(static_cast<PhysicalLogHandle&>(*physicalLog).IsFunctional);
            VERIFY_IS_TRUE(logManagerService->IsLoaded);
            VERIFY_ARE_EQUAL(1, logManagerService->HandlesCount);
            VERIFY_ARE_EQUAL(1, logManagerService->LogsCount);
            VERIFY_IS_TRUE(static_cast<PhysicalLogHandle&>(*physicalLog).Owner.IsOpen());
            VERIFY_ARE_EQUAL(2, static_cast<PhysicalLogHandle&>(*physicalLog).Owner.HandlesCount);
            VERIFY_ARE_EQUAL(0, static_cast<PhysicalLogHandle&>(*physicalLog).Owner.LogsCount);
            VERIFY_IS_TRUE(static_cast<PhysicalLogHandle&>(*physicalLog2).Owner.IsOpen());
            VERIFY_ARE_EQUAL(2, static_cast<PhysicalLogHandle&>(*physicalLog2).Owner.HandlesCount);
            VERIFY_ARE_EQUAL(0, static_cast<PhysicalLogHandle&>(*physicalLog2).Owner.LogsCount);
            logicalLog = nullptr;


            //* Prove LLog delete works
            status = SyncAwait(physicalLog->DeleteLogicalLogOnlyAsync(logicalLogId, CancellationToken::None));
            VERIFY_STATUS_SUCCESS("PhysicalLogHandle::DeleteLogicalLogOnlyAsync", status);
            {
                ILogicalLog::SPtr logicalLog1;
                status = OpenLogicalLog(*physicalLog2, logicalLogId, *logicalLogName, logicalLog1);
                VERIFY_IS_FALSE(NT_SUCCESS(status)); // todo: check correct status
            }

            VERIFY_IS_TRUE(static_cast<PhysicalLogHandle&>(*physicalLog).IsFunctional);
            VERIFY_IS_TRUE(static_cast<PhysicalLogHandle&>(*physicalLog2).IsFunctional);

            // Prove close down
            status = SyncAwait(physicalLog->CloseAsync(CancellationToken::None));
            VERIFY_STATUS_SUCCESS("PhysicalLogHandle::CloseAsync", status);
            VERIFY_IS_FALSE(static_cast<PhysicalLogHandle&>(*physicalLog).IsFunctional);
            VERIFY_IS_TRUE(static_cast<PhysicalLogHandle&>(*physicalLog2).IsFunctional);
            VERIFY_IS_TRUE(logManagerService->IsLoaded);
            VERIFY_ARE_EQUAL(1, logManagerService->HandlesCount);
            VERIFY_ARE_EQUAL(1, logManagerService->LogsCount);
            VERIFY_IS_TRUE(static_cast<PhysicalLogHandle&>(*physicalLog).Owner.IsOpen());
            VERIFY_ARE_EQUAL(1, static_cast<PhysicalLogHandle&>(*physicalLog).Owner.HandlesCount);
            VERIFY_ARE_EQUAL(0, static_cast<PhysicalLogHandle&>(*physicalLog).Owner.LogsCount);
            VERIFY_IS_TRUE(static_cast<PhysicalLogHandle&>(*physicalLog2).Owner.IsOpen());
            VERIFY_ARE_EQUAL(1, static_cast<PhysicalLogHandle&>(*physicalLog2).Owner.HandlesCount);
            VERIFY_ARE_EQUAL(0, static_cast<PhysicalLogHandle&>(*physicalLog2).Owner.LogsCount);
            physicalLog = nullptr;

            status = SyncAwait(physicalLog2->CloseAsync(CancellationToken::None));
            VERIFY_STATUS_SUCCESS("PhysicalLogHandle::CloseAsync", status);
            VERIFY_STATUS_SUCCESS("PhysicalLogHandle::CloseAsync", status);
            VERIFY_IS_FALSE(static_cast<PhysicalLogHandle&>(*physicalLog2).IsFunctional);
            VERIFY_IS_TRUE(logManagerService->IsLoaded);
            VERIFY_ARE_EQUAL(1, logManagerService->HandlesCount);
            VERIFY_ARE_EQUAL(0, logManagerService->LogsCount);
            VERIFY_IS_FALSE(static_cast<PhysicalLogHandle&>(*physicalLog2).Owner.IsOpen());
            VERIFY_ARE_EQUAL(0, static_cast<PhysicalLogHandle&>(*physicalLog2).Owner.HandlesCount);
            VERIFY_ARE_EQUAL(0, static_cast<PhysicalLogHandle&>(*physicalLog2).Owner.LogsCount);
            physicalLog2 = nullptr;

            status = SyncAwait(logManager->DeletePhysicalLogAsync(*physicalLogName, physicalLogId, CancellationToken::None));
            VERIFY_STATUS_SUCCESS("LogManagerHandle::DeletePhysicalLogAsync", status);

            status = SyncAwait(logManager->CloseAsync(CancellationToken::None));
            VERIFY_STATUS_SUCCESS("LogManagerHandle::CloseAsync", status);
            VERIFY_IS_FALSE(logManagerService->IsLoaded);
            VERIFY_ARE_EQUAL(0, logManagerService->HandlesCount);
            VERIFY_ARE_EQUAL(0, logManagerService->LogsCount);

            status = SyncAwait(logManager1->CloseAsync(CancellationToken::None));
            VERIFY_IS_TRUE(!NT_SUCCESS(status)); // handle is already closed
        }

        VOID DeleteLogicalLogAndPhysicalLogTest()
        {
            NTSTATUS status;

            KString::CSPtr alias;

            ILogManagerHandle::SPtr logManager;
            status = CreateAndOpenLogManager(logManager);
            VERIFY_STATUS_SUCCESS("CreateAndOpenLogManager", status);

            KString::SPtr physicalLogName;
            GenerateUniqueFilename(physicalLogName);
            KWString physicalLogNameWString(GetThisAllocator());
            physicalLogNameWString = *physicalLogName;
            VERIFY_STATUS_SUCCESS("KWString assignment", status);

            KGuid physicalLogId;
            physicalLogId.CreateNew();

            // Don't care if this fails
            SyncAwait(logManager->DeletePhysicalLogAsync(*physicalLogName, physicalLogId, CancellationToken::None));

            IPhysicalLogHandle::SPtr physicalLog;
            status = CreatePhysicalLog(*logManager, physicalLogId, *physicalLogName, physicalLog);
            VERIFY_STATUS_SUCCESS("CreatePhysicalLog", status);

            KString::SPtr logicalLogName;
            GenerateUniqueFilename(logicalLogName);
            KGuid logicalLogId;
            logicalLogId.CreateNew();
            ILogicalLog::SPtr logicalLog;
            status = CreateLogicalLog(*physicalLog, logicalLogId, *logicalLogName, logicalLog);
            VERIFY_STATUS_SUCCESS("CreateLogicalLog", status);

            KString::SPtr logicalLogName1;
            GenerateUniqueFilename(logicalLogName1);
            KGuid logicalLogId1;
            logicalLogId1.CreateNew();
            ILogicalLog::SPtr logicalLog1;

            // prove physical log works
            status = SyncAwait(physicalLog->AssignAliasAsync(logicalLogId, *logicalLogName, CancellationToken::None));
            VERIFY_STATUS_SUCCESS("AssignAliasAsync", status);

            KGuid id;
            status = SyncAwait(physicalLog->ResolveAliasAsync(*logicalLogName, CancellationToken::None, id));
            VERIFY_STATUS_SUCCESS("ResolveAliasAsync", status);

            status = SyncAwait(TryOpenFileAsync(physicalLogNameWString, GetThisAllocator()));
            VERIFY_IS_TRUE(NT_SUCCESS(status) || status == STATUS_SHARING_VIOLATION);

            KBuffer::SPtr buf;
            PUCHAR bufferPtr;
            AllocBuffer(10, buf, bufferPtr);
            status = SyncAwait(logicalLog->AppendAsync(*buf, 0, 10, CancellationToken::None));
            VERIFY_STATUS_SUCCESS("AppendAsync", status);

            //
            // Test deleting both the single logical log and the physical log
            //
            {
                // prove logical log works
                status = SyncAwait(logicalLog->FlushWithMarkerAsync(CancellationToken::None));
                VERIFY_STATUS_SUCCESS("FlushWithMarkerAsync", status);

                status = SyncAwait(physicalLog->DeleteLogicalLogAsync(logicalLogId, CancellationToken::None));
                VERIFY_STATUS_SUCCESS("DeleteLogicalLogAsync", status);

                // prove neither physical log nor logical log work
                SyncAwait(logicalLog->AppendAsync(*buf, 0, 10, CancellationToken::None));
                status = SyncAwait(logicalLog->FlushWithMarkerAsync(CancellationToken::None));
                VERIFY_IS_FALSE(NT_SUCCESS(status));
                
                // Commented out as this bugchecks the machine
                /*logicalLog1 = nullptr;
                status = SyncAwait(physicalLog->CreateAndOpenLogicalLogAsync(
                    logicalLogId1,
                    alias,
                    *logicalLogName1,
                    nullptr,
                    1024 * 1024 * 100,
                    128 * 1024,
                    LogCreationFlags::UseSparseFile,
                    CancellationToken::None,
                    logicalLog1));
                VERIFY_IS_FALSE(NT_SUCCESS(status));
                VERIFY_IS_TRUE(logicalLog1 == nullptr);*/

                status = SyncAwait(physicalLog->AssignAliasAsync(logicalLogId, *logicalLogName, CancellationToken::None));
                VERIFY_IS_FALSE(NT_SUCCESS(status));

                status = SyncAwait(TryOpenFileAsync(physicalLogNameWString, GetThisAllocator()));
                VERIFY_IS_TRUE(!NT_SUCCESS(status) && status == STATUS_OBJECT_NAME_NOT_FOUND);

                status = SyncAwait(logicalLog->CloseAsync(CancellationToken::None));
                VERIFY_STATUS_SUCCESS("LogicalLog::CloseAsync", status);

                status = SyncAwait(physicalLog->CloseAsync(CancellationToken::None));
                VERIFY_STATUS_SUCCESS("PhysicalLog::CloseAsync", status);
            }

            //
            // Test calling DeleteLogicalLogAsync after DeleteLogicalLogOnlyAsync
            //
            {
                status = CreatePhysicalLog(*logManager, physicalLogId, *physicalLogName, physicalLog);
                VERIFY_STATUS_SUCCESS("CreatePhysicalLog", status);

                status = CreateLogicalLog(*physicalLog, logicalLogId, *logicalLogName, logicalLog);
                VERIFY_STATUS_SUCCESS("CreateLogicalLog", status);

                status = SyncAwait(physicalLog->AssignAliasAsync(logicalLogId, *logicalLogName, CancellationToken::None));
                VERIFY_STATUS_SUCCESS("AssignAliasAsync", status);

                status = SyncAwait(physicalLog->ResolveAliasAsync(*logicalLogName, CancellationToken::None, id));
                VERIFY_STATUS_SUCCESS("ResolveAliasAsync", status);

                status = SyncAwait(TryOpenFileAsync(physicalLogNameWString, GetThisAllocator()));
                VERIFY_IS_TRUE(NT_SUCCESS(status) || status == STATUS_SHARING_VIOLATION);

                status = SyncAwait(logicalLog->AppendAsync(*buf, 0, 10, CancellationToken::None));
                VERIFY_STATUS_SUCCESS("AppendAsync", status);

                status = SyncAwait(logicalLog->FlushWithMarkerAsync(CancellationToken::None));
                VERIFY_STATUS_SUCCESS("FlushWithMarkerAsync", status);

                status = SyncAwait(physicalLog->DeleteLogicalLogOnlyAsync(logicalLogId, CancellationToken::None));
                VERIFY_STATUS_SUCCESS("DeleteLogicalLogOnlyAsync", status);

                SyncAwait(logicalLog->AppendAsync(*buf, 0, 10, CancellationToken::None));
                status = SyncAwait(logicalLog->FlushWithMarkerAsync(CancellationToken::None));
                VERIFY_IS_FALSE(NT_SUCCESS(status));

                status = SyncAwait(logicalLog->CloseAsync(CancellationToken::None));
                VERIFY_STATUS_SUCCESS("LogicalLog::CloseAsync", status);

                status = CreateLogicalLog(*physicalLog, logicalLogId, *logicalLogName, logicalLog);
                VERIFY_STATUS_SUCCESS("CreateLogicalLog", status);

                status = SyncAwait(physicalLog->AssignAliasAsync(logicalLogId, *logicalLogName, CancellationToken::None));
                VERIFY_STATUS_SUCCESS("AssignAliasAsync", status);

                status = SyncAwait(physicalLog->ResolveAliasAsync(*logicalLogName, CancellationToken::None, id));
                VERIFY_STATUS_SUCCESS("ResolveAliasAsync", status);

                status = SyncAwait(TryOpenFileAsync(physicalLogNameWString, GetThisAllocator()));
                VERIFY_IS_TRUE(NT_SUCCESS(status) || status == STATUS_SHARING_VIOLATION);

                status = SyncAwait(physicalLog->DeleteLogicalLogAsync(logicalLogId, CancellationToken::None));

                SyncAwait(logicalLog->AppendAsync(*buf, 0, 10, CancellationToken::None));
                status = SyncAwait(logicalLog->FlushWithMarkerAsync(CancellationToken::None));
                VERIFY_IS_FALSE(NT_SUCCESS(status));

                // Commented out as this bugchecks the machine
                /*logicalLog1 = nullptr;
                status = SyncAwait(physicalLog->CreateAndOpenLogicalLogAsync(
                    logicalLogId1,
                    alias,
                    *logicalLogName1,
                    nullptr,
                    1024 * 1024 * 100,
                    128 * 1024,
                    LogCreationFlags::UseSparseFile,
                    CancellationToken::None,
                    logicalLog1));
                VERIFY_IS_FALSE(NT_SUCCESS(status));
                VERIFY_IS_TRUE(logicalLog1 == nullptr);*/

                status = SyncAwait(physicalLog->ResolveAliasAsync(*logicalLogName, CancellationToken::None, id));
                VERIFY_IS_FALSE(NT_SUCCESS(status));

                status = SyncAwait(TryOpenFileAsync(physicalLogNameWString, GetThisAllocator()));
                VERIFY_IS_TRUE(!NT_SUCCESS(status) && status == STATUS_OBJECT_NAME_NOT_FOUND);

                status = SyncAwait(logicalLog->CloseAsync(CancellationToken::None));
                VERIFY_STATUS_SUCCESS("LogicalLog::CloseAsync", status);

                status = SyncAwait(physicalLog->CloseAsync(CancellationToken::None));
                VERIFY_STATUS_SUCCESS("PhysicalLog::CloseAsync", status);
            }

            //
            // Test the physical log being deleted only when the last logical log is deleted
            //
            {
                status = CreatePhysicalLog(*logManager, physicalLogId, *physicalLogName, physicalLog);
                VERIFY_STATUS_SUCCESS("CreatePhysicalLog", status);

                status = CreateLogicalLog(*physicalLog, logicalLogId, *logicalLogName, logicalLog);
                VERIFY_STATUS_SUCCESS("CreateLogicalLog", status);

                status = SyncAwait(physicalLog->AssignAliasAsync(logicalLogId, *logicalLogName, CancellationToken::None));
                VERIFY_STATUS_SUCCESS("AssignAliasAsync", status);

                status = SyncAwait(physicalLog->ResolveAliasAsync(*logicalLogName, CancellationToken::None, id));
                VERIFY_STATUS_SUCCESS("ResolveAliasAsync", status);

                status = SyncAwait(TryOpenFileAsync(physicalLogNameWString, GetThisAllocator()));
                VERIFY_IS_TRUE(NT_SUCCESS(status) || status == STATUS_SHARING_VIOLATION);

                status = CreateLogicalLog(*physicalLog, logicalLogId1, *logicalLogName1, logicalLog1);
                VERIFY_STATUS_SUCCESS("CreateLogicalLog", status);

                // prove both logical logs work
                status = SyncAwait(logicalLog->AppendAsync(*buf, 0, 10, CancellationToken::None));
                VERIFY_STATUS_SUCCESS("AppendAsync", status);

                status = SyncAwait(logicalLog->FlushWithMarkerAsync(CancellationToken::None));
                VERIFY_STATUS_SUCCESS("FlushWithMarkerAsync", status);

                status = SyncAwait(logicalLog1->AppendAsync(*buf, 0, 10, CancellationToken::None));
                VERIFY_STATUS_SUCCESS("AppendAsync", status);

                status = SyncAwait(logicalLog1->FlushWithMarkerAsync(CancellationToken::None));
                VERIFY_STATUS_SUCCESS("FlushWithMarkerAsync", status);

                // Delete logicalLog1 and prove that logicalLog and physicalLog are not deleted
                status = SyncAwait(physicalLog->DeleteLogicalLogAsync(logicalLogId1, CancellationToken::None));
                VERIFY_STATUS_SUCCESS("DeleteLogicalLogAsync", status);

                SyncAwait(logicalLog1->AppendAsync(*buf, 0, 10, CancellationToken::None));
                status = SyncAwait(logicalLog1->FlushWithMarkerAsync(CancellationToken::None));
                VERIFY_IS_FALSE(NT_SUCCESS(status));

                status = SyncAwait(logicalLog1->CloseAsync(CancellationToken::None));
                VERIFY_STATUS_SUCCESS("LogicalLog::CloseAsync", status);

                status = SyncAwait(logicalLog->AppendAsync(*buf, 0, 10, CancellationToken::None));
                VERIFY_STATUS_SUCCESS("AppendAsync", status);

                status = SyncAwait(logicalLog->FlushWithMarkerAsync(CancellationToken::None));
                VERIFY_STATUS_SUCCESS("FlushWithMarkerAsync", status);

                status = SyncAwait(physicalLog->AssignAliasAsync(logicalLogId, *logicalLogName, CancellationToken::None));
                VERIFY_STATUS_SUCCESS("AssignAliasAsync", status);

                status = SyncAwait(physicalLog->ResolveAliasAsync(*logicalLogName, CancellationToken::None, id));
                VERIFY_STATUS_SUCCESS("ResolveAliasAsync", status);

                status = SyncAwait(TryOpenFileAsync(physicalLogNameWString, GetThisAllocator()));
                VERIFY_IS_TRUE(NT_SUCCESS(status) || status == STATUS_SHARING_VIOLATION);

                // Delete logicalLog and prove that logicalLog and physicalLog are deleted
                status = SyncAwait(physicalLog->DeleteLogicalLogAsync(logicalLogId, CancellationToken::None));
                VERIFY_STATUS_SUCCESS("DeleteLogicalLogAsync", status);

                SyncAwait(logicalLog->AppendAsync(*buf, 0, 10, CancellationToken::None));
                status = SyncAwait(logicalLog->FlushWithMarkerAsync(CancellationToken::None));
                VERIFY_IS_FALSE(NT_SUCCESS(status));

                // Commented out as this bugchecks the machine
                /*logicalLog1 = nullptr;
                status = SyncAwait(physicalLog->CreateAndOpenLogicalLogAsync(
                    logicalLogId1,
                    alias,
                    *logicalLogName1,
                    nullptr,
                    1024 * 1024 * 100,
                    128 * 1024,
                    LogCreationFlags::UseSparseFile,
                    CancellationToken::None,
                    logicalLog1));
                VERIFY_IS_FALSE(NT_SUCCESS(status));
                VERIFY_IS_TRUE(logicalLog1 == nullptr);*/

                status = SyncAwait(physicalLog->ResolveAliasAsync(*logicalLogName, CancellationToken::None, id));
                VERIFY_IS_FALSE(NT_SUCCESS(status));
                
                status = SyncAwait(TryOpenFileAsync(physicalLogNameWString, GetThisAllocator()));
                VERIFY_IS_TRUE(!NT_SUCCESS(status) && status == STATUS_OBJECT_NAME_NOT_FOUND);

                status = SyncAwait(logicalLog->CloseAsync(CancellationToken::None));
                VERIFY_STATUS_SUCCESS("LogicalLog::CloseAsync", status);

                status = SyncAwait(physicalLog->CloseAsync(CancellationToken::None));
                VERIFY_STATUS_SUCCESS("PhysicalLog::CloseAsync", status);
            }

            //
            // Test that the physical log should only be deleted when the logical log is successfully deleted
            //
            {
                status = CreatePhysicalLog(*logManager, physicalLogId, *physicalLogName, physicalLog);
                VERIFY_STATUS_SUCCESS("CreatePhysicalLog", status);

                status = SyncAwait(TryOpenFileAsync(physicalLogNameWString, GetThisAllocator()));
                VERIFY_IS_TRUE(NT_SUCCESS(status) || status == STATUS_SHARING_VIOLATION);

                status = SyncAwait(physicalLog->DeleteLogicalLogAsync(KGuid(TRUE), CancellationToken::None));
                VERIFY_IS_FALSE(NT_SUCCESS(status));

                status = SyncAwait(TryOpenFileAsync(physicalLogNameWString, GetThisAllocator()));
                VERIFY_IS_TRUE(NT_SUCCESS(status) || status == STATUS_SHARING_VIOLATION);

                status = CreateLogicalLog(*physicalLog, logicalLogId, *logicalLogName, logicalLog);
                VERIFY_STATUS_SUCCESS("CreateLogicalLog", status);

                status = SyncAwait(TryOpenFileAsync(physicalLogNameWString, GetThisAllocator()));
                VERIFY_IS_TRUE(NT_SUCCESS(status) || status == STATUS_SHARING_VIOLATION);

                status = SyncAwait(physicalLog->DeleteLogicalLogAsync(logicalLogId, CancellationToken::None));
                VERIFY_STATUS_SUCCESS("DeleteLogicalLogAsync", status);

                SyncAwait(logicalLog->AppendAsync(*buf, 0, 10, CancellationToken::None));
                status = SyncAwait(logicalLog->FlushWithMarkerAsync(CancellationToken::None));
                VERIFY_IS_FALSE(NT_SUCCESS(status));

                // Commented out as this bugchecks the machine
                /*logicalLog1 = nullptr;
                status = SyncAwait(physicalLog->CreateAndOpenLogicalLogAsync(
                    logicalLogId1,
                    alias,
                    *logicalLogName1,
                    nullptr,
                    1024 * 1024 * 100,
                    128 * 1024,
                    LogCreationFlags::UseSparseFile,
                    CancellationToken::None,
                    logicalLog1));
                VERIFY_IS_FALSE(NT_SUCCESS(status));
                VERIFY_IS_TRUE(logicalLog1 == nullptr);*/

                status = SyncAwait(TryOpenFileAsync(physicalLogNameWString, GetThisAllocator()));
                VERIFY_IS_TRUE(!NT_SUCCESS(status) && status == STATUS_OBJECT_NAME_NOT_FOUND);

                status = SyncAwait(logicalLog->CloseAsync(CancellationToken::None));
                VERIFY_STATUS_SUCCESS("LogicalLog::CloseAsync", status);

                status = SyncAwait(physicalLog->CloseAsync(CancellationToken::None));
                VERIFY_STATUS_SUCCESS("PhysicalLog::CloseAsync", status);
            }

            status = SyncAwait(logManager->CloseAsync(CancellationToken::None));
            VERIFY_STATUS_SUCCESS("LogManager::CloseAsync", status);
        }

    private:

        LogicalLogTest(__in Data::Utilities::PartitionedReplicaId & prId, __in KtlLoggerMode ktlLoggerMode);

        KtlLoggerMode ktlLoggerMode_;
        Data::Utilities::PartitionedReplicaId::SPtr prId_;
        LogManager::SPtr logManager_;
    };

    LogicalLogTest::LogicalLogTest(__in Data::Utilities::PartitionedReplicaId & prId, __in KtlLoggerMode ktlLoggerMode)
        : prId_(&prId)
        , ktlLoggerMode_(ktlLoggerMode)
    {
        NTSTATUS status;

        auto settings = std::make_unique<KtlLogManager::SharedLogContainerSettings>();
        KString::SPtr filename;
        GenerateUniqueFilename(filename);

        KInvariant(filename->LengthInBytes() + sizeof(WCHAR) < 512 * sizeof(WCHAR)); // check to make sure there is space for the null terminator
        KMemCpySafe(&settings->Path[0], 512 * sizeof(WCHAR), filename->operator PVOID(), filename->LengthInBytes());
        settings->Path[filename->LengthInBytes() / sizeof(WCHAR)] = L'\0'; // set the null terminator
        settings->LogContainerId = static_cast<RvdLogId const &>(KGuid(Common::Guid::NewGuid().AsGUID())); // This must not be the default application id, as that ignores the path and uses the mapping set up in the driver
        settings->LogSize = 1024 * 1024 * 1024;
        settings->MaximumNumberStreams = 0;
        settings->MaximumRecordSize = 0;
        settings->Flags = static_cast<ULONG>(LogCreationFlags::UseSparseFile);

        KtlLogger::SharedLogSettingsSPtr sharedLogSettings = make_shared<KtlLogger::SharedLogSettings>(std::move(settings));
        
        status = LogManager::Create(GetThisAllocator(), logManager_);
        VERIFY_STATUS_SUCCESS("LogManager::Create", status);

        status = SyncAwait(logManager_->OpenAsync(CancellationToken::None, sharedLogSettings, ktlLoggerMode));
        VERIFY_STATUS_SUCCESS("LogManager::OpenAsync", status);
    }

    LogicalLogTest::~LogicalLogTest()
    {
        //
        // No need to check the return status as it may fail due to
        // multiple log manager handles being open and closing in which
        // case the second log manager close would return an error
        //
        SyncAwait(logManager_->CloseAsync(CancellationToken::None));
    }

    class LogicalLogTests
    {
    protected:

        VOID BeginTest()
        {
            NTSTATUS status;

            status = LogicalLogTest::Create(*allocator_, *prId_, ktlLoggerMode_, testContext_);
            VERIFY_STATUS_SUCCESS("LogicalLogTest::Create", status);

#if defined(UDRIVER)
            //
            // For UDRIVER, need to perform work done in PNP Device Add
            //
            status = SyncAwait(FileObjectTable::CreateAndRegisterOverlayManagerAsync(underlyingSystem_->NonPagedAllocator(), KTL_TAG_TEST));
            VERIFY_STATUS_SUCCESS("FileObjectTable::CreateAndRegisterOverlayManagerAsync", NT_SUCCESS(status));
#endif

            testContext_->CreateTestDirectory();
        }

        VOID EndTest()
        {
            // todo: assert that the test cleaned up after itself (e.g. deletephysicallog etc. deleted all the files)
            testContext_->DeleteTestDirectory();
            testContext_ = nullptr;

#if defined(UDRIVER)
            NTSTATUS status;
            //
            // For UDRIVER need to perform work done in PNP RemoveDevice
            //
            status = SyncAwait(FileObjectTable::StopAndUnregisterOverlayManagerAsync(underlyingSystem_->NonPagedAllocator()));
            VERIFY_STATUS_SUCCESS("FileObjectTable::StopAndUnregisterOverlayManagerAsync", NT_SUCCESS(status));
#endif
        };

        LogicalLogTests() {}
        ~LogicalLogTests() {}
        
        LogicalLogTest::SPtr testContext_;
        Common::CommonConfig config; // load the config object as its needed for the tracing to work
        ::FABRIC_REPLICA_ID rId_;
        KGuid pId_;
        Data::Utilities::PartitionedReplicaId::SPtr prId_;
        KtlSystem * underlyingSystem_;
        KAllocator* allocator_;
        KtlLoggerMode ktlLoggerMode_ = KtlLoggerMode::Default;
    };

#pragma region FabricOpenClose

    void Test_Fabric_FinishLogManagerOpen(
        __in Common::AsyncOperationSPtr const & openOperation,
        __in LogManager& logManager,
        __in KAsyncEvent& e,
        __out bool& openSuccess)
    {
        Common::ErrorCode error = logManager.EndOpen(openOperation);
        openSuccess = error.IsSuccess();

        e.SetEvent();
    }

    void Test_Fabric_OnOpenLogManagerCompleted(
        __in Common::AsyncOperationSPtr const & openOperation,
        __in LogManager& logManager,
        __in KAsyncEvent& e,
        __out bool& openSuccess)
    {
        if (!openOperation->CompletedSynchronously)
        {
            Test_Fabric_FinishLogManagerOpen(openOperation, logManager, e, openSuccess);
        }
    }

    void Test_Fabric_OpenLogManager(
        __in LogManager& logManager,
        __in KAsyncEvent& e,
        __out bool& openSuccess)
    {
        auto openOperation = logManager.BeginOpen(
            [&](Common::AsyncOperationSPtr const & openOperation) { Test_Fabric_OnOpenLogManagerCompleted(openOperation, logManager, e, openSuccess); },
            Common::AsyncOperationRoot<bool>::Create(true),
            CancellationToken::None);

        if (openOperation->CompletedSynchronously)
        {
            Test_Fabric_FinishLogManagerOpen(openOperation, logManager, e, openSuccess);
        }
    }

    void Test_Fabric_FinishLogManagerClose(
        __in Common::AsyncOperationSPtr const & closeOperation,
        __in LogManager& logManager,
        __in KAsyncEvent& e,
        __out bool& closeSuccess)
    {
        Common::ErrorCode error = logManager.EndClose(closeOperation);
        closeSuccess = error.IsSuccess();

        e.SetEvent();
    }

    void Test_Fabric_OnCloseLogManagerCompleted(
        __in Common::AsyncOperationSPtr const & closeOperation,
        __in LogManager& logManager,
        __in KAsyncEvent& e,
        __out bool& closeSuccess)
    {
        if (!closeOperation->CompletedSynchronously)
        {
            Test_Fabric_FinishLogManagerClose(closeOperation, logManager, e, closeSuccess);
        }
    }

    void Test_Fabric_CloseLogManager(
        __in LogManager& logManager,
        __in KAsyncEvent& e,
        __out bool& closeSuccess)
    {
        auto closeOperation = logManager.BeginClose(
            [&](Common::AsyncOperationSPtr const & closeOperation) { Test_Fabric_OnCloseLogManagerCompleted(closeOperation, logManager, e, closeSuccess); },
            Common::AsyncOperationRoot<bool>::Create(true),
            CancellationToken::None);

        if (closeOperation->CompletedSynchronously)
        {
            Test_Fabric_FinishLogManagerClose(closeOperation, logManager, e, closeSuccess);
        }
    }

    class TestFabricApphostAnalog
    {
    public:

        TestFabricApphostAnalog()
            : operationRoot_(Common::AsyncOperationRoot<bool>::Create(false))
            , openEvent_()
            , closeEvent_()
        {
        }

    private:

        void FinishLogManagerOpen(__in Common::AsyncOperationSPtr const & openOperation)
        {
            Common::ErrorCode error = logManager_->EndOpen(openOperation);
            VERIFY_IS_TRUE(error.IsSuccess());
            openEvent_.SetEvent();
        }

        void OnOpenLogManagerCompleted(__in Common::AsyncOperationSPtr const & openOperation)
        {
            if (!openOperation->CompletedSynchronously)
            {
                FinishLogManagerOpen(openOperation);
            }
        }

        void OpenLogManager()
        {
            NTSTATUS status;

            status = LogManager::Create(ktlSystem_->NonPagedAllocator(), logManager_);
            VERIFY_STATUS_SUCCESS("LogManager::Create", status);

            auto openOperation = logManager_->BeginOpen(
                [&](Common::AsyncOperationSPtr const & openOperation) { OnOpenLogManagerCompleted(openOperation); },
                Common::AsyncOperationRoot<bool>::Create(true),
                CancellationToken::None);

            if (openOperation->CompletedSynchronously)
            {
                FinishLogManagerOpen(openOperation);
            }
        }

        void OpenKtlSystem()
        {
            KGuid ktlSystemGuid;
            ktlSystemGuid.CreateNew();

            ktlSystem_ = make_unique<KtlSystemBase>(ktlSystemGuid);
            ktlSystem_->Activate(10000);
            VERIFY_STATUS_SUCCESS("ktlSystem", ktlSystem_->Status());

            ktlSystem_->SetStrictAllocationChecks(TRUE);

            OpenLogManager();
        }

        void ShutdownKtlSystem()
        {
            ktlSystem_->Deactivate(10000);
            VERIFY_STATUS_SUCCESS("ktlSystem", ktlSystem_->Status());
            closeEvent_.SetEvent();
        }

        void FinishCloseLogManager(__in Common::AsyncOperationSPtr const & closeOperation)
        {
            Common::ErrorCode error = logManager_->EndClose(closeOperation);
            VERIFY_IS_TRUE(error.IsSuccess());

            logManager_ = nullptr;


            // Post this on the threadpool since we are currently on a ktl thread, and deactivating on a ktl
            // thread is not allowed
            Common::Threadpool::Post(
                [this]()
            {
                ShutdownKtlSystem();
            });
        }

        void OnCloseLogManagerCompleted(__in Common::AsyncOperationSPtr const & closeOperation)
        {
            if (!closeOperation->CompletedSynchronously)
            {
                FinishCloseLogManager(closeOperation);
            }
        }

        void CloseLogManager()
        {
            auto closeOperation =
                logManager_->BeginClose(
                    [this](Common::AsyncOperationSPtr const & operation) { OnCloseLogManagerCompleted(operation); },
                    operationRoot_,
                    CancellationToken::None);

            if (closeOperation->CompletedSynchronously)
            {
                FinishCloseLogManager(closeOperation);
            }
        }

    public:

        void StartOpen()
        {
            OpenKtlSystem();
        }

        void StartClose()
        {
            CloseLogManager();
        }

        KEvent openEvent_;
        KEvent closeEvent_;
        std::unique_ptr<KtlSystemBase> ktlSystem_;
        Common::AsyncOperationSPtr operationRoot_;
        LogManager::SPtr logManager_;
    };

    void FabricOpenCloseSanityTest(KtlSystem* underlyingSystem)
    {
        NTSTATUS tStatus;

        LogManager::SPtr logManager;
        tStatus = LogManager::Create(underlyingSystem->NonPagedAllocator(), logManager);
        VERIFY_STATUS_SUCCESS("LogManager::Create", tStatus);

        KAsyncEvent openedEvent;
        KAsyncEvent::WaitContext::SPtr openWaitContext;
        tStatus = openedEvent.CreateWaitContext(KTL_TAG_TEST, underlyingSystem->NonPagedAllocator(), openWaitContext);
        VERIFY_STATUS_SUCCESS("KAsyncEvent::WaitContext::Create", tStatus);

        KAsyncEvent closedEvent;
        KAsyncEvent::WaitContext::SPtr closeWaitContext;
        tStatus = closedEvent.CreateWaitContext(KTL_TAG_TEST, underlyingSystem->NonPagedAllocator(), closeWaitContext);
        VERIFY_STATUS_SUCCESS("KAsyncEvent::WaitContext::Create", tStatus);

        VERIFY_IS_FALSE(logManager->IsOpen());

        bool openSuccess;
        Test_Fabric_OpenLogManager(*logManager, openedEvent, openSuccess);
        tStatus = SyncAwait(openWaitContext->StartWaitUntilSetAsync(nullptr));
        VERIFY_STATUS_SUCCESS("openWaitContext->StartWaitUntilSetAsync", tStatus);
        VERIFY_IS_TRUE(openSuccess);

        VERIFY_IS_TRUE(logManager->IsOpen());

        bool closeSuccess;
        Test_Fabric_CloseLogManager(*logManager, closedEvent, closeSuccess);
        tStatus = SyncAwait(closeWaitContext->StartWaitUntilSetAsync(nullptr));
        VERIFY_STATUS_SUCCESS("closeWaitContext->StartWaitUntilSetAsync", tStatus);
        VERIFY_IS_TRUE(closeSuccess);

        VERIFY_IS_FALSE(logManager->IsOpen());
    }

    void LogManagerAppHostTypeTest()
    {
        TestFabricApphostAnalog testContext;

        VERIFY_ARE_EQUAL(nullptr, testContext.logManager_.RawPtr());
        VERIFY_ARE_EQUAL(nullptr, testContext.ktlSystem_);

        testContext.StartOpen();
        testContext.openEvent_.WaitUntilSet();

        VERIFY_ARE_NOT_EQUAL(nullptr, testContext.logManager_.RawPtr());
        VERIFY_ARE_NOT_EQUAL(nullptr, testContext.ktlSystem_);
        VERIFY_IS_TRUE(testContext.logManager_->IsOpen());

        testContext.StartClose();
        testContext.closeEvent_.WaitUntilSet();

        VERIFY_ARE_EQUAL(nullptr, testContext.logManager_.RawPtr());
        VERIFY_STATUS_SUCCESS("ktlSystem", testContext.ktlSystem_->Status());
    }

#pragma endregion FabricOpenClose

    BOOST_GLOBAL_FIXTURE(TracingInit);
    
    BOOST_FIXTURE_TEST_SUITE(LogicalLogTestsSuite, LogicalLogTests)

#if !defined(UDRIVER)
    BOOST_AUTO_TEST_CASE(LogicalLog_LogSize_SpaceRemaining_InProc)
    {
        ktlLoggerMode_ = KtlLoggerMode::InProc;
        TEST_TRACE_BEGIN("LogicalLog_LogSize_SpaceRemaining_InProc")
        {
            testContext_->LogSizeSpaceRemainingTest();
        }
    }
#endif

#if !defined(PLATFORM_UNIX)
    BOOST_AUTO_TEST_CASE(LogicalLog_LogSize_SpaceRemaining_OutOfProc)
    {
        ktlLoggerMode_ = KtlLoggerMode::OutOfProc;
        TEST_TRACE_BEGIN("LogicalLog_LogSize_SpaceRemaining_OutOfProc")
        {
            testContext_->LogSizeSpaceRemainingTest();
        }
    }
#endif

#if !defined(UDRIVER)
    // This test case is in-proc only, as deleting the shared log when the last logical log within it is deleted
    // is disabled for out-of-proc (system shared) cases
    BOOST_AUTO_TEST_CASE(LogicalLog_DeleteLogicalLogAndPhysicalLogTest_InProc)
    {
        ktlLoggerMode_ = KtlLoggerMode::InProc;
        TEST_TRACE_BEGIN("LogicalLog_DeleteLogicalLogAndPhysicalLogTest_InProc")
        {
            testContext_->DeleteLogicalLogAndPhysicalLogTest();
        }
    }
#endif

#if !defined(UDRIVER)
    BOOST_AUTO_TEST_CASE(LogicalLog_LogManager_FabricOpenCloseSanity_InProc)
    {
        ktlLoggerMode_ = KtlLoggerMode::InProc;
        TEST_TRACE_BEGIN("LogicalLog_LogManager_FabricOpenCloseSanity_InProc")
        {
            FabricOpenCloseSanityTest(underlyingSystem_);
        }
    }
#endif

// todo: when this failure is made graceful (it currently fails via KInvariant), add this as a negative test case on Linux
#if !defined(PLATFORM_UNIX)
    BOOST_AUTO_TEST_CASE(LogicalLog_LogManager_FabricOpenCloseSanity_OutOfProc)
    {
        ktlLoggerMode_ = KtlLoggerMode::OutOfProc;
        TEST_TRACE_BEGIN("LogicalLog_LogManager_FabricOpenCloseSanity_OutOfProc")
        {
            FabricOpenCloseSanityTest(underlyingSystem_);
        }
    }
#endif
    
#if !defined(UDRIVER)
    BOOST_AUTO_TEST_CASE(LogicalLog_LogManager_ApphostTypeTest)
    {
        // Will open the log in default mode
        LogManagerAppHostTypeTest();
    }
#endif

#if !defined(UDRIVER)
    BOOST_AUTO_TEST_CASE(LogicalLog_OpenManyStreams_InProc)
    {
        ktlLoggerMode_ = KtlLoggerMode::InProc;
        TEST_TRACE_BEGIN("LogicalLog_OpenManyStreams_InProc")
        {
            testContext_->OpenManyStreamsTest(ktlLoggerMode_);
        }
    }
#endif

// todo: when this failure is made graceful (it currently fails via KInvariant), add this as a negative test case on Linux
#if !defined(PLATFORM_UNIX)
    BOOST_AUTO_TEST_CASE(LogicalLog_OpenManyStreams_OutOfProc)
    {
        ktlLoggerMode_ = KtlLoggerMode::OutOfProc;
        TEST_TRACE_BEGIN("LogicalLog_OpenManyStreams_OutOfProc")
        {
            testContext_->OpenManyStreamsTest(ktlLoggerMode_);
        }
    }
#endif

#if !defined(UDRIVER)
    BOOST_AUTO_TEST_CASE(LogicalLog_BasicTest_Default)
    {
        ktlLoggerMode_ = KtlLoggerMode::Default;
        TEST_TRACE_BEGIN("LogicalLog_BasicTest_Default")
        {
            testContext_->BasicTest();
        }
    }
#endif

#if !defined(UDRIVER)
    BOOST_AUTO_TEST_CASE(LogicalLog_BasicTest_InProc)
    {
        ktlLoggerMode_ = KtlLoggerMode::InProc;
        TEST_TRACE_BEGIN("LogicalLog_BasicTest_InProc")
        {
            testContext_->BasicTest();
        }
    }
#endif

// todo: when this failure is made graceful (it currently fails via KInvariant), add this as a negative test case on Linux
#if !defined(PLATFORM_UNIX)
    BOOST_AUTO_TEST_CASE(LogicalLog_BasicTest_OutOfProc)
    {
        ktlLoggerMode_ = KtlLoggerMode::OutOfProc;
        TEST_TRACE_BEGIN("LogicalLog_BasicTest_OutOfProc")
        {
            testContext_->BasicTest();
        }
    }
#endif

#if !defined(UDRIVER)
    //        KGuid id;
    //        id.CreateNew();
    BOOST_AUTO_TEST_CASE(LogicalLog_BasicIOTest_InProc)
    {
        ktlLoggerMode_ = KtlLoggerMode::InProc;
        TEST_TRACE_BEGIN("LogicalLog_BasicIOTest_InProc")
        {
            KGuid id;
            id.CreateNew();
            testContext_->BasicIOTest(ktlLoggerMode_, id);
        }
    }

    BOOST_AUTO_TEST_CASE(LogicalLog_BasicIOTest_InProc_StagingLog_KtlPrefix)
    {
        KInvariant(LogTestBase::g_UseKtlFilePrefix);
        ktlLoggerMode_ = KtlLoggerMode::InProc;
        TEST_TRACE_BEGIN("LogicalLog_BasicIOTest_InProc_StagingLog_KtlPrefix")
        {
            KGuid id = KtlLogger::Constants::DefaultApplicationSharedLogId.AsGUID();
            testContext_->BasicIOTest(ktlLoggerMode_, id);
        }
        KInvariant(LogTestBase::g_UseKtlFilePrefix);
    }

    BOOST_AUTO_TEST_CASE(LogicalLog_BasicIOTest_InProc_StagingLog_NoKtlPrefix)
    {
        KInvariant(LogTestBase::g_UseKtlFilePrefix);
        LogTestBase::g_UseKtlFilePrefix = false;
        ktlLoggerMode_ = KtlLoggerMode::InProc;
        TEST_TRACE_BEGIN("LogicalLog_BasicIOTest_InProc_StagingLog_NoKtlPrefix")
        {
            KGuid id = KtlLogger::Constants::DefaultApplicationSharedLogId.AsGUID();
            testContext_->BasicIOTest(ktlLoggerMode_, id);
        }
        KInvariant(!LogTestBase::g_UseKtlFilePrefix);
        LogTestBase::g_UseKtlFilePrefix = true;
    }
#endif

// todo: when this failure is made graceful (it currently fails via KInvariant), add this as a negative test case on Linux
#if !defined(PLATFORM_UNIX)
    BOOST_AUTO_TEST_CASE(LogicalLog_BasicIOTest_OutOfProc)
    {
        ktlLoggerMode_ = KtlLoggerMode::OutOfProc;
        TEST_TRACE_BEGIN("LogicalLog_BasicIOTest_OutOfProc")
        {
            KGuid id;
            id.CreateNew();
            testContext_->BasicIOTest(ktlLoggerMode_, id);
        }
    }
#endif

#if !defined(UDRIVER)
    BOOST_AUTO_TEST_CASE(LogicalLog_WriteAtTruncationPointsTest_InProc)
    {
        ktlLoggerMode_ = KtlLoggerMode::InProc;
        TEST_TRACE_BEGIN("LogicalLog_WriteAtTruncationPointsTest_InProc")
        {
            testContext_->WriteAtTruncationPointsTest(ktlLoggerMode_);
        }
    }
#endif

// todo: when this failure is made graceful (it currently fails via KInvariant), add this as a negative test case on Linux
#if !defined(PLATFORM_UNIX)
    BOOST_AUTO_TEST_CASE(LogicalLog_WriteAtTruncationPointsTest_OutOfProc)
    {
        ktlLoggerMode_ = KtlLoggerMode::OutOfProc;
        TEST_TRACE_BEGIN("LogicalLog_WriteAtTruncationPointsTest_OutOfProc")
        {
            testContext_->WriteAtTruncationPointsTest(ktlLoggerMode_);
        }
    }
#endif

#if !defined(UDRIVER)
    BOOST_AUTO_TEST_CASE(LogicalLog_RemoveFalseProgressTest_InProc)
    {
        ktlLoggerMode_ = KtlLoggerMode::InProc;
        TEST_TRACE_BEGIN("LogicalLog_RemoveFalseProgressTest_InProc")
        {
            testContext_->RemoveFalseProgressTest(ktlLoggerMode_);
        }
    }
#endif

// todo: when this failure is made graceful (it currently fails via KInvariant), add this as a negative test case on Linux
#if !defined(PLATFORM_UNIX)
    BOOST_AUTO_TEST_CASE(LogicalLog_RemoveFalseProgressTest_OutOfProc)
    {
        ktlLoggerMode_ = KtlLoggerMode::OutOfProc;
        TEST_TRACE_BEGIN("LogicalLog_RemoveFalseProgressTest_OutOfProc")
        {
            testContext_->RemoveFalseProgressTest(ktlLoggerMode_);
        }
    }
#endif

#if !defined(UDRIVER)
    BOOST_AUTO_TEST_CASE(LogicalLog_ReadAheadCacheTest_InProc)
    {
        ktlLoggerMode_ = KtlLoggerMode::InProc;
        TEST_TRACE_BEGIN("LogicalLog_ReadAheadCacheTest_InProc")
        {
            testContext_->ReadAheadCacheTest(ktlLoggerMode_);
        }
    }
#endif

// todo: when this failure is made graceful (it currently fails via KInvariant), add this as a negative test case on Linux
#if !defined(PLATFORM_UNIX)
    BOOST_AUTO_TEST_CASE(LogicalLog_ReadAheadCacheTest_OutOfProc)
    {
        ktlLoggerMode_ = KtlLoggerMode::OutOfProc;
        TEST_TRACE_BEGIN("LogicalLog_ReadAheadCacheTest_OutOfProc")
        {
            testContext_->ReadAheadCacheTest(ktlLoggerMode_);
        }
    }
#endif

#if !defined(UDRIVER)
    BOOST_AUTO_TEST_CASE(LogicalLog_TruncateInDataBufferTest_InProc)
    {
        ktlLoggerMode_ = KtlLoggerMode::InProc;
        TEST_TRACE_BEGIN("LogicalLog_TruncateInDataBufferTest_InProc")
        {
            testContext_->TruncateInDataBufferTest(ktlLoggerMode_);
        }
    }
#endif

// todo: when this failure is made graceful (it currently fails via KInvariant), add this as a negative test case on Linux
#if !defined(PLATFORM_UNIX)
    BOOST_AUTO_TEST_CASE(LogicalLog_TruncateInDataBufferTest_OutOfProc)
    {
        ktlLoggerMode_ = KtlLoggerMode::OutOfProc;
        TEST_TRACE_BEGIN("LogicalLog_TruncateInDataBufferTest_OutOfProc")
        {
            testContext_->TruncateInDataBufferTest(ktlLoggerMode_);
        }
    }
#endif

#if !defined(UDRIVER)
    BOOST_AUTO_TEST_CASE(LogicalLog_SequentialAndRandomStreamTest_InProc)
    {
        ktlLoggerMode_ = KtlLoggerMode::InProc;
        TEST_TRACE_BEGIN("LogicalLog_SequentialAndRandomStreamTest_InProc")
        {
            testContext_->SequentialAndRandomStreamTest(ktlLoggerMode_);
        }
    }
#endif

// todo: when this failure is made graceful (it currently fails via KInvariant), add this as a negative test case on Linux
#if !defined(PLATFORM_UNIX)
    BOOST_AUTO_TEST_CASE(LogicalLog_SequentialAndRandomStreamTest_OutOfProc)
    {
        ktlLoggerMode_ = KtlLoggerMode::OutOfProc;
        TEST_TRACE_BEGIN("LogicalLog_SequentialAndRandomStreamTest_OutOfProc")
        {
            testContext_->SequentialAndRandomStreamTest(ktlLoggerMode_);
        }
    }
#endif

#if !defined(UDRIVER)
    BOOST_AUTO_TEST_CASE(LogicalLog_ReadWriteCloseRaceTest_InProc)
    {
        ktlLoggerMode_ = KtlLoggerMode::InProc;
        TEST_TRACE_BEGIN("LogicalLog_ReadWriteCloseRaceTest_InProc")
        {
            testContext_->ReadWriteCloseRaceTest(ktlLoggerMode_);
        }
    }
#endif

// todo: when this failure is made graceful (it currently fails via KInvariant), add this as a negative test case on Linux
#if !defined(PLATFORM_UNIX)
    BOOST_AUTO_TEST_CASE(LogicalLog_ReadWriteCloseRaceTest_OutOfProc)
    {
        ktlLoggerMode_ = KtlLoggerMode::OutOfProc;
        TEST_TRACE_BEGIN("LogicalLog_ReadWriteCloseRaceTest_OutOfProc")
        {
            testContext_->ReadWriteCloseRaceTest(ktlLoggerMode_);
        }
    }
#endif

#if defined(UPASSTHROUGH) && !defined(UDRIVER)
    BOOST_AUTO_TEST_CASE(LogicalLog_UPassthroughErrors_InProc)
    {
        ktlLoggerMode_ = KtlLoggerMode::InProc;
        TEST_TRACE_BEGIN("LogicalLog_UPassthroughErrors_InProc")
        {
            testContext_->UPassthroughErrorsTest(ktlLoggerMode_);
        }
    }
#endif

    BOOST_AUTO_TEST_SUITE_END()
}
