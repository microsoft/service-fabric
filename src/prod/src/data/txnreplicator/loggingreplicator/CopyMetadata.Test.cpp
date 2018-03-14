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
    using namespace Data::LogRecordLib;
    using namespace Data::LoggingReplicator;
    using namespace TxnReplicator;
    using namespace Data::Utilities;
    using namespace Common;

    StringLiteral const TraceComponent = "CopyMetadataTests";

    class CopyMetadataTests
    {
    protected:
        void EndTest();

        KGuid pId_;
        ::FABRIC_REPLICA_ID rId_;
        PartitionedReplicaId::SPtr prId_;
        KtlSystem * underlyingSystem_;
    };

    void CopyMetadataTests::EndTest()
    {
        prId_.Reset();
    }

    class CopyMetadataTestUtility
    {
        public:
            static CopyMetadata::SPtr CreateCopyMetadata(
                __in ULONG32 expectedStateMetadataVersion,
                __in LONG64 expectedEpochDataLossNumber,
                __in LONG64 expectedEpochConfigurationNumber,
                __in LONG64 expectedStartingLogicalSequenceNumber,
                __in LONG64 expectedCheckpointLsn,
                __in LONG64 expectedUpToLsn,
                __in LONG64 expectedHighestStateProviderCopiedLsn,
                __in LONG64 expectedReplicaId,
                __out DateTime & expectedTimeStamp,
                __in KAllocator & allocator
                )
            {
                Epoch expectedStartingEpoch(expectedEpochDataLossNumber, expectedEpochConfigurationNumber);
                ProgressVector::SPtr expectedProgressVector = ProgressVector::Create(allocator);

                expectedTimeStamp = Common::DateTime::Now();
                ProgressVectorEntry e1(Epoch(expectedEpochDataLossNumber, expectedEpochConfigurationNumber), expectedStartingLogicalSequenceNumber, expectedReplicaId, expectedTimeStamp);
                expectedProgressVector->Insert(e1);

                CopyMetadata::SPtr copyMetadata = CopyMetadata::Create(
                    expectedStateMetadataVersion, 
                    *expectedProgressVector, 
                    expectedStartingEpoch, 
                    expectedStartingLogicalSequenceNumber, 
                    expectedCheckpointLsn, 
                    expectedUpToLsn, 
                    expectedHighestStateProviderCopiedLsn, 
                    allocator);                
                
                return copyMetadata;
            }
    };

    BOOST_FIXTURE_TEST_SUITE(CopyMetadataTestsSuite, CopyMetadataTests)

    BOOST_AUTO_TEST_CASE(VerifyCreation)
    {
        TEST_TRACE_BEGIN("VerifyCreation")
        {
            ULONG32 expectedStateMetadataVersion = 1;
            LONG64 expectedEpochDataLossNumber = 3;
            LONG64 expectedEpochConfigurationNumber = 2;
            LONG64 expectedStartingLogicalSequenceNumber = 5;
            LONG64 expectedCheckpointLsn = 6;
            LONG64 expectedUpToLsn = 7;
            LONG64 expectedHighestStateProviderCopiedLsn = 8;
            LONG64 expectedReplicaId = 12345;
            DateTime expectedTimeStamp;

            CopyMetadata::SPtr copyMetadata = CopyMetadataTestUtility::CreateCopyMetadata(
                expectedStateMetadataVersion, 
                expectedEpochDataLossNumber,
                expectedEpochConfigurationNumber,
                expectedStartingLogicalSequenceNumber,
                expectedCheckpointLsn,
                expectedUpToLsn,
                expectedHighestStateProviderCopiedLsn,
                expectedReplicaId,
                expectedTimeStamp,
                allocator);

            VERIFY_ARE_EQUAL(copyMetadata->get_CopyStateMetadataVersion(), expectedStateMetadataVersion);
            
            VERIFY_ARE_EQUAL(copyMetadata->get_ProgressVector()->get_LastProgressVectorEntry().get_Epoch().DataLossVersion, expectedEpochDataLossNumber);
            VERIFY_ARE_EQUAL(copyMetadata->get_ProgressVector()->get_LastProgressVectorEntry().get_Epoch().ConfigurationVersion, expectedEpochConfigurationNumber);
            VERIFY_ARE_EQUAL(copyMetadata->get_ProgressVector()->get_LastProgressVectorEntry().get_Lsn(), expectedStartingLogicalSequenceNumber);
            VERIFY_ARE_EQUAL(copyMetadata->get_ProgressVector()->get_LastProgressVectorEntry().get_PrimaryReplicaId(), expectedReplicaId);
            VERIFY_ARE_EQUAL(copyMetadata->get_ProgressVector()->get_LastProgressVectorEntry().get_TimeStamp(), expectedTimeStamp);

            VERIFY_ARE_EQUAL(copyMetadata->get_StartingEpoch().DataLossVersion, expectedEpochDataLossNumber);
            VERIFY_ARE_EQUAL(copyMetadata->get_StartingEpoch().ConfigurationVersion, expectedEpochConfigurationNumber);

            VERIFY_ARE_EQUAL(copyMetadata->get_StartingLogicalSequenceNumber(), expectedStartingLogicalSequenceNumber);
            VERIFY_ARE_EQUAL(copyMetadata->get_CheckpointLsn(), expectedCheckpointLsn);
            VERIFY_ARE_EQUAL(copyMetadata->get_UptoLsn(), expectedUpToLsn);
            VERIFY_ARE_EQUAL(copyMetadata->get_HighestStateProviderCopiedLsn(), expectedHighestStateProviderCopiedLsn);
        }
    }

    BOOST_AUTO_TEST_CASE(VerifySerializeDeserialize)
    {
        TEST_TRACE_BEGIN("VerifySerializeDeserialize")
        {
            ULONG32 expectedStateMetadataVersion = 1;
            LONG64 expectedEpochDataLossNumber = 3;
            LONG64 expectedEpochConfigurationNumber = 2;
            LONG64 expectedStartingLogicalSequenceNumber = 5;
            LONG64 expectedCheckpointLsn = 6;
            LONG64 expectedUpToLsn = 7;
            LONG64 expectedHighestStateProviderCopiedLsn = 8;
            LONG64 expectedReplicaId = 12345;
            DateTime expectedTimeStamp;

            CopyMetadata::SPtr copyMetadata = CopyMetadataTestUtility::CreateCopyMetadata(
                expectedStateMetadataVersion,
                expectedEpochDataLossNumber,
                expectedEpochConfigurationNumber,
                expectedStartingLogicalSequenceNumber,
                expectedCheckpointLsn,
                expectedUpToLsn,
                expectedHighestStateProviderCopiedLsn,
                expectedReplicaId,
                expectedTimeStamp,
                allocator);

            KArray<KBuffer::CSPtr> buffers(allocator);
            buffers.Append(CopyMetadata::Serialize(*copyMetadata, allocator).RawPtr());

            OperationData::CSPtr opData = OperationData::Create(buffers, allocator);

            CopyMetadata::SPtr readCopyMetadata = CopyMetadata::Deserialize(*opData, allocator);

            VERIFY_ARE_EQUAL(readCopyMetadata->get_CopyStateMetadataVersion(), expectedStateMetadataVersion);
            
            VERIFY_ARE_EQUAL(readCopyMetadata->get_ProgressVector()->get_LastProgressVectorEntry().get_Epoch().DataLossVersion, expectedEpochDataLossNumber);
            VERIFY_ARE_EQUAL(readCopyMetadata->get_ProgressVector()->get_LastProgressVectorEntry().get_Epoch().ConfigurationVersion, expectedEpochConfigurationNumber);
            VERIFY_ARE_EQUAL(readCopyMetadata->get_ProgressVector()->get_LastProgressVectorEntry().get_Lsn(), expectedStartingLogicalSequenceNumber);
            VERIFY_ARE_EQUAL(readCopyMetadata->get_ProgressVector()->get_LastProgressVectorEntry().get_PrimaryReplicaId(), expectedReplicaId);
            VERIFY_ARE_EQUAL(readCopyMetadata->get_ProgressVector()->get_LastProgressVectorEntry().get_TimeStamp(), expectedTimeStamp);

            VERIFY_ARE_EQUAL(readCopyMetadata->get_StartingEpoch().DataLossVersion, expectedEpochDataLossNumber);
            VERIFY_ARE_EQUAL(readCopyMetadata->get_StartingEpoch().ConfigurationVersion, expectedEpochConfigurationNumber);

            VERIFY_ARE_EQUAL(readCopyMetadata->get_StartingLogicalSequenceNumber(), expectedStartingLogicalSequenceNumber);
            VERIFY_ARE_EQUAL(readCopyMetadata->get_CheckpointLsn(), expectedCheckpointLsn);
            VERIFY_ARE_EQUAL(readCopyMetadata->get_UptoLsn(), expectedUpToLsn);
            VERIFY_ARE_EQUAL(readCopyMetadata->get_HighestStateProviderCopiedLsn(), expectedHighestStateProviderCopiedLsn);
        }
    }

    BOOST_AUTO_TEST_SUITE_END()
}
