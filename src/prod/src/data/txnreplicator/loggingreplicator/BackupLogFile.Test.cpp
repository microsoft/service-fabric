// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestHeaders.h"

#include <stdlib.h>
#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace LoggingReplicatorTests
{
    using namespace Common;
    using namespace ktl;
    using namespace Data::LogRecordLib;
    using namespace Data::LoggingReplicator;
    using namespace Data::Utilities;

    StringLiteral const TraceComponent("BackupLogFileTests");

    class BackupLogFileTests
    {
    public:
        Awaitable<void> Test_IncrementalBackup_AllLogRecordsWrittenAndRead(
            __in wstring & testName,
            __in ULONG32 iterationCount,
            __in ULONG32 randomRecordPerIteration,
            __in bool startWithElectionBarrier,
            __in bool includeMultipleBackupLogRecord, 
            __in bool includeMultipleUpdateEpochLogRecord);

    protected:
        KWString CreateFileName(
            __in KStringView const & inputName,
            __in KAllocator & allocator);

        void EndTest();

        CommonConfig config; // load the config object as its needed for the tracing to work
        FABRIC_REPLICA_ID rId_;
        KGuid pId_;
        PartitionedReplicaId::SPtr prId_;
        KtlSystem * underlyingSystem_;
    };

    Awaitable<void> BackupLogFileTests::Test_IncrementalBackup_AllLogRecordsWrittenAndRead(
        __in wstring & testName,
        __in ULONG32 iterationCount,
        __in ULONG32 randomRecordPerIteration,
        __in bool startWithElectionBarrier,
        __in bool includeMultipleBackupLogRecord,
        __in bool includeMultipleUpdateEpochLogRecord)
    {
        KFinally([&]
        {
            // Ignore error since this is only for clean up.
            File::Delete2(testName); 
        });

        TEST_TRACE_BEGIN(testName)
        {
            KWString fileName = CreateFileName(testName.c_str(), allocator);

            InvalidLogRecords::SPtr invalidLogRecords = InvalidLogRecords::Create(allocator);

            TestLogRecords::SPtr testLogRecords = TestLogRecords::Create(allocator);
            testLogRecords->InitializeWithNewLogRecords();
            testLogRecords->AddUpdateEpochLogRecord();

            if (startWithElectionBarrier == true)
            {
                testLogRecords->AddBarrierLogRecord();
            }

            // Insert full backup log record
            KGuid fullBackupId;
            fullBackupId.CreateNew();
            TxnReplicator::Epoch fullBackupEpoch = testLogRecords->GetLastEpoch();
            FABRIC_SEQUENCE_NUMBER fullBackupHighestBackedUpLSN = testLogRecords->GetLSN();

            BackupLogRecord::SPtr fullBackupLogRecordSPtr = BackupLogRecord::Create(
                fullBackupId,
                fullBackupEpoch,                // Highest backed up Epoch
                fullBackupHighestBackedUpLSN,   // Highest backed up LSN 
                testLogRecords->GetCount(),     // Backup log record count.
                300,                            // Size of the backup in KB
                *invalidLogRecords->Inv_PhysicalLogRecord,
                allocator);
            testLogRecords->AddBackupLogRecord(*fullBackupLogRecordSPtr);

            for (ULONG32 i = 0; i < iterationCount; i++)
            {
                testLogRecords->PopulateWithRandomRecords(
                    randomRecordPerIteration,
                    includeMultipleBackupLogRecord,
                    includeMultipleUpdateEpochLogRecord);
            }

            testLogRecords->Truncate(fullBackupHighestBackedUpLSN + 1, false);

            TestReplicatedLogManager::SPtr loggingReplicatorSPtr = TestReplicatedLogManager::Create(allocator);

            loggingReplicatorSPtr->SetProgressVector(Ktl::Move(testLogRecords->CopyProgressVector()));

            IncrementalBackupLogRecordsAsyncEnumerator::SPtr incrementalBackupLogRecordsSPtr = IncrementalBackupLogRecordsAsyncEnumerator::Create(
                *testLogRecords,
                *testLogRecords->GetLastBackupLogRecord(),
                *loggingReplicatorSPtr,
                allocator);
            KFinally([&] {incrementalBackupLogRecordsSPtr->Dispose(); });

            // Test write the file
            {
                BackupLogFile::SPtr backupLogFileSPtr = BackupLogFile::Create(
                    *prId_,
                    fileName,
                    allocator);

                // #9353924: co_awaiting this call causes the Strict Allocation check to fail
                SyncAwait(backupLogFileSPtr->WriteAsync(
                    *incrementalBackupLogRecordsSPtr,
                    *testLogRecords->GetLastBackupLogRecord(),
                    CancellationToken::None));
            }
            
            // Read Test
            {
                // Read the backup file
                BackupLogFile::SPtr readBackupLogFileSPtr = BackupLogFile::Create(
                    *prId_,
                    fileName,
                    allocator);

                SyncAwait(readBackupLogFileSPtr->ReadAsync(CancellationToken::None));

                // Verification
                VERIFY_ARE_EQUAL(readBackupLogFileSPtr->IndexingRecordEpoch.DataLossVersion, fullBackupEpoch.DataLossVersion);
                VERIFY_ARE_EQUAL(readBackupLogFileSPtr->IndexingRecordEpoch.ConfigurationVersion, fullBackupEpoch.ConfigurationVersion);
                VERIFY_ARE_EQUAL(readBackupLogFileSPtr->IndexingRecordLSN, fullBackupHighestBackedUpLSN + 1);

                VERIFY_ARE_EQUAL(readBackupLogFileSPtr->LastBackedUpEpoch.DataLossVersion, testLogRecords->GetLastEpoch().DataLossVersion);
                VERIFY_ARE_EQUAL(readBackupLogFileSPtr->LastBackedUpEpoch.ConfigurationVersion, testLogRecords->GetLastEpoch().ConfigurationVersion);
                VERIFY_ARE_EQUAL(readBackupLogFileSPtr->LastBackedUpLSN, testLogRecords->GetLSN());

                VERIFY_ARE_EQUAL(readBackupLogFileSPtr->Count, testLogRecords->GetLogicalLogRecordCount());

                // Verification of the log records.
                KArray<LogRecord::SPtr> outputLogRecords(allocator, testLogRecords->GetCount());
                BackupLogFileAsyncEnumerator::SPtr enumerator = readBackupLogFileSPtr->GetAsyncEnumerator();
                while ((SyncAwait(enumerator->MoveNextAsync(CancellationToken::None))) == true)
                {
                    status = outputLogRecords.Append(enumerator->GetCurrent());
                    CODING_ERROR_ASSERT(NT_SUCCESS(status));
                }

                SyncAwait(enumerator->CloseAsync());

                VERIFY_ARE_EQUAL(outputLogRecords.Count(), testLogRecords->GetCount());
            }
        }

        co_return;
    }

    KWString BackupLogFileTests::CreateFileName(
        __in KStringView const & inputName,
        __in KAllocator & allocator)
    {
        KString::SPtr fileName;

        wstring currentDirectoryPathCharArray = Common::Directory::GetCurrentDirectoryW();

        NTSTATUS status = KString::Create(fileName, allocator, Common::Path::GetPathGlobalNamespaceWstr().c_str());
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        BOOLEAN concatSuccess = fileName->Concat(currentDirectoryPathCharArray.c_str());
        CODING_ERROR_ASSERT(concatSuccess == TRUE);

        fileName = KPath::Combine(*fileName, inputName, allocator);
        KWString result(allocator, *fileName);

        return result;
    }

    void BackupLogFileTests::EndTest()
    {
        prId_.Reset();
    }

    // TODO: #9164634: Tests to be added.
    // - GetAsyncEnumerator_Reset
    // - OpenAsync_WithExistingReader_Succeeds
    // - OpenAsync_PartialFile_ThrowsInvalidDataException (shrink)
    // - OpenAsync_CorruptData_ThrowsInvalidDataException (bit flip detection)
    // - Stronger verification then count and latest lsn
    // - Add coverage for all log record types.
    BOOST_FIXTURE_TEST_SUITE(BackupLogFileTestSuite, BackupLogFileTests);

    BOOST_AUTO_TEST_CASE(WriteAsync_MinimalLogRecords_AllLogRecordsWrittenAndRead)
    {
        TEST_TRACE_BEGIN("WriteAsync_MinimalLogRecords_AllLogRecordsWrittenAndRead")
        {
            InvalidLogRecords::SPtr invalidLogRecords = InvalidLogRecords::Create(allocator);

            TestLogRecords::SPtr testLogRecords = TestLogRecords::Create(allocator);
            testLogRecords->InitializeWithNewLogRecords();

            IAsyncEnumerator<LogRecord::SPtr>::SPtr logRecordsEnumerator(testLogRecords.RawPtr());

            KWString fileName = CreateFileName(L"BLP_WriteAsync_MinimalLogRecords_AllLogRecordsWrittenAndRead.txt", allocator);

            // Test write the file
            BackupLogRecord::SPtr backupLogRecordSPtr = BackupLogRecord::CreateZeroBackupLogRecord(
                *invalidLogRecords->Inv_PhysicalLogRecord,
                allocator);
            BackupLogFile::SPtr backupLogFileSPtr = BackupLogFile::Create(
                *prId_,
                fileName,
                allocator);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            SyncAwait(backupLogFileSPtr->WriteAsync(
                *logRecordsEnumerator,
                *backupLogRecordSPtr,
                CancellationToken::None));

            // Verification
            VERIFY_ARE_EQUAL(backupLogFileSPtr->IndexingRecordEpoch.DataLossVersion, testLogRecords->GetLastIndexingLogRecord()->CurrentEpoch.DataLossVersion);
            VERIFY_ARE_EQUAL(backupLogFileSPtr->IndexingRecordEpoch.ConfigurationVersion, testLogRecords->GetLastIndexingLogRecord()->CurrentEpoch.ConfigurationVersion);
            VERIFY_ARE_EQUAL(backupLogFileSPtr->IndexingRecordLSN, testLogRecords->GetLastIndexingLogRecord()->Lsn);

            VERIFY_ARE_EQUAL(backupLogFileSPtr->LastBackedUpEpoch.DataLossVersion, testLogRecords->GetLastEpoch().DataLossVersion);
            VERIFY_ARE_EQUAL(backupLogFileSPtr->LastBackedUpEpoch.ConfigurationVersion, testLogRecords->GetLastEpoch().ConfigurationVersion);
            VERIFY_ARE_EQUAL(backupLogFileSPtr->LastBackedUpLSN, testLogRecords->GetLSN());

            VERIFY_ARE_EQUAL(backupLogFileSPtr->Count, testLogRecords->GetCount());
        }

        // Ignore error since this is only for clean up.
        File::Delete2(L"BLP_WriteAsync_MinimalLogRecords_AllLogRecordsWrittenAndRead.txt");
    }

    BOOST_AUTO_TEST_CASE(WriteAsync_MoreThanOneBlockSize_AllLogRecordsWrittenAndRead)
    {
        TEST_TRACE_BEGIN("WriteAsync_MoreThanOneBlockSize_AllLogRecordsWrittenAndRead")
        {
            InvalidLogRecords::SPtr invalidLogRecords = InvalidLogRecords::Create(allocator);

            TestLogRecords::SPtr testLogRecords = TestLogRecords::Create(allocator);
            testLogRecords->InitializeWithNewLogRecords();

            IAsyncEnumerator<LogRecord::SPtr>::SPtr logRecordsEnumerator(testLogRecords.RawPtr());

            KWString fileName = CreateFileName(L"BLP_WriteAsync_MoreThanOneBlockSize_AllLogRecordsWrittenAndRead.txt", allocator);

            // Test write the file
            BackupLogRecord::SPtr backupLogRecordSPtr = BackupLogRecord::CreateZeroBackupLogRecord(
                *invalidLogRecords->Inv_PhysicalLogRecord,
                allocator);
            BackupLogFile::SPtr backupLogFileSPtr = BackupLogFile::Create(
                *prId_,
                fileName,
                allocator);

            SyncAwait(backupLogFileSPtr->WriteAsync(
                *logRecordsEnumerator,
                *backupLogRecordSPtr,
                CancellationToken::None));

            // Verification
            VERIFY_ARE_EQUAL(backupLogFileSPtr->IndexingRecordEpoch.DataLossVersion, testLogRecords->GetLastIndexingLogRecord()->CurrentEpoch.DataLossVersion);
            VERIFY_ARE_EQUAL(backupLogFileSPtr->IndexingRecordEpoch.ConfigurationVersion, testLogRecords->GetLastIndexingLogRecord()->CurrentEpoch.ConfigurationVersion);
            VERIFY_ARE_EQUAL(backupLogFileSPtr->IndexingRecordLSN, testLogRecords->GetLastIndexingLogRecord()->Lsn);

            VERIFY_ARE_EQUAL(backupLogFileSPtr->LastBackedUpEpoch.DataLossVersion, testLogRecords->GetLastEpoch().DataLossVersion);
            VERIFY_ARE_EQUAL(backupLogFileSPtr->LastBackedUpEpoch.ConfigurationVersion, testLogRecords->GetLastEpoch().ConfigurationVersion);
            VERIFY_ARE_EQUAL(backupLogFileSPtr->LastBackedUpLSN, testLogRecords->GetLSN());

            VERIFY_ARE_EQUAL(backupLogFileSPtr->Count, testLogRecords->GetCount());
        }

        // Ignore error since this is only for clean up.
        File::Delete2(L"BLP_WriteAsync_MoreThanOneBlockSize_AllLogRecordsWrittenAndRead.txt");
    }

    BOOST_AUTO_TEST_CASE(CreateAndOpen_MinimalLogRecords_AllLogRecordsWrittenAndRead)
    {
        TEST_TRACE_BEGIN("CreateAndOpen_MinimalLogRecords_AllLogRecordsWrittenAndRead")
        {
            InvalidLogRecords::SPtr invalidLogRecords = InvalidLogRecords::Create(allocator);

            TestLogRecords::SPtr testLogRecords = TestLogRecords::Create(allocator);
            testLogRecords->InitializeWithNewLogRecords();

            IAsyncEnumerator<LogRecord::SPtr>::SPtr logRecordsEnumerator(testLogRecords.RawPtr());

            KWString fileName = CreateFileName(L"BLP_CreateAndOpen_MinimalLogRecords_AllLogRecordsWrittenAndRead.txt", allocator);

            // Test write the file
            BackupLogRecord::SPtr backupLogRecordSPtr = BackupLogRecord::CreateZeroBackupLogRecord(
                *invalidLogRecords->Inv_PhysicalLogRecord,
                allocator);
            BackupLogFile::SPtr backupLogFileSPtr = BackupLogFile::Create(
                *prId_,
                fileName,
                allocator);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            SyncAwait(backupLogFileSPtr->WriteAsync(
                *logRecordsEnumerator,
                *backupLogRecordSPtr,
                CancellationToken::None));

            // Read Test
            {
                // Test read the file.
                BackupLogFile::SPtr readBackupLogFileSPtr = BackupLogFile::Create(
                    *prId_,
                    fileName,
                    allocator);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));

                SyncAwait(readBackupLogFileSPtr->ReadAsync(CancellationToken::None));

                // Verification
                VERIFY_ARE_EQUAL(readBackupLogFileSPtr->IndexingRecordEpoch.DataLossVersion, testLogRecords->GetLastIndexingLogRecord()->CurrentEpoch.DataLossVersion);
                VERIFY_ARE_EQUAL(readBackupLogFileSPtr->IndexingRecordEpoch.ConfigurationVersion, testLogRecords->GetLastIndexingLogRecord()->CurrentEpoch.ConfigurationVersion);
                VERIFY_ARE_EQUAL(readBackupLogFileSPtr->IndexingRecordLSN, testLogRecords->GetLastIndexingLogRecord()->Lsn);

                VERIFY_ARE_EQUAL(readBackupLogFileSPtr->LastBackedUpEpoch.DataLossVersion, testLogRecords->GetLastEpoch().DataLossVersion);
                VERIFY_ARE_EQUAL(readBackupLogFileSPtr->LastBackedUpEpoch.ConfigurationVersion, testLogRecords->GetLastEpoch().ConfigurationVersion);
                VERIFY_ARE_EQUAL(readBackupLogFileSPtr->LastBackedUpLSN, testLogRecords->GetLSN());

                VERIFY_ARE_EQUAL(readBackupLogFileSPtr->Count, testLogRecords->GetCount());

                // Verification of the log records.
                KArray<LogRecord::SPtr> outputLogRecords(allocator, testLogRecords->GetCount());
                BackupLogFileAsyncEnumerator::SPtr enumerator = readBackupLogFileSPtr->GetAsyncEnumerator();

                while (SyncAwait(enumerator->MoveNextAsync(CancellationToken::None)) == true)
                {
                    status = outputLogRecords.Append(enumerator->GetCurrent());
                    CODING_ERROR_ASSERT(NT_SUCCESS(status));
                }

                SyncAwait(enumerator->CloseAsync());

                VERIFY_ARE_EQUAL(outputLogRecords.Count(), testLogRecords->GetCount());
            }
        }

        // Ignore error since this is only for clean up.
        File::Delete2(L"BLP_CreateAndOpen_MinimalLogRecords_AllLogRecordsWrittenAndRead.txt");
    }

    BOOST_AUTO_TEST_CASE(CreateAndOpen_MoreThanOneBlockSize_AllLogRecordsWrittenAndRead)
    {
        TEST_TRACE_BEGIN("CreateAndOpen_MoreThanOneBlockSize_AllLogRecordsWrittenAndRead")
        {
            InvalidLogRecords::SPtr invalidLogRecords = InvalidLogRecords::Create(allocator);

            TestLogRecords::SPtr testLogRecords = TestLogRecords::Create(allocator);
            testLogRecords->InitializeWithNewLogRecords();
            testLogRecords->AddUpdateEpochLogRecord();

            for (int i = 0; i < 4; i++)
            {
                testLogRecords->PopulateWithRandomRecords(32);
                testLogRecords->AddUpdateEpochLogRecord();
            }

            IAsyncEnumerator<LogRecord::SPtr>::SPtr logRecordsEnumerator(testLogRecords.RawPtr());

            KWString fileName = CreateFileName(L"BLP_CreateAndOpen_MoreThanOneBlockSize_AllLogRecordsWrittenAndRead.txt", allocator);

            // Test write the file
            BackupLogRecord::SPtr backupLogRecordSPtr = BackupLogRecord::CreateZeroBackupLogRecord(
                *invalidLogRecords->Inv_PhysicalLogRecord,
                allocator);
            BackupLogFile::SPtr backupLogFileSPtr = BackupLogFile::Create(
                *prId_,
                fileName,
                allocator);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            SyncAwait(backupLogFileSPtr->WriteAsync(
                *logRecordsEnumerator,
                *backupLogRecordSPtr,
                CancellationToken::None));

            // Read Test
            {
                // Read the backup file
                BackupLogFile::SPtr readBackupLogFileSPtr = BackupLogFile::Create(
                    *prId_,
                    fileName,
                    allocator);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));

                SyncAwait(readBackupLogFileSPtr->ReadAsync(CancellationToken::None));

                // Verification
                VERIFY_ARE_EQUAL(readBackupLogFileSPtr->IndexingRecordEpoch.DataLossVersion, testLogRecords->GetLastIndexingLogRecord()->CurrentEpoch.DataLossVersion);
                VERIFY_ARE_EQUAL(readBackupLogFileSPtr->IndexingRecordEpoch.ConfigurationVersion, testLogRecords->GetLastIndexingLogRecord()->CurrentEpoch.ConfigurationVersion);
                VERIFY_ARE_EQUAL(readBackupLogFileSPtr->IndexingRecordLSN, testLogRecords->GetLastIndexingLogRecord()->Lsn);

                VERIFY_ARE_EQUAL(readBackupLogFileSPtr->LastBackedUpEpoch.DataLossVersion, testLogRecords->GetLastEpoch().DataLossVersion);
                VERIFY_ARE_EQUAL(readBackupLogFileSPtr->LastBackedUpEpoch.ConfigurationVersion, testLogRecords->GetLastEpoch().ConfigurationVersion);
                VERIFY_ARE_EQUAL(readBackupLogFileSPtr->LastBackedUpLSN, testLogRecords->GetLSN());

                VERIFY_ARE_EQUAL(readBackupLogFileSPtr->Count, testLogRecords->GetCount());

                // Verification of the log records.
                KArray<LogRecord::SPtr> outputLogRecords(allocator, testLogRecords->GetCount());
                BackupLogFileAsyncEnumerator::SPtr enumerator = readBackupLogFileSPtr->GetAsyncEnumerator();
                while (SyncAwait(enumerator->MoveNextAsync(CancellationToken::None)) == true)
                {
                    status = outputLogRecords.Append(enumerator->GetCurrent());
                    CODING_ERROR_ASSERT(NT_SUCCESS(status));
                }

                SyncAwait(enumerator->CloseAsync());

                VERIFY_ARE_EQUAL(outputLogRecords.Count(), testLogRecords->GetCount());
            }
        }

        // Ignore error since this is only for clean up.
        File::Delete2(L"BLP_CreateAndOpen_MoreThanOneBlockSize_AllLogRecordsWrittenAndRead.txt");
    }

    // Scenario: Incremental backup from a replica that has full backed up when the log was new born with only UpdateEpoch for election.
    BOOST_AUTO_TEST_CASE(CreateAndOpen_IncrementalBackup_BaseWithoutElectionBarrier_AllLogRecordsWrittenAndRead)
    {
        wstring testName(L"BLP_CreateAndOpen_IncrementalBackup_BaseWithoutElectionBarrier_AllLogRecordsWrittenAndRead.txt");

        SyncAwait(Test_IncrementalBackup_AllLogRecordsWrittenAndRead(testName, 1, 1, false, false, false));
    }

    // Scenario: Incremental backup from a replica that has full backed up when the log was new born with only UpdateEpoch and barrier for election.
    BOOST_AUTO_TEST_CASE(CreateAndOpen_IncrementalBackup_BaseWithElectionBarrier_AllLogRecordsWrittenAndRead)
    {
        wstring testName(L"BLP_CreateAndOpen_IncrementalBackup_BaseWithElectionBarrier_AllLogRecordsWrittenAndRead.txt");

        SyncAwait(Test_IncrementalBackup_AllLogRecordsWrittenAndRead(testName, 8, 32, true, false, false));
    }

    // Scenario: Incremental backup from a replica that has full backed up when the log was new borh with only UpdateEpoch and barrier for election.
    // Incremental log records contain backups and updateEpochs (randomly)
    BOOST_AUTO_TEST_CASE(CreateAndOpen_IncrementalBackup_MultipleSubBarrierAndUpdateEpoch_AllLogRecordsWrittenAndRead)
    {
        wstring testName(L"BLP_CreateAndOpen_IncrementalBackup_MultipleSubBarrierAndUpdateEpoch_AllLogRecordsWrittenAndRead.txt");

        SyncAwait(Test_IncrementalBackup_AllLogRecordsWrittenAndRead(testName, 4, 64, true, false, true));
    }

    BOOST_AUTO_TEST_SUITE_END();
}
