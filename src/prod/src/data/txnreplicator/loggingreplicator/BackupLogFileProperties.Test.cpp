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

    Common::StringLiteral const TraceComponent("BackupLogFilePropertiesTests");

    class BackupLogFilePropertiesTests
    {
    protected:
        void EndTest();

        Common::CommonConfig config; // load the config object as its needed for the tracing to work
        ::FABRIC_REPLICA_ID rId_;
        KGuid pId_;
        PartitionedReplicaId::SPtr prId_;
        KtlSystem * underlyingSystem_;
    };

    void BackupLogFilePropertiesTests::EndTest()
    {
        prId_.Reset();
    }

    BOOST_FIXTURE_TEST_SUITE(BackupLogFilePropertiesTestSuite, BackupLogFilePropertiesTests)

    BOOST_AUTO_TEST_CASE(BackupLogFileProperties_WriteRead_SUCCESS)
    {
        // verifies the sptr is released correctly after test case ends as there is memory leak checks by ktl
        TEST_TRACE_BEGIN("WriteRead_SUCCESS")
        {
            // Expected
            const ULONG32 expectedCount = 16;
            const ULONG32 expectedRecordsHandleOffset = 5;
            const ULONG32 expectedRecordsHandleSize = 15;
            const TxnReplicator::Epoch expectedIndexingRecordEpoch(19, 87);
            const FABRIC_SEQUENCE_NUMBER expectedIndexingRecordLSN = 16;
            const TxnReplicator::Epoch expectedLastBackedupEpoch(20, 88);
            const FABRIC_SEQUENCE_NUMBER expectedLastBackedupLSN = 6;

            BlockHandle::SPtr expectedRecordsHandle = nullptr;
            status = BlockHandle::Create(expectedRecordsHandleOffset, expectedRecordsHandleSize, allocator, expectedRecordsHandle);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            // Setup
            KBlockFile::SPtr fakeFileSPtr = nullptr;
            ktl::io::KFileStream::SPtr fakeStream = nullptr;

            KString::SPtr tmpFileName;
            wstring currentDirectoryPathCharArray = Common::Directory::GetCurrentDirectoryW();

            status = KString::Create(tmpFileName, allocator, Common::Path::GetPathGlobalNamespaceWstr().c_str());
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            BOOLEAN concatSuccess = tmpFileName->Concat(currentDirectoryPathCharArray.c_str());
            CODING_ERROR_ASSERT(concatSuccess == TRUE);

            tmpFileName = KPath::Combine(*tmpFileName, L"BLFP_WriteRead_SUCCESS.txt", allocator);
            KWString fileName(allocator, *tmpFileName);

            status = SyncAwait(Data::TestCommon::FileFormatTestHelper::CreateBlockFile(fileName, allocator, CancellationToken::None, fakeFileSPtr, fakeStream));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            status = SyncAwait(fakeStream->OpenAsync(*fakeFileSPtr));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            // Prepare the file properties
            BackupLogFileProperties::SPtr filePropertiesSPtr = nullptr;
            status = BackupLogFileProperties::Create(allocator, filePropertiesSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            filePropertiesSPtr->Count = expectedCount;
            filePropertiesSPtr->IndexingRecordEpoch = expectedIndexingRecordEpoch;
            filePropertiesSPtr->IndexingRecordLSN = expectedIndexingRecordLSN;
            filePropertiesSPtr->LastBackedUpEpoch = expectedLastBackedupEpoch;
            filePropertiesSPtr->LastBackedUpLSN = expectedLastBackedupLSN;
            filePropertiesSPtr->RecordsHandle = *expectedRecordsHandle;

            // Write Test
            BlockHandle::SPtr writeHandle = nullptr;
            FileBlock<BackupLogFileProperties::SPtr>::SerializerFunc serializerFunction(filePropertiesSPtr.RawPtr(), &BackupLogFileProperties::Write);
            status = SyncAwait(FileBlock<BackupLogFileProperties::SPtr>::WriteBlockAsync(*fakeStream, serializerFunction, allocator, CancellationToken::None, writeHandle));

            // Write Verification
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            VERIFY_ARE_EQUAL(writeHandle->Offset, 0);
            VERIFY_ARE_EQUAL(writeHandle->Size, 130);

            // Low priority verification that write operation did not change any property.
            VERIFY_ARE_EQUAL(filePropertiesSPtr->Count, expectedCount);
            VERIFY_ARE_EQUAL(filePropertiesSPtr->IndexingRecordEpoch, expectedIndexingRecordEpoch);
            VERIFY_ARE_EQUAL(filePropertiesSPtr->IndexingRecordLSN, expectedIndexingRecordLSN);
            VERIFY_ARE_EQUAL(filePropertiesSPtr->LastBackedUpEpoch, expectedLastBackedupEpoch);
            VERIFY_ARE_EQUAL(filePropertiesSPtr->LastBackedUpLSN, expectedLastBackedupLSN);
            VERIFY_ARE_EQUAL(filePropertiesSPtr->RecordsHandle->Offset, expectedRecordsHandleOffset);
            VERIFY_ARE_EQUAL(filePropertiesSPtr->RecordsHandle->Size, expectedRecordsHandleSize);

            // Read Test
            BackupLogFileProperties::SPtr readHandle = nullptr;
            FileBlock<BackupLogFileProperties::SPtr>::DeserializerFunc deserializerFuncFunction(&BackupLogFileProperties::Read);
            readHandle = SyncAwait(FileBlock<BackupLogFileProperties::SPtr>::ReadBlockAsync(*fakeStream, *writeHandle, deserializerFuncFunction, allocator, CancellationToken::None));

            // Read Verification
            VERIFY_ARE_EQUAL(readHandle->Count, expectedCount);
            VERIFY_ARE_EQUAL(readHandle->IndexingRecordEpoch, expectedIndexingRecordEpoch);
            VERIFY_ARE_EQUAL(readHandle->IndexingRecordLSN, expectedIndexingRecordLSN);
            VERIFY_ARE_EQUAL(readHandle->LastBackedUpEpoch, expectedLastBackedupEpoch);
            VERIFY_ARE_EQUAL(readHandle->LastBackedUpLSN, expectedLastBackedupLSN);
            VERIFY_ARE_EQUAL(readHandle->RecordsHandle->Offset, expectedRecordsHandleOffset);
            VERIFY_ARE_EQUAL(readHandle->RecordsHandle->Size, expectedRecordsHandleSize);

            // Shutdown
            status = SyncAwait(fakeStream->CloseAsync());
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            fakeFileSPtr->Close();
            Common::File::Delete(static_cast<LPCWSTR>(fileName), true);
        }
    }

    BOOST_AUTO_TEST_SUITE_END()
}
