// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "ComTestOperation.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace ReplicationUnitTest
{
    using namespace Common;
    using namespace std;
    using namespace Reliability::ReplicationComponent;
    using namespace Transport;
    static Common::StringLiteral const SecondaryTraceHandlerTestSource("SecondaryTraceHandlerTest");
    int const BatchMessageLength = 4;

    class TestSecondaryTraceHandler
    {
    protected:
        TestSecondaryTraceHandler() { BOOST_REQUIRE(Setup()); }

        void CreateAndEnqueueOp(FABRIC_SEQUENCE_NUMBER sequenceNumber)
        {
            traceHandler_->AddOperation(sequenceNumber);
        }

        bool TestSecondaryTraceHandler::Setup()
        {
            root_ = std::make_shared<DummyRoot>();
            config_ = REInternalSettings::Create(nullptr, std::make_shared<REConfig>());

            traceHandler_ = std::make_shared<SecondaryReplicatorTraceHandler>(static_cast<ULONG64>(config_->SecondaryReplicatorBatchTracingArraySize));

            return true;
        }

        // Trace all pending messages, clear the std::vector in traceHandler_
        void TracePendingMessages(wstring caller)
        {
            std::vector<SecondaryReplicatorTraceInfo> msgs;
            msgs = traceHandler_->GetTraceMessages();
            Trace.WriteInfo(
                SecondaryTraceHandlerTestSource,
                "{0} : {1}",
                caller,
                msgs);

            ASSERT_IFNOT(
                traceHandler_->Size == 0,
                "Expected 0 pending trace messages, found {0}",
                traceHandler_->Size);
        }

        void AddPendingTraceMessages(
            __in LONG64 numberOfOperations,
            __in FABRIC_SEQUENCE_NUMBER startingLsn)
        {
            LONG64 operationsToAdd = numberOfOperations;
            currentLsn_ = startingLsn;
            FABRIC_SEQUENCE_NUMBER batchLowLsn;
            FABRIC_SEQUENCE_NUMBER batchHighLsn;
            LONG64 lastIndex;

            while(operationsToAdd > 0)
            {
                Trace.WriteInfo(
                    SecondaryTraceHandlerTestSource,
                    "AddPendingTraceMessages: Current LSN={0}, Operations added={1}, Operations Remaining={2}, TraceHandler Info : (Size={3}, Index={4})",
                    currentLsn_,
                    numberOfOperations - operationsToAdd,
                    operationsToAdd,
                    traceHandler_->Size,
                    traceHandler_->Index);

                if(currentLsn_ % 2 == 0)
                {
                    // Add batch operation
                    batchLowLsn = currentLsn_;
                    batchHighLsn = batchLowLsn + BatchMessageLength;
                    currentLsn_ = batchHighLsn;

                    traceHandler_->AddBatchOperation(batchLowLsn, batchHighLsn);
                }
                else
                {
                    batchLowLsn = currentLsn_;
                    batchHighLsn = batchLowLsn;

                    // Add single operation
                    traceHandler_->AddOperation(currentLsn_);
                }

                LONG64 lowLsn;
                LONG64 highLsn;
                ErrorCode err;
                lastIndex = traceHandler_->Index > 0 ?
                    traceHandler_->Index - 1 :
                    config_->SecondaryReplicatorBatchTracingArraySize - 1;

                err = traceHandler_->Test_GetLsnAtIndex(
                    lastIndex,
                    lowLsn,
                    highLsn);

                ASSERT_IFNOT(
                    err.ReadValue() == ErrorCodeValue::Success,
                    "Failed to read LSN from index {0}, enqueued LSN=(low:{1}, high:{2}), LSN at index=(low:{3}, high:{4})",
                    lastIndex,
                    batchLowLsn,
                    batchHighLsn,
                    lowLsn,
                    highLsn);

                ASSERT_IFNOT(
                    (batchLowLsn == lowLsn) && (batchHighLsn == highLsn),
                    "Failed to match LSN from index {0}, enqueued LSN=(low:{1}, high:{2}), LSN at index=(low:{3}, high:{4})",
                    lastIndex,
                    batchLowLsn,
                    batchHighLsn,
                    lowLsn,
                    highLsn);

                operationsToAdd--;
                currentLsn_++;
            }
        }

        void TestSecondaryTraceHandler::ReplicationAckCallback(ComOperation & op)
        {
            UNREFERENCED_PARAMETER(op);
        }

        class DummyRoot : public Common::ComponentRoot
        {
        public:
            ~DummyRoot() {}
        };

        std::shared_ptr<DummyRoot> root_;
        REInternalSettingsSPtr config_;
        std::shared_ptr<SecondaryReplicatorTraceHandler> traceHandler_;
        FABRIC_SEQUENCE_NUMBER currentLsn_;
    };

    BOOST_FIXTURE_TEST_SUITE(TestSecondaryTraceHandlerSuite,TestSecondaryTraceHandler)
    
    BOOST_AUTO_TEST_CASE(Populate_LessThanOrEqualTo_MaxConfiguredValue)
    {
        // Add 1 operation and confirm size is expected
        this->CreateAndEnqueueOp(1);
        ASSERT_IFNOT(traceHandler_->Size == 1, "Expected 1 element in pending trace messages");

        this->TracePendingMessages(wstring(L"Single operation"));

        // Add operations less than maximum size and confirm vector status
        auto numPendingTraceMessages = config_->SecondaryReplicatorBatchTracingArraySize / 2;
        this->AddPendingTraceMessages(numPendingTraceMessages, 1);

        ASSERT_IFNOT(
            traceHandler_->Size == numPendingTraceMessages,
            "Unexpected std::vector size. Expected {0}, found {1}",
            numPendingTraceMessages,
            traceHandler_->Size);

        this->TracePendingMessages(Common::wformatString(L"{0} operations", numPendingTraceMessages));

        // Add maximum configured operations and validate size
        this->AddPendingTraceMessages(config_->SecondaryReplicatorBatchTracingArraySize, 1);

        ASSERT_IFNOT(
            traceHandler_->Size == config_->SecondaryReplicatorBatchTracingArraySize,
            "Unexpected std::vector size. Expected {0}, found {1}",
            config_->SecondaryReplicatorBatchTracingArraySize,
            traceHandler_->Size);

        this->TracePendingMessages(
            Common::wformatString(
                L"{0} operations",
                config_->SecondaryReplicatorBatchTracingArraySize));
    }

    BOOST_AUTO_TEST_CASE(Populate_Overflow_MaxConfiguredValue)
    {
        // Add maximum configured operations and validate size
        this->AddPendingTraceMessages(
            config_->SecondaryReplicatorBatchTracingArraySize,
            1);

        ASSERT_IFNOT(
            traceHandler_->Size == config_->SecondaryReplicatorBatchTracingArraySize,
            "Unexpected std::vector size. Expected {0}, found {1}",
            config_->SecondaryReplicatorBatchTracingArraySize,
            traceHandler_->Size);

        // Add 5 operations after maximum configured size is reached
        int additionalOperationCount = 5;
        this->AddPendingTraceMessages(
            additionalOperationCount,
            currentLsn_ + 1);

        // Confirm vector size has not increased
        ASSERT_IFNOT(
            traceHandler_->Size == config_->SecondaryReplicatorBatchTracingArraySize,
            "Unexpected std::vector size. Expected {0}, found {1}",
            config_->SecondaryReplicatorBatchTracingArraySize,
            traceHandler_->Size);

        this->TracePendingMessages(
            Common::wformatString(
                L"{0} operations",
                config_->SecondaryReplicatorBatchTracingArraySize + additionalOperationCount));
    }

    BOOST_AUTO_TEST_SUITE_END()
}
