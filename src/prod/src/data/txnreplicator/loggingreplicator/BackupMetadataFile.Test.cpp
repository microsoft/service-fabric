// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestHeaders.h"

#include <stdlib.h>
#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

#define BACKUPMETADATAFILETEST_TAG 'tfMB'

namespace LoggingReplicatorTests
{
    using namespace Common;
    using namespace ktl;
    using namespace Data::LogRecordLib;
    using namespace Data::LoggingReplicator;
    using namespace Data::Utilities;

    StringLiteral const TraceComponent("BackupMetadataFileTests");

    class BackupMetadataFileTests
    {
    public:
        Awaitable<void> Test_BackupMetadataFile_WriteAndRead(
            __in KString const & fileName);

        Awaitable<void> Test_BackupMetadataFile_Equal(
            __in KString const & fileName);

        Awaitable<void> Test_BackupMetadataFile_WriteAsync_WithCanceledToken_Throws(
            __in KString const & fileName);

        Awaitable<void> Test_BackupMetadataFile_ReadAsync_WithCanceledToken_Throws(
            __in KString const & fileName);

    protected:
        KString::CSPtr CreateFileName(
            __in KStringView const & name,
            __in KAllocator & allocator);

        void EndTest();

        CommonConfig config; // load the config object as its needed for the tracing to work
        FABRIC_REPLICA_ID rId_;
        KGuid pId_;
        PartitionedReplicaId::SPtr prId_;
        KtlSystem * underlyingSystem_;
    };

    // BackupMetadataFile WriteAsync and ReadAsync test
    // 1. Set up the expected values.
    // 2. Write the backup metadata file.
    // 3. Read the backup metadata file.
    // 4. Verify all the properties are as expected.
    Awaitable<void> BackupMetadataFileTests::Test_BackupMetadataFile_WriteAndRead(
        __in KString const & fileName)
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;

        KAllocator & allocator = underlyingSystem_->PagedAllocator();
        KWString filePath(allocator, fileName);

        BackupMetadataFile::SPtr backupMetadataFileSPtr = nullptr;
        status = BackupMetadataFile::Create(
            *prId_,
            filePath,
            allocator,
            backupMetadataFileSPtr);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        // Expected 
        const FABRIC_BACKUP_OPTION expectedBackupOption = FABRIC_BACKUP_OPTION_FULL;
        KGuid expectedParentBackupId;
        expectedParentBackupId.CreateNew();
        KGuid expectedBackupId;
        expectedBackupId.CreateNew();
        KGuid expectedPartitionId;
        expectedPartitionId.CreateNew();
        const FABRIC_REPLICA_ID expectedReplicaId = 16;
        const LONG64 expectedDataLossNumber = 32;
        const LONG64 expectedConfigurationNumber = 64;
        TxnReplicator::Epoch expectedStartingEpoch = TxnReplicator::Epoch(expectedDataLossNumber, expectedConfigurationNumber);
        FABRIC_SEQUENCE_NUMBER expectedStartingLSN = 8;
        TxnReplicator::Epoch expectedBackupEpoch = TxnReplicator::Epoch(expectedDataLossNumber, expectedConfigurationNumber);
        FABRIC_SEQUENCE_NUMBER expectedBackupLSN = 128;

        status = co_await backupMetadataFileSPtr->WriteAsync(
            expectedBackupOption,
            expectedParentBackupId,
            expectedBackupId,
            expectedPartitionId,
            expectedReplicaId,
            expectedStartingEpoch,
            expectedStartingLSN,
            expectedBackupEpoch,
            expectedBackupLSN,
            CancellationToken::None);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KGuid readId;
        readId.CreateNew();
        status = co_await backupMetadataFileSPtr->ReadAsync(readId, CancellationToken::None);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        CODING_ERROR_ASSERT(backupMetadataFileSPtr->BackupOption == expectedBackupOption);
        CODING_ERROR_ASSERT(backupMetadataFileSPtr->ParentBackupId == expectedParentBackupId);
        CODING_ERROR_ASSERT(backupMetadataFileSPtr->BackupId == expectedBackupId);
        CODING_ERROR_ASSERT(backupMetadataFileSPtr->PropertiesPartitionId == expectedPartitionId);
        CODING_ERROR_ASSERT(backupMetadataFileSPtr->PropertiesReplicaId == expectedReplicaId);
        CODING_ERROR_ASSERT(backupMetadataFileSPtr->StartingEpoch.DataLossVersion == expectedDataLossNumber);
        CODING_ERROR_ASSERT(backupMetadataFileSPtr->StartingEpoch.ConfigurationVersion == expectedConfigurationNumber);
        CODING_ERROR_ASSERT(backupMetadataFileSPtr->StartingLSN == expectedStartingLSN);
        CODING_ERROR_ASSERT(backupMetadataFileSPtr->BackupEpoch.DataLossVersion == expectedDataLossNumber);
        CODING_ERROR_ASSERT(backupMetadataFileSPtr->BackupEpoch.ConfigurationVersion == expectedConfigurationNumber);
        CODING_ERROR_ASSERT(backupMetadataFileSPtr->BackupLSN == expectedBackupLSN);
        CODING_ERROR_ASSERT(backupMetadataFileSPtr->FileName == filePath);

        Common::File::Delete(static_cast<LPCWSTR>(fileName), true);
    }

    // BackupMetadataFile Equal function test
    // 1. Set up the expected values.
    // 2. Equal call on same BackupMetadataFile should return true.
    // 3. Equal call on different BackupMetadataFile should return false.
    Awaitable<void> BackupMetadataFileTests::Test_BackupMetadataFile_Equal(
        __in KString const & fileName)
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;

        KAllocator & allocator = underlyingSystem_->PagedAllocator();
        KWString filePath(allocator, fileName);

        BackupMetadataFile::SPtr backupMetadataFileSPtr = nullptr;
        status = BackupMetadataFile::Create(
            *prId_,
            filePath,
            allocator,
            backupMetadataFileSPtr);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        // Expected 
        const FABRIC_BACKUP_OPTION expectedBackupOption = FABRIC_BACKUP_OPTION_FULL;
        KGuid expectedParentBackupId;
        expectedParentBackupId.CreateNew();
        KGuid expectedBackupId;
        expectedBackupId.CreateNew();
        KGuid expectedPartitionId;
        expectedPartitionId.CreateNew();
        const FABRIC_REPLICA_ID expectedReplicaId = 16;
        const LONG64 expectedDataLossNumber = 32;
        const LONG64 expectedConfigurationNumber = 64;
        TxnReplicator::Epoch expectedStartingEpoch = TxnReplicator::Epoch(expectedDataLossNumber, expectedConfigurationNumber);
        FABRIC_SEQUENCE_NUMBER expectedStartingLSN = 8;
        TxnReplicator::Epoch expectedBackupEpoch = TxnReplicator::Epoch(expectedDataLossNumber, expectedConfigurationNumber);
        FABRIC_SEQUENCE_NUMBER expectedBackupLSN = 128;

        status = co_await backupMetadataFileSPtr->WriteAsync(
            expectedBackupOption,
            expectedParentBackupId,
            expectedBackupId,
            expectedPartitionId,
            expectedReplicaId,
            expectedStartingEpoch,
            expectedStartingLSN,
            expectedBackupEpoch,
            expectedBackupLSN,
            CancellationToken::None);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KGuid readId;
        readId.CreateNew();
        status = co_await backupMetadataFileSPtr->ReadAsync(readId, CancellationToken::None);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        // Same BackupMetadataFile should equal
        CODING_ERROR_ASSERT(backupMetadataFileSPtr->Equals(*backupMetadataFileSPtr));

        BackupMetadataFile::SPtr newBackupMetadataFileSPtr = nullptr;
        status = BackupMetadataFile::Create(
            *prId_,
            filePath,
            allocator,
            newBackupMetadataFileSPtr);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        // The new BackupMetadataFile backup option set to incremental.
        status = co_await newBackupMetadataFileSPtr->WriteAsync(
            FABRIC_BACKUP_OPTION_INCREMENTAL,
            expectedParentBackupId,
            expectedBackupId,
            expectedPartitionId,
            expectedReplicaId,
            expectedStartingEpoch,
            expectedStartingLSN,
            expectedBackupEpoch,
            expectedBackupLSN,
            CancellationToken::None);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = co_await newBackupMetadataFileSPtr->ReadAsync(readId, CancellationToken::None);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        // Different BackupMetadataFiles should return false
        CODING_ERROR_ASSERT(backupMetadataFileSPtr->Equals(*newBackupMetadataFileSPtr) == false);

        Common::File::Delete(static_cast<LPCWSTR>(fileName), true);
    }

    // BackupMetadataFile WriteAysnc function with cancellation token canceled test
    // 1. Set up the expected values.
    // 2. WriteAsync call with cancellation token got canceled 
    // 3. Verify it throws and the exception is as expected
    Awaitable<void> BackupMetadataFileTests::Test_BackupMetadataFile_WriteAsync_WithCanceledToken_Throws(
        __in KString const & fileName)
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;

        KAllocator & allocator = underlyingSystem_->PagedAllocator();
        KWString filePath(allocator, fileName);

        BackupMetadataFile::SPtr backupMetadataFileSPtr = nullptr;
        status = BackupMetadataFile::Create(
            *prId_,
            filePath,
            allocator,
            backupMetadataFileSPtr);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        // Expected 
        const FABRIC_BACKUP_OPTION expectedBackupOption = FABRIC_BACKUP_OPTION_FULL;
        KGuid expectedParentBackupId;
        expectedParentBackupId.CreateNew();
        KGuid expectedBackupId;
        expectedBackupId.CreateNew();
        KGuid expectedPartitionId;
        expectedPartitionId.CreateNew();
        const FABRIC_REPLICA_ID expectedReplicaId = 16;
        const LONG64 expectedDataLossNumber = 32;
        const LONG64 expectedConfigurationNumber = 64;
        TxnReplicator::Epoch expectedStartingEpoch = TxnReplicator::Epoch(expectedDataLossNumber, expectedConfigurationNumber);
        FABRIC_SEQUENCE_NUMBER expectedStartingLSN = 8;
        TxnReplicator::Epoch expectedBackupEpoch = TxnReplicator::Epoch(expectedDataLossNumber, expectedConfigurationNumber);
        FABRIC_SEQUENCE_NUMBER expectedBackupLSN = 128;

        CancellationTokenSource::SPtr cts;
        status = CancellationTokenSource::Create(allocator, BACKUPMETADATAFILETEST_TAG, cts);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        cts->Cancel();

        status = co_await backupMetadataFileSPtr->WriteAsync(
            expectedBackupOption,
            expectedParentBackupId,
            expectedBackupId,
            expectedPartitionId,
            expectedReplicaId,
            expectedStartingEpoch,
            expectedStartingLSN,
            expectedBackupEpoch,
            expectedBackupLSN,
            cts->Token);
        VERIFY_ARE_EQUAL(STATUS_CANCELLED, status);

        Common::File::Delete(static_cast<LPCWSTR>(fileName), true);
    }

    // BackupMetadataFile ReadAsync function with cancellation token canceled test
    // 1. Set up the expected values.
    // 2. ReadAsync call with cancellation token got canceled 
    // 3. Verify it throws and the exception is as expected
    Awaitable<void> BackupMetadataFileTests::Test_BackupMetadataFile_ReadAsync_WithCanceledToken_Throws(
        __in KString const & fileName)
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;

        KAllocator & allocator = underlyingSystem_->PagedAllocator();
        KWString filePath(allocator, fileName);

        BackupMetadataFile::SPtr backupMetadataFileSPtr = nullptr;
        status = BackupMetadataFile::Create(
            *prId_,
            filePath,
            allocator,
            backupMetadataFileSPtr);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        // Expected 
        const FABRIC_BACKUP_OPTION expectedBackupOption = FABRIC_BACKUP_OPTION_FULL;
        KGuid expectedParentBackupId;
        expectedParentBackupId.CreateNew();
        KGuid expectedBackupId;
        expectedBackupId.CreateNew();
        KGuid expectedPartitionId;
        expectedPartitionId.CreateNew();
        const FABRIC_REPLICA_ID expectedReplicaId = 16;
        const LONG64 expectedDataLossNumber = 32;
        const LONG64 expectedConfigurationNumber = 64;
        TxnReplicator::Epoch expectedStartingEpoch = TxnReplicator::Epoch(expectedDataLossNumber, expectedConfigurationNumber);
        FABRIC_SEQUENCE_NUMBER expectedStartingLSN = 8;
        TxnReplicator::Epoch expectedBackupEpoch = TxnReplicator::Epoch(expectedDataLossNumber, expectedConfigurationNumber);
        FABRIC_SEQUENCE_NUMBER expectedBackupLSN = 128;

        status = co_await backupMetadataFileSPtr->WriteAsync(
            expectedBackupOption,
            expectedParentBackupId,
            expectedBackupId,
            expectedPartitionId,
            expectedReplicaId,
            expectedStartingEpoch,
            expectedStartingLSN,
            expectedBackupEpoch,
            expectedBackupLSN,
            CancellationToken::None);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        KGuid readId;
        readId.CreateNew();

        CancellationTokenSource::SPtr cts;
        status = CancellationTokenSource::Create(allocator, BACKUPMETADATAFILETEST_TAG, cts);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        cts->Cancel();

        status = co_await backupMetadataFileSPtr->ReadAsync(readId, cts->Token);
        VERIFY_ARE_EQUAL(STATUS_CANCELLED, status);

        Common::File::Delete(static_cast<LPCWSTR>(fileName), true);
    }

    KString::CSPtr BackupMetadataFileTests::CreateFileName(
        __in KStringView const & name,
        __in KAllocator & allocator)
    {
        KString::SPtr fileName;

        wstring currentDirectoryPathCharArray = Common::Directory::GetCurrentDirectoryW();
        NTSTATUS status = KString::Create(fileName, allocator, Common::Path::GetPathGlobalNamespaceWstr().c_str());
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        BOOLEAN concatSuccess = fileName->Concat(currentDirectoryPathCharArray.c_str());
        CODING_ERROR_ASSERT(concatSuccess == TRUE);

        fileName = KPath::Combine(*fileName, name, allocator);

        return fileName.RawPtr();
    }

    void BackupMetadataFileTests::EndTest()
    {
        prId_.Reset();
    }

    BOOST_FIXTURE_TEST_SUITE(BackupMetadataFileTestSuite, BackupMetadataFileTests);

    //
    // Scenario:        Write the backup metadata file and read the file
    // Expected Result: The properties read from file should as expected
    // 
    BOOST_AUTO_TEST_CASE(BackupMetadataFile_WriteAndRead)
    {
        TEST_TRACE_BEGIN("BackupMetadataFile_WriteAndRead")
        {
            KString::CSPtr fileName = CreateFileName(L"BackupMetadataFile_WriteAndRead.txt", allocator);

            SyncAwait(Test_BackupMetadataFile_WriteAndRead(*fileName));
        }
    }

    //
    // Scenario:        BackupMetadataFile Equal function test
    // Expected Result: Same BackupMetadataFile should return true, different BackupMetadataFiles should return false
    // 
    BOOST_AUTO_TEST_CASE(BackupMetadataFile_Equal)
    {
        TEST_TRACE_BEGIN("BackupMetadataFile_Equal")
        {
            KString::CSPtr fileName = CreateFileName(L"BackupMetadataFile_Equal.txt", allocator);

            SyncAwait(Test_BackupMetadataFile_Equal(*fileName));
        }
    }

    //
    // Scenario:        Cancellation token got canceled during WriteAsync call
    // Expected Result: Throw STATUS_CANCELLED exception 
    // 
    BOOST_AUTO_TEST_CASE(BackupMetadataFile_WriteAsync_WithCanceledToken_Throws)
    {
        TEST_TRACE_BEGIN("BackupMetadataFile_WriteAsync_WithCanceledToken_Throws")
        {
            KString::CSPtr fileName = CreateFileName(L"BackupMetadataFile_WriteAsync_WithCanceledToken_Throws.txt", allocator);

            SyncAwait(Test_BackupMetadataFile_WriteAsync_WithCanceledToken_Throws(*fileName));
        }
    }

    //
    // Scenario:        Cancellation token got canceled during ReadAsync call
    // Expected Result: Throw STATUS_CANCELLED exception 
    // 
    BOOST_AUTO_TEST_CASE(BackupMetadataFile_ReadAsync_WithCanceledToken_Throws)
    {
        TEST_TRACE_BEGIN("BackupMetadataFile_ReadAsync_WithCanceledToken_Throws")
        {
            KString::CSPtr fileName = CreateFileName(L"BackupMetadataFile_ReadAsync_WithCanceledToken_Throws.txt", allocator);

            SyncAwait(Test_BackupMetadataFile_ReadAsync_WithCanceledToken_Throws(*fileName));
        }
    }

    BOOST_AUTO_TEST_SUITE_END();
}
