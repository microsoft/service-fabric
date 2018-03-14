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
    static Common::StringLiteral const ReplCopyTestSource("ReplCopyTest");

    class TestReplicateCopyOperations
    {
    protected:
        TestReplicateCopyOperations() { BOOST_REQUIRE(Setup()); }
    public:
        TEST_CLASS_SETUP(Setup);
    
        static void TestReplicateWithRetry(bool hasPersistedState);
        static void TestCopyWithRetry(bool hasPersistedState);
        static void TestReplicateAndCopy(bool hasPersistedState);
                
        static REConfigSPtr CreateGenericConfig();
        static ReplicationTransportSPtr CreateTransport(REConfigSPtr const & config);

        class DummyRoot : public Common::ComponentRoot
        {
        };
        static Common::ComponentRoot const & GetRoot();
    };

    namespace {
    typedef std::function<void(std::wstring const &)> MessageReceivedCallback;

    static uint MaxQueueCapacity = 8;
    class ReplicatorTestWrapper;

    class PrimaryReplicatorHelper : public Common::ComponentRoot
    {
    public:
         PrimaryReplicatorHelper(
            REConfigSPtr const & config,
            bool hasPersistedState,
            FABRIC_EPOCH const & epoch, 
            int64 numberOfCopyOperations,
            int64 numberOfCopyContextOperations,
            Common::Guid partitionId);

         virtual ~PrimaryReplicatorHelper() {}

        __declspec(property(get=get_PrimaryId)) FABRIC_REPLICA_ID PrimaryId;
        FABRIC_REPLICA_ID get_PrimaryId() const { return primaryId_; }

        __declspec(property(get=get_EndpointUniqueId)) ReplicationEndpointId const & EndpointUniqueId;
        ReplicationEndpointId const & get_EndpointUniqueId() const { return endpointUniqueId_; }

        ReplicationEndpointId const & get_ReplicationEndpointId() const { return endpointUniqueId_; }

        virtual void ProcessMessage(__in Transport::Message & message, __out Transport::ReceiverContextUPtr &);

        class PrimaryReplicatorWrapper : public PrimaryReplicator
        {
        public:
            PrimaryReplicatorWrapper(
                REConfigSPtr const & config,
                REPerformanceCountersSPtr & perfCounters,
                FABRIC_REPLICA_ID replicaId,
                bool hasPersistedState,
                ReplicationEndpointId const & endpointUniqueId,
                FABRIC_EPOCH const & epoch,
                ReplicationTransportSPtr const & transport,
                ComProxyStateProvider const & provider,
                IReplicatorHealthClientSPtr & healthClient,
                ApiMonitoringWrapperSPtr & monitor);

            virtual ~PrimaryReplicatorWrapper() {}
            virtual void ReplicationAckMessageHandler(
                __in Transport::Message & message, ReplicationFromHeader const & fromHeader, PrimaryReplicatorWPtr primaryWPtr);
            virtual void CopyContextMessageHandler(
                __in Transport::Message & message, ReplicationFromHeader const & fromHeader);

            void AddExpectedAck(int64 secondaryId, FABRIC_SEQUENCE_NUMBER replLSN, FABRIC_SEQUENCE_NUMBER copyLSN)
            {
                Common::AcquireExclusiveLock lock(this->lock_);

                AddAckCallerHoldsLock(expectedAcks_, secondaryId, replLSN, copyLSN);
            }

            void CheckAcks() const;

            void AddExpectedCopyContextMessage(ReplicationEndpointId const & fromDemuxer, FABRIC_SEQUENCE_NUMBER sequuenceNumber);

            void CheckMessages();

        private:
            typedef pair<FABRIC_SEQUENCE_NUMBER, FABRIC_SEQUENCE_NUMBER> AckReplCopyPair;
            typedef map<int64, AckReplCopyPair> AckReplCopyMap;

            static void AddAckCallerHoldsLock(
                __in AckReplCopyMap & ackMap, int64 secondaryId, FABRIC_SEQUENCE_NUMBER replLSN, FABRIC_SEQUENCE_NUMBER copyLSN);

            static void DumpAckTable(AckReplCopyMap const & mapToDump);

            vector<wstring> expectedMessages_;
            vector<wstring> receivedMessages_;
            AckReplCopyMap receivedAcks_;
            AckReplCopyMap expectedAcks_;

            mutable Common::ExclusiveLock lock_;

            FABRIC_REPLICA_ID id_;

        };

        __declspec(property(get=get_Replicator)) PrimaryReplicatorWrapper & Replicator;
        PrimaryReplicatorWrapper & get_Replicator() { return *(wrapper_.get()); }
    
        void Open();
        void Close();
                
        void CheckCurrentProgress(FABRIC_SEQUENCE_NUMBER expectedLsn) const
        {
            FABRIC_SEQUENCE_NUMBER lsn;
            ErrorCode error = wrapper_->GetCurrentProgress(lsn);
            VERIFY_IS_TRUE(error.IsSuccess(), L"Primary.GetCurrentProgress succeeded");
            VERIFY_ARE_EQUAL_FMT(
                lsn, 
                expectedLsn, 
                "GetCurrentProgress returned {0}, expected {1}", lsn, expectedLsn);
        }

        void CheckCatchUpCapability(FABRIC_SEQUENCE_NUMBER expectedLsn) const
        {
            // Catchup capability check is done usually after "catchup" async operation completes

            // In the past, catchup async op completed by looking at the state of the operation queue's -> If there were NO pending operations, catchup completed.

            // However, from now, operations are completed in the queue as and when all replicas send a receive ACK + quorum of them send apply ACK. 
            // As a result, the catchup operation does not depend on the pending operation check in the queue
            //
            // Catchup completes purely based on the progress of replicas in the CC and in some cases, when there are idle replicas, before their receive ACK's are processed,
            // catch up operation completes.

            // So we have a retry logic here

            FABRIC_SEQUENCE_NUMBER lsn;

            for (int i = 0; i < 30; i++)
            {
                ErrorCode error = wrapper_->GetCatchUpCapability(lsn);
                VERIFY_IS_TRUE(error.IsSuccess(), L"Primary.GetCatchUpCapability succeeded");
                if (lsn == expectedLsn) break;
                else Sleep(10);
            }

            VERIFY_ARE_EQUAL_FMT(
                lsn, 
                expectedLsn, 
                "GetCatchUpCapability returned {0}, expected {1}", lsn, expectedLsn);
        }
    private:

        static FABRIC_REPLICA_ID PrimaryReplicaId;

        REPerformanceCountersSPtr perfCounters_;
        FABRIC_REPLICA_ID primaryId_;
        ReplicationEndpointId endpointUniqueId_;
        ReplicationTransportSPtr primaryTransport_;
        shared_ptr<PrimaryReplicatorWrapper> wrapper_;
    };

    class SecondaryReplicatorHelper  : public Common::ComponentRoot
    {
    public:
        SecondaryReplicatorHelper(
            REConfigSPtr const & config,
            FABRIC_REPLICA_ID replicaId,
            bool hasPersistedState,
            int64 numberOfCopyOps,
            int64 numberOfCopyContextOps,
            Common::Guid partitionId,
            bool dropReplicationAcks);

        virtual ~SecondaryReplicatorHelper() {}

        __declspec(property(get=get_SecondaryId)) FABRIC_REPLICA_ID SecondaryId;
        FABRIC_REPLICA_ID get_SecondaryId() const { return secondaryId_; }

        __declspec(property(get=get_ReplicationEndpoint)) std::wstring const & ReplicationEndpoint;
        std::wstring const & get_ReplicationEndpoint() const { return endpoint_; }

        __declspec(property(get=get_EndpointUniqueId)) ReplicationEndpointId const & EndpointUniqueId;
        ReplicationEndpointId const & get_EndpointUniqueId() const { return endpointUniqueId_; }

        ReplicationEndpointId const & get_ReplicationEndpointId() const { return endpointUniqueId_; }

        virtual void ProcessMessage(__in Transport::Message & message, __out Transport::ReceiverContextUPtr &);

        class SecondaryReplicatorWrapper : public SecondaryReplicator
        {
        public:
            SecondaryReplicatorWrapper(
                REConfigSPtr const & config,
                REPerformanceCountersSPtr & perfCounters,
                FABRIC_REPLICA_ID replicaId,
                bool hasPersistedState,
                ReplicationEndpointId const & endpointUniqueId,
                ReplicationTransportSPtr const & transport,
                ComProxyStateProvider const & provider,
                bool dropReplicationAcks,
                IReplicatorHealthClientSPtr & healthClient,
                ApiMonitoringWrapperSPtr & monitor);

            virtual ~SecondaryReplicatorWrapper() {}

            void SetDropCopyAcks(bool dropCopyAcks);

            void SetMessageReceivedCallback(MessageReceivedCallback callback) { callback_ = callback; }
            
            virtual void ReplicationOperationMessageHandler(
                __in Transport::Message & message, ReplicationFromHeader const & fromHeader);

            virtual void CopyOperationMessageHandler(
                __in Transport::Message & message, ReplicationFromHeader const & fromHeader);

            virtual void StartCopyMessageHandler(
                __in Transport::Message & message, ReplicationFromHeader const & fromHeader);

            virtual void CopyContextAckMessageHandler(
                __in Transport::Message & message, ReplicationFromHeader const & fromHeader);

            virtual void RequestAckMessageHandler(
                __in Transport::Message & message, ReplicationFromHeader const & fromHeader);

            void CheckMessages();

            void AddExpectedCopyMessage(ReplicationEndpointId const & fromDemuxer, FABRIC_SEQUENCE_NUMBER seq);

            void AddExpectedReplicationMessage(ReplicationEndpointId const & fromDemuxer, FABRIC_SEQUENCE_NUMBER sequenceNumber);

            void AddExpectedStartCopyMessage(ReplicationEndpointId const & fromDemuxer);

            void AddExpectedCopyContextAckMessage(ReplicationEndpointId const & fromDemuxer, FABRIC_SEQUENCE_NUMBER sequuenceNumber);

        private:

            vector<wstring> expectedMessages_;
            vector<wstring> receivedMessages_;
            bool dropReplicationAcks_;
            bool dropCopyAcks_;
            bool dropCurrentAck_;
            bool dropCurrentStartCopyAck_;
            MessageReceivedCallback callback_;
            FABRIC_REPLICA_ID id_;
            int maxReplDropCount_;
            int maxCopyDropCount_;
            mutable Common::ExclusiveLock lock_;
        };

        __declspec(property(get=get_Replicator)) SecondaryReplicatorWrapper & Replicator;
        SecondaryReplicatorWrapper & get_Replicator() { return *(wrapper_.get()); }
                
        void CheckCurrentProgress(FABRIC_SEQUENCE_NUMBER expectedLsn) const
        {
            FABRIC_SEQUENCE_NUMBER lsn;
            ErrorCode error = wrapper_->GetCurrentProgress(lsn);
            VERIFY_IS_TRUE(error.IsSuccess(), L"Secondary.GetCurrentProgress succeeded");
            VERIFY_ARE_EQUAL_FMT(
                lsn, 
                expectedLsn, 
                "GetCurrentProgress returned {0}, expected {1}", lsn, expectedLsn);
        }

        void CheckCatchUpCapability(FABRIC_SEQUENCE_NUMBER expectedLsn) const
        {
            FABRIC_SEQUENCE_NUMBER lsn;
            ErrorCode error = wrapper_->GetCatchUpCapability(lsn);
            VERIFY_IS_TRUE(error.IsSuccess(), L"Secondary.GetCatchUpCapability succeeded");
            VERIFY_ARE_EQUAL_FMT(
                lsn, 
                expectedLsn, 
                "GetCatchUpCapability returned {0}, expected {1}", lsn, expectedLsn);
        }

        void Open();
        void Close();

        void StartCopyOperationPump() 
        { 
            auto copyStream = wrapper_->GetCopyStream(std::dynamic_pointer_cast<SecondaryReplicator>(wrapper_->shared_from_this())); 
            StartGetOperation(copyStream); 
        }
        
        void StartReplicationOperationPump() 
        {
            auto replicationStream = wrapper_->GetReplicationStream(std::dynamic_pointer_cast<SecondaryReplicator>(wrapper_->shared_from_this())); 
            StartGetOperation(replicationStream); 
        }

    private:
        void StartGetOperation(OperationStreamSPtr const & stream);
        bool EndGetOperation(AsyncOperationSPtr const & asyncOp, OperationStreamSPtr const & stream);
        
        REPerformanceCountersSPtr perfCounters_;
        FABRIC_REPLICA_ID secondaryId_;
        ReplicationEndpointId endpointUniqueId_;
        ReplicationTransportSPtr secondaryTransport_;
        wstring endpoint_;
        shared_ptr<SecondaryReplicatorWrapper> wrapper_;
    };
    
    typedef std::shared_ptr<PrimaryReplicatorHelper> PrimaryReplicatorHelperSPtr;
    typedef std::shared_ptr<SecondaryReplicatorHelper> SecondaryReplicatorHelperSPtr;

    class ReplicatorTestWrapper
    {
    public:
        ReplicatorTestWrapper() 
            : operationList_(), 
              event_()
        {
        }

        void CreatePrimary(
            int64 numberOfCopyOps,
            int64 numberOfCopyContextOps,
            bool smallRetryInterval, 
            FABRIC_EPOCH const & epoch,
            bool hasPersistedState,
            Common::Guid partitionId,
            __out PrimaryReplicatorHelperSPtr & primary);
        void CreateSecondary(
            int64 primaryId,
            uint index,
            int64 numberOfCopyOps,
            int64 numberOfCopyContextOps,
            bool dropReplicationAcks,
            bool hasPersistedState,
            Common::Guid partitionId,
            __out int64 & secondaryId,
            __out SecondaryReplicatorHelperSPtr & secondary);
        void CreateConfiguration(
            int numberSecondaries,
            int numberIdles, 
            FABRIC_EPOCH const & epoch,
            int64 numberOfCopyOps,
            int64 numberOfCopyContextOps,
            bool hasPersistedState,
            Common::Guid partitionId,
            __out PrimaryReplicatorHelperSPtr & primary,
            __out vector<SecondaryReplicatorHelperSPtr> & secondaries,
            __out vector<int64> & secondaryIds,
            bool dropReplicationAcks = false);
        void PromoteIdlesToSecondaries(
            int currentNumberSecondaries,
            int newNumberSecondaries,
            __in PrimaryReplicatorHelperSPtr const & primary,
            __in vector<SecondaryReplicatorHelperSPtr> & secondaries,
            vector<int64> const & secondaryIds);
        void BuildIdles(
            int64 startReplSeq,
            __in PrimaryReplicatorHelperSPtr & primary,
            __in vector<SecondaryReplicatorHelperSPtr> & secondaries,
            vector<int64> const & secondaryIds,
            int64 numberOfCopyOps,
            int64 numberOfCopyContextOps);
        void BeginAddIdleCopyNotFinished(
            int index,
            int64 numberOfCopyOps,
            int64 numberOfCopyContextOps,
            PrimaryReplicatorHelperSPtr const & primary,
            bool hasPersistedState,
            Common::Guid partitionId,
            __out SecondaryReplicatorHelperSPtr & secondary,
            __out int64 & secondaryId,
            __in ManualResetEvent & copyDoneEvent);
        void EndAddIdleCopyNotFinished(
            __in SecondaryReplicatorHelperSPtr & secondary,
            __in ManualResetEvent & copyDoneEvent);
            
        static void Replicate(__in PrimaryReplicatorHelper & repl, FABRIC_SEQUENCE_NUMBER expectedSequenceNumber, uint numberOfOperations = 1);
        static void WaitForProgress(
            __in PrimaryReplicatorHelper & repl, FABRIC_REPLICA_SET_QUORUM_MODE catchUpMode, bool expectTimeout = false);

        static void CheckMessages(
            FABRIC_REPLICA_ID id,
            __in vector<wstring> & expectedMessages, 
            __in vector<wstring> & receivedMessages);
        static void AddReceivedMessage(
            __in vector<wstring> & receivedMessages,
            wstring const & content);

    private:

        void Copy(__in PrimaryReplicatorHelper & repl, vector<ReplicaInformation> const & replicas);

        wstring operationList_;
        ManualResetEvent event_;
    };
    } // anonymous namespace

    /***********************************
    * TestReplicateCopyOperations methods
    **********************************/

    BOOST_FIXTURE_TEST_SUITE(TestReplicateCopyOperationsSuite,TestReplicateCopyOperations)

    BOOST_AUTO_TEST_CASE(TestReplicateWithRetry1)
    {
        TestReplicateWithRetry(true);
    }

    BOOST_AUTO_TEST_CASE(TestReplicateWithRetry2)
    {
        TestReplicateWithRetry(false);
    }

    BOOST_AUTO_TEST_CASE(TestCopyWithRetry1)
    {
        TestCopyWithRetry(true);
    }

    BOOST_AUTO_TEST_CASE(TestCopyWithRetry2)
    {
        TestCopyWithRetry(false);
    }

    BOOST_AUTO_TEST_CASE(TestReplicateAndCopy1)
    {
        TestReplicateAndCopy(true);
    }

    BOOST_AUTO_TEST_CASE(TestReplicateAndCopy2)
    {
        TestReplicateAndCopy(false);
    }

    BOOST_AUTO_TEST_CASE(TestReplicateAndCopyWithIdleReceivedLSN)
    {
        bool hasPersistedState = true;
        Common::Guid partitionId = Common::Guid::NewGuid();
        ComTestOperation::WriteInfo(
            ReplCopyTestSource,
            "Start TestReplicateAndCopyWithIdleReceivedLSN, persisted state {0}, Partitionid = {1}",
            hasPersistedState,
            partitionId);

        ReplicatorTestWrapper wrapper;

        PrimaryReplicatorHelperSPtr primary;
        vector<SecondaryReplicatorHelperSPtr> secondaries;
        vector<int64> secondaryIds;

        // Configuration: 1 Primary, 1 secondary, Quorum count = 2
        // Replicate done when primary receives 2 ACKs
        int numberOfIdles = 1;
        int numberOfSecondaries = 2;
        int64 numberOfCopyOperations = 0;
        int64 numberOfCopyContextOperations = hasPersistedState ? 0 : -1;
        FABRIC_EPOCH epoch;
        epoch.DataLossNumber = 1;
        epoch.ConfigurationNumber = 100;
        epoch.Reserved = NULL;
        wrapper.CreateConfiguration(numberOfSecondaries, numberOfIdles, epoch, 
            numberOfCopyOperations, numberOfCopyContextOperations, hasPersistedState, partitionId, primary, secondaries, secondaryIds);

        PrimaryReplicatorHelper & repl = *primary.get();

        wrapper.Replicate(repl, 1);
        // Full progress achieved at replicate done,
        // so it should return immediately
        wrapper.WaitForProgress(repl, FABRIC_REPLICA_SET_QUORUM_ALL);
        repl.CheckCurrentProgress(1);
        repl.CheckCatchUpCapability(0);

        // All replicas should receive the replicate message
        for(size_t i = 0; i < secondaries.size(); ++i)
        {
            secondaries[i]->Replicator.AddExpectedReplicationMessage(primary->EndpointUniqueId, 1);
            primary->Replicator.AddExpectedAck(secondaries[i]->SecondaryId, 1, Constants::NonInitializedLSN);
        }

        // Add an idle replica with copy in progress (drop all acks for copy)
        // The copy should provide repl seq = 2, because 1 is already committed(and completed)
        SecondaryReplicatorHelperSPtr secondary;
        int64 secondaryId;
        ManualResetEvent copyDoneEvent;
        int index = numberOfSecondaries + numberOfIdles;
        wrapper.BeginAddIdleCopyNotFinished(index, numberOfCopyOperations, numberOfCopyContextOperations, primary, hasPersistedState, partitionId, secondary, secondaryId, copyDoneEvent);
        
        wrapper.Replicate(repl, 2);
        // Even though there is an idle which is in-build, the replica would have
        // received repl-2 and ACK'd that it has received it (ReceivedACK)

        // Hence, we clear out our queues
        wrapper.WaitForProgress(repl, FABRIC_REPLICA_SET_QUORUM_ALL);
        repl.CheckCurrentProgress(2);
        repl.CheckCatchUpCapability(0);

        for(size_t i = 0; i < secondaries.size(); ++i)
        {
            secondaries[i]->Replicator.AddExpectedReplicationMessage(primary->EndpointUniqueId, 2);
            primary->Replicator.AddExpectedAck(secondaries[i]->SecondaryId, 2, Constants::NonInitializedLSN);
        }
        secondary->Replicator.AddExpectedReplicationMessage(primary->EndpointUniqueId, 2);
        
        // Finish copy for the last idle
        wrapper.EndAddIdleCopyNotFinished(secondary, copyDoneEvent);
        primary->Replicator.AddExpectedAck(secondary->SecondaryId, 2, Constants::NonInitializedLSN);
        
        // Full progress should be achieved now
        wrapper.WaitForProgress(repl, FABRIC_REPLICA_SET_QUORUM_ALL, false);
        repl.CheckCurrentProgress(2);
        repl.CheckCatchUpCapability(0);

        wrapper.Replicate(repl, 3);
        for(size_t i = 0; i < secondaries.size(); ++i)
        {
            primary->Replicator.AddExpectedAck(secondaries[i]->SecondaryId, 3, Constants::NonInitializedLSN);
            secondaries[i]->Replicator.AddExpectedReplicationMessage(primary->EndpointUniqueId, 3);
        }
        primary->Replicator.AddExpectedAck(secondary->SecondaryId, 3, Constants::NonInitializedLSN);
        secondary->Replicator.AddExpectedReplicationMessage(primary->EndpointUniqueId, 3);
        
        // Wait for all operations to be completed on primary
        wrapper.WaitForProgress(repl, FABRIC_REPLICA_SET_QUORUM_ALL);
        repl.CheckCurrentProgress(3);
        repl.CheckCatchUpCapability(0);

        for(size_t i = 0; i < secondaries.size(); ++i)
        {
            secondaries[i]->Replicator.CheckMessages();
            secondaries[i]->CheckCurrentProgress(3);
            secondaries[i]->CheckCatchUpCapability(3);
            secondaries[i]->Close();
        }
                
        secondary->Replicator.CheckMessages();
        secondary->CheckCurrentProgress(3);
        secondary->CheckCatchUpCapability(3);
        secondary->Close();
        
        primary->Replicator.CheckAcks();
        primary->Close();
    }


    BOOST_AUTO_TEST_CASE(TestCancelCopy)
    {
        bool hasPersistedState = true;
        Common::Guid partitionId = Common::Guid::NewGuid();
        ComTestOperation::WriteInfo(
            ReplCopyTestSource,
            "Start TestCancelCopy, persisted state {0}, partitionId = {1}",
            hasPersistedState, partitionId);

        ReplicatorTestWrapper wrapper;

        PrimaryReplicatorHelperSPtr primary;
        SecondaryReplicatorHelperSPtr secondary;
        FABRIC_REPLICA_ID secondaryId;

        FABRIC_EPOCH epoch;
        epoch.DataLossNumber = 132984;
        epoch.ConfigurationNumber = 999;
        epoch.Reserved = NULL;
        int64 numberOfCopyOps = 12132;
        int64 numberOfCopyContextOps = hasPersistedState ? 100 : -1;
        wrapper.CreatePrimary(numberOfCopyOps, numberOfCopyContextOps, false, epoch, hasPersistedState, partitionId, primary);
        wrapper.CreateSecondary(primary->PrimaryId, 0, numberOfCopyOps, numberOfCopyContextOps, false, hasPersistedState, partitionId, secondaryId, secondary);
      
        PrimaryReplicatorHelper & repl = *primary.get();
        
        ReplicaInformation replica(
            secondaryId,
            ::FABRIC_REPLICA_ROLE_IDLE_SECONDARY,
            secondary->ReplicationEndpoint,
            false);
      
        ManualResetEvent copyDoneEvent;
        FABRIC_REPLICA_ID replicaId = replica.Id;
        auto copyOp = repl.Replicator.BeginBuildReplica(
            replica,
            [&repl, replicaId, &copyDoneEvent](AsyncOperationSPtr const& operation) -> void
            {
                ErrorCode error = repl.Replicator.EndBuildReplica(operation);
                if (error.IsSuccess())
                {
                    VERIFY_FAIL_FMT("Copy succeed when failure expected for replica {0}.", replicaId);
                }

                Trace.WriteInfo(ReplCopyTestSource, "Copy failed for replica {0} with error {1}.", replicaId, error);
                copyDoneEvent.Set();
            }, AsyncOperationSPtr());

        Sleep(60);
        copyOp->Cancel();

        copyDoneEvent.WaitOne();
        primary->Close();
        secondary->Close();
    }

    BOOST_AUTO_TEST_CASE(TestCopyOperationsExceedQueueCapacity)
    {
        bool hasPersistedState = false;
        Common::Guid partitionId = Common::Guid::NewGuid();
        ComTestOperation::WriteInfo(
            ReplCopyTestSource,
            "Start TestCopyOperationsExceedQueueCapacity, persisted state {0}, partitionId= {1}",
            hasPersistedState,
            partitionId);
        
        ReplicatorTestWrapper wrapper;

        PrimaryReplicatorHelperSPtr primary;
        vector<SecondaryReplicatorHelperSPtr> secondaries;
        vector<int64> secondaryIds;

        // The provider has more operations that the queue capacity
        // Because the state provider gives the operations very quickly, 
        // the queue full will be reached.
        int64 numberOfCopyOps = MaxQueueCapacity + 50;
        FABRIC_EPOCH epoch;
        epoch.DataLossNumber = 232;
        epoch.ConfigurationNumber = 323;
        epoch.Reserved = NULL;
        int64 numberOfCopyContextOps = hasPersistedState ? 0 : -1;
        wrapper.CreatePrimary(numberOfCopyOps, numberOfCopyContextOps, false, epoch, hasPersistedState, partitionId, primary);

        // Create the secondary that will be built
        int64 secondaryId;
        SecondaryReplicatorHelperSPtr secondary;
        wrapper.CreateSecondary(primary->PrimaryId, 0, numberOfCopyOps, numberOfCopyContextOps, false, hasPersistedState, partitionId, secondaryId, secondary);
        secondaryIds.push_back(secondaryId);
        secondaries.push_back(move(secondary));
        
        PrimaryReplicatorHelper & repl = *primary.get();

        ManualResetEvent buildIdleEvent;
        ManualResetEvent replEvent;
        // Replicate and build idle in parallel
        Threadpool::Post([&wrapper, &repl, &replEvent]() 
            {
                wrapper.Replicate(repl, 1, 2);
                Sleep(40);
                wrapper.Replicate(repl, 3, 2);
                replEvent.Set();
            });
        
        Threadpool::Post([&]()
            {
                wrapper.BuildIdles(0, primary, secondaries, secondaryIds, numberOfCopyOps, numberOfCopyContextOps);
                buildIdleEvent.Set();
            });
                
        buildIdleEvent.WaitOne();
        replEvent.WaitOne();
        
        primary->Close();
        secondaries[0]->Close();
    }

    BOOST_AUTO_TEST_CASE(TestReplicatePrimaryOnly)
    {
        bool hasPersistedState = true;
        Common::Guid partitionId = Common::Guid::NewGuid();
        ComTestOperation::WriteInfo(
            ReplCopyTestSource,
            "Start TestReplicatePrimaryOnly, persisted state {0}, partitionId = {1}",
            hasPersistedState, partitionId);
        
        ReplicatorTestWrapper wrapper;
        PrimaryReplicatorHelperSPtr primary;
        
        FABRIC_EPOCH epoch;
        epoch.DataLossNumber = 23424;
        epoch.ConfigurationNumber = 666;
        epoch.Reserved = NULL;
        int64 numberOfCopyOps = 2;
        int64 numberOfCopyContextOps = 0;
        wrapper.CreatePrimary(numberOfCopyOps, numberOfCopyContextOps, false, epoch, hasPersistedState, partitionId, primary);
        PrimaryReplicatorHelper & repl = *primary.get();

        // Replicate more operations 
        // than the queue limit 
        uint numberOfReplicateOps = MaxQueueCapacity + 2;
        wrapper.Replicate(repl, 1, numberOfReplicateOps);

        repl.CheckCurrentProgress(static_cast<FABRIC_SEQUENCE_NUMBER>(numberOfReplicateOps));
        repl.CheckCatchUpCapability(0);

        primary->Close();
    }

    BOOST_AUTO_TEST_CASE(TestNullCopyContext)
    {
        bool hasPersistedState = true;
        Common::Guid partitionId = Common::Guid::NewGuid();
        ComTestOperation::WriteInfo(
            ReplCopyTestSource,
            "Start TestNullCopyContext, persisted state {0}, partitionId = {1}",
            hasPersistedState, partitionId);

        ReplicatorTestWrapper wrapper;
        PrimaryReplicatorHelperSPtr primary;
        vector<SecondaryReplicatorHelperSPtr> secondaries;
        vector<int64> secondaryIds;

        int numberOfIdles = 0;
        int numberOfSecondaries = 1;
        int64 numberOfCopyOperations = 4;
        int64 numberOfCopyContextOperations = -1;
                
        FABRIC_EPOCH epoch;
        epoch.DataLossNumber = 4387483;
        epoch.ConfigurationNumber = 1111;
        epoch.Reserved = NULL;
        wrapper.CreateConfiguration(numberOfSecondaries, numberOfIdles, epoch, 
            numberOfCopyOperations, numberOfCopyContextOperations, hasPersistedState, partitionId, primary, secondaries, secondaryIds);
                
        primary->Replicator.AddExpectedAck(
            secondaries[0]->SecondaryId, 0, Constants::NonInitializedLSN);
            
        primary->Replicator.CheckAcks();
        primary->Replicator.CheckMessages();
        primary->Close();
        secondaries[0]->Replicator.CheckMessages();
        secondaries[0]->Close();
    }

    BOOST_AUTO_TEST_CASE(TestBatchAckFromPendingItems)
    {
        bool hasPersistedState = true;
        Common::Guid partitionId = Common::Guid::NewGuid();
        ComTestOperation::WriteInfo(
            ReplCopyTestSource,
            "Start TestBatchAckFromPendingItems, persisted state {0}, partitionId = {1}",
            hasPersistedState, partitionId);

        ReplicatorTestWrapper wrapper;
        int64 numberOfCopyOps = 3;
        FABRIC_EPOCH epoch;
        epoch.DataLossNumber = 1111;
        epoch.ConfigurationNumber = 1111;
        epoch.Reserved = NULL;
        int64 numberOfCopyContextOps = hasPersistedState ? 0 : -1;

        PrimaryReplicatorHelperSPtr primary;
        wrapper.CreatePrimary(numberOfCopyOps, numberOfCopyContextOps, false, epoch, hasPersistedState, partitionId, primary);

        FABRIC_REPLICA_ID secondaryId;
        SecondaryReplicatorHelperSPtr secondary;

        // Create an idle
        REConfigSPtr config = TestReplicateCopyOperations::CreateGenericConfig();
        config->MaxPendingAcknowledgements = 2;
        // Set batch ACK period to a large value, to check
        // that we force send when the number of pending ack is 
        // greater or equal than specified value.
        config->BatchAcknowledgementInterval = TimeSpan::FromSeconds(60);
        config->RetryInterval = TimeSpan::FromSeconds(61);
        
        secondaryId = primary->PrimaryId + 1;
        secondary = make_shared<SecondaryReplicatorHelper>(
            config, 
            secondaryId, 
            hasPersistedState,
            numberOfCopyOps,
            numberOfCopyContextOps,
            partitionId,
            false /*dropReplicationAcks*/);
        secondary->Open();
        // Start copy operation pump; the replication pump will be started
        // when all copy received
        secondary->StartCopyOperationPump();
        
        vector<SecondaryReplicatorHelperSPtr> secondaries;
        vector<FABRIC_REPLICA_ID> secondaryIds;
        secondaryIds.push_back(secondaryId);
        secondaries.push_back(move(secondary));

        // Build idle should succeed
        // because the ACKs are sent when there are 2 pending operations
        wrapper.BuildIdles(0, primary, secondaries, secondaryIds, numberOfCopyOps, numberOfCopyContextOps);
        
        PrimaryReplicatorHelper & repl = *primary.get();
        wrapper.Replicate(repl, 1, 2);
        // Full progress should return, because there are 2 pending operations,
        // so secondary should force send ACK.
        wrapper.WaitForProgress(repl, FABRIC_REPLICA_SET_QUORUM_ALL);
        
        primary->Close();
        secondaries[0]->Close();
    }

    // This test only has sense for persisted replicas
    BOOST_AUTO_TEST_CASE(TestRequestAck)
    {
        bool hasPersistedState = true;
        Common::Guid partitionId = Common::Guid::NewGuid();
        ComTestOperation::WriteInfo(
            ReplCopyTestSource,
            "Start TestRequestAck, persisted state {0}, partitionId = {1}",
            hasPersistedState, partitionId);

        ReplicatorTestWrapper wrapper;
        int64 numberOfCopyOps = 2;
        FABRIC_EPOCH epoch;
        epoch.DataLossNumber = 9999;
        epoch.ConfigurationNumber = 9999;
        epoch.Reserved = NULL;
        int64 numberOfCopyContextOps = hasPersistedState ? 0 : -1;

        PrimaryReplicatorHelperSPtr primary;
        wrapper.CreatePrimary(numberOfCopyOps, numberOfCopyContextOps, true, epoch, hasPersistedState, partitionId, primary);

        FABRIC_REPLICA_ID secondaryId;
        SecondaryReplicatorHelperSPtr secondary;

        // Create an idle
        REConfigSPtr config = TestReplicateCopyOperations::CreateGenericConfig();
        config->BatchAcknowledgementInterval = TimeSpan::Zero;
        
        secondaryId = primary->PrimaryId + 1;
        secondary = make_shared<SecondaryReplicatorHelper>(
            config, 
            secondaryId, 
            hasPersistedState,
            numberOfCopyOps,
            numberOfCopyContextOps,
            partitionId,
            false /*dropReplicationAcks*/);
        // Open the secondary, but do not start the operation pump
        secondary->Open();

        secondary->Replicator.SetMessageReceivedCallback(
            [secondary] (wstring const & action) 
            {
                if(action == ReplicationTransport::RequestAckAction)
                {
                    secondary->StartCopyOperationPump();
                }
            });

        vector<SecondaryReplicatorHelperSPtr> secondaries;
        vector<FABRIC_REPLICA_ID> secondaryIds;
        secondaryIds.push_back(secondaryId);
        secondaries.push_back(move(secondary));

        // Build idle should succeed
        // because the pump should be started when request ack is received
        wrapper.BuildIdles(0, primary, secondaries, secondaryIds, numberOfCopyOps, numberOfCopyContextOps);
        
        primary->Close();
        secondaries[0]->Close();
    }

    BOOST_AUTO_TEST_SUITE_END()

    Common::ComponentRoot const & TestReplicateCopyOperations::GetRoot()
    {
        static std::shared_ptr<DummyRoot> root = std::make_shared<DummyRoot>();
        return *root;
    }

    void TestReplicateCopyOperations::TestReplicateAndCopy(bool hasPersistedState)
    {
        Common::Guid partitionId = Common::Guid::NewGuid();
        ComTestOperation::WriteInfo(
            ReplCopyTestSource,
            "Start TestReplicateAndCopy, persisted state {0}, partitionId = {1}",
            hasPersistedState,
            partitionId);

        ReplicatorTestWrapper wrapper;

        PrimaryReplicatorHelperSPtr primary;
        vector<SecondaryReplicatorHelperSPtr> secondaries;
        vector<int64> secondaryIds;

        // Configuration: 1 Primary, 2 secondaries, 1 idle, Quorum count = 2
        // Replicate done when primary receives sufficient ACKs
        int numberOfIdles = 1;
        int numberOfSecondaries = 2;
        int64 numberOfCopyOperations = 0;
        int64 numberOfCopyContextOperations = hasPersistedState ? 0 : -1;
        FABRIC_EPOCH epoch;
        epoch.DataLossNumber = 1;
        epoch.ConfigurationNumber = 100;
        epoch.Reserved = NULL;
        wrapper.CreateConfiguration(numberOfSecondaries, numberOfIdles, epoch, 
            numberOfCopyOperations, numberOfCopyContextOperations, hasPersistedState, partitionId, primary, secondaries, secondaryIds);

        PrimaryReplicatorHelper & repl = *primary.get();

        wrapper.Replicate(repl, 1);
        // Full progress achieved at replicate done,
        // so it should return immediately
        wrapper.WaitForProgress(repl, FABRIC_REPLICA_SET_QUORUM_ALL);
        repl.CheckCurrentProgress(1);
        repl.CheckCatchUpCapability(0);

        // All replicas should receive the replicate message
        for(size_t i = 0; i < secondaries.size(); ++i)
        {
            secondaries[i]->Replicator.AddExpectedReplicationMessage(primary->EndpointUniqueId, 1);
            primary->Replicator.AddExpectedAck(secondaries[i]->SecondaryId, 1, Constants::NonInitializedLSN);
        }

        // Add an idle replica with copy in progress (drop all acks for copy)
        // The copy should provide repl seq = 2, because 1 is already committed
        SecondaryReplicatorHelperSPtr secondary;
        int64 secondaryId;
        ManualResetEvent copyDoneEvent;
        int index = numberOfSecondaries + numberOfIdles;
        wrapper.BeginAddIdleCopyNotFinished(index, numberOfCopyOperations, numberOfCopyContextOperations, primary, hasPersistedState, partitionId, secondary, secondaryId, copyDoneEvent);
        
        wrapper.Replicate(repl, 2);

        // For Non-persisted services, the replicate operation 
        // is ACKed optimistically immediately (before copy is done).
        wrapper.WaitForProgress(repl, FABRIC_REPLICA_SET_QUORUM_ALL, false);
        repl.CheckCurrentProgress(2);
        repl.CheckCatchUpCapability(0);

        for(size_t i = 0; i < secondaries.size(); ++i)
        {
            secondaries[i]->Replicator.AddExpectedReplicationMessage(primary->EndpointUniqueId, 2);
            primary->Replicator.AddExpectedAck(secondaries[i]->SecondaryId, 2, Constants::NonInitializedLSN);
        }
        secondary->Replicator.AddExpectedReplicationMessage(primary->EndpointUniqueId, 2);
        
        // Finish copy for the last idle
        wrapper.EndAddIdleCopyNotFinished(secondary, copyDoneEvent);
        primary->Replicator.AddExpectedAck(secondary->SecondaryId, 2, Constants::NonInitializedLSN);
        
        // Full progress should be achieved now
        wrapper.WaitForProgress(repl, FABRIC_REPLICA_SET_QUORUM_ALL, false);
        repl.CheckCurrentProgress(2);
        repl.CheckCatchUpCapability(0);

        wrapper.Replicate(repl, 3);
        for(size_t i = 0; i < secondaries.size(); ++i)
        {
            primary->Replicator.AddExpectedAck(secondaries[i]->SecondaryId, 3, Constants::NonInitializedLSN);
            secondaries[i]->Replicator.AddExpectedReplicationMessage(primary->EndpointUniqueId, 3);
        }
        primary->Replicator.AddExpectedAck(secondary->SecondaryId, 3, Constants::NonInitializedLSN);
        secondary->Replicator.AddExpectedReplicationMessage(primary->EndpointUniqueId, 3);
        
        // Wait for all operations to be completed on primary
        wrapper.WaitForProgress(repl, FABRIC_REPLICA_SET_QUORUM_ALL);
        repl.CheckCurrentProgress(3);
        repl.CheckCatchUpCapability(0);

        for(size_t i = 0; i < secondaries.size(); ++i)
        {
            secondaries[i]->Replicator.CheckMessages();
            secondaries[i]->CheckCurrentProgress(3);
            secondaries[i]->CheckCatchUpCapability(3);
            secondaries[i]->Close();
        }
                
        secondary->Replicator.CheckMessages();
        secondary->CheckCurrentProgress(3);
        secondary->CheckCatchUpCapability(3);
        secondary->Close();
        
        primary->Replicator.CheckAcks();
        primary->Close();
    }


    void TestReplicateCopyOperations::TestReplicateWithRetry(bool hasPersistedState)
    {
        Common::Guid partitionId = Common::Guid::NewGuid();
        ComTestOperation::WriteInfo(
            ReplCopyTestSource,
            "Start TestReplicateWithRetry, persisted state {0}, partitionId = {1}",
            hasPersistedState, partitionId);

        ReplicatorTestWrapper wrapper;

        PrimaryReplicatorHelperSPtr primary;
        vector<SecondaryReplicatorHelperSPtr> secondaries;
        vector<int64> secondaryIds;
                
        int numberOfSecondaries = 2;
        int64 numberOfCopyOperations = 0;
        int64 numberOfCopyContextOperations = hasPersistedState ? 0 : -1;
        FABRIC_EPOCH epoch;
        epoch.DataLossNumber = 2;
        epoch.ConfigurationNumber = 100;
        epoch.Reserved = NULL;
        wrapper.CreateConfiguration(
            numberOfSecondaries, 0, epoch, numberOfCopyOperations, numberOfCopyContextOperations, hasPersistedState, partitionId, primary, secondaries, secondaryIds, true);

        PrimaryReplicatorHelper &repl = *(primary.get());

        uint numberOfOperations = 3;
        // All replicas should receive the replicate messages;
        for(size_t i = 0; i < secondaries.size(); ++i)
        {
            for(int64 j = 1; j <= numberOfOperations; ++j)
            {
                // Primary retries on timer because some of the ACKs are dropped.
                // Since we don't know which ACKs are dropped,
                // primary may receive different replication numbers each time,
                // so we don't check it.
                secondaries[i]->Replicator.AddExpectedReplicationMessage(primary->EndpointUniqueId, j);
            }
        }
        
        wrapper.Replicate(repl, 1, numberOfOperations);
        
        // Wait for all operations to be completed on primary
        wrapper.WaitForProgress(repl, FABRIC_REPLICA_SET_QUORUM_ALL);
        for(size_t i = 0; i < secondaries.size(); ++i)
        {
            primary->Replicator.AddExpectedAck(
                secondaries[i]->SecondaryId, numberOfOperations, Constants::NonInitializedLSN);
            secondaries[i]->Replicator.CheckMessages();
            secondaries[i]->CheckCurrentProgress(3);
            // Catchup capability depends on if the secondary has received the header about primary's completed LSN or not
            // Since this variation drops aCKs randomly, it is hard to be deterministic
            // secondaries[i]->CheckCatchUpCapability(3); 
            secondaries[i]->Close();
        }

        primary->Replicator.CheckAcks();
        primary->CheckCurrentProgress(3);
        primary->CheckCatchUpCapability(0);
        primary->Close();
    }

    void TestReplicateCopyOperations::TestCopyWithRetry(bool hasPersistedState)
    {
        Common::Guid partitionId = Common::Guid::NewGuid();
        ComTestOperation::WriteInfo(
            ReplCopyTestSource,
            "Start TestCopyWithRetry, persisted state {0}, partitionID = {1}",
            hasPersistedState, partitionId);

        ReplicatorTestWrapper wrapper;

        PrimaryReplicatorHelperSPtr primary;
        // The copy provider will send 6 operations until copy is finished
        int64 numberOfCopyOps = 6;
        int64 numberOfCopyContextOps = hasPersistedState ? 4 : -1;
        FABRIC_EPOCH epoch;
        epoch.DataLossNumber = 1;
        epoch.ConfigurationNumber = 156;
        epoch.Reserved = NULL;

        wrapper.CreatePrimary(numberOfCopyOps, numberOfCopyContextOps, true, epoch, hasPersistedState, partitionId, primary);

        // Add an idle replica with copy in progress (drop all acks for copy)
        SecondaryReplicatorHelperSPtr secondary;
        FABRIC_REPLICA_ID secondaryId;
        ManualResetEvent copyDoneEvent;
        wrapper.BeginAddIdleCopyNotFinished(0, numberOfCopyOps, numberOfCopyContextOps, primary, hasPersistedState, partitionId, secondary, secondaryId, copyDoneEvent);

        primary->Replicator.AddExpectedAck(secondary->SecondaryId, 0, Constants::NonInitializedLSN);

        // Finish copy for the last idle
        wrapper.EndAddIdleCopyNotFinished(secondary, copyDoneEvent);

        secondary->Replicator.CheckMessages();
        secondary->Close();
        
        primary->Replicator.CheckAcks();
        primary->Close();
    }

    bool TestReplicateCopyOperations::Setup()
    {
        return TRUE;
    }

    REConfigSPtr TestReplicateCopyOperations::CreateGenericConfig()
    {
        REConfigSPtr config = std::make_shared<REConfig>();
        config->BatchAcknowledgementInterval = TimeSpan::FromMilliseconds(100);
        config->InitialCopyQueueSize = 4;
        config->MaxCopyQueueSize = MaxQueueCapacity;
        config->InitialReplicationQueueSize = 4;
        config->MaxReplicationQueueSize = MaxQueueCapacity;
        config->QueueHealthMonitoringInterval = TimeSpan::Zero;
        config->UseStreamFaultsAndEndOfStreamOperationAck = true;
        config->SlowApiMonitoringInterval = TimeSpan::Zero;
        
        return config;
    }

    ReplicationTransportSPtr TestReplicateCopyOperations::CreateTransport(REConfigSPtr const & config)
    {
        auto transport = ReplicationTransport::Create(config->ReplicatorListenAddress);

        auto errorCode = transport->Start(L"localhost:0");
        ASSERT_IFNOT(errorCode.IsSuccess(), "Failed to start: {0}", errorCode);

        return transport;
    }
   
    /***********************************
    * PrimaryReplicatorHelper methods
    **********************************/
    PrimaryReplicatorHelper::PrimaryReplicatorHelper(
        REConfigSPtr const & config, 
        bool hasPersistedState,
        FABRIC_EPOCH const & epoch, 
        int64 numberOfCopyOperations,
        int64 numberOfCopyContextOperations,
        Common::Guid partitionId)
        : 
        primaryId_(PrimaryReplicaId),
        perfCounters_(REPerformanceCounters::CreateInstance(partitionId.ToString(), PrimaryReplicaId)),
        endpointUniqueId_(partitionId, PrimaryReplicaId),
        primaryTransport_(TestReplicateCopyOperations::CreateTransport(config)),
        wrapper_()
    {
        ComProxyStateProvider stateProvider = move(ComTestStateProvider::GetProvider(
            numberOfCopyOperations, 
            numberOfCopyContextOperations,
            *this));
        IReplicatorHealthClientSPtr temp;
        ApiMonitoringWrapperSPtr temp2 = ApiMonitoringWrapper::Create(temp, REInternalSettings::Create(nullptr, config), partitionId, endpointUniqueId_);
        wrapper_ = make_shared<PrimaryReplicatorWrapper>(
            config, perfCounters_, primaryId_, hasPersistedState, endpointUniqueId_, epoch, primaryTransport_, stateProvider, temp, temp2);
    }

    int64 PrimaryReplicatorHelper::PrimaryReplicaId = 1;
    
    void PrimaryReplicatorHelper::ProcessMessage(
        __in Transport::Message & message, __out Transport::ReceiverContextUPtr &) 
    {
        wstring const & action = message.Action;
        if (action != ReplicationTransport::ReplicationAckAction &&
            action != ReplicationTransport::CopyContextOperationAction)
        {
            VERIFY_FAIL(L"Message doesn't have correct action - only ACK and CopyContext supported on primary");
        }

        ReplicationFromHeader fromHeader;
        if (!message.Headers.TryReadFirst(fromHeader))
        {
            VERIFY_FAIL(L"Message doesn't have correct headers");
        }

        if (action == ReplicationTransport::ReplicationAckAction)
        {
            wrapper_->ReplicationAckMessageHandler(message, fromHeader, wrapper_);
        }
        else
        {
            wrapper_->CopyContextMessageHandler(message, fromHeader);
        }
    }

    void PrimaryReplicatorHelper::Open()
    {
        primaryTransport_->RegisterMessageProcessor(*this);
        wrapper_->Open();
    }

    void PrimaryReplicatorHelper::Close()
    {
        ManualResetEvent closeCompleteEvent;
        wrapper_->BeginClose(
            false,
            Common::TimeSpan::Zero,
            [this, &closeCompleteEvent](AsyncOperationSPtr const& operation) -> void
            {
                ErrorCode error = this->wrapper_->EndClose(operation);
                VERIFY_IS_TRUE(error.IsSuccess(), L"Primary Replicator closed successfully.");
                closeCompleteEvent.Set();
            }, AsyncOperationSPtr());

        VERIFY_IS_TRUE(closeCompleteEvent.WaitOne(10000), L"Close should succeed");
        primaryTransport_->UnregisterMessageProcessor(*this);
        primaryTransport_->Stop();
    }

    PrimaryReplicatorHelper::PrimaryReplicatorWrapper::PrimaryReplicatorWrapper(
        REConfigSPtr const & config,
        REPerformanceCountersSPtr & perfCounters,
        FABRIC_REPLICA_ID replicaId,
        bool hasPersistedState,
        ReplicationEndpointId const & endpointUniqueId,
        FABRIC_EPOCH const & epoch,
        ReplicationTransportSPtr const & transport,
        ComProxyStateProvider const & provider,
        IReplicatorHealthClientSPtr & temp,
        ApiMonitoringWrapperSPtr & temp2)
        : PrimaryReplicator(
                REInternalSettings::Create(nullptr, config),
                perfCounters,
                replicaId,
                hasPersistedState,
                endpointUniqueId,
                epoch,
                provider,
                ComProxyStatefulServicePartition(),
                Replicator::V1,
                transport,
                0, /* initial service progress */
                endpointUniqueId.PartitionId.ToString(),
                temp,
                temp2),
          expectedMessages_(),
          receivedMessages_(),
          receivedAcks_(),
          expectedAcks_(),
          lock_(),
          id_(replicaId)
    {
    }

    void PrimaryReplicatorHelper::PrimaryReplicatorWrapper::ReplicationAckMessageHandler(
        __in Transport::Message & message, ReplicationFromHeader const & fromHeader, PrimaryReplicatorWPtr primary)
    {
        FABRIC_SEQUENCE_NUMBER replRLSN;
        FABRIC_SEQUENCE_NUMBER replQLSN;
        FABRIC_SEQUENCE_NUMBER copyRLSN;
        FABRIC_SEQUENCE_NUMBER copyQLSN;
        int errorCodeValue;
        ReplicationTransport::GetAckFromMessage(message, replRLSN, replQLSN, copyRLSN, copyQLSN, errorCodeValue);

        {
            Common::AcquireExclusiveLock lock(this->lock_);
            this->AddAckCallerHoldsLock(receivedAcks_, fromHeader.DemuxerActor.ReplicaId, replRLSN, copyRLSN);
        }

        PrimaryReplicator::ReplicationAckMessageHandler(message, fromHeader, primary);
    }

    void PrimaryReplicatorHelper::PrimaryReplicatorWrapper::CopyContextMessageHandler(
        __in Transport::Message & message, ReplicationFromHeader const & fromHeader)
    {
        CopyContextOperationHeader header;
        wstring received;
        if (message.Headers.TryReadFirst(header))
        {
            StringWriter writer(received);
            writer.Write("CC:{0}:{1}", fromHeader.DemuxerActor, header.OperationMetadata.SequenceNumber);
        }

        {
            Common::AcquireExclusiveLock lock(this->lock_);
            ReplicatorTestWrapper::AddReceivedMessage(receivedMessages_, received);
        }

        PrimaryReplicator::CopyContextMessageHandler(message, fromHeader);
    }

    void PrimaryReplicatorHelper::PrimaryReplicatorWrapper::DumpAckTable(AckReplCopyMap const & mapToDump)
    {
        for(auto it = mapToDump.begin(); it != mapToDump.end(); ++it)
        {
            auto ackPair = it->second;
            Trace.WriteInfo(ReplCopyTestSource, "ID = {0} -> [{1}:{2}]", it->first, ackPair.first, ackPair.second);
        }
    }

    void PrimaryReplicatorHelper::PrimaryReplicatorWrapper::CheckAcks() const
    {
        Common::AcquireExclusiveLock lock(this->lock_);

        bool failed = false;
        for(auto it = receivedAcks_.begin(); it != receivedAcks_.end(); ++it)
        {
            auto ackPair = it->second;
            Trace.WriteInfo(ReplCopyTestSource, "{0} -> {1}:{2}", it->first, ackPair.first, ackPair.second);
            auto itExpected = expectedAcks_.find(it->first);
            if (itExpected == expectedAcks_.end())
            {
                Trace.WriteInfo(ReplCopyTestSource, "**** Received ACK from not expected secondary : {0}", it->first);
                failed = true;
            }
            else
            {
                auto ackPairExpected = itExpected->second;
                if (ackPair.first != ackPairExpected.first || ackPair.second != ackPairExpected.second)
                {
                    Trace.WriteInfo(ReplCopyTestSource, "**** Expected Ack != received: {0}:{1}",
                        ackPairExpected.first, ackPairExpected.second);
                    failed = true;
                }
            }
        }

        if (!failed && (expectedAcks_.size() != receivedAcks_.size()))
        {
            Trace.WriteInfo(ReplCopyTestSource, "***** Expected ACKs not received: {0} != {1}", expectedAcks_.size(), receivedAcks_.size());

            Trace.WriteInfo(ReplCopyTestSource, "***** Dumping received acks");
            DumpAckTable(receivedAcks_);

            Trace.WriteInfo(ReplCopyTestSource, "***** Dumping expected acks");
            DumpAckTable(expectedAcks_);

            failed = true;
        }

        VERIFY_IS_FALSE(failed, L"Check that expected Acks are received");
    }

    void PrimaryReplicatorHelper::PrimaryReplicatorWrapper::AddAckCallerHoldsLock(
        __in AckReplCopyMap & ackMap, int64 secondaryId, FABRIC_SEQUENCE_NUMBER replLSN, FABRIC_SEQUENCE_NUMBER copyLSN)
    {
        auto it = ackMap.find(secondaryId);
        if (it == ackMap.end())
        {
            ackMap.insert(pair<int64, AckReplCopyPair>(
                secondaryId, pair<FABRIC_SEQUENCE_NUMBER, FABRIC_SEQUENCE_NUMBER>(replLSN, copyLSN)));
        }
        else
        {
            pair<FABRIC_SEQUENCE_NUMBER, FABRIC_SEQUENCE_NUMBER> & ackPair = it->second;
            // Update the replication LSN, if the number id higher than the existing one
            if (replLSN > ackPair.first)
            {
                ackPair.first = replLSN;
            }

            if ((copyLSN == Constants::NonInitializedLSN) || (copyLSN > ackPair.second))
            { 
                ackPair.second = copyLSN; 
            }
        }
    }

    void PrimaryReplicatorHelper::PrimaryReplicatorWrapper::AddExpectedCopyContextMessage(
        ReplicationEndpointId const & fromDemuxer,
        FABRIC_SEQUENCE_NUMBER sequenceNumber) 
    {
        Common::AcquireExclusiveLock lock(this->lock_);

        wstring content;
        StringWriter writer(content);
        writer.Write("CC:{0}:{1}", fromDemuxer, sequenceNumber);
        expectedMessages_.push_back(content); 
    }

    void PrimaryReplicatorHelper::PrimaryReplicatorWrapper::CheckMessages()
    {
        Common::AcquireExclusiveLock lock(this->lock_);

        return ReplicatorTestWrapper::CheckMessages(id_, expectedMessages_, receivedMessages_); 
    }

    /***********************************
    * SecondaryReplicatorHelper methods
    **********************************/
    SecondaryReplicatorHelper::SecondaryReplicatorHelper(
        REConfigSPtr const & config,
        FABRIC_REPLICA_ID replicaId,
        bool hasPersistedState,
        int64 numberOfCopyOps,
        int64 numberOfCopyContextOps,
        Common::Guid partitionId,
        bool dropReplicationAcks)
        :
        secondaryId_(replicaId),
        perfCounters_(REPerformanceCounters::CreateInstance(partitionId.ToString(), replicaId)),
        endpointUniqueId_(partitionId, replicaId),
        secondaryTransport_(TestReplicateCopyOperations::CreateTransport(config)),
        endpoint_(),
        wrapper_()
    {
        ComProxyStateProvider stateProvider = move(ComTestStateProvider::GetProvider(
            numberOfCopyOps, 
            numberOfCopyContextOps,
            *this));
        IReplicatorHealthClientSPtr temp;
        ApiMonitoringWrapperSPtr temp2 = ApiMonitoringWrapper::Create(temp, REInternalSettings::Create(nullptr, config), partitionId, endpointUniqueId_);
        wrapper_ = std::shared_ptr<SecondaryReplicatorWrapper>(new SecondaryReplicatorWrapper(
            config, perfCounters_, secondaryId_, hasPersistedState, endpointUniqueId_, secondaryTransport_, stateProvider, dropReplicationAcks, temp, temp2));

        secondaryTransport_->GeneratePublishEndpoint(endpointUniqueId_, endpoint_);
        wrapper_->Open();
    }

    void SecondaryReplicatorHelper::StartGetOperation(OperationStreamSPtr const & stream)
    {
        for(;;)
        {
            auto op = stream->BeginGetOperation(
                [this, stream](AsyncOperationSPtr const& asyncOp)->void 
                { 
                    if(!asyncOp->CompletedSynchronously && EndGetOperation(asyncOp, stream))
                    {
                        this->StartGetOperation(stream);
                    }
                }, 
                wrapper_->CreateAsyncOperationRoot());

            if(!op->CompletedSynchronously || (!EndGetOperation(op, stream)))
            {
                break;
            }
        }
    }

    bool SecondaryReplicatorHelper::EndGetOperation(
        AsyncOperationSPtr const & asyncOp,
        OperationStreamSPtr const & stream)
    {
        IFabricOperation * operation = nullptr;
        ErrorCode error = stream->EndGetOperation(asyncOp, operation);
        if (!error.IsSuccess())
        {
            VERIFY_FAIL(L"GetOperation completed successfully.");
        }

        if (operation && operation->get_Metadata()->Type != FABRIC_OPERATION_TYPE_END_OF_STREAM)
        {
            FABRIC_OPERATION_METADATA const *opMetadata = operation->get_Metadata();

            ComTestOperation::WriteInfo(
                ReplCopyTestSource, 
                "{0}: GetOperation returned {1} type, {2} sequence number.", 
                endpointUniqueId_,
                opMetadata->Type, 
                opMetadata->SequenceNumber);

            ULONG bufferCount = 0;
            FABRIC_OPERATION_DATA_BUFFER const * replicaBuffers = nullptr;

            VERIFY_SUCCEEDED(operation->GetData(&bufferCount, &replicaBuffers));
            VERIFY_IS_TRUE(bufferCount <= 1);

            if (opMetadata->SequenceNumber <= 0)
            {
                VERIFY_FAIL(L"EndGetOperation returned non-null op with invalid lsn");
            }

            operation->Acknowledge();
            operation->Release();
            return true;
        }
        else
        {
            ComTestOperation::WriteInfo(ReplCopyTestSource, "{0}: Received NULL operation.", stream->Purpose);

            if (operation)
            {
                VERIFY_IS_TRUE(operation->get_Metadata()->Type == FABRIC_OPERATION_TYPE_END_OF_STREAM);
                operation->Acknowledge();
            }

            if (stream->Purpose == Constants::CopyOperationTrace)
            {
                StartReplicationOperationPump();
            }
        }

        return false;
    }
        
    void SecondaryReplicatorHelper::ProcessMessage(
        __in Transport::Message & message, __out Transport::ReceiverContextUPtr &) 
    {
        wstring const & action = message.Action;
        ReplicationFromHeader fromHeader;
        if (!message.Headers.TryReadFirst(fromHeader))
        {
            VERIFY_FAIL(L"Message doesn't have correct headers");
        }

        if (action == ReplicationTransport::ReplicationOperationAction)
        {
            wrapper_->ReplicationOperationMessageHandler(message, fromHeader);
        }
        else if (action == ReplicationTransport::CopyOperationAction)
        {
            wrapper_->CopyOperationMessageHandler(message, fromHeader);
        }
        else if (action == ReplicationTransport::StartCopyAction)
        {
            wrapper_->StartCopyMessageHandler(message, fromHeader);
        }
        else if (action == ReplicationTransport::CopyContextAckAction)
        {
            wrapper_->CopyContextAckMessageHandler(message, fromHeader);
        }
        else if (action == ReplicationTransport::RequestAckAction)
        {
            wrapper_->RequestAckMessageHandler(message, fromHeader);
        }
        else
        {
            VERIFY_FAIL(L"Unknown action");
        }
    }

    void SecondaryReplicatorHelper::Open()
    {
        secondaryTransport_->RegisterMessageProcessor(*this);
    }

    void SecondaryReplicatorHelper::Close()
    {
        ManualResetEvent closeCompleteEvent;
        wrapper_->BeginClose(
            false /*waitForOperationsToBeDrained*/,
            [this, &closeCompleteEvent](AsyncOperationSPtr const& operation) -> void
            {
                ErrorCode error = this->wrapper_->EndClose(operation);
                VERIFY_IS_TRUE(error.IsSuccess(), L"Secondary Replicator closed successfully.");
                closeCompleteEvent.Set();
            }, AsyncOperationSPtr());

        VERIFY_IS_TRUE(closeCompleteEvent.WaitOne(10000), L"Close should succeed");
        secondaryTransport_->UnregisterMessageProcessor(*this);
        secondaryTransport_->Stop();
    }

    SecondaryReplicatorHelper::SecondaryReplicatorWrapper::SecondaryReplicatorWrapper(
        REConfigSPtr const & config,
        REPerformanceCountersSPtr & perfCounters,
        FABRIC_REPLICA_ID replicaId,
        bool hasPersistedState,
        ReplicationEndpointId const & endpointUniqueId,
        ReplicationTransportSPtr const & transport,
        ComProxyStateProvider const & provider,
        bool dropReplicationAcks,
        IReplicatorHealthClientSPtr & temp,
        ApiMonitoringWrapperSPtr & temp2) 
        :
        SecondaryReplicator(
            REInternalSettings::Create(nullptr, config),
            perfCounters,
            replicaId,
            hasPersistedState,
            endpointUniqueId,
            provider,
            ComProxyStatefulServicePartition(),
            Replicator::V1,
            transport,
            endpointUniqueId.PartitionId.ToString(),
            temp,
            temp2),
        expectedMessages_(),
        receivedMessages_(),
        dropReplicationAcks_(dropReplicationAcks),
        dropCopyAcks_(false),
        dropCurrentAck_(dropReplicationAcks),
        dropCurrentStartCopyAck_(dropReplicationAcks),
        callback_(),
        id_(replicaId),
        lock_(),
        maxReplDropCount_(100),
        maxCopyDropCount_(100)
    {
    }

    void SecondaryReplicatorHelper::SecondaryReplicatorWrapper::SetDropCopyAcks(bool dropCopyAcks)
    {
        dropCopyAcks_ = dropCopyAcks;
    }

     void SecondaryReplicatorHelper::SecondaryReplicatorWrapper::ReplicationOperationMessageHandler(
        __in Transport::Message & message, 
        ReplicationFromHeader const & fromHeader)
    {
        vector<const_buffer> msgBuffers;
        bool isBodyValid = message.GetBody(msgBuffers);
        ASSERT_IF(!isBodyValid, "GetBody() in GetReplicationOperationFromMessage failed with message status {0}", message.Status);

        ReplicationOperationHeader header;
        wstring received;

        bool headerBodyDetected = ReplicationTransport::ReadInBodyOperationHeader(message, msgBuffers, header);
        if (headerBodyDetected || message.Headers.TryReadFirst(header))
        {
            StringWriter writer(received);
            writer.Write("R:{0}:{1}", fromHeader.DemuxerActor, header.OperationMetadata.SequenceNumber);
        }
            
        if (dropReplicationAcks_)
        {
            if (dropCurrentAck_ &&
                --maxReplDropCount_ > 0)
            {
                // Drop one of 2 consecutive ACKs
                ComTestOperation::WriteInfo(
                    ReplCopyTestSource,
                    "-----{0}: TEST drop REPL operation {1}",
                    id_,
                    received);
                dropCurrentAck_ = false;
                return;
            }

            dropCurrentAck_ = true;
        }

        {
            Common::AcquireExclusiveLock lock(this->lock_);
            ReplicatorTestWrapper::AddReceivedMessage(receivedMessages_, received);
        }

        SecondaryReplicator::ReplicationOperationMessageHandler(message, fromHeader);
    }

     void SecondaryReplicatorHelper::SecondaryReplicatorWrapper::StartCopyMessageHandler(
        __in Transport::Message & message, 
        ReplicationFromHeader const & fromHeader)
     {
        wstring received;
        StringWriter(received).Write("SC:{0}", fromHeader.DemuxerActor);
                  
        if (dropReplicationAcks_)
        {
            if (dropCurrentStartCopyAck_)
            {
                // Drop the first StartCopy, accept the next one
                ComTestOperation::WriteInfo(
                    ReplCopyTestSource,
                    "-----{0}: TEST drop StartCOPY {1}",
                    id_,
                    received);
                dropCurrentStartCopyAck_ = false;
                return;
            }

            dropCurrentStartCopyAck_ = true;
        }

        {
            Common::AcquireExclusiveLock lock(this->lock_);
            ReplicatorTestWrapper::AddReceivedMessage(receivedMessages_, received);
        }

        SecondaryReplicator::StartCopyMessageHandler(message, fromHeader);
     }

    void SecondaryReplicatorHelper::SecondaryReplicatorWrapper::CopyContextAckMessageHandler(
        __in Transport::Message & message, 
        ReplicationFromHeader const & fromHeader)
     {
        FABRIC_SEQUENCE_NUMBER lsn;
        int errorCodeValue;
        ReplicationTransport::GetCopyContextAckFromMessage(message, lsn, errorCodeValue);
        wstring received;
        StringWriter(received).Write("CCAck:{0}:{1}:{2}", fromHeader.DemuxerActor, lsn, errorCodeValue);
        Trace.WriteInfo(ReplCopyTestSource, "Received{ 0 }", received);
        SecondaryReplicator::CopyContextAckMessageHandler(message, fromHeader);
     }

    void SecondaryReplicatorHelper::SecondaryReplicatorWrapper::RequestAckMessageHandler(
        __in Transport::Message & message, 
        ReplicationFromHeader const & fromHeader)
     {
        wstring received;
        StringWriter(received).Write("RequestAck:{0}", fromHeader.DemuxerActor);
        Trace.WriteInfo(ReplCopyTestSource, "Received {0}", received);
        if (callback_)
        {
            callback_(message.Action);
        }

        SecondaryReplicator::RequestAckMessageHandler(message, fromHeader);
     }

     void SecondaryReplicatorHelper::SecondaryReplicatorWrapper::CopyOperationMessageHandler(
        __in Transport::Message & message, 
        ReplicationFromHeader const & fromHeader)
    {
         vector<const_buffer> msgBuffers;
         bool isBodyValid = message.GetBody(msgBuffers);
         ASSERT_IF(!isBodyValid, "GetBody() in GetReplicationOperationFromMessage failed with message status {0}", message.Status);

        CopyOperationHeader header;
        wstring received;
        bool headerBodyDetected = ReplicationTransport::ReadInBodyOperationHeader(message, msgBuffers, header);
        if (headerBodyDetected || message.Headers.TryReadFirst(header))
        {
            StringWriter writer(received);
            writer.Write("C:{0}:{1}", fromHeader.DemuxerActor, header.OperationMetadata.SequenceNumber);
        }

        if (dropCopyAcks_ &&
            --maxCopyDropCount_ > 0)
        {
            ComTestOperation::WriteInfo(
                ReplCopyTestSource,
                "-----{0}: TEST drop COPY operation {1}",
                id_,
                received);
        }
        else
        {
            {
                Common::AcquireExclusiveLock lock(this->lock_);
                ReplicatorTestWrapper::AddReceivedMessage(receivedMessages_, received);
            }

            SecondaryReplicator::CopyOperationMessageHandler(message, fromHeader);
        }
    }

    void SecondaryReplicatorHelper::SecondaryReplicatorWrapper::AddExpectedCopyMessage(
        ReplicationEndpointId const & fromDemuxer,
        FABRIC_SEQUENCE_NUMBER seq)
    { 
        wstring content;
        StringWriter writer(content);
        writer.Write("C:{0}:{1}", fromDemuxer, seq);

        Common::AcquireExclusiveLock lock(this->lock_);
        expectedMessages_.push_back(content);
    }

    void SecondaryReplicatorHelper::SecondaryReplicatorWrapper::AddExpectedReplicationMessage(
        ReplicationEndpointId const & fromDemuxer,
        FABRIC_SEQUENCE_NUMBER sequenceNumber)
    { 
        wstring content;
        StringWriter writer(content);
        writer.Write("R:{0}:{1}", fromDemuxer, sequenceNumber);

        Common::AcquireExclusiveLock lock(this->lock_);
        expectedMessages_.push_back(content);
    }
    
    void SecondaryReplicatorHelper::SecondaryReplicatorWrapper::AddExpectedStartCopyMessage(
        ReplicationEndpointId const & fromDemuxer)
    { 
        wstring content;
        StringWriter writer(content);
        writer.Write("SC:{0}", fromDemuxer);

        Common::AcquireExclusiveLock lock(this->lock_);
        expectedMessages_.push_back(content);
    }

    void SecondaryReplicatorHelper::SecondaryReplicatorWrapper::AddExpectedCopyContextAckMessage(
        ReplicationEndpointId const & fromDemuxer,
        FABRIC_SEQUENCE_NUMBER sequenceNumber)
    { 
        wstring content;
        StringWriter writer(content);
        writer.Write("CCAck:{0}:{1}", fromDemuxer, sequenceNumber);

        Common::AcquireExclusiveLock lock(this->lock_);
        expectedMessages_.push_back(content);
    }

    void SecondaryReplicatorHelper::SecondaryReplicatorWrapper::CheckMessages()
    { 
        Common::AcquireExclusiveLock lock(this->lock_);
        return ReplicatorTestWrapper::CheckMessages(id_, expectedMessages_, receivedMessages_); 
    }

    /***********************************
    * ReplicatorTestWrapper methods
    **********************************/
    void ReplicatorTestWrapper::CheckMessages(
        FABRIC_REPLICA_ID id,
        __in vector<wstring> & expectedMessages,
        __in vector<wstring> & receivedMessages)
    {
        // Wait for the messages to be received
        int tryCount = 5;
        while(--tryCount > 0)
        {
            if (expectedMessages.size() <= receivedMessages.size())
            {
                break;
            }

            Sleep(300);
        }

        std::sort(expectedMessages.begin(), expectedMessages.end());
        std::sort(receivedMessages.begin(), receivedMessages.end());

        size_t size = std::min(expectedMessages.size(), receivedMessages.size());
        Trace.WriteInfo(ReplCopyTestSource, "{0}: Check expected messages:", id);
        for(size_t i = 0; i < size; ++i)
        {
            if (expectedMessages[i] != receivedMessages[i])
            {
                Trace.WriteInfo(ReplCopyTestSource, "Expected message != received: {0} != {1}",
                    expectedMessages[i], receivedMessages[i]);
            }
            else
            {
                Trace.WriteInfo(ReplCopyTestSource, "{0}", receivedMessages[i]);
            }
        }

        for(size_t i = size; i < expectedMessages.size(); ++i)
        {
            Trace.WriteInfo(ReplCopyTestSource, "***** Expected message not received: {0}", expectedMessages[i]);
        }
        
        for(size_t i = size; i < receivedMessages.size(); ++i)
        {
            Trace.WriteInfo(ReplCopyTestSource, "**** Received message not expected: {0}", receivedMessages[i]);
        }

        VERIFY_ARE_EQUAL(expectedMessages, receivedMessages);
    }

    void ReplicatorTestWrapper::AddReceivedMessage(
        __in vector<wstring> & receivedMessages, 
        wstring const & content)
    {
        // Ignore duplicate messages, 
        // they could happen because of retries on timer
        auto it = std::find(receivedMessages.begin(), receivedMessages.end(), content);
        if (it == receivedMessages.end())
        {
            receivedMessages.push_back(content);
        }
    }
    
    void ReplicatorTestWrapper::CreatePrimary(
        int64 numberOfCopyOps,
        int64 numberOfCopyContextOps,
        bool smallRetryInterval, 
        FABRIC_EPOCH const & epoch,
        bool hasPersistedState,
        Common::Guid partitionId,
        __out PrimaryReplicatorHelperSPtr & primary)
    {
        REConfigSPtr primaryConfig = TestReplicateCopyOperations::CreateGenericConfig();
        // Minimize the retry interval
        if (smallRetryInterval)
        {
            primaryConfig->RetryInterval = TimeSpan::FromMilliseconds(200);
        }
       
        primary = make_shared<PrimaryReplicatorHelper>(
            primaryConfig, 
            hasPersistedState,
            epoch, 
            numberOfCopyOps,
            numberOfCopyContextOps,
            partitionId);
        primary->Open();
    }

    void ReplicatorTestWrapper::CreateSecondary(
        int64 primaryId,
        uint index,
        int64 numberOfCopyOps,
        int64 numberOfCopyContextOps,
        bool dropReplicationAcks,
        bool hasPersistedState,
        Common::Guid partitionId,
        __out int64 & secondaryId,
        __out SecondaryReplicatorHelperSPtr & secondary)
    {
        REConfigSPtr config = TestReplicateCopyOperations::CreateGenericConfig();
        
        secondaryId = primaryId + index + 1;

        secondary = make_shared<SecondaryReplicatorHelper>(
            config, 
            secondaryId, 
            hasPersistedState,
            numberOfCopyOps,
            numberOfCopyContextOps,
            partitionId,
            dropReplicationAcks);
        secondary->Open();
        // Start copy operation pump, which will start the replication pump when 
        // last copy operation is received
        secondary->StartCopyOperationPump();
    }

    void ReplicatorTestWrapper::CreateConfiguration(
        int numberSecondaries,
        int numberIdles, 
        FABRIC_EPOCH const & epoch,
        int64 numberOfCopyOps,
        int64 numberOfCopyContextOps,
        bool hasPersistedState,
        Common::Guid partitionId,
        __out PrimaryReplicatorHelperSPtr & primary,
        __out vector<SecondaryReplicatorHelperSPtr> & secondaries,
        __out vector<int64> & secondaryIds,
        bool dropReplicationAcks)
    {
        CreatePrimary(numberOfCopyOps, numberOfCopyContextOps, dropReplicationAcks, epoch, hasPersistedState, partitionId, primary);

        for(int i = 0; i < numberIdles + numberSecondaries; ++i)
        {
            int64 secondaryId;
            SecondaryReplicatorHelperSPtr secondary;
            CreateSecondary(primary->PrimaryId, i, numberOfCopyOps, numberOfCopyContextOps, dropReplicationAcks, hasPersistedState, partitionId, secondaryId, secondary);
            secondaryIds.push_back(secondaryId);
            secondaries.push_back(move(secondary));
        }

        // Build all replicas
        int expectedReplSeq = 0;
        if (hasPersistedState && numberOfCopyContextOps == -1)
        {
            BuildIdles(expectedReplSeq, primary, secondaries, secondaryIds, numberOfCopyOps, 0);
        }
        else
        {
            BuildIdles(expectedReplSeq, primary, secondaries, secondaryIds, numberOfCopyOps, numberOfCopyContextOps);
        }
        
        // Change active replicas
        if (numberSecondaries > 0)
        {
            PromoteIdlesToSecondaries(0, numberSecondaries, primary, secondaries, secondaryIds);
        }
    }
     
    void ReplicatorTestWrapper::PromoteIdlesToSecondaries(
        int currentNumberSecondaries,
        int newNumberSecondaries,
        __in PrimaryReplicatorHelperSPtr const & primary,
        __in vector<SecondaryReplicatorHelperSPtr> & secondaries,
        vector<int64> const & secondaryIds)
    {
        ReplicaInformationVector replicas;
        // Transform idles to secondaries for as many secondaries are required
        for(int i = currentNumberSecondaries; i < newNumberSecondaries; ++i)
        {
            ReplicaInformation replica(
                secondaryIds[i],
                ::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY,
                secondaries[i]->ReplicationEndpoint,
                false);
            
            replicas.push_back(std::move(replica));
        }
                
        ULONG quorum = static_cast<ULONG>((replicas.size() + 1) / 2 + 1);
        ULONG previousQuorum = 0;
        ReplicaInformationVector previousReplicas;
        ErrorCode error = primary->Replicator.UpdateCatchUpReplicaSetConfiguration(
            previousReplicas, 
            previousQuorum, 
            replicas, 
            quorum);
        if (!error.IsSuccess())
        {
            VERIFY_FAIL(L"UpdateCatchUpReplicaSetConfiguration should have succeeded");
        }
        error = primary->Replicator.UpdateCurrentReplicaSetConfiguration(
            replicas, 
            quorum);
        if (!error.IsSuccess())
        {
            VERIFY_FAIL(L"UpdateCurrentReplicaSetConfiguration should have succeeded");
        }
    }

    void ReplicatorTestWrapper::BuildIdles(
        int64 startReplSeq,
        __in PrimaryReplicatorHelperSPtr & primary,
        __in vector<SecondaryReplicatorHelperSPtr> & secondaries,
        vector<int64> const & secondaryIds,
        int64 numberOfCopyOps,
        int64 numberOfCopyContextOps)
    {
        vector<ReplicaInformation> replicas;
        // Transform idles to secondaries for as many secondaries are required
        // Secondary should receive StartCopy and all copy messages (including last NULL).
        for(size_t i = 0; i < secondaries.size(); ++i)
        {
             ReplicaInformation replica(
                secondaryIds[i],
                ::FABRIC_REPLICA_ROLE_IDLE_SECONDARY,
                secondaries[i]->ReplicationEndpoint,
                false,
                Constants::NonInitializedLSN,
                Constants::NonInitializedLSN);
            
            replicas.push_back(std::move(replica));
            
            primary->Replicator.AddExpectedAck(secondaries[i]->SecondaryId, startReplSeq, Constants::NonInitializedLSN);
            for (int64 j = 1; j <= numberOfCopyOps + 1; ++j)
            {
                secondaries[i]->Replicator.AddExpectedCopyMessage(primary->EndpointUniqueId, j);
            }

            if (numberOfCopyContextOps >= 0)
            {
                for (int64 j = 1; j <= numberOfCopyContextOps + 1; ++j)
                {
                    primary->Replicator.AddExpectedCopyContextMessage(secondaries[i]->EndpointUniqueId, j);
                }
            }

            secondaries[i]->Replicator.AddExpectedStartCopyMessage(primary->EndpointUniqueId);
        }

        Copy(*(primary.get()), replicas);    
    }

    void ReplicatorTestWrapper::BeginAddIdleCopyNotFinished(
        int index,
        int64 numberOfCopyOps,
        int64 numberOfCopyContextOps,
        PrimaryReplicatorHelperSPtr const & primary,
        bool hasPersistedState,
        Common::Guid partitionId,
        __out SecondaryReplicatorHelperSPtr & secondary,
        __out int64 & secondaryId,
        __in ManualResetEvent & copyDoneEvent)
    {
        CreateSecondary(primary->PrimaryId, index, numberOfCopyOps, numberOfCopyContextOps, false, hasPersistedState, partitionId, secondaryId, secondary);
      
        ReplicaInformation replica(
            secondaryId,
            ::FABRIC_REPLICA_ROLE_IDLE_SECONDARY,
            secondary->ReplicationEndpoint,
            false);
      
        // Drop copy operations, to force retry sending them
        secondary->Replicator.SetDropCopyAcks(true);
        
        PrimaryReplicatorHelper & repl = *(primary.get());
        FABRIC_REPLICA_ID replicaId = replica.Id;
        repl.Replicator.BeginBuildReplica(replica,
            [&repl, replicaId, &copyDoneEvent](AsyncOperationSPtr const& operation) -> void
            {
                ErrorCode error = repl.Replicator.EndBuildReplica(operation);
                if (!error.IsSuccess())
                {
                    VERIFY_FAIL_FMT("Copy failed with error {0}.", error);
                }

                Trace.WriteInfo(ReplCopyTestSource, "Copy done for replica {0}.", replicaId);
                copyDoneEvent.Set();
            }, AsyncOperationSPtr());

        for (int64 i = 1; i <= numberOfCopyOps + 1; ++i)
        {
            secondary->Replicator.AddExpectedCopyMessage(primary->EndpointUniqueId, i);
        }

        if (numberOfCopyContextOps >= 0)
        {
            for (int64 j = 1; j <= numberOfCopyContextOps + 1; ++j)
            {
                primary->Replicator.AddExpectedCopyContextMessage(secondary->EndpointUniqueId, j);
            }
        }

        secondary->Replicator.AddExpectedStartCopyMessage(primary->EndpointUniqueId);

        // Don't wait for the copy to finish -
        // It won't, until drop acks is set to false
    }

    void ReplicatorTestWrapper::EndAddIdleCopyNotFinished(
        __in SecondaryReplicatorHelperSPtr & secondary,
        __in ManualResetEvent & copyDoneEvent)
    {
        secondary->Replicator.SetDropCopyAcks(false);
        // The primary should retry sending copy operations
        // until it gets ACK for the last one.
        copyDoneEvent.WaitOne();
    }
    
     void ReplicatorTestWrapper::WaitForProgress(
        __in PrimaryReplicatorHelper & repl, FABRIC_REPLICA_SET_QUORUM_MODE catchUpMode, bool expectTimeout)
    {
        ManualResetEvent completeEvent;
        AsyncOperationSPtr asyncOp;
        volatile bool finished = false;
        asyncOp = repl.Replicator.BeginWaitForCatchUpQuorum(
            catchUpMode,
            [&repl, &completeEvent, &finished, expectTimeout](AsyncOperationSPtr const& operation) -> void
            {
                ErrorCode error = repl.Replicator.EndWaitForCatchUpQuorum(operation);
                finished = true;
                if (!expectTimeout)
                {
                    VERIFY_IS_TRUE(error.IsSuccess(), L"Wait for full progress completed successfully.");
                }
                else
                {
                    Trace.WriteInfo(ReplCopyTestSource, "WaitForFullProgress completed with error {0}", error);
                }
                completeEvent.Set();
            }, AsyncOperationSPtr());

        if (expectTimeout)
        {
            completeEvent.WaitOne(2000);
            VERIFY_IS_TRUE(!finished, L"WaitForProgress should have timed out");
            asyncOp->Cancel();
            completeEvent.WaitOne();
            VERIFY_IS_TRUE(finished, L"WaitForProgress completed after cancel");
        }
        else
        {
            completeEvent.WaitOne();
            VERIFY_IS_TRUE(finished, L"WaitForProgress completed in time");
        }
    }

    void ReplicatorTestWrapper::Replicate(
        __in PrimaryReplicatorHelper & repl, 
        FABRIC_SEQUENCE_NUMBER startExpectedSequenceNumber,
        uint numberOfOperations)
    {
        ManualResetEvent replicateDoneEvent;
        volatile ULONGLONG count = numberOfOperations;
        FABRIC_SEQUENCE_NUMBER sequenceNumber;

        for(FABRIC_SEQUENCE_NUMBER i = 0; i < numberOfOperations; ++i)
        {
            auto operation = make_com<ComTestOperation,IFabricOperationData>(L"ReplicateCopyTest - test replicate operation");
            repl.Replicator.BeginReplicate(
                std::move(operation),
                sequenceNumber,
                [&repl, &count, &replicateDoneEvent](AsyncOperationSPtr const& operation) -> void
                {
                    ErrorCode error = repl.Replicator.EndReplicate(operation);
                    if (!error.IsSuccess())
                    {
                        VERIFY_FAIL_FMT("Replication failed with error {0}.", error);
                    }

                    if (0 == InterlockedDecrement64((volatile LONGLONG *)&count))
                    {
                        replicateDoneEvent.Set();
                    }
                }, AsyncOperationSPtr());

            Trace.WriteInfo(ReplCopyTestSource, "Replicate operation returned {0} sequence number.", sequenceNumber);
            VERIFY_ARE_EQUAL(i + startExpectedSequenceNumber, sequenceNumber, 
                L"Replicate operation returned expected sequence number");
        }

        replicateDoneEvent.WaitOne();
    }

    void ReplicatorTestWrapper::Copy(
        __in PrimaryReplicatorHelper & repl,
        vector<ReplicaInformation> const & replicas)
    {
        ManualResetEvent copyDoneEvent;
        volatile size_t count = replicas.size();

        for(size_t i = 0; i < replicas.size(); ++i)
        {
            FABRIC_REPLICA_ID replicaId = replicas[i].Id;
            repl.Replicator.BeginBuildReplica(replicas[i],
                [&repl, replicaId, &count, &copyDoneEvent](AsyncOperationSPtr const& operation) -> void
                {
                    ErrorCode error = repl.Replicator.EndBuildReplica(operation);
                    if (!error.IsSuccess())
                    {
                        VERIFY_FAIL_FMT("Copy failed with error {0}.", error);
                    }

                    Trace.WriteInfo(ReplCopyTestSource, "Copy done for replica {0}.", replicaId);
                    if (0 == InterlockedDecrement64((volatile LONGLONG *)&count))
                    {
                        copyDoneEvent.Set();
                    }

                }, AsyncOperationSPtr());
        }
    
        copyDoneEvent.WaitOne();
    }
}
