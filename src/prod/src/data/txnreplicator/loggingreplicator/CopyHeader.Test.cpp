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

    StringLiteral const TraceComponent = "CopyHeaderTests";

    class CopyHeaderTests
    {
    protected:
        void EndTest();

        KGuid pId_;
        ::FABRIC_REPLICA_ID rId_;
        PartitionedReplicaId::SPtr prId_;
        KtlSystem * underlyingSystem_;
    };

    void CopyHeaderTests::EndTest()
    {
        prId_.Reset();
    }

    BOOST_FIXTURE_TEST_SUITE(CopyHeaderTestsSuite, CopyHeaderTests)

    BOOST_AUTO_TEST_CASE(VerifyCreation)
    {
        TEST_TRACE_BEGIN("VerifyCreation")
        {
            ULONG32 expectedVersion = 1;
            CopyStage::Enum expectedCopyStage = CopyStage::Enum::CopyMetadata;
            LONG64 expectedReplicaId = 12345;
            CopyHeader::SPtr copyHeader = CopyHeader::Create(expectedVersion, expectedCopyStage, expectedReplicaId, allocator);

            VERIFY_ARE_EQUAL(copyHeader->get_Version(), expectedVersion);
            VERIFY_ARE_EQUAL(copyHeader->get_CopyStage(), expectedCopyStage);
            VERIFY_ARE_EQUAL(copyHeader->get_PrimaryReplicaId(), expectedReplicaId);
        }
    }

    BOOST_AUTO_TEST_CASE(VerifySerializeDeserialize)
    {
        TEST_TRACE_BEGIN("VerifySerializeDeserialize")
        {
            ULONG32 expectedVersion = 1;
            CopyStage::Enum expectedCopyStage = CopyStage::Enum::CopyMetadata;
            LONG64 expectedReplicaId = 12345;
            CopyHeader::SPtr copyHeader = CopyHeader::Create(expectedVersion, expectedCopyStage, expectedReplicaId, allocator);

            KArray<KBuffer::CSPtr> buffers(allocator);
            buffers.Append(CopyHeader::Serialize(*copyHeader, allocator).RawPtr());

            OperationData::CSPtr opData = OperationData::Create(buffers, allocator);

            CopyHeader::SPtr readCopyHeader = CopyHeader::Deserialize(*opData, allocator);

            VERIFY_ARE_EQUAL(readCopyHeader->get_Version(), expectedVersion);
            VERIFY_ARE_EQUAL(readCopyHeader->get_CopyStage(), expectedCopyStage);
            VERIFY_ARE_EQUAL(readCopyHeader->get_PrimaryReplicaId(), expectedReplicaId);
        }
    }

    BOOST_AUTO_TEST_SUITE_END()
}
