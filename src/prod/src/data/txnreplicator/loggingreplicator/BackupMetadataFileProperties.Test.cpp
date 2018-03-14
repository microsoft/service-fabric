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
    using namespace std;
    using namespace Common;
    using namespace ktl;
    using namespace Data::LogRecordLib;
    using namespace Data::LoggingReplicator;
    using namespace Data::Utilities;

    Common::StringLiteral const TraceComponent("BackupMetadataFilePropertiesTests");

    class BackupMetadataFilePropertiesTests
    {
    protected:
        void EndTest();

        Common::CommonConfig config; // load the config object as its needed for the tracing to work
        ::FABRIC_REPLICA_ID rId_;
        KGuid pId_;
        PartitionedReplicaId::SPtr prId_;
        KtlSystem * underlyingSystem_;
    };

    void BackupMetadataFilePropertiesTests::EndTest()
    {
        prId_.Reset();
    }

    BOOST_FIXTURE_TEST_SUITE(BackupMetadataFilePropertiesTestSuite, BackupMetadataFilePropertiesTests)

    BOOST_AUTO_TEST_CASE(BackupMetadataFileProperties_WriteRead_SUCCESS)
    {
        // verifies the sptr is released correctly after test case ends as there is memory leak checks by ktl
        TEST_TRACE_BEGIN("WriteRead_SUCCESS")
        {
            // Expected
            const FABRIC_BACKUP_OPTION expectedBackupOption = FABRIC_BACKUP_OPTION_FULL;
            KGuid expectedParentBackupId;
            expectedParentBackupId.CreateNew();
            KGuid expectedBackupId;
            expectedBackupId.CreateNew();
            KGuid expectedPartitionId;
            expectedPartitionId.CreateNew();
            const FABRIC_REPLICA_ID expectedReplicaId = 16;
            const TxnReplicator::Epoch expectedStartingEpoch(19, 87);
            const FABRIC_SEQUENCE_NUMBER expectedStartingLSN = 8;
            const TxnReplicator::Epoch expectedBackupEpoch(20, 88);
            const FABRIC_SEQUENCE_NUMBER expectedBackupLSN = 128;

            // Setup
            KBlockFile::SPtr fakeFileSPtr = nullptr;
            ktl::io::KFileStream::SPtr fakeStream = nullptr;

            KString::SPtr tmpFileName;
            wstring currentDirectoryPathCharArray = Common::Directory::GetCurrentDirectoryW();

            status = KString::Create(tmpFileName, allocator, Common::Path::GetPathGlobalNamespaceWstr().c_str());
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            BOOLEAN concatSuccess = tmpFileName->Concat(currentDirectoryPathCharArray.c_str());
            CODING_ERROR_ASSERT(concatSuccess == TRUE);

            tmpFileName = KPath::Combine(*tmpFileName, L"WriteRead_SUCCESS.txt", allocator);
            KWString fileName(allocator, *tmpFileName);

            status = SyncAwait(Data::TestCommon::FileFormatTestHelper::CreateBlockFile(fileName, allocator, CancellationToken::None, fakeFileSPtr, fakeStream));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            status = SyncAwait(fakeStream->OpenAsync(*fakeFileSPtr));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            // Prepare the file properties
            BackupMetadataFileProperties::SPtr BackupMetadataFilePropertiesSPtr = nullptr;
            status = BackupMetadataFileProperties::Create(
                expectedBackupOption,
                expectedParentBackupId,
                expectedBackupId,
                expectedPartitionId,
                expectedReplicaId,
                expectedStartingEpoch,
                expectedStartingLSN,
                expectedBackupEpoch,
                expectedBackupLSN,
                allocator, 
                BackupMetadataFilePropertiesSPtr);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            // Write Test
            BlockHandle::SPtr writeHandle = nullptr;
            FileBlock<BackupMetadataFileProperties::SPtr>::SerializerFunc serializerFunction(BackupMetadataFilePropertiesSPtr.RawPtr(), &BackupMetadataFileProperties::Write);
            status = SyncAwait(FileBlock<BackupMetadataFileProperties::SPtr>::WriteBlockAsync(*fakeStream, serializerFunction, allocator, CancellationToken::None, writeHandle));

            // Write Verification
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            VERIFY_ARE_EQUAL(writeHandle->Offset, 0);
            VERIFY_ARE_EQUAL(writeHandle->Size, 206);

            // Read Test
            BackupMetadataFileProperties::SPtr readHandle = nullptr;
            FileBlock<BackupMetadataFileProperties::SPtr>::DeserializerFunc deserializerFuncFunction(&BackupMetadataFileProperties::Read);
            readHandle = SyncAwait(FileBlock<BackupMetadataFileProperties::SPtr>::ReadBlockAsync(*fakeStream, *writeHandle, deserializerFuncFunction, allocator, CancellationToken::None));

            // Read Verification
            VERIFY_ARE_EQUAL(readHandle->BackupOption, expectedBackupOption);
            VERIFY_ARE_EQUAL(readHandle->ParentBackupId, expectedParentBackupId);
            VERIFY_ARE_EQUAL(readHandle->BackupId, expectedBackupId);
            VERIFY_ARE_EQUAL(readHandle->PartitionId, expectedPartitionId);
            VERIFY_ARE_EQUAL(readHandle->ReplicaId, expectedReplicaId);
            VERIFY_ARE_EQUAL(readHandle->StartingEpoch, expectedStartingEpoch);
            VERIFY_ARE_EQUAL(readHandle->StartingLSN, expectedStartingLSN);
            VERIFY_ARE_EQUAL(readHandle->BackupEpoch, expectedBackupEpoch);
            VERIFY_ARE_EQUAL(readHandle->BackupLSN, expectedBackupLSN);

            // Shutdown
            status = SyncAwait(fakeStream->CloseAsync());
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            fakeFileSPtr->Close();
            Common::File::Delete(static_cast<LPCWSTR>(fileName), true);
        }
    }

    BOOST_AUTO_TEST_SUITE_END()
}
