// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "ComTestStatefulServicePartition.h"
#include "ComTestStateProvider.h"
#include "ComTestOperation.h"

namespace ReplicationUnitTest
{
    using namespace Common;
    using namespace std;
    using namespace Reliability::ReplicationComponent;

    typedef std::shared_ptr<REConfig> REConfigSPtr;
    static uint QueueSize = 8;
    int64 PrimaryReplicaId = 1;
    Common::StringLiteral const TraceType("CopyAsyncOperationTest");
    std::wstring Purpose = L"TestCopyAsyncOp";
    std::wstring Desc = L"Testing copy operations";

    // Validate behaviors with different state in the CopyAsyncOperation->OperationQueue
    enum ExpectedCancelAction
    {
        None = 0,                               // No cancellation
        CancelQueueNotFull = 1,                 // Cancellation in SendCallback w/ CopyAsyncOperation->OperationQueue populated but not full
        CancelledQueueFull = 2,                 // Cancellation in SendCallback w/ full CopyAsyncOperation->OperationQueue
        CancelledInLastOperationCallback = 3    // Cancellation in EnumerateLastOpCallback - regardless of CopyAsyncOperation->OperationQueue status
    };

    class TestCopyComponent : public ComponentRoot
    {
        DENY_COPY(TestCopyComponent)

    public:
        TestCopyComponent(
            int64 numCopyOps,
            int64 numCopyContextOps,
            int64 maxAllowedSequenceNumber,
            ExpectedCancelAction status,
            int64 cancellationSequenceNum) :
            copyAsyncOp_(nullptr),
            numberOfCopyOps_(numCopyOps),
            numberOfCopyContextOps_(numCopyContextOps),
            maxAckSequenceNumber_(maxAllowedSequenceNumber),
            expectedCancelAction_(status),
            cancellationSequenceNumber_(cancellationSequenceNum),
            queueFull_(false),
            ctestSP_(nullptr),
            ComponentRoot()
        {
            config_ = std::make_shared<REConfig>();
            config_->InitialCopyQueueSize = QueueSize;
            config_->MaxCopyQueueSize = QueueSize;
            config_->QueueHealthMonitoringInterval = TimeSpan::Zero;
            config_->SlowApiMonitoringInterval = TimeSpan::Zero;
            config_->RetryInterval = TimeSpan::FromSeconds(1);
            internalConfig_ = REInternalSettings::Create(nullptr, config_);

            // PartitionId/ReplicaId/ReplicationEndpointID
            partitionId_ = Guid::NewGuid();
            replicaId_ = PrimaryReplicaId;
            endpointId_ = ReplicationEndpointId(partitionId_, PrimaryReplicaId);
        }

        void TestGetCopyProgress(
            __in CopyAsyncOperation * copyOp,
            __out FABRIC_SEQUENCE_NUMBER & notReceivedCopyCount)
        {
            FABRIC_SEQUENCE_NUMBER copyReceivedLSN;
            FABRIC_SEQUENCE_NUMBER copyAppliedLSN;
            Common::TimeSpan avgCopyReceiveAckDuration;
            Common::TimeSpan avgCopyApplyAckDuration;
            FABRIC_SEQUENCE_NUMBER receivedAndNotAppliedCopyCount;
            Common::DateTime lastAckProcessedTime;

            copyOp->GetCopyProgress(
                copyReceivedLSN,
                copyAppliedLSN,
                avgCopyReceiveAckDuration,
                avgCopyApplyAckDuration,
                notReceivedCopyCount,
                receivedAndNotAppliedCopyCount,
                lastAckProcessedTime);

            Trace.WriteInfo(
                TraceType,
                "TestGetCopyProgress : copyReceivedLSN {0}, copyAppliedLSN {1}, notReceivedCopyCount {2}, receivedAndNotAppliedCopyCount {3}",
                copyReceivedLSN,
                copyAppliedLSN,
                notReceivedCopyCount,
                receivedAndNotAppliedCopyCount);
        }

        AsyncOperationSPtr BeginCopyAsyncTest(
            const AsyncCallback callback,
            AsyncOperationSPtr const& parent)
        {
            Trace.WriteInfo(TraceType, "BeginCopyAsyncTest");

            // Create state provider with copy and copy context operations
            ctestSP_ = std::shared_ptr<ComTestStateProvider>(
                new ComTestStateProvider(
                    numberOfCopyOps_,
                    numberOfCopyContextOps_,
                    *this));

            // Populate data stream and return ComProxyAsyncEnum
            Common::ComPointer<IFabricOperationDataStream> enumerator;
            ctestSP_->GetCopyContext(enumerator.InitializationAddress());
            ComProxyAsyncEnumOperationData cpAsyncEnum(move(enumerator));

            // Create ApiMonitoringWrapper
            IReplicatorHealthClientSPtr healthClient;
            ApiMonitoringWrapperSPtr apiMonitoringWrapper = ApiMonitoringWrapper::Create(
                healthClient,
                REInternalSettings::Create(nullptr, config_),
                partitionId_,
                endpointId_);

            copyAsyncOp_ = std::shared_ptr<CopyAsyncOperation>(new CopyAsyncOperation(
                internalConfig_,
                partitionId_,
                Purpose,
                Desc,
                endpointId_,
                replicaId_,
                move(cpAsyncEnum),
                apiMonitoringWrapper,
                ApiMonitoring::ApiName::Enum::GetNextCopyState,
                CopySendOperationCallback(),
                LastOperationCallback(),
                callback,
                parent));

            // Open reliable operation sender
            dynamic_cast<CopyAsyncOperation*>(copyAsyncOp_.get())->OpenReliableOperationSender();

            // Start async operation
            copyAsyncOp_->Start(copyAsyncOp_);

            return copyAsyncOp_;
        }

        static ErrorCode EndCopyAsyncTest(AsyncOperationSPtr const& operation)
        {
            Common::ErrorCode relOpSenderCloseStatus;
            std::wstring relOpSenderCloseDesc;
            ErrorCode err = CopyAsyncOperation::End(operation, relOpSenderCloseStatus, relOpSenderCloseDesc);

            Trace.WriteInfo(
                TraceType,
                "EndCopyAsyncTest : err={0}. relOpSenderCloseStatus={1}, relOpSenderCloseDesc={2}",
                err,
                relOpSenderCloseStatus,
                relOpSenderCloseDesc);

            ASSERT_IFNOT(
                relOpSenderCloseStatus.IsSuccess(),
                "CopyAsyncOperation->copyOperations_ expected to close successfully. Returned: {0}, Desc {1}",
                relOpSenderCloseStatus,
                relOpSenderCloseDesc);

            return err;
        }

        SendOperationCallback CopySendOperationCallback() 
        {
            return [this](ComOperationCPtr const & op, bool requestAck, FABRIC_SEQUENCE_NUMBER completedSeqNumber)
            {
                UNREFERENCED_PARAMETER(requestAck);
                UNREFERENCED_PARAMETER(completedSeqNumber);

                auto cachedOp = copyAsyncOp_;
                auto copyOp = AsyncOperation::Get<CopyAsyncOperation>(cachedOp);

                // Trace copy progress and retrieve not received copy count
                FABRIC_SEQUENCE_NUMBER notReceivedCopyCount = -1;
                TestGetCopyProgress(copyOp, notReceivedCopyCount);

                // Cancel copy async operation if target # reached or queue full + cancel action are satisfied
                if ((op->SequenceNumber == cancellationSequenceNumber_ ||
                    (queueFull_ &&
                        expectedCancelAction_ == ExpectedCancelAction::CancelledQueueFull)))
                {
                    Trace.WriteInfo(
                        TraceType,
                        "CopySendOperationCallback | Signalling Cancellation. Cancellation Sequence Number={0}, Op SequenceNumber={1}, QueueFull={2}",
                        cancellationSequenceNumber_,
                        op->SequenceNumber,
                        queueFull_);

                    CancelAndVerifyOperationStatus();

                    Trace.WriteInfo(
                        TraceType,
                        "CopySendOperationCallback | Test verification complete.");
                }

                // Queue is expected to be full after not received copy count >= queue size
                // Subsequent TryEnqueue operation will mark queue as full
                if (notReceivedCopyCount >= QueueSize)
                {
                    queueFull_ = true;
                }

                // Update progress if sequence number is less than target maximum
                // Allows for queue to be filled to varying degree for each test case
                if (op->SequenceNumber < maxAckSequenceNumber_)
                {
                    copyOp->UpdateProgress(op->SequenceNumber, op->SequenceNumber, cachedOp, false);
                    return true;
                }
                else
                {
                    return false;
                }
            };
        }

        EnumeratorLastOpCallback LastOperationCallback() 
        {
            return [this](FABRIC_SEQUENCE_NUMBER lastCopySequenceNumber)
            {
                auto cachedOp = copyAsyncOp_;
                auto op = AsyncOperation::Get<CopyAsyncOperation>(cachedOp);
                op->UpdateProgress(lastCopySequenceNumber, lastCopySequenceNumber, cachedOp, true);

                if (expectedCancelAction_ != ExpectedCancelAction::None)
                {
                    Trace.WriteInfo(
                        TraceType,
                        "LastOperationCallback | Signalling Cancellation. LastCopySequenceNumber={0}, QueueFull={1}",
                        lastCopySequenceNumber,
                        queueFull_);

                    // Signal cancellation
                    CancelAndVerifyOperationStatus();

                    Trace.WriteInfo(
                        TraceType,
                        "LastOperationCallback | Test Verification complete");
                }
                else
                {
                    bool status = op->TryCompleteWithSuccessIfCopyDone(cachedOp);
                    Trace.WriteInfo(TraceType, "TryCompleteWithSuccessIfCopyDone : {0}", status);
                }
            };
        }

    private:
        Common::AsyncOperationSPtr copyAsyncOp_;
        REConfigSPtr config_;
        REInternalSettingsSPtr internalConfig_;

        Common::Guid partitionId_;
        FABRIC_REPLICA_ID replicaId_;
        ReplicationEndpointId endpointId_;

        int64 numberOfCopyOps_;
        int64 numberOfCopyContextOps_;
        int64 maxAckSequenceNumber_;

        ExpectedCancelAction expectedCancelAction_;
        int64 cancellationSequenceNumber_;
        bool queueFull_;

        std::shared_ptr<ComTestStateProvider> ctestSP_;

        void CancelAndVerifyOperationStatus() 
        {
            ASSERT_IF(copyAsyncOp_ == nullptr, "Copy operation must be non-null");
            ASSERT_IF(copyAsyncOp_->IsCancelRequested, "CancelAndVerifyOperationStatus - cancellation must not be requested more than once");

            copyAsyncOp_->Cancel();

            Trace.WriteInfo(
                TraceType,
                "CancelAndVerifyOperationStatus | CopyAsyncOperation cancelled. IsCompleted {0}, IsCancelRequested {1}",
                copyAsyncOp_->IsCompleted,
                copyAsyncOp_->IsCancelRequested);

            switch (expectedCancelAction_)
            {
            case ExpectedCancelAction::None:
                break;

            case ExpectedCancelAction::CancelQueueNotFull:
                ASSERT_IFNOT(copyAsyncOp_->IsCompleted == false, "Unexpected completion");
                ASSERT_IFNOT(copyAsyncOp_->IsCancelRequested, "Copy op cancellation must be requested");
                break;

            case ExpectedCancelAction::CancelledQueueFull:
            case ExpectedCancelAction::CancelledInLastOperationCallback:
                ASSERT_IFNOT(copyAsyncOp_->IsCompleted == true, "Expected completion");
                ASSERT_IFNOT(copyAsyncOp_->IsCancelRequested, "Copy op cancellation must be requested");
                break;
            }

            Trace.WriteInfo(
                TraceType,
                "CancelAndVerifyOperationStatus | CopyAsyncOperation Verification complete : Verification IsCompleted={0} IsCancelRequested={1}",
                copyAsyncOp_->IsCompleted,
                copyAsyncOp_->IsCancelRequested);
        }
    };


    class CopyAsyncOperationUnitTests
    {
    protected:
        const TimeSpan MaxTestDuration = TimeSpan::FromSeconds(5);
    };

    BOOST_FIXTURE_TEST_SUITE(TestCopyAsyncOperationSuite, CopyAsyncOperationUnitTests);

    BOOST_AUTO_TEST_CASE(CopyAsync_QueueDrained_Success)
    {
        auto testComponent = make_shared<TestCopyComponent>(
            12,
            12,
            12,
            ExpectedCancelAction::None,
            -1);

        auto testCompletedEvent = make_shared<AutoResetEvent>(false);
        auto testComponentPtr = testComponent.get();
        auto testCompletedEventPtr = testCompletedEvent.get();

        // CopyAsyncOperation without queue full
        // All operations complete, no cancellation
        auto operation = testComponent->BeginCopyAsyncTest(
            [testComponentPtr, testCompletedEventPtr](AsyncOperationSPtr const& operation)
                {
                    ASSERT_IFNOT(
                        TestCopyComponent::EndCopyAsyncTest(operation).IsSuccess(),
                        "Failed ending test");

                    testCompletedEventPtr->Set();
                },
            testComponent->CreateAsyncOperationRoot());

        testCompletedEvent->WaitOne(MaxTestDuration);
    }

    BOOST_AUTO_TEST_CASE(CopyAsync_QueueDrained_Cancelled)
    {
        auto testComponent = make_shared<TestCopyComponent>(
            12,
            12,
            12,
            ExpectedCancelAction::CancelledInLastOperationCallback,
            -1);

        auto testCompletedEvent = make_shared<AutoResetEvent>(false);
        auto testComponentPtr = testComponent.get();
        auto testCompletedEventPtr = testCompletedEvent.get();

        // Cancellation without queue full
        // GetNext calls are finished, expect OperationCanceled
        auto operation = testComponent->BeginCopyAsyncTest(
            [testComponentPtr, testCompletedEventPtr](AsyncOperationSPtr const& operation)
                {
                    Common::ErrorCode endTestStatus = TestCopyComponent::EndCopyAsyncTest(operation);

                    ASSERT_IFNOT(
                        endTestStatus.ReadValue() == Common::ErrorCodeValue::OperationCanceled,
                        "Failed ending test - expected {0}, EndCopyAsyncTest returned {1}",
                        Common::ErrorCodeValue::OperationCanceled,
                        endTestStatus);

                    testCompletedEventPtr->Set();
                },
            testComponent->CreateAsyncOperationRoot());

        testCompletedEvent->WaitOne(MaxTestDuration);
    }

    BOOST_AUTO_TEST_CASE(CopyAsync_QueueDraining_Cancelled)
    {
        auto testComponent = make_shared<TestCopyComponent>(
            12,
            12,
            10,
            ExpectedCancelAction::CancelQueueNotFull,
            10);

        auto testCompletedEvent = make_shared<AutoResetEvent>(false);
        auto testComponentPtr = testComponent.get();
        auto testCompletedEventPtr = testCompletedEvent.get();

        // Cancellation without queue full
        // GetNext calls pending, expect OperationCanceled
        auto operation = testComponent->BeginCopyAsyncTest(
            [testComponentPtr, testCompletedEventPtr](AsyncOperationSPtr const& operation)
                {
                    Common::ErrorCode endTestStatus = TestCopyComponent::EndCopyAsyncTest(operation);

                    ASSERT_IFNOT(
                        endTestStatus.ReadValue() == Common::ErrorCodeValue::OperationCanceled,
                        "Failed ending test - expected {0}, EndCopyAsyncTest returned {1}",
                        Common::ErrorCodeValue::OperationCanceled,
                        endTestStatus);

                    testCompletedEventPtr->Set();
                },
            testComponentPtr->CreateAsyncOperationRoot());

        testCompletedEvent->WaitOne(MaxTestDuration);
    }

    BOOST_AUTO_TEST_CASE(CopyAsync_QueueFull_Cancelled)
    {
        auto testComponent = make_shared<TestCopyComponent>(
            15,
            15,
            4,
            ExpectedCancelAction::CancelledQueueFull,
            12);

        auto testCompletedEvent = make_shared<AutoResetEvent>(false);
        auto testComponentPtr = testComponent.get();
        auto testCompletedEventPtr = testCompletedEvent.get();

        // Cancellation with queue full, expect OperationCanceled
        // GetNext calls pending
        auto operation = testComponent->BeginCopyAsyncTest(
            [testComponentPtr, testCompletedEventPtr](AsyncOperationSPtr const& operation)
                {
                    Common::ErrorCode endTestStatus = TestCopyComponent::EndCopyAsyncTest(operation);

                    ASSERT_IFNOT(
                        endTestStatus.ReadValue() == Common::ErrorCodeValue::OperationCanceled,
                        "Failed ending test - expected {0}, EndCopyAsyncTest returned {1}",
                        Common::ErrorCodeValue::OperationCanceled,
                        endTestStatus);

                    testCompletedEventPtr->Set();
                },
            testComponent->CreateAsyncOperationRoot());

        testCompletedEvent->WaitOne(MaxTestDuration);
    }

    BOOST_AUTO_TEST_SUITE_END();
}
