// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
 #include "ComTestStatefulServicePartition.h"
#include "ComTestOperation.h"
#include "ComTestStateProvider.h"

namespace ReplicationUnitTest
{
    using namespace Common;
    using namespace std;
    using namespace Reliability::ReplicationComponent;


    using Transport::MessageUPtr;
    class ReplicatorWrapper;

    typedef std::shared_ptr<REConfig> REConfigSPtr;
    static Common::StringLiteral const SecondaryTestSource("SecondaryTest");

    class TestSecondary
    {
    protected:
        //TestSecondary() { BOOST_REQUIRE(Setup()); }
        TEST_CLASS_SETUP(Setup);

        TestSecondary();

        ~TestSecondary()
        {
            transport_->Stop();
        }
        static REConfigSPtr CreateGenericConfig();
        static ReplicationTransportSPtr CreateTransport(REConfigSPtr const & config);
        class DummyRoot : public Common::ComponentRoot
        {
          //  ~DummyRoot() {}
        };
        static Common::ComponentRoot const & GetRoot();

        shared_ptr<ReplicatorWrapper> CreateAndOpenWrapper(
            int option,
            bool supportsParallelStreams = false,
            bool getBatchOperation = false);
        
        REConfigSPtr config_;
        REPerformanceCountersSPtr perfCounters_;
        ReplicationTransportSPtr transport_;
        ComProxyStateProvider persistedStateProvider_;
        ComProxyStateProvider volatileStateProvider_;
        FABRIC_EPOCH epoch_;  

    public: 
        void DuplicateInOrderCopy(int acks);

        void FirstCopy(int acks);

        void OutOfOrderCopy(int acks);

        void CloseIdlePendingOps(int acks, bool supportsParallelStreams);

        void ChangeRoleNoCopyOperations(int acks);

        void ChangeRoleIdleAllCopyReceivedNotTakenByService(bool supportsParallelStreams);

        void DuplicateOutOfOrderCopy(int acks);

        void CopyCompleted(int acks);

        void CopyCompletedWithOutOfOrderReplication(int acks);

        void OutOfOrderCopyExceedingQueueCapacity(int acks);

        void CopyCompletedAndInOrderReplication(int acks);

        void ParallelCopyAndReplication(int acks);

        void StaleReplication(int acks);

        void OutOfOrderReplication(int acks, bool supportsParallelStreams);

        void OutOfOrderReplicationNoReply(int acks);

        void OutOfOrderReplicationExceedingQueueCapacity(int acks, bool supportsParallelStreams);

        void DuplicateReplication(int acks);

        void NewConfiguration(int acks, bool supportsParallelStreams);

        void GetProgress(int acks);

        void GetProgressWithMultipleConfigurationChanges(int acks);

        void MultipleOperationEpochs(int acks);

        void VerifyStateProviderEstablishedEpochTest();
    };

    BOOST_FIXTURE_TEST_SUITE(TestReplicationSecondarySuite, TestSecondary)
    
    BOOST_AUTO_TEST_CASE(DuplicateInOrderCopy1)
    {
            DuplicateInOrderCopy(0);
    }
    BOOST_AUTO_TEST_CASE(DuplicateInOrderCopy2)
    {
        DuplicateInOrderCopy(1);
    }
    BOOST_AUTO_TEST_CASE(DuplicateInOrderCopy3)
    {
        DuplicateInOrderCopy(2);
    }
    BOOST_AUTO_TEST_CASE(DuplicateInOrderCopy4)
    {
        DuplicateInOrderCopy(3);
    }
    BOOST_AUTO_TEST_CASE(DuplicateInOrderCopy5)
    {
        DuplicateInOrderCopy(4);
    }


    BOOST_AUTO_TEST_CASE(FirstCopy1)
    {
        FirstCopy(0);
    }
    BOOST_AUTO_TEST_CASE(FirstCopy2)
    {
        FirstCopy(1);
    }
    BOOST_AUTO_TEST_CASE(FirstCopy3)
    {
        FirstCopy(2);
    }
    BOOST_AUTO_TEST_CASE(FirstCopy4)
    {
        FirstCopy(3);
    }
    BOOST_AUTO_TEST_CASE(FirstCopy5)
    {
        FirstCopy(4);
    }


    BOOST_AUTO_TEST_CASE(OutOfOrderCopy1)
    {
        OutOfOrderCopy(0);
    }
    BOOST_AUTO_TEST_CASE(OutOfOrderCopy2)
    {
        OutOfOrderCopy(1);
    }
    BOOST_AUTO_TEST_CASE(OutOfOrderCopy3)
    {
        OutOfOrderCopy(2);
    }
    BOOST_AUTO_TEST_CASE(OutOfOrderCopy4)
    {
        OutOfOrderCopy(3);
    }
    BOOST_AUTO_TEST_CASE(OutOfOrderCopy5)
    {
        OutOfOrderCopy(4);
    }

    BOOST_AUTO_TEST_CASE(CloseIdlePendingOps1)
    {
        CloseIdlePendingOps(0, true);
    }

    BOOST_AUTO_TEST_CASE(CloseIdlePendingOps2)
    {
        CloseIdlePendingOps(1, true);
    }
    BOOST_AUTO_TEST_CASE(CloseIdlePendingOps3)
    {
        CloseIdlePendingOps(3, true);
    }
    BOOST_AUTO_TEST_CASE(CloseIdlePendingOps4)
    {
        CloseIdlePendingOps(0, false);
    }

    BOOST_AUTO_TEST_CASE(CloseIdlePendingOps5)
    {
        CloseIdlePendingOps(1, false);
    }
    BOOST_AUTO_TEST_CASE(CloseIdlePendingOps6)
    {
        CloseIdlePendingOps(3, false);
    }

    BOOST_AUTO_TEST_CASE(ChangeRoleNoCopyOperations1)
    {
        ChangeRoleNoCopyOperations(0);
    }
    BOOST_AUTO_TEST_CASE(ChangeRoleNoCopyOperations2)
    {
        ChangeRoleNoCopyOperations(1);
    }
    BOOST_AUTO_TEST_CASE(ChangeRoleNoCopyOperations3)
    {
        ChangeRoleNoCopyOperations(3);
    }

    BOOST_AUTO_TEST_CASE(ChangeRoleIdleAllCopyReceivedNotTakenByService1)
    {
        ChangeRoleIdleAllCopyReceivedNotTakenByService(true);
    }

    BOOST_AUTO_TEST_CASE(ChangeRoleIdleAllCopyReceivedNotTakenByService2)
    {
        ChangeRoleIdleAllCopyReceivedNotTakenByService(false);
    }

    BOOST_AUTO_TEST_CASE(DuplicateOutOfOrderCopy1)
    {
        DuplicateOutOfOrderCopy(0);
    }
    BOOST_AUTO_TEST_CASE(DuplicateOutOfOrderCopy2)
    {
        DuplicateOutOfOrderCopy(1);
    }
    BOOST_AUTO_TEST_CASE(DuplicateOutOfOrderCopy3)
    {
        DuplicateOutOfOrderCopy(2);
    }
    BOOST_AUTO_TEST_CASE(DuplicateOutOfOrderCopy4)
    {
        DuplicateOutOfOrderCopy(3);
    }
    BOOST_AUTO_TEST_CASE(DuplicateOutOfOrderCopy5)
    {
        DuplicateOutOfOrderCopy(4);
    }

    BOOST_AUTO_TEST_CASE(CopyCompleted1)
    {
        CopyCompleted(0);
    }
    BOOST_AUTO_TEST_CASE(CopyCompleted2)
    {
        CopyCompleted(1);
    }
    BOOST_AUTO_TEST_CASE(CopyCompleted3)
    {
        CopyCompleted(2);
    }
    BOOST_AUTO_TEST_CASE(CopyCompleted4)
    {
        CopyCompleted(3);
    }
    BOOST_AUTO_TEST_CASE(CopyCompleted5)
    {
        CopyCompleted(4);
    }

    BOOST_AUTO_TEST_CASE(CopyCompletedWithOutOfOrderReplication1)
    {
        CopyCompletedWithOutOfOrderReplication(0);
    }
    BOOST_AUTO_TEST_CASE(CopyCompletedWithOutOfOrderReplication2)
    {
        CopyCompletedWithOutOfOrderReplication(1);
    }
    BOOST_AUTO_TEST_CASE(CopyCompletedWithOutOfOrderReplication3)
    {
        CopyCompletedWithOutOfOrderReplication(2);
    }
    BOOST_AUTO_TEST_CASE(CopyCompletedWithOutOfOrderReplication4)
    {
        CopyCompletedWithOutOfOrderReplication(3);
    }
    BOOST_AUTO_TEST_CASE(CopyCompletedWithOutOfOrderReplication5)
    {
        CopyCompletedWithOutOfOrderReplication(4);
    }

    BOOST_AUTO_TEST_CASE(OutOfOrderCopyExceedingQueueCapacity1)
    {
        OutOfOrderCopyExceedingQueueCapacity(0);
    }
    BOOST_AUTO_TEST_CASE(OutOfOrderCopyExceedingQueueCapacity2)
    {
        OutOfOrderCopyExceedingQueueCapacity(1);
    }
    BOOST_AUTO_TEST_CASE(OutOfOrderCopyExceedingQueueCapacity3)
    {
        OutOfOrderCopyExceedingQueueCapacity(2);
    }
    BOOST_AUTO_TEST_CASE(OutOfOrderCopyExceedingQueueCapacity4)
    {
        OutOfOrderCopyExceedingQueueCapacity(3);
    }
    BOOST_AUTO_TEST_CASE(OutOfOrderCopyExceedingQueueCapacity5)
    {
        OutOfOrderCopyExceedingQueueCapacity(4);
    }

    BOOST_AUTO_TEST_CASE(CopyCompletedAndInOrderReplication1)
    {
        CopyCompletedAndInOrderReplication(0);
    }
    BOOST_AUTO_TEST_CASE(CopyCompletedAndInOrderReplication2)
    {
        CopyCompletedAndInOrderReplication(1);
    }
    BOOST_AUTO_TEST_CASE(CopyCompletedAndInOrderReplication3)
    {
        CopyCompletedAndInOrderReplication(2);
    }
    BOOST_AUTO_TEST_CASE(CopyCompletedAndInOrderReplication4)
    {
        CopyCompletedAndInOrderReplication(3);
    }
    BOOST_AUTO_TEST_CASE(CopyCompletedAndInOrderReplication5)
    {
        CopyCompletedAndInOrderReplication(4);
    }

    BOOST_AUTO_TEST_CASE(ParallelCopyAndReplication1)
    {
        ParallelCopyAndReplication(0);
    }
    BOOST_AUTO_TEST_CASE(ParallelCopyAndReplication2)
    {
        ParallelCopyAndReplication(1);
    }
    BOOST_AUTO_TEST_CASE(ParallelCopyAndReplication3)
    {
        ParallelCopyAndReplication(3);
    }


    BOOST_AUTO_TEST_CASE(StaleReplication1)
    {
        StaleReplication(0);
    }
    BOOST_AUTO_TEST_CASE(StaleReplication2)
    {
        StaleReplication(1);
    }
    BOOST_AUTO_TEST_CASE(StaleReplication3)
    {
        StaleReplication(2);
    }
    BOOST_AUTO_TEST_CASE(StaleReplication4)
    {
        StaleReplication(3);
    }
    BOOST_AUTO_TEST_CASE(StaleReplication5)
    {
        StaleReplication(4);
    }


    BOOST_AUTO_TEST_CASE(OutOfOrderReplication1)
    {
        OutOfOrderReplication(0, true);
    }
    BOOST_AUTO_TEST_CASE(OutOfOrderReplication2)
    {
        OutOfOrderReplication(1, true);
    }
    BOOST_AUTO_TEST_CASE(OutOfOrderReplication3)
    {
        OutOfOrderReplication(2, true);
    }
    BOOST_AUTO_TEST_CASE(OutOfOrderReplication4)
    {
        OutOfOrderReplication(3, true);
    }
    BOOST_AUTO_TEST_CASE(OutOfOrderReplication5)
    {
        OutOfOrderReplication(4, true);
    }
    BOOST_AUTO_TEST_CASE(OutOfOrderReplication6)
    {
        OutOfOrderReplication(0, false);
    }
    BOOST_AUTO_TEST_CASE(OutOfOrderReplication7)
    {
        OutOfOrderReplication(1, false);
    }
    BOOST_AUTO_TEST_CASE(OutOfOrderReplication8)
    {
        OutOfOrderReplication(2, false);
    }
    BOOST_AUTO_TEST_CASE(OutOfOrderReplication9)
    {
        OutOfOrderReplication(3, false);
    }
    BOOST_AUTO_TEST_CASE(OutOfOrderReplication10)
    {
        OutOfOrderReplication(4, false);
    }

    BOOST_AUTO_TEST_CASE(OutOfOrderReplicationNoReply1)
    {
        OutOfOrderReplicationNoReply(0);
    }
    BOOST_AUTO_TEST_CASE(OutOfOrderReplicationNoReply2)
    {
        OutOfOrderReplicationNoReply(1);
    }
    BOOST_AUTO_TEST_CASE(OutOfOrderReplicationNoReply3)
    {
        OutOfOrderReplicationNoReply(2);
    }
    BOOST_AUTO_TEST_CASE(OutOfOrderReplicationNoReply4)
    {
        OutOfOrderReplicationNoReply(3);
    }
    BOOST_AUTO_TEST_CASE(OutOfOrderReplicationNoReply5)
    {
        OutOfOrderReplicationNoReply(4);
    }

    BOOST_AUTO_TEST_CASE(OutOfOrderReplicationExceedingQueueCapacity1)
    {
        OutOfOrderReplicationExceedingQueueCapacity(0, true);
    }
    BOOST_AUTO_TEST_CASE(OutOfOrderReplicationExceedingQueueCapacity2)
    {
        OutOfOrderReplicationExceedingQueueCapacity(1, true);
    }
    BOOST_AUTO_TEST_CASE(OutOfOrderReplicationExceedingQueueCapacity3)
    {
        OutOfOrderReplicationExceedingQueueCapacity(2, true);
    }
    BOOST_AUTO_TEST_CASE(OutOfOrderReplicationExceedingQueueCapacity4)
    {
        OutOfOrderReplicationExceedingQueueCapacity(3, true);
    }
    BOOST_AUTO_TEST_CASE(OutOfOrderReplicationExceedingQueueCapacity5)
    {
        OutOfOrderReplicationExceedingQueueCapacity(4, true);
    }
    BOOST_AUTO_TEST_CASE(OutOfOrderReplicationExceedingQueueCapacity6)
    {
        OutOfOrderReplicationExceedingQueueCapacity(0, false);
    }
    BOOST_AUTO_TEST_CASE(OutOfOrderReplicationExceedingQueueCapacity7)
    {
        OutOfOrderReplicationExceedingQueueCapacity(1, false);
    }
    BOOST_AUTO_TEST_CASE(OutOfOrderReplicationExceedingQueueCapacity8)
    {
        OutOfOrderReplicationExceedingQueueCapacity(2, false);
    }
    BOOST_AUTO_TEST_CASE(OutOfOrderReplicationExceedingQueueCapacity9)
    {
        OutOfOrderReplicationExceedingQueueCapacity(3, false);
    }
    BOOST_AUTO_TEST_CASE(OutOfOrderReplicationExceedingQueueCapacity10)
    {
        OutOfOrderReplicationExceedingQueueCapacity(4, false);
    }

    BOOST_AUTO_TEST_CASE(DuplicateReplication1)
    {
        DuplicateReplication(0);
    }
    BOOST_AUTO_TEST_CASE(DuplicateReplication2)
    {
        DuplicateReplication(1);
    }
    BOOST_AUTO_TEST_CASE(DuplicateReplication3)
    {
        DuplicateReplication(2);
    }
    BOOST_AUTO_TEST_CASE(DuplicateReplication4)
    {
        DuplicateReplication(3);
    }
    BOOST_AUTO_TEST_CASE(DuplicateReplication5)
    {
        DuplicateReplication(4);
    }


    BOOST_AUTO_TEST_CASE(NewConfiguration1)
    {
        NewConfiguration(0, true);
    }
    BOOST_AUTO_TEST_CASE(NewConfiguration2)
    {
        NewConfiguration(1, true);
    }
    BOOST_AUTO_TEST_CASE(NewConfiguration3)
    {
        NewConfiguration(2, true);
    }
    BOOST_AUTO_TEST_CASE(NewConfiguration4)
    {
        NewConfiguration(3, true);
    }
    BOOST_AUTO_TEST_CASE(NewConfiguration5)
    {
        NewConfiguration(4, true);
    }
    BOOST_AUTO_TEST_CASE(NewConfiguration6)
    {
        NewConfiguration(0, false);
    }
    BOOST_AUTO_TEST_CASE(NewConfiguration7)
    {
        NewConfiguration(1, false);
    }
    BOOST_AUTO_TEST_CASE(NewConfiguration8)
    {
        NewConfiguration(2, false);
    }
    BOOST_AUTO_TEST_CASE(NewConfiguration9)
    {
        NewConfiguration(3, false);
    }
    BOOST_AUTO_TEST_CASE(NewConfiguration10)
    {
        NewConfiguration(4, false);
    }

    // Get progress can't be done if copy is not finished;
    // so we need to ACK in order for persisted state.

    BOOST_AUTO_TEST_CASE(GetProgress1)
    {
        GetProgress(0);
    }
    BOOST_AUTO_TEST_CASE(GetProgress2)
    {
        GetProgress(1);
    }
    BOOST_AUTO_TEST_CASE(GetProgress3)
    {
        GetProgress(3);
    }

    BOOST_AUTO_TEST_CASE(GetProgressWithMultipleConfigurationChanges1)
    {
        GetProgressWithMultipleConfigurationChanges(0);
    }
    BOOST_AUTO_TEST_CASE(GetProgressWithMultipleConfigurationChanges2)
    {
        GetProgressWithMultipleConfigurationChanges(1);
    }
    BOOST_AUTO_TEST_CASE(GetProgressWithMultipleConfigurationChanges3)
    {
        GetProgressWithMultipleConfigurationChanges(3);
    }

    BOOST_AUTO_TEST_CASE(MultipleOperationEpochs1)
    {
        MultipleOperationEpochs(0);
    }
    BOOST_AUTO_TEST_CASE(MultipleOperationEpochs2)
    {
        MultipleOperationEpochs(1);
    }
    BOOST_AUTO_TEST_CASE(MultipleOperationEpochs3)
    {
        MultipleOperationEpochs(2);
    }
    BOOST_AUTO_TEST_CASE(MultipleOperationEpochs4)
    {
        MultipleOperationEpochs(3);
    }
    BOOST_AUTO_TEST_CASE(MultipleOperationEpochs5)
    {
        MultipleOperationEpochs(4);
    }

    BOOST_AUTO_TEST_CASE(VerifyStateProviderEstablishedEpoch)
    {
        VerifyStateProviderEstablishedEpochTest();
    }

    BOOST_AUTO_TEST_SUITE_END()

    class ReplicatorWrapper : public SecondaryReplicator
    {
    public:
        ReplicatorWrapper(
            REConfigSPtr const & config,
            REPerformanceCountersSPtr & perfCounters,
            ReplicationTransportSPtr const & transport,
            ComProxyStateProvider const & stateProvider,
            Guid const & incarnationId,
            bool hasPersistedState,
            bool supportsParallelStreams,
            IReplicatorHealthClientSPtr  & temp,
            ApiMonitoringWrapperSPtr & temp2,
            bool ackInOrder = true,
            bool getBatchOperation = false)
            :   SecondaryReplicator(
                    REInternalSettings::Create(nullptr, config), 
                    perfCounters,
                    ReplicatorWrapper::ReplicaId, 
                    hasPersistedState,
                    ReplicationEndpointId(Guid::NewGuid(), 1, incarnationId), 
                    stateProvider,
                    ComProxyStatefulServicePartition(),
                    Replicator::V1Plus,
                    transport,
                    ReplicatorWrapper::PartitionId,
                    temp,
                    temp2),
                ackInOrder_(ackInOrder),
                getBatchOperation_(getBatchOperation),
                hasPersistedState_(hasPersistedState),
                requireServiceAck_(hasPersistedState || config->RequireServiceAck),
                supportsParallelStreams_(supportsParallelStreams),
                operationList_(), 
                ackList_(),
                event_(),
                testStateProvider_(),
                ops_(),
                opsReceived_(),
                opsLock_(),
                closeCompleteEvent_(),
                incarnationId_(incarnationId),
                updateEpochCompleteEvent_(true)
        {
            testStateProvider_.SetAndAddRef(static_cast<ComTestStateProvider*>(stateProvider.InnerStateProvider));
            testStateProvider_->ClearProgressVector();
        }

        __declspec (property(get=get_HasPersistedState)) bool HasPersistedState;
        bool get_HasPersistedState() const { return hasPersistedState_; }

        __declspec (property(get=get_RequireServiceAck)) bool RequireServiceAck;
        bool get_RequireServiceAck() const { return requireServiceAck_; }

        static wstring PartitionId;
        static FABRIC_REPLICA_ID ReplicaId;

        void ProcessSecondaryOperations(
            wstring const & testName, 
            wstring const & operations, 
            wstring const & expectedReply, 
            wstring const & expectedQueue,
            wstring const & expectedFinalState,
            wstring const & expectedProgressVector,
            wstring const & waitStatesForAckOutofOrder = L"",
            wstring const & waitStatesAfterClose = L"");

        void ProcessSecondaryOperationsForClose(
            wstring const & testName, 
            wstring const & operations, 
            wstring const & expectedReply, 
            wstring const & expectedQueue,
            wstring const & expectedFinalState,
            wstring const & expectedProgressVector);

        void ProcessSecondaryOperationsForChangeRole(
            wstring const & testName, 
            wstring const & operations, 
            wstring const & expectedReply, 
            wstring const & expectedQueue,
            wstring const & expectedFinalState,
            wstring const & expectedProgressVector,
            bool completeCopyBeforeClose);

        void ProcessSecondaryOperationsWithInvalidChangeRole(
            wstring const & testName, 
            wstring const & operations);
        
    private:
        static void CreateOp(
            wstring const & description,
            bool isCopy,
            FABRIC_SEQUENCE_NUMBER sequenceNumber,
            OperationAckCallback ackCallback,
            __out ComOperationCPtr & operation,
            FABRIC_SEQUENCE_NUMBER lastOperationInBatch,
            bool isLast,
            FABRIC_EPOCH const & opEpoch = Constants::InvalidEpoch);
        
        void GetFinalState(
            __out wstring & state);
        void Close(bool waitForOperationsToDrain, bool closeEventsBeforeClose = false);
        void Close(ErrorCodeValue::Enum expectedErrorCodeValue, bool waitForOperationsToDrain, bool closeEventsBeforeClose = false);
        void StartClose(ErrorCodeValue::Enum expectedErrorCodeValue, bool waitForOperationsToDrain, bool closeEventsBeforeClose = false);
        void VerifyCloseCompleted();
        void ProcessOperation(wstring const & data);
        void ProcessWaitForState(wstring const & data);
        void WaitForSequenceNumberReceived(wstring const & data);
        void GetAckString();
        void StartCopyOperationPump();
        void StartReplicationOperationPump();
        void StartGetOperation(OperationStreamSPtr const & stream);
        void GetOperationCallback(AsyncOperationSPtr const & asyncOp, OperationStreamSPtr const & stream);
        void GetSingleOperation(OperationStreamSPtr const & stream);
        void GetSingleOperationCallback(AsyncOperationSPtr const & asyncOp, OperationStreamSPtr const & stream);
        void AckOutOfOrderOperations();
        void AckOperations(bool stopOnFirstReplOperation);
        void AckOperationsCallerHoldsLock(bool stopOnFirstReplOperation);
        void CheckOutput(
            wstring const & expectedReply, 
            wstring const & expectedQueue,
            wstring const & expectedFinalState,
            wstring const & expectedProgressVector);
        void StartTest(wstring const & testName);
        void EndTest(wstring const & testName);
        void StartReplicatorUpdateEpoch(FABRIC_EPOCH const & newEpoch);
        void VerifyReplicatorUpdateEpochCompleted();

        static ReplicationFromHeader PrimaryHeader;

        bool ackInOrder_;
        bool hasPersistedState_;
        bool requireServiceAck_;
        bool supportsParallelStreams_;
        wstring operationList_;
        wstring ackList_;
        ManualResetEvent event_;
        ComPointer<ComTestStateProvider> testStateProvider_;        
        vector<IFabricOperation*> ops_;  
        vector<FABRIC_SEQUENCE_NUMBER> opsReceived_;
        mutable Common::ExclusiveLock opsLock_;
        ManualResetEvent closeCompleteEvent_;
        ManualResetEvent updateEpochCompleteEvent_;
        Guid incarnationId_;
        bool getBatchOperation_;
    };

    wstring ReplicatorWrapper::PartitionId(Common::Guid::NewGuid().ToString());
    FABRIC_REPLICA_ID ReplicatorWrapper::ReplicaId = 1;
    ReplicationFromHeader ReplicatorWrapper::PrimaryHeader(L"127.0.0.1:11111", ReplicationEndpointId(Guid::NewGuid(), 1));
    
    void ReplicatorWrapper::ProcessSecondaryOperations(
        wstring const & testName, 
        wstring const & operations, 
        wstring const & expectedReply, 
        wstring const & expectedQueue,
        wstring const & expectedFinalState,
        wstring const & expectedProgressVector,
        wstring const & waitStatesForAckOutofOrder,
        wstring const & waitStatesAfterClose)
    {
        StartTest(testName);
        
        StartCopyOperationPump();
        if (supportsParallelStreams_)
        {
            StartReplicationOperationPump();
        }
            
        StringCollection tokens;
        StringUtility::Split<wstring>(operations, tokens, L";");
        for (auto operation : tokens)
        { 
            ProcessOperation(operation);
        }

        if (!ackInOrder_)
        {
            AckOutOfOrderOperations();
            vector<wstring> items;
            StringUtility::Split<wstring>(waitStatesForAckOutofOrder, items, L";");
            for (vector<wstring>::iterator it = items.begin(); it != items.end(); it++)
            {
                ProcessWaitForState(*it);
                AckOutOfOrderOperations();
            }
        }

        StartClose(ErrorCodeValue::Success, false);
        ProcessWaitForState(waitStatesAfterClose);

        VERIFY_IS_TRUE(event_.WaitOne(10000), L"Null operation should be received");

        CheckOutput(expectedReply, expectedQueue, expectedFinalState, expectedProgressVector);
        VerifyCloseCompleted();

        EndTest(testName);
    }

    void ReplicatorWrapper::ProcessSecondaryOperationsForClose(
        wstring const & testName, 
        wstring const & operations, 
        wstring const & expectedReply, 
        wstring const & expectedQueue,
        wstring const & expectedFinalState,
        wstring const & expectedProgressVector)
    {
        StartTest(testName);

        OperationStreamSPtr copyStream = GetCopyStream(std::dynamic_pointer_cast<SecondaryReplicator>(shared_from_this()));
        OperationStreamSPtr replicationStream = GetReplicationStream(std::dynamic_pointer_cast<SecondaryReplicator>(shared_from_this()));

        GetSingleOperation(copyStream);
        if (supportsParallelStreams_)
        {
            GetSingleOperation(replicationStream);
        }

        StringCollection tokens;
        StringUtility::Split<wstring>(operations, tokens, L";");
        for (auto operation : tokens)
        { 
            ProcessOperation(operation);
        }

        // Close the service;
        // only one operation was taken, all other operations will be dropped;
        // NULL operation will not be taken by the service, 
        // but this won't block close
        StartClose(ErrorCodeValue::Success, false);

        // Let the service take more operations;
        // since all operations have been aborted,
        // only NULL should be taken
        StartGetOperation(copyStream);
        if (supportsParallelStreams_)
        {
            StartGetOperation(replicationStream);
        }

        size_t desiredOpsSize = supportsParallelStreams_ ? 2 : 1;

        {
            AcquireExclusiveLock lock(opsLock_);
            if (ops_.size() != desiredOpsSize)
            {
                VERIFY_FAIL_FMT("There should be {0} operation in ops_; instead, there are {1}", desiredOpsSize, ops_.size());
            }
        }

        AckOperations(false);

        CheckOutput(expectedReply, expectedQueue, expectedFinalState, expectedProgressVector);
        
        // Ack the operations the service took; this should be NOP
        AckOperations(false /*stopOnFirstReplOperation*/);
        
        VerifyCloseCompleted();
                
        EndTest(testName);
    }

    void ReplicatorWrapper::ProcessSecondaryOperationsWithInvalidChangeRole(
        wstring const & testName, 
        wstring const & operations)
    {
        StartTest(testName);

        // Do not start the operation pumps
        StringCollection tokens;
        StringUtility::Split<wstring>(operations, tokens, L";");
        for (auto operation : tokens)
        { 
            ProcessOperation(operation);
        }

        Close(ErrorCodeValue::InvalidState, true, true);
        EndTest(testName);
    }

    void ReplicatorWrapper::ProcessSecondaryOperationsForChangeRole(
        wstring const & testName, 
        wstring const & operations,
        wstring const & expectedReply, 
        wstring const & expectedQueue,
        wstring const & expectedFinalState,
        wstring const & expectedProgressVector,
        bool completeCopyBeforeClose)
    {
        StartTest(testName);

        // If complete copy before close specified, start copy pump only;
        // Only valid for parallel streams, because we don't want to start replication pump.
        // Otherwise, delay starting the operation pumps until after BeginClose is called;
        if (completeCopyBeforeClose)
        {
            ASSERT_IFNOT(supportsParallelStreams_, "CompleteCopyBeforeComplete only valid for parallel streams");
            StartCopyOperationPump();
        }

        StringCollection tokens;
        StringUtility::Split<wstring>(operations, tokens, L";");
        for (auto operation : tokens)
        { 
            ProcessOperation(operation);
        }

        // Close the service and wait for the operations to be drained;
        // this won't happen until the operation pumps are started
        StartClose(ErrorCodeValue::Success, true);
        if (!completeCopyBeforeClose)
        {
            StartCopyOperationPump();
        }

        if (supportsParallelStreams_)
        {
            StartReplicationOperationPump();
        }
        
        VerifyCloseCompleted();
        VERIFY_IS_TRUE(event_.WaitOne(10000), L"Null operation should be received");

        CheckOutput(expectedReply, expectedQueue, expectedFinalState, expectedProgressVector);
        
        EndTest(testName);
    }

    void ReplicatorWrapper::CheckOutput(
        wstring const & expectedReply, 
        wstring const & expectedQueue,
        wstring const & expectedFinalState,
        wstring const & expectedProgressVector)
    {
        Trace.WriteInfo(SecondaryTestSource, "Expected queue: \"{0}\", received: \"{1}\"", expectedQueue, operationList_);
        VERIFY_ARE_EQUAL(expectedQueue, operationList_);

        wstring expectedAcks;
        auto it = expectedReply.begin();
        while (it != expectedReply.end())
        {
            if (*it == L'(')
            {
                if (!requireServiceAck_ || ackInOrder_)
                {
                    // No need to copy anything until ) is reached
                    while (*it != L')')
                    {
                        ++it;
                    }
                }
            }
            else if (*it != L')')
            {
                expectedAcks.push_back(*it);
            }

            ++it;
        }      

        Trace.WriteInfo(SecondaryTestSource, "Expected acks: \"{0}\", received: \"{1}\"", expectedAcks, ackList_);
        VERIFY_ARE_EQUAL(expectedAcks, ackList_);

        wstring finalState;
        GetFinalState(finalState);
        Trace.WriteInfo(SecondaryTestSource, "Expected final state: \"{0}\", actual: \"{1}\"", expectedFinalState, finalState);
        VERIFY_ARE_EQUAL(expectedFinalState, finalState);

        wstring progressVector = testStateProvider_->GetProgressVectorString();
        Trace.WriteInfo(SecondaryTestSource, "Expected progress vector: \"{0}\", received: \"{1}\"", expectedProgressVector, progressVector);
        VERIFY_ARE_EQUAL(expectedProgressVector, progressVector);
    }

    void ReplicatorWrapper::StartTest(wstring const & testName)
    {
        ComTestOperation::WriteInfo(
            SecondaryTestSource,
            "Start {0}, persisted state {1}, acks in order {2}, supports parallel streams {3}",
            testName,
            hasPersistedState_,
            ackInOrder_,
            supportsParallelStreams_);

        Trace.WriteInfo(SecondaryTestSource, "**************************************************************");
        Trace.WriteInfo(SecondaryTestSource, "\t{0}", testName);
        Trace.WriteInfo(SecondaryTestSource, "**************************************************************");
    }

    void ReplicatorWrapper::EndTest(wstring const & testName)
    {
        ComTestOperation::WriteInfo(
            SecondaryTestSource,
            "Done {0}, persisted state {1}, acks in order {2}, supports parallel streams {3}",
            testName,
            hasPersistedState_,
            ackInOrder_,
            supportsParallelStreams_);
    }

    void ReplicatorWrapper::Close(bool waitForOperationsToBeDrained, bool closeEventsBeforeClose)
    {
        StartClose(ErrorCodeValue::Success,waitForOperationsToBeDrained, closeEventsBeforeClose);
        VerifyCloseCompleted();
    }

    void ReplicatorWrapper::Close(ErrorCodeValue::Enum expectedErrorCodeValue, bool waitForOperationsToBeDrained, bool closeEventsBeforeClose)
    {
        StartClose(expectedErrorCodeValue, waitForOperationsToBeDrained, closeEventsBeforeClose);
        VerifyCloseCompleted();
    }

    void ReplicatorWrapper::StartClose(ErrorCodeValue::Enum expectedErrorCodeValue, bool waitForOperationsToBeDrained, bool closeEventsBeforeClose)
    {
        if (closeEventsBeforeClose)
        {
            closeCompleteEvent_.Close();
            updateEpochCompleteEvent_.Close();
        }
        else
        {
            closeCompleteEvent_.Reset();
        }
        this->BeginClose(
            waitForOperationsToBeDrained,
            [this, closeEventsBeforeClose, expectedErrorCodeValue](AsyncOperationSPtr const& operation) -> void
            {
                ErrorCode error = this->EndClose(operation);
                VERIFY_IS_TRUE(error.IsError(expectedErrorCodeValue), L"Replication engine closed with expected error code value.");
                if (!closeEventsBeforeClose)
                {
                    closeCompleteEvent_.Set();
                }
            }, AsyncOperationSPtr());
    }
    
    void ReplicatorWrapper::VerifyCloseCompleted()
    {
        VERIFY_IS_TRUE(closeCompleteEvent_.WaitOne(10000), L"Close should succeed");
    }
    
    void ReplicatorWrapper::WaitForSequenceNumberReceived(wstring const & data)
    {
        bool found = false;
        FABRIC_SEQUENCE_NUMBER lsn = (FABRIC_SEQUENCE_NUMBER)Int64_Parse(data);
        while (!found)
        {
            {
                AcquireExclusiveLock lock(opsLock_);
                auto itOp = std::find_if(opsReceived_.begin(), opsReceived_.end(), [lsn](FABRIC_SEQUENCE_NUMBER r) -> bool
                    {
                        return r == lsn;
                    });
                found = itOp != opsReceived_.end();
            }
            if (!found)
            {
                Sleep(100);
            }
        }
    }

    void ReplicatorWrapper::ProcessWaitForState(wstring const & data)
    {
        if (data.empty())
        {
            return;
        }

        StringCollection tokens;
        StringUtility::Split<wstring>(data, tokens, L":");
        if (tokens[0] == L"U")
        {
            testStateProvider_->WaitForConfigurationNumber(Int64_Parse(tokens[1]));
        }
        else if (tokens[0] == L"S")
        {
            WaitForSequenceNumberReceived(tokens[1]);
        }
    }

    void ReplicatorWrapper::ProcessOperation(wstring const & data)
    {
        StringCollection tokens;
        StringUtility::Split<wstring>(data, tokens, L":");
        if (tokens[0] == L"R")
        {
            // Replication operation
            // R:primaryEpoch:sequenceNumber:operationEpoch
            ASSERT_IFNOT(tokens.size() == 4 || tokens.size() == 5, "Parse Replication Operation: Test parameter is incorrect {0}", data);
            FABRIC_EPOCH primaryEpoch;
            primaryEpoch.ConfigurationNumber = Int64_Parse(tokens[1]);
            primaryEpoch.DataLossNumber = 1;            
            primaryEpoch.Reserved = NULL;

            StringCollection batchSequence;
            StringUtility::Split<wstring>(tokens[2], batchSequence, L"-");
            FABRIC_SEQUENCE_NUMBER firstSequenceNumber = Int64_Parse(batchSequence[0]);
            FABRIC_SEQUENCE_NUMBER lastSequenceNumber = firstSequenceNumber;
            if (batchSequence.size() == 2)
            {
                lastSequenceNumber = Int64_Parse(batchSequence[1]);
            }
            FABRIC_EPOCH operationEpoch;
            operationEpoch.ConfigurationNumber = Int64_Parse(tokens[3]);
            operationEpoch.DataLossNumber = 1;            
            operationEpoch.Reserved = NULL;

            std::vector<ComOperationCPtr> batchOperation;
            for (FABRIC_SEQUENCE_NUMBER lsn = firstSequenceNumber; lsn <= lastSequenceNumber; lsn++)
            {
                wstring description;
                StringWriter(description).Write("R:{0}", lsn);
                ComOperationCPtr operation;
                CreateOp(description, false /*isCopy*/, lsn, ReplicationAckCallback, operation, lastSequenceNumber, false /* isLast */, operationEpoch);
                batchOperation.push_back(std::move(operation));
            }
            if (ProcessReplicationOperation(
                PrimaryHeader,
                move(batchOperation), 
                primaryEpoch, 
                Constants::InvalidLSN))
            {
                Test_ForceDispatch();
                if (ackInOrder_ && tokens.size() == 5)
                {
                    WaitForSequenceNumberReceived(tokens[4]);
                }
                
                GetAckString();
            }
            else
            {
                ackList_.push_back(L';');
            }
        }
        else if (tokens[0] == L"SC")
        {
            // Start copy operation
            // SC:primaryEpoch:replicaId:replicationStartSeq
            ASSERT_IFNOT(tokens.size() >= 3, "Parse Start Copy: Test parameter is incorrect {0}", data);
            FABRIC_EPOCH primaryEpoch;
            primaryEpoch.ConfigurationNumber = Int64_Parse(tokens[1]);
            primaryEpoch.DataLossNumber = 1;            
            primaryEpoch.Reserved = NULL;
            FABRIC_REPLICA_ID replicaId = Int64_Parse(tokens[2]);
            FABRIC_SEQUENCE_NUMBER replicationStartSeq = Int64_Parse(tokens[3]);
            
            wstring description;
            StringWriter(description).Write("SC:{0}:{1}:{2}",primaryEpoch.ConfigurationNumber, replicaId, replicationStartSeq);
            
            bool progressDone;
            bool createCopyContext;
            ProcessStartCopy(
                PrimaryHeader,
                primaryEpoch,
                replicaId,
                replicationStartSeq,
                progressDone,
                createCopyContext);

            if (!hasPersistedState_ && createCopyContext)
            {
                VERIFY_FAIL(L"Create copy context shouldn't be done for non persisted services");
            }

            if (progressDone) 
            {
                Test_ForceDispatch();
            }
            GetAckString();
        }
        else if (tokens[0] == L"C")
        {
            // Copy operation
            // C:replicaId:configNumber:sequenceNumber:isLast
            ASSERT_IFNOT(tokens.size() >= 4, "Parse Copy Operation: Test parameter is incorrect {0}", data);
            FABRIC_REPLICA_ID replicaId = Int64_Parse(tokens[1]);
            FABRIC_EPOCH primaryEpoch;
            primaryEpoch.ConfigurationNumber = Int64_Parse(tokens[2]);
            primaryEpoch.DataLossNumber = 1;            
            primaryEpoch.Reserved = NULL;
            
            FABRIC_SEQUENCE_NUMBER sequenceNumber = Int64_Parse(tokens[3]);
            int isLastInt = (tokens.size() > 4) ? Int32_Parse(tokens[4]) : 0;
            bool isLast = isLastInt > 0;

            wstring description;
            StringWriter(description).Write("C:{0}:{1}", replicaId, sequenceNumber);
            
            ComOperationCPtr operation;
            CreateOp(description, true /*isCopy*/, sequenceNumber, CopyAckCallback, operation, sequenceNumber, isLast);

            bool progressDone;
            if (ProcessCopyOperation(
                move(operation), 
                replicaId, 
                primaryEpoch,
                isLast, 
                progressDone))
            {
                if (progressDone)
                {
                    Test_ForceDispatch();
                }
                if (ackInOrder_ && tokens.size() > 5)
                {
                    WaitForSequenceNumberReceived(tokens[5]);
                }
                GetAckString();
            }
            else 
            {
                ackList_.push_back(L';');
            }
        }
        else if (tokens[0] == L"G")
        {
            // Get progress status
            // G:configNumber:sequenceNumber
            FABRIC_EPOCH epoch;
            epoch.DataLossNumber = 1;
            epoch.ConfigurationNumber = Int64_Parse(tokens[1]);
            epoch.Reserved = NULL;
            FABRIC_SEQUENCE_NUMBER sequenceNumber = Int64_Parse(tokens[2]);
            FABRIC_SEQUENCE_NUMBER progress;
            FABRIC_SEQUENCE_NUMBER cathupCapability;

            StartReplicatorUpdateEpoch(epoch);
            if (!ackInOrder_)
            {
                AckOutOfOrderOperations();
            }
            VerifyReplicatorUpdateEpochCompleted();
            
            ErrorCode error = GetCatchUpCapability(cathupCapability);
            VERIFY_IS_TRUE(error.IsSuccess(), L"GetCatchUpCapability should succeed");
            error = GetCurrentProgress(progress);
            VERIFY_IS_TRUE(error.IsSuccess(), L"GetCurrentProgress should succeed");
            VERIFY_ARE_EQUAL(sequenceNumber, progress, L"Expected sequence number should equal last Acked sequence number");
        }
        else if (tokens[0] == L"Ack")
        {
            // Service ACKs up to the first replication operation (if it exists)
            AckOperations(true /*stopOnFirstReplOperation*/);
        }
        else if (tokens[0] == L"S")
        {
            WaitForSequenceNumberReceived(tokens[1]);
        }
        else
        {
            Assert::CodingError("Unknown operation type");
        }
    }

    void ReplicatorWrapper::StartReplicatorUpdateEpoch(
        FABRIC_EPOCH const & newEpoch)
    {
        updateEpochCompleteEvent_.Reset();
        this->BeginUpdateEpoch(
            newEpoch,
            [this](AsyncOperationSPtr const & asyncOperation)
            {
                ErrorCode error = this->EndUpdateEpoch(asyncOperation);
                VERIFY_IS_TRUE(error.IsSuccess(), L"Replication engine UpdateEpoch successfully.");
                updateEpochCompleteEvent_.Set();
            },
            AsyncOperationSPtr());
    }

    void ReplicatorWrapper::VerifyReplicatorUpdateEpochCompleted()
    {
        VERIFY_IS_TRUE(updateEpochCompleteEvent_.WaitOne(10000), L"UpdateEpoch should succeed");
    }

    void ReplicatorWrapper::CreateOp(
        wstring const & description,
        bool isCopy,
        FABRIC_SEQUENCE_NUMBER sequenceNumber,
        OperationAckCallback ackCallback,
        __out ComOperationCPtr & operation,
        FABRIC_SEQUENCE_NUMBER lastOperationInBatch,
        bool isLast,
        FABRIC_EPOCH const & opEpoch
        )
    {
        ULONG bufferCount = 0;
        FABRIC_OPERATION_DATA_BUFFER const * replicaBuffers = nullptr;

        auto op = make_com<ComTestOperation, IFabricOperationData>(description);
        op->GetData(&bufferCount, &replicaBuffers);

        vector<Common::const_buffer> buffers;
        vector<ULONG> sizes;

        for (ULONG i = 0; i < bufferCount; ++i)
        {
            buffers.push_back(const_buffer(replicaBuffers[i].Buffer, replicaBuffers[i].BufferSize));
            sizes.push_back(replicaBuffers[i].BufferSize);
        }

        FABRIC_OPERATION_METADATA metadata;
        metadata.SequenceNumber = sequenceNumber;
        metadata.Type = isLast ? FABRIC_OPERATION_TYPE_END_OF_STREAM : FABRIC_OPERATION_TYPE_NORMAL;
        metadata.Reserved = NULL;
        if (isCopy)
        {
            operation = make_com<ComFromBytesOperation, ComOperation>(
                buffers,
                sizes,
                metadata,
                ackCallback,
                lastOperationInBatch);
        }
        else
        {
            operation = make_com<ComFromBytesOperation, ComOperation>(
                buffers,
                sizes,
                metadata,
                opEpoch,
                ackCallback,
                lastOperationInBatch);
        }
    }

    void ReplicatorWrapper::GetAckString()
    {
        FABRIC_SEQUENCE_NUMBER replRLSN;
        FABRIC_SEQUENCE_NUMBER replQLSN;
        FABRIC_SEQUENCE_NUMBER copyRLSN;
        FABRIC_SEQUENCE_NUMBER copyQLSN;
        wstring primary;

        KBuffer::SPtr headersStream;
        Transport::ISendTarget::SPtr target;
        wstring receivedAckSummaryMsg;
        int errorCodeValue;
        std::vector<SecondaryReplicatorTraceInfo> pendingReplTraces;
        std::vector<SecondaryReplicatorTraceInfo> pendingCopyTraces;

        GetAck(replRLSN, replQLSN, copyRLSN, copyQLSN, primary, headersStream, target, errorCodeValue, pendingReplTraces, pendingCopyTraces);
        StringWriter w(ackList_);
        
        w.Write(replRLSN);
        if (replQLSN != replRLSN)
        {
            w.Write(".{0}", replQLSN);
        }
        

        if (copyRLSN != Reliability::ReplicationComponent::Constants::NonInitializedLSN ||
            copyQLSN != Reliability::ReplicationComponent::Constants::NonInitializedLSN)
        {
            w.Write(":{0}", copyRLSN);
            if (copyQLSN != copyRLSN)
            {
                w.Write(".{0}", copyQLSN);
            }
        }
               
        w.Write(L";");
    }

    void ReplicatorWrapper::GetFinalState(
        __out wstring & state)
    {
        FABRIC_SEQUENCE_NUMBER replRLSN;
        FABRIC_SEQUENCE_NUMBER replQLSN;
        FABRIC_SEQUENCE_NUMBER copyRLSN;
        FABRIC_SEQUENCE_NUMBER copyQLSN;
        wstring primary;

        KBuffer::SPtr headersStream;
        Transport::ISendTarget::SPtr target;
        wstring receivedAckSummaryMsg;
        int errorCodeValue;
        std::vector<SecondaryReplicatorTraceInfo> pendingReplTraces;
        std::vector<SecondaryReplicatorTraceInfo> pendingCopyTraces;

        GetAck(replRLSN, replQLSN, copyRLSN, copyQLSN, primary, headersStream, target, errorCodeValue, pendingReplTraces, pendingCopyTraces);
        StringWriter writer(state);
        writer.Write("{0},{1}", replRLSN, replQLSN);
        if (copyRLSN != Reliability::ReplicationComponent::Constants::NonInitializedLSN)
        {
            writer.Write(":{0},{1}", copyRLSN, copyQLSN);
        }
    }

    void ReplicatorWrapper::StartCopyOperationPump()
    {
        OperationStreamSPtr copyStream = GetCopyStream(std::dynamic_pointer_cast<SecondaryReplicator>(shared_from_this()));
        StartGetOperation(copyStream);
    }

    void ReplicatorWrapper::StartReplicationOperationPump()
    {
        OperationStreamSPtr replicationStream = GetReplicationStream(std::dynamic_pointer_cast<SecondaryReplicator>(shared_from_this()));
        if (getBatchOperation_)
        {
            VERIFY_FAIL_FMT("Batch is not supported");
        }
        else
        {
            StartGetOperation(replicationStream);
        }
    }

    void ReplicatorWrapper::StartGetOperation(OperationStreamSPtr const & stream)
    {
        ComTestOperation::WriteNoise(SecondaryTestSource, "StartGetOperation called for {0}", stream->Purpose);
        stream->BeginGetOperation(
            [this, stream](AsyncOperationSPtr const& asyncOp)->void 
            { 
                this->GetOperationCallback(asyncOp, stream);
            }, 
            nullptr);
    }

    void ReplicatorWrapper::GetOperationCallback(AsyncOperationSPtr const & asyncOp, OperationStreamSPtr const & stream)
    {
        IFabricOperation * operation = nullptr;
        
        ErrorCode error = stream->EndGetOperation(asyncOp, operation); 
        if (!error.IsSuccess())
        {
            VERIFY_FAIL_FMT("EndGetOperation returned error {0}", error);
        }

        if (operation != nullptr && operation->get_Metadata()->Type != FABRIC_OPERATION_TYPE_END_OF_STREAM)
        {
            FABRIC_OPERATION_METADATA const *opMetadata = operation->get_Metadata();

            FABRIC_SEQUENCE_NUMBER lsn = opMetadata->SequenceNumber;
            
            ComTestOperation::WriteInfo(SecondaryTestSource, "{0}: GetOperation returned {1}.", stream->Purpose, lsn);

            ULONG bufferCount = 0;
            FABRIC_OPERATION_DATA_BUFFER const * replicaBuffers = nullptr;

            VERIFY_SUCCEEDED(operation->GetData(&bufferCount, &replicaBuffers));
            VERIFY_IS_TRUE(bufferCount <= 1);

            BYTE const * value = (bufferCount > 0 ? replicaBuffers[0].Buffer : nullptr);

            if (lsn <= 0)
            {
                VERIFY_FAIL(L"EndGetOperation returned non-null op with invalid lsn");
            }
        
            wstring data(reinterpret_cast<wchar_t *>(const_cast<BYTE*>(value)), replicaBuffers[0].BufferSize/sizeof(wchar_t));

            if (ackInOrder_)
            {
                ComTestOperation::WriteInfo(SecondaryTestSource, "Ack operation {0}.", lsn);
                operation->Acknowledge();
                operation->Release();
            }
            else
            {
                ComTestOperation::WriteInfo(SecondaryTestSource, "Queue operation {0}.", lsn);
                AcquireExclusiveLock lock(opsLock_);
                ops_.push_back(operation);
            }

            StringWriter writer(operationList_);
            writer.Write("{0};", data);
            opsReceived_.push_back(lsn);
        
            // Continue getting operations
            StartGetOperation(stream);
        }
        else 
        {
            if (operation)
            {
                ComTestOperation::WriteInfo(SecondaryTestSource, "{0}: Received sentinel operation.", stream->Purpose);

                ULONG bufferCount = 0;
                FABRIC_OPERATION_DATA_BUFFER const * replicaBuffers = nullptr;

                VERIFY_SUCCEEDED(operation->GetData(&bufferCount, &replicaBuffers));
                VERIFY_IS_TRUE(bufferCount <= 1);
                
                if (replicaBuffers != nullptr)
                {
                    BYTE const * value = (bufferCount > 0 ? replicaBuffers[0].Buffer : nullptr);
                    wstring data(reinterpret_cast<wchar_t *>(const_cast<BYTE*>(value)), replicaBuffers[0].BufferSize/sizeof(wchar_t));
                    FABRIC_SEQUENCE_NUMBER lsn = operation->get_Metadata()->SequenceNumber;

                    if (ackInOrder_)
                    {
                        ComTestOperation::WriteInfo(SecondaryTestSource, "Ack operation {0}.", lsn);
                        operation->Acknowledge();
                        operation->Release();
                    }
                    else
                    {
                        ComTestOperation::WriteInfo(SecondaryTestSource, "Queue operation {0}.", lsn);
                        AcquireExclusiveLock lock(opsLock_);
                        ops_.push_back(operation);
                    }

                    StringWriter writer(operationList_);
                    writer.Write("{0};", data);
                    opsReceived_.push_back(lsn);
                }
                else
                {
                    operation->Acknowledge();
                    operation->Release();
                }
            }

            ComTestOperation::WriteInfo(SecondaryTestSource, "{0}: Received null operation.", stream->Purpose);
            if (stream->Purpose == Constants::CopyOperationTrace)
            {
                if (!supportsParallelStreams_)
                {
                    StartReplicationOperationPump();
                }
                // Stop the copy stream
                return;
            }

            Trace.WriteInfo(SecondaryTestSource, "Received null operation.");
            AckOutOfOrderOperations();
            event_.Set();
        }
    }

    // Get one operation only from the stream, do not ACK it
    void ReplicatorWrapper::GetSingleOperation(OperationStreamSPtr const & stream)
    {
        ComTestOperation::WriteInfo(SecondaryTestSource, "StartGetOperation called for {0}", stream->Purpose);
        stream->BeginGetOperation(
            [this, stream](AsyncOperationSPtr const& asyncOp)->void 
            { 
                this->GetSingleOperationCallback(asyncOp, stream);
            }, 
            nullptr);
    }

    void ReplicatorWrapper::GetSingleOperationCallback(
        AsyncOperationSPtr const & asyncOp, 
        OperationStreamSPtr const & stream)
    {
        IFabricOperation * operation = nullptr;
        
        ErrorCode error = stream->EndGetOperation(asyncOp, operation); 
        if (!error.IsSuccess())
        {
            VERIFY_FAIL_FMT("EndGetOperation returned error {0}", error);
        }

        if (operation != nullptr)
        {
            FABRIC_OPERATION_METADATA const *opMetadata = operation->get_Metadata();
            
            FABRIC_SEQUENCE_NUMBER lsn = opMetadata->SequenceNumber;
            ComTestOperation::WriteInfo(SecondaryTestSource, "{0}: GetOperation returned {1}.", stream->Purpose, lsn);

            ULONG bufferCount = 0;
            FABRIC_OPERATION_DATA_BUFFER const * replicaBuffers = nullptr;

            VERIFY_SUCCEEDED(operation->GetData(&bufferCount, &replicaBuffers));
            VERIFY_IS_TRUE(bufferCount <= 1);
            
            if (replicaBuffers != nullptr)
            {
                BYTE const * value = (bufferCount > 0 ? replicaBuffers[0].Buffer : nullptr);

                wstring data(reinterpret_cast<wchar_t *>(const_cast<BYTE*>(value)), replicaBuffers[0].BufferSize / sizeof(wchar_t));
                StringWriter writer(operationList_);
                writer.Write("{0};", data);
                opsReceived_.push_back(lsn);

                ComTestOperation::WriteInfo(SecondaryTestSource, "Queue operation {0}.", lsn);
                AcquireExclusiveLock lock(opsLock_);
                ops_.push_back(operation);
            }
            
            if (lsn <= 0 && operation->get_Metadata()->Type != FABRIC_OPERATION_TYPE_END_OF_STREAM)
            {
                VERIFY_FAIL(L"EndGetOperation returned non-null op with invalid lsn");
            }

            if (operation->get_Metadata()->Type == FABRIC_OPERATION_TYPE_END_OF_STREAM)
            {
                ComTestOperation::WriteInfo(SecondaryTestSource, "{0}: Received Sentinel operation.", stream->Purpose);
                event_.Set();
            }
        }
        else 
        {
            if (stream->Purpose == Constants::CopyOperationTrace)
            {
                ComTestOperation::WriteInfo(SecondaryTestSource, "{0}: Received NULL operation.", stream->Purpose);
            }
            Trace.WriteInfo(SecondaryTestSource, "Received null operation.");
            event_.Set();
        }
    }

    void ReplicatorWrapper::AckOperations(bool stopOnFirstReplOperation)
    {
        AckOperationsCallerHoldsLock(stopOnFirstReplOperation);
    }

    void ReplicatorWrapper::AckOperationsCallerHoldsLock(bool stopOnFirstReplOperation)
    {
        list<std::pair<IFabricOperation*, FABRIC_SEQUENCE_NUMBER>> opsToAck;
        {
            AcquireExclusiveLock lock(opsLock_);
            auto it = ops_.begin();
            for (; it != ops_.end(); ++it)
            {
                FABRIC_OPERATION_METADATA const *opMetadata = (*it)->get_Metadata();
                opsToAck.push_back(pair<IFabricOperation*, FABRIC_SEQUENCE_NUMBER>(*it, opMetadata->SequenceNumber));

                // The test generates copy operations with LSN < 10 and replication ops with LSN > 10
                if (stopOnFirstReplOperation && opMetadata->SequenceNumber >= 10)
                {
                    ++it;
                    break;
                }
            }

            ops_.erase(ops_.begin(), it);
        }
                
        for (auto it = opsToAck.begin(); it != opsToAck.end(); ++it)
        {
            ComTestOperation::WriteInfo(SecondaryTestSource, "Ack {0}.", it->second);
            it->first->Acknowledge();
            it->first->Release();
        }
    }

    void ReplicatorWrapper::AckOutOfOrderOperations()
    {
        if (!ackInOrder_)
        {
            {
                AcquireExclusiveLock lock(opsLock_);
                if (ops_.size() == 2)
                {
                    // Exchange the operations
                    swap(ops_[0], ops_[1]);
                }
                else
                {
                    random_shuffle(ops_.begin(), ops_.end());
                }
            }

            AckOperations(false /*stopOnFirstReplOperation*/);
        }
    }
    
    /***********************************
    * TestSecondary methods
    **********************************/
        






    BOOST_FIXTURE_TEST_SUITE(TestSecondarySuite,TestSecondary)

    BOOST_AUTO_TEST_CASE(InvalidChangeRoleIdleDueToPendingCopy)
    {
        // Non persisted service: not all copy operations received
        shared_ptr<ReplicatorWrapper> npWrapper = CreateAndOpenWrapper(0, false);
        npWrapper->ProcessSecondaryOperationsWithInvalidChangeRole(
            L"Invalid change role with not all copy received",
            L"SC:100:1:10;C:1:100:1;R:100:10:100");

        // Persisted services: all copy operations must have been received and ACKed
        shared_ptr<ReplicatorWrapper> pWrapper = CreateAndOpenWrapper(1, false);
        pWrapper->ProcessSecondaryOperationsWithInvalidChangeRole(
            L"Invalid change role with pending copy ACKs",
            L"SC:100:1:10;C:1:100:1:1;R:100:10:100");
    }




    BOOST_AUTO_TEST_CASE(UpdateEpochCopyGap)
    {
        // Only relevant for has persisted state 1, in order ACKs 0, supports parallel streams 0, requireServiceAck 0
        int acks = 2;
        shared_ptr<ReplicatorWrapper> wrapper = CreateAndOpenWrapper(acks);
     
        wrapper->ProcessSecondaryOperations(
                L"UpdateEpoch when COPY operations have a gap",
                L"SC:100:1:10;C:1:100:1:0;C:1:100:4:0;C:1:100:3:0;Ack",
                L"9:0;9:1.0;;;",
                L"C:1:1;",
                L"9,9:1,1",
                L"0.0:-1;");
    }

    BOOST_AUTO_TEST_CASE(UpdateEpochAcksPending)
    {
        int acks = 2;
        shared_ptr<ReplicatorWrapper> wrapper = CreateAndOpenWrapper(acks);
        
        // Send repl 10, 11 and 13
        // then call UpdateEpoch - 13 should be discarded.
        // Then enqueue 12 - UpdateEpoch is called; 
        // Close aborts the queues, but the final dispatch gives the operation to the service 
        wrapper->ProcessSecondaryOperations(
            L"Update epoch with ACKs pending",
            L"SC:100:1:10;C:1:100:1:1;R:100:10:100;R:100:11:100;S:11;R:100:13:100;Ack;G:105:11;R:105:12:105", 
            L"9:0;9(:-1.0);10.9(:-1.0);11.9(:-1.0);;12.11;",
            L"C:1:1;R:10;R:11;R:12;",
            L"12,12",
            L"0.0:11;1.105:-1;",
            L"S:12",
            L"U:105");
    }

    BOOST_AUTO_TEST_SUITE_END()

    TestSecondary::TestSecondary()
        :   config_(CreateGenericConfig()),
            perfCounters_(REPerformanceCounters::CreateInstance(ReplicatorWrapper::PartitionId, ReplicatorWrapper::ReplicaId)),
            transport_(CreateTransport(config_)),
            volatileStateProvider_(ComTestStateProvider::GetProvider(1, -1, GetRoot())),
            persistedStateProvider_(ComTestStateProvider::GetProvider(1, 1, GetRoot())),
            epoch_()
    {
        epoch_.DataLossNumber = 1;
        epoch_.ConfigurationNumber = 100;
        BOOST_REQUIRE(Setup());
    }

    void TestSecondary::DuplicateInOrderCopy(int acks)
    {
        shared_ptr<ReplicatorWrapper> wrapper = CreateAndOpenWrapper(acks);
        
        // Use the following formats:
        // Copy operation:
        // IN: C:replicaId:sequenceNumber:isLast
        // OUT: C:replicaId:sequenceNumber

        // Replication operation:
        // R:primaryEpoch.ConfigurationNumber:sequenceNumber:operationEpoch.ConfigurationNumber

        // Get current progress
        // G:epoch:sequenceNumber

        // Expected progress vector entry
        // epoch.ConfigurationNumber:lastSequenceNumber

        wrapper->ProcessSecondaryOperations(
            L"Duplicate in-order copy operations",
            L"SC:100:1:10;C:1:100:1;C:1:100:2;C:1:100:2", 
            L"9:0;9:1(.0);9:2(.0);9:2(.0);", 
            L"C:1:1;C:1:2;", 
            L"9,9:2,2",
            L"0.0:-1;");
    }

    void TestSecondary::FirstCopy(int acks)
    {
        shared_ptr<ReplicatorWrapper> wrapper = CreateAndOpenWrapper(acks);
        
        wrapper->ProcessSecondaryOperations(
            L"First copy operation", 
            L"C:1:100:1;SC:100:1:10;C:1:100:1", 
            L";9:0;9:1(.0);",
            L"C:1:1;", 
            L"9,9:1,1",
            L"0.0:-1;");
    }

    void TestSecondary::OutOfOrderCopy(int acks)
    {
        shared_ptr<ReplicatorWrapper> wrapper = CreateAndOpenWrapper(acks);
            
        wrapper->ProcessSecondaryOperations(
            L"Out-of-order copy", 
            L"SC:100:1:10;C:1:100:2;C:1:100:1;C:1:100:4", 
            L"9:0;;9:2(.0);;", 
            L"C:1:1;C:1:2;", 
            L"9,9:2,2",
            L"0.0:-1;");
    }

    void TestSecondary::CloseIdlePendingOps(int acks, bool supportsParallelStreams)
    {
         
        shared_ptr<ReplicatorWrapper> wrapper = CreateAndOpenWrapper(acks, supportsParallelStreams);
        
        // Service takes only one operation (in each stream if parallel streams are supported)
        // and doesn't Ack it before close is called.

        // Non-persisted: Complete all in order operations, but service only takes C1 
        // (and R10 for parallel streams);
        // When close, all in order copy operations, i.e. C1 and C2 are waited to be drained; and discard all repl ops.
        // Other operations are never seen by the service.

        // Persisted: Commit all in order operations, complete none, service takes C1 
        // (and R10 for parallel streams)
        // When close, all in order copy operations, i.e. C1 and C2 are waited to be drained and ACK'ed; all not seen repl ops are discarded.
        // Other operations are never seen by the service.
        // The operations that are taken are ACKed after close,
        // and no callback is executed.        
            
        wstring expectedAcks = wrapper->RequireServiceAck ?
            L"9:0;10.9:0;;;10.9:2.0;11.9:2.0;11.9:2.0;" :
            L"9:0;10:0;;;10:2;11:2;;";
            
        wstring expectedQueue = supportsParallelStreams ? L"R:10;C:1:1;" : L"C:1:1;";
        wstring expectedFinalState = wrapper->RequireServiceAck ? L"9,9:0,0" : L"11,11:2,2"; /*TODO - 935703 Change it back to 9,9:2,2*/
        wstring expectedProgressVector = supportsParallelStreams ? L"0.0:-1;" : L"0.0:-1;";
        
        wrapper->ProcessSecondaryOperationsForClose(
            L"Close idle with pending copy and replication", 
            supportsParallelStreams ?
                L"SC:100:1:10;R:100:10:100:10;C:1:100:4;C:1:100:2;C:1:100:1;R:100:11:100;R:100:14:100" :
                L"SC:100:1:10;R:100:10:100;C:1:100:4;C:1:100:2;C:1:100:1;R:100:11:100;R:100:14:100",
            expectedAcks,
            expectedQueue,
            expectedFinalState,
            expectedProgressVector);
    }

    void TestSecondary::ChangeRoleNoCopyOperations(int acks)
    {
        // Take all copy operations and process them;
        // but delay start the replication pump
        shared_ptr<ReplicatorWrapper> wrapper = CreateAndOpenWrapper(acks, true /*supportsParallelStreams*/);
        wstring expectedAcks = wrapper->RequireServiceAck ? 
            L"9:0;9;10.9;;11.9;13.9;" :
            L"9:0;9;10;;11;13;";
        wrapper->ProcessSecondaryOperationsForChangeRole(
            L"Change role when only replication is pending",
            L"SC:100:1:10;C:1:100:1:1;R:100:10:100;Ack;R:100:13:100;R:100:11:100;R:100:12:100", 
            expectedAcks,
            L"C:1:1;R:10;R:11;R:12;R:13;",
            L"13,13",
            L"0.0:-1;",
            true /*completeCopyBeforeClose*/);
    }

    void TestSecondary::ChangeRoleIdleAllCopyReceivedNotTakenByService(bool supportsParallelStreams)
    {
        // Change role  clled when all copy operations received, but not yet taken by the service.
        // Non-persisted: 
        // - Close should wait for all copy and replication to be taken
        
        shared_ptr<ReplicatorWrapper> wrapper = CreateAndOpenWrapper(0 /*non-persisted*/, supportsParallelStreams);
        
        wrapper->ProcessSecondaryOperationsForChangeRole(
            L"Change idle role when all copy received but not taken by the service",
            L"SC:100:1:10;C:1:100:1;C:1:100:2:1;R:100:10:100;R:100:13:100;R:100:11:100", 
            L"9:0;9:1;9;10;;11;",
            L"C:1:1;C:1:2;R:10;R:11;",
            L"11,11",
            L"0.0:-1;",
            false /*completeCopyBeforeClose*/);
    }

    void TestSecondary::DuplicateOutOfOrderCopy(int acks)
    {
        shared_ptr<ReplicatorWrapper> wrapper = CreateAndOpenWrapper(acks);
        
        wrapper->ProcessSecondaryOperations(
            L"Duplicate out-of-order copy operations", 
            L"C:1:100:2;SC:100:1:10;C:1:100:2;C:1:100:1", 
            L";9:0;;9:2(.0);",
            L"C:1:1;C:1:2;", 
            L"9,9:2,2",
            L"0.0:-1;"); 
    }

    void TestSecondary::CopyCompleted(int acks)
    {
        shared_ptr<ReplicatorWrapper> wrapper = CreateAndOpenWrapper(acks);
        
        wrapper->ProcessSecondaryOperations(
            L"Copy completed", 
            L"SC:100:1:10;C:1:100:1;C:1:100:2:1", 
            L"9:0;9:1(.0);9(:-1.0);",
            L"C:1:1;C:1:2;", 
            L"9,9",  
            L"0.0:-1;"); 
    }

    void TestSecondary::CopyCompletedWithOutOfOrderReplication(int acks)
    {
        shared_ptr<ReplicatorWrapper> wrapper = CreateAndOpenWrapper(acks);
        
        wrapper->ProcessSecondaryOperations(
            L"Copy completed with out-of-order replication", 
            L"C:1:100:1;R:100:11:100;SC:100:1:10;C:1:100:2:1;C:1:100:1", 
            L";;9:0;;9(:-1.0);",
            L"C:1:1;C:1:2;", 
            L"9,9",
            L"0.0:-1;"); 
    }

    void TestSecondary::OutOfOrderCopyExceedingQueueCapacity(int acks)
    {
        shared_ptr<ReplicatorWrapper> wrapper = CreateAndOpenWrapper(acks);
        
        wrapper->ProcessSecondaryOperations(
            L"Out-of-order Copy exceeding queue capacity",
            L"SC:100:1:10;C:1:100:4;C:1:100:2;C:1:100:3;C:1:100:5:1;C:1:100:1", 
            L"9:0;;;;9:0;9:4(.0);",
            L"C:1:1;C:1:2;C:1:3;C:1:4;", 
            L"9,9:4,4",
            L"0.0:-1;"); 
    }

    void TestSecondary::CopyCompletedAndInOrderReplication(int acks)
    {
        shared_ptr<ReplicatorWrapper> wrapper = CreateAndOpenWrapper(acks);
        
        wstring expectedAcks = wrapper->RequireServiceAck ? L"9:0;;10.9:0;10.9:0;10(.9:-1.0);" : L"9:0;;10:0;10:0;10;";
        wrapper->ProcessSecondaryOperations(
            L"Copy completed followed by in-order replication",
            L"SC:100:1:10;C:1:100:2:1;R:100:10:100;C:1:100:2:1;C:1:100:1:0:10",
            expectedAcks,
            L"C:1:1;C:1:2;R:10;", 
            L"10,10",
            L"0.0:-1;",
            L"S:10",
            L"S:10");
    }

    void TestSecondary::ParallelCopyAndReplication(int acks)
    {
        shared_ptr<ReplicatorWrapper> wrapper = CreateAndOpenWrapper(acks, true /*supportsParallelStreams*/);
        
        wstring expectedAcks = wrapper->HasPersistedState ? 
            L"9:0;9:1;10:1;;;10:3;12:3;12;13;" : 
            L"9:0;9:1;10:1;;;10:3;12:3;12;13;";
        wrapper->ProcessSecondaryOperations(
            L"Copy and replication in parallel streams",
            L"SC:100:1:10;C:1:100:1;R:100:10:100:10;C:1:100:3;R:100:12:100;C:1:100:2;R:100:11:100:12;C:1:100:4:1;R:100:13:100:13",
            expectedAcks,
            L"C:1:1;R:10;C:1:2;C:1:3;R:11;R:12;C:1:4;R:13;",
            L"13,13",
            L"0.0:-1;");
    }

    void TestSecondary::StaleReplication(int acks)
    {
        shared_ptr<ReplicatorWrapper> wrapper = CreateAndOpenWrapper(acks);
        
        wrapper->ProcessSecondaryOperations(
            L"Stale replication", 
            L"SC:100:1:10;C:1:100:1;R:99:10:99", 
            L"9:0;9:1(.0);;",
            L"C:1:1;",
            L"9,9:1,1",
            L"0.0:-1;");
    }

    void TestSecondary::OutOfOrderReplication(int acks,    bool supportsParallelStreams)
    {
        
        shared_ptr<ReplicatorWrapper> wrapper = CreateAndOpenWrapper(acks, supportsParallelStreams);
        
        wrapper->ProcessSecondaryOperations(
            L"Out-of-order replication",
            L"SC:100:1:10;C:1:100:1:1;R:100:11:100;R:100:10:100:11", 
            L"9:0;9(:-1.0);;11(.9:-1.0);",
            L"C:1:1;R:10;R:11;", 
            L"11,11",
            L"0.0:-1;",
            L"S:11");
    }

    void TestSecondary::OutOfOrderReplicationNoReply(int acks)
    {
        shared_ptr<ReplicatorWrapper> wrapper = CreateAndOpenWrapper(acks);
            
        wrapper->ProcessSecondaryOperations(
            L"Out-of-order replication with no reply",
            L"SC:100:1:10;C:1:100:1:1;C:1:100:1:1;R:100:10:100:10;R:100:12:100", 
            L"9:0;9(:-1.0);9(:-1.0);10(.9:-1.0);;",
            L"C:1:1;R:10;", 
            L"10,10",
            L"0.0:-1;",
            L"S:10");
    }

    void TestSecondary::OutOfOrderReplicationExceedingQueueCapacity(int acks, bool supportsParallelStreams)
    {
        
        shared_ptr<ReplicatorWrapper> wrapper = CreateAndOpenWrapper(acks, supportsParallelStreams);
                
        wrapper->ProcessSecondaryOperations(
            L"Out-of-order replication exceeding queue capacity",
            L"SC:100:1:10;C:1:100:1:1;R:100:12:100;R:100:12:100;R:100:13:100;R:100:11:100;R:100:14:100;R:100:10:100:13", 
            L"9:0;9(:-1.0);;9(:-1.0);;;9(:-1.0);13(.9:-1.0);",
            L"C:1:1;R:10;R:11;R:12;R:13;",
            L"13,13",
            L"0.0:-1;",
            L"S:13");
    }

    void TestSecondary::DuplicateReplication(int acks)
    {
        shared_ptr<ReplicatorWrapper> wrapper = CreateAndOpenWrapper(acks);
        
        wrapper->ProcessSecondaryOperations(
            L"Duplicate replication",
            L"SC:100:1:10;C:1:100:1:1;R:100:11:100;R:100:12:100;R:100:11:100;R:100:10:100:12", 
            L"9:0;9(:-1.0);;;9(:-1.0);12(.9:-1.0);",
            L"C:1:1;R:10;R:11;R:12;",
            L"12,12",
            L"0.0:-1;",
            L"S:12");
    }

    void TestSecondary::NewConfiguration(int acks, bool supportsParallelStreams)
    {
        
        shared_ptr<ReplicatorWrapper> wrapper = CreateAndOpenWrapper(acks, supportsParallelStreams);
                
        wrapper->ProcessSecondaryOperations(
            L"New configuration version",
            L"SC:100:1:10;C:1:100:1:1;R:100:10:100:10;R:100:12:100;R:101:11:100;S:12", 
            L"9:0;9(:-1.0);10(.9:-1.0);;12(.9:-1.0);",
            L"C:1:1;R:10;R:11;R:12;",
            L"12,12",
            L"0.0:-1;",
            L"S:12");
    }

    void TestSecondary::GetProgress(int acks)
    {
        shared_ptr<ReplicatorWrapper> wrapper = CreateAndOpenWrapper(acks);
        
        wrapper->ProcessSecondaryOperations(
            L"Get Progress status",
            L"SC:100:1:10;C:1:100:1:1;R:100:10:100:10;R:100:12:100;G:102:10;R:100:11:100;R:102:11:101:11;R:100:12:100", 
            L"9:0;9(:1);10(:1);;;11(:1);;",
            L"C:1:1;R:10;R:11;",
            L"11,11",
            L"0.0:10;1.101:-1;",
            L"S:11");
    }

    void TestSecondary::GetProgressWithMultipleConfigurationChanges(int acks)
    {
         shared_ptr<ReplicatorWrapper> wrapper = CreateAndOpenWrapper(acks);
        
        wrapper->ProcessSecondaryOperations(
            L"Get Progress status with multiple configuration changes",
            L"SC:100:1:10;C:1:100:1:1;R:100:10:100:10;R:101:11:100;G:105:11;R:105:13:105;R:105:12:100:13", 
            L"9:0;9(:1);10(:1);11(:1);;13(:1);",
            L"C:1:1;R:10;R:11;R:12;R:13;",
            L"13,13",
            L"0.0:12;1.105:-1;",
            L"S:13");
    }

    void TestSecondary::MultipleOperationEpochs(int acks)
    {
      
        // Increase the queue size
        int previousMaxQueueSize = config_->MaxReplicationQueueSize;
        int previousMaxSecondaryQueueSize = config_->MaxSecondaryReplicationQueueSize;
        config_->MaxReplicationQueueSize = 8;
        config_->MaxSecondaryReplicationQueueSize = 8;
        config_->SecondaryClearAcknowledgedOperations = false;

        shared_ptr<ReplicatorWrapper> wrapper = CreateAndOpenWrapper(acks);
        if (acks != 2 && acks != 4)
        {
            wrapper->ProcessSecondaryOperations(
                L"Multiple operations generated in multiple epochs",
                L"SC:100:1:10;C:1:100:1:1;R:100:10:100:10;R:100:11:100:11;R:101:12:101:12;R:102:13:102:13;R:102:14:102:14;R:103:15:103:15", 
                L"9:0;9(:1);10(:1);11(:1);12(:1);13(:1);14(:1);15(:1);",
                L"C:1:1;R:10;R:11;R:12;R:13;R:14;R:15;",
                L"15,15",
                L"0.0:11;1.101:12;1.102:14;1.103:-1;");
        }
        else
        {
            // When ACKs are out of order, close will remove all operations that are pending 
            // and not yet requested by user
            wrapper->ProcessSecondaryOperations(
                L"Multiple operations generated in multiple epochs",
                L"SC:100:1:10;C:1:100:1:1;R:100:10:100;R:100:11:100;S:11;R:101:12:101;R:102:13:102;R:102:14:102;R:103:15:103", 
                L"9:0;9:-1.0;10.9:-1.0;11.9:-1.0;12.9:-1.0;13.9:-1.0;14.9:-1.0;15.9:-1.0;",
                L"C:1:1;R:10;R:11;R:12;R:13;R:14;R:15;",
                L"15,15",
                L"0.0:11;1.101:12;1.102:14;1.103:-1;",
                L"S:12;S:14;S:15",
                L"U:103");
        }

        config_->MaxReplicationQueueSize = previousMaxQueueSize;
        config_->MaxSecondaryReplicationQueueSize = previousMaxSecondaryQueueSize;
    }

    void TestSecondary::VerifyStateProviderEstablishedEpochTest()
    {
        shared_ptr<ReplicatorWrapper> wrapper = CreateAndOpenWrapper(0, 0);
        
        wrapper->ProcessSecondaryOperations(
            L"VerifyStateProviderEstablishedEpoch during out of order replication",
            L"SC:100:1:10;C:1:100:1:1;R:100:11:101;R:100:10:100:11", // REPL 11 with epoch 101 reaches before 10 with epcoh 100
            L"9:0;9(:-1.0);;11(.9:-1.0);",
            L"C:1:1;R:10;R:11;",
            L"11,11",
            L"0.0:10;1.101:-1;", // this is the expected progress vector
            L"S:11");
    }

    bool TestSecondary::Setup()
    {
        return TRUE;
    }

    REConfigSPtr TestSecondary::CreateGenericConfig()
    {
        REConfigSPtr config = std::make_shared<REConfig>();
        config->BatchAcknowledgementInterval = TimeSpan::FromMilliseconds(300);
        config->InitialSecondaryReplicationQueueSize = 4;
        config->MaxSecondaryReplicationQueueSize = 4;
        config->InitialReplicationQueueSize = 4;
        config->MaxReplicationQueueSize = 4;
        config->InitialCopyQueueSize = 4;
        config->MaxCopyQueueSize = 4;
        config->ReplicatorListenAddress = L"127.0.0.1:0";
        config->ReplicatorPublishAddress = L"127.0.0.1:0";
        config->SecondaryClearAcknowledgedOperations = (Random().NextDouble() > 0.5)? true : false;
        config->QueueHealthMonitoringInterval = TimeSpan::Zero;
        config->SlowApiMonitoringInterval = TimeSpan::Zero;
        return config;
    }

    ReplicationTransportSPtr TestSecondary::CreateTransport(REConfigSPtr const & config)
    {
        auto transport = ReplicationTransport::Create(config->ReplicatorListenAddress);

        auto errorCode = transport->Start(L"localhost:0");
        ASSERT_IFNOT(errorCode.IsSuccess(), "Failed to start: {0}", errorCode);

        return transport;
    }

    shared_ptr<ReplicatorWrapper> TestSecondary::CreateAndOpenWrapper(
        int option,
        bool supportsParallelStreams,
        bool getBatchOperation)
    {
        bool hasPersistedState;
        bool requireServiceAck;
        bool ackInOrder = true;
        switch (option)
        {
        case 0:
            hasPersistedState = false;
            requireServiceAck = false;
            ackInOrder = true;
            break;
        case 1: 
            hasPersistedState = true;
            requireServiceAck = false;
            ackInOrder = true;
            break;
        case 2:
            hasPersistedState = true;
            requireServiceAck = false;
            ackInOrder = false;
            break;
        case 3:
            hasPersistedState = false;
            requireServiceAck = true;
            ackInOrder = true;
            break;
        case 4:
            hasPersistedState = false;
            requireServiceAck = true;
            ackInOrder = false;
            break;
        default:
            Assert::CodingError("option not supported");
        }

        Trace.WriteInfo(SecondaryTestSource,
            "Test param: has persisted state {0}, in order ACKs {1}, supports parallel streams {2}, requireServiceAck {3}", 
            hasPersistedState, 
            ackInOrder,
            supportsParallelStreams,
            requireServiceAck);
        shared_ptr<ReplicatorWrapper> wrapper;
        config_->RequireServiceAck = requireServiceAck;
        IReplicatorHealthClientSPtr temp;
        ApiMonitoringWrapperSPtr temp2 = ApiMonitoringWrapper::Create(temp, REInternalSettings::Create(nullptr,config_), ReplicatorWrapper::PartitionId, ReplicationEndpointId(Guid::NewGuid(), 1));
        if (hasPersistedState)
        {
            wrapper =  std::shared_ptr<ReplicatorWrapper>(new ReplicatorWrapper(
                config_, perfCounters_, transport_, persistedStateProvider_, Guid::NewGuid(), hasPersistedState, supportsParallelStreams, temp, temp2, ackInOrder, getBatchOperation));
        }
        else
        {
            wrapper = std::shared_ptr<ReplicatorWrapper>(new ReplicatorWrapper(
                config_, perfCounters_, transport_, volatileStateProvider_, Guid::NewGuid(), hasPersistedState, supportsParallelStreams, temp, temp2, ackInOrder, getBatchOperation));
        }
        wrapper->Open();
        return wrapper;           
    }

    Common::ComponentRoot const & TestSecondary::GetRoot()
    {
        static std::shared_ptr<DummyRoot> root = std::make_shared<DummyRoot>();
        return *root;
    }
}
