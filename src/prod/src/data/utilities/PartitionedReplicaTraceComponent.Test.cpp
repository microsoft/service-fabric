// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace UtilitiesTests
{
    using namespace ktl;
    using namespace Data::Utilities;

    class PartitionedReplicaTraceComponentTest 
    {
    public:
        Common::CommonConfig config; // load the config object as its needed for the tracing to work

        PartitionedReplicaTraceComponentTest()
        {
            NTSTATUS status;
            status = KtlSystem::Initialize(FALSE, &ktlSystem_);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            ktlSystem_->SetStrictAllocationChecks(TRUE);
        }

        ~PartitionedReplicaTraceComponentTest()
        {
            ktlSystem_->Shutdown();
        }

        KAllocator& GetAllocator()
        {
            return ktlSystem_->NonPagedAllocator();
        }

    private:
        KtlSystem* ktlSystem_;
    };

    Common::StringLiteral const type("SampleClass");

    class SampleClass : public PartitionedReplicaTraceComponent<Common::TraceTaskCodes::Enum::LR>
    {
    public:
        SampleClass(PartitionedReplicaId & id)
            : PartitionedReplicaTraceComponent(id)
        {
        }

        void Trace()
        {
            WriteNoise(
                type,
                "{0}: Test noise Trace",
                this->TraceId);

            WriteInfo(
                type,
                "{0}: Test info Trace",
                this->TraceId);

            WriteWarning(
                type,
                "{0}: Test warning Trace",
                this->TraceId);

            WriteError(
                type,
                "{0}: Test error Trace",
                this->TraceId);
        }
    };

    BOOST_FIXTURE_TEST_SUITE(PartitionedReplicaTraceComponentTestSuite, PartitionedReplicaTraceComponentTest)

    BOOST_AUTO_TEST_CASE(Basic)
    {
        KAllocator& allocator = PartitionedReplicaTraceComponentTest::GetAllocator();
        KGuid guid;
        guid.CreateNew();

        Trace.WriteInfo(type, "{0}", "ss");

        ::FABRIC_REPLICA_ID replicaId = 1;

        PartitionedReplicaId::SPtr id = PartitionedReplicaId::Create(guid, replicaId, allocator);

        SampleClass object(*id);

        object.Trace();

        KGuid traceGuid(object.PartitionId);

        VERIFY_ARE_EQUAL(guid, traceGuid);
    }

    BOOST_AUTO_TEST_SUITE_END()
}
