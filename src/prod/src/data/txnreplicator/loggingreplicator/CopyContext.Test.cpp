// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestHeaders.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace LoggingReplicatorTests
{
    using namespace std;
    using namespace ktl;
    using namespace TxnReplicator;
    using namespace Data::LogRecordLib;
    using namespace Data::Utilities;
    using namespace Common;

    StringLiteral const TraceComponent = "LoggingReplicatorCopyContextTests";

    class LoggingReplicatorCopyContextTests
    {
    protected:
        void EndTest();

        KGuid pId_;
        ::FABRIC_REPLICA_ID rId_;
        PartitionedReplicaId::SPtr prId_;
        KtlSystem * underlyingSystem_;
    };

    void LoggingReplicatorCopyContextTests::EndTest()
    {
        prId_.Reset();
    }

    BOOST_FIXTURE_TEST_SUITE(LoggingReplicatorCopyContextTestsSuite, LoggingReplicatorCopyContextTests)

    BOOST_AUTO_TEST_CASE(Verify)
    {
        TEST_TRACE_BEGIN("Verify")
        {
            TestLogRecordUtility::SPtr util = TestLogRecordUtility::Create(allocator);

            LONG64 expectedReplicaId = 1234;
            LONG64 expectedLogTailLsn = 10;
            LONG64 expectedLatestRecoveredAtomicRedoOperationLsn = 8;

            LONG64 expectedEpochDataLossNumber = 2;
            LONG64 expectedEpochConfigurationNumber = 3;
            LONG64 expectedStartingLogicalSequenceNumber = 2;
            ProgressVector::SPtr expectedProgressVector = ProgressVector::Create(allocator);
            DateTime expectedTimeStamp = Common::DateTime::Now();
            ProgressVectorEntry e1(Epoch(expectedEpochDataLossNumber, expectedEpochConfigurationNumber), expectedStartingLogicalSequenceNumber, expectedReplicaId, expectedTimeStamp);
            expectedProgressVector->Insert(e1);

            LONG64 expectedLogHeadRecordLsn = 20;
            IndexingLogRecord::CSPtr expectedLogHeadRecord = IndexingLogRecord::Create(
                LogRecordType::Enum::Indexing,
                10,
                expectedLogHeadRecordLsn,
                *util->InvalidRecords->Inv_PhysicalLogRecord,
                allocator).RawPtr();

            CopyContext::SPtr copyContext = CopyContext::Create(
                *prId_, 
                expectedReplicaId, 
                *expectedProgressVector, 
                *expectedLogHeadRecord, 
                expectedLogTailLsn, 
                expectedLatestRecoveredAtomicRedoOperationLsn, 
                allocator);

            OperationData::CSPtr operationData;
            SyncAwait(copyContext->GetNextAsync(CancellationToken::None, operationData));

            KBuffer::CSPtr buffer = (*operationData)[0];
            BinaryReader br(*buffer, allocator);
            LONG64 replicaId;
            LONG64 logTailLsn;
            LONG64 latestRecoveredAtomicRedoOperationLsn;
            LONG64 epochDataLossNumber;
            LONG64 epochConfigurationNumber;
            ProgressVector::SPtr progressVector = ProgressVector::Create(allocator);

            LONG64 logHeadRecordLsn;

            br.Read(replicaId);
            progressVector->Read(br, false);
            br.Read(epochDataLossNumber);
            br.Read(epochConfigurationNumber);
            br.Read(logHeadRecordLsn);
            br.Read(logTailLsn);
            br.Read(latestRecoveredAtomicRedoOperationLsn);

            VERIFY_ARE_EQUAL(expectedReplicaId, replicaId);

            VERIFY_ARE_EQUAL(progressVector->get_LastProgressVectorEntry().get_Epoch().DataLossVersion, expectedEpochDataLossNumber);
            VERIFY_ARE_EQUAL(progressVector->get_LastProgressVectorEntry().get_Epoch().ConfigurationVersion, expectedEpochConfigurationNumber);
            VERIFY_ARE_EQUAL(progressVector->get_LastProgressVectorEntry().get_Lsn(), expectedStartingLogicalSequenceNumber);
            VERIFY_ARE_EQUAL(progressVector->get_LastProgressVectorEntry().get_PrimaryReplicaId(), expectedReplicaId);
            VERIFY_ARE_EQUAL(progressVector->get_LastProgressVectorEntry().get_TimeStamp(), expectedTimeStamp);

            VERIFY_ARE_EQUAL(expectedLogHeadRecord->CurrentEpoch.DataLossVersion, epochDataLossNumber);
            VERIFY_ARE_EQUAL(expectedLogHeadRecord->CurrentEpoch.ConfigurationVersion, epochConfigurationNumber);
            VERIFY_ARE_EQUAL(expectedLogHeadRecord->Lsn, logHeadRecordLsn);
            VERIFY_ARE_EQUAL(expectedLogTailLsn, logTailLsn);
            VERIFY_ARE_EQUAL(expectedLatestRecoveredAtomicRedoOperationLsn, latestRecoveredAtomicRedoOperationLsn);
        }
    }

    BOOST_AUTO_TEST_SUITE_END()
}
