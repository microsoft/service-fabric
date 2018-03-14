// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability::ReplicationComponent;

namespace FabricTest
{
    class SGStatefulService;
    typedef std::shared_ptr<SGStatefulService> SGStatefulServiceSPtr;

    class SGStatefulService : 
        public Common::ComponentRoot
    {
        DENY_COPY(SGStatefulService);

    public:
        //
        // Constructor/Destructor.
        //
        SGStatefulService(
            __in NodeId nodeId,
            __in LPCWSTR serviceType,
            __in LPCWSTR serviceName,
            __in ULONG initializationDataLength,
            __in const byte* initializationData,
            __in FABRIC_REPLICA_ID replicaId);
        virtual ~SGStatefulService();
        //
        // Initialization/Cleanup methods.
        //
        virtual HRESULT FinalConstruct(
            __in bool returnEmptyCopyContext,
            __in bool returnEmptyCopyState,
            __in bool returnNullCopyContext,
            __in bool returnNullCopyState,
            __in std::wstring const & initData);

        //
        // Methods exposed to SGComStatefulService
        //
        HRESULT OnOpen(
            __in ComPointer<IFabricStatefulServicePartition> && statefulPartition,
            __in ComPointer<IFabricStateReplicator> && stateReplicator);

        HRESULT OnChangeRole(
            __in FABRIC_REPLICA_ROLE newRole);

        HRESULT OnClose();

        void OnAbort();

        HRESULT OnDataLoss();

        HRESULT UpdateEpoch(
            __in FABRIC_EPOCH const* epoch, 
            __in FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber);

        HRESULT GetCurrentProgress(
            __out FABRIC_SEQUENCE_NUMBER* sequenceNumber);

        HRESULT GetCopyContext(
            __out IFabricOperationDataStream** copyContextEnumerator);

        HRESULT GetCopyState(
            __in FABRIC_SEQUENCE_NUMBER uptoSequenceNumber,
            __in IFabricOperationDataStream* copyContextEnumerator,
            __out IFabricOperationDataStream** copyStateEnumerator);

        HRESULT OnAtomicGroupCommit(
            __in FABRIC_ATOMIC_GROUP_ID atomicGroupId, 
            __in FABRIC_SEQUENCE_NUMBER commitSequenceNumber);

        HRESULT OnAtomicGroupRollback(
            __in FABRIC_ATOMIC_GROUP_ID atomicGroupId, 
            __in FABRIC_SEQUENCE_NUMBER rollbackSequenceNumber);

        HRESULT OnUndoProgress(__in FABRIC_SEQUENCE_NUMBER fromCommitSequenceNumber);

        //
        // Properties
        //
        __declspec(property(get=get_ServiceName)) std::wstring const & ServiceName;
        std::wstring const & get_ServiceName() { return serviceName_; }

        __declspec(property(get=get_Settings)) TestCommon::CommandLineParser & Settings;
        TestCommon::CommandLineParser & get_Settings() { return *settings_; }

        __declspec(property(get=get_Partition)) ComPointer<IFabricStatefulServicePartition> const & Partition;
        ComPointer<IFabricStatefulServicePartition> const & get_Partition() { return statefulPartition_; }

        //
        // Inject failure related
        //
        bool ShouldFailOn(ApiFaultHelper::ComponentName compName, std::wstring operationName, ApiFaultHelper::FaultType faultType=ApiFaultHelper::Failure);
        bool IsSignalSet(std::wstring strFlag);

        static std::wstring const StatefulServiceType;
        static std::wstring const StatefulServiceECCType;
        static std::wstring const StatefulServiceECSType;
        static std::wstring const StatefulServiceNCCType;
        static std::wstring const StatefulServiceNCSType;

    protected:
        //
        // Helper methods
        //
        __inline BOOLEAN IsSecondaryRole(FABRIC_REPLICA_ROLE role) { return role == FABRIC_REPLICA_ROLE_IDLE_SECONDARY || role == FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY; }
        __inline BOOLEAN IsPrimaryRole(FABRIC_REPLICA_ROLE role) { return role == FABRIC_REPLICA_ROLE_PRIMARY; }
        //
        // Operation processing.
        //
        virtual HRESULT ApplyState(
            __in BOOLEAN isCopy,
            __in FABRIC_SEQUENCE_NUMBER operationSequenceNumber, 
            __in ULONGLONG state);
        virtual HRESULT ApplyStateInPrimary(
            __in FABRIC_SEQUENCE_NUMBER operationSequenceNumber, 
            __in ULONGLONG state);
        virtual HRESULT GetCurrentState(
            __out ULONGLONG* state);

        //
        // Thread pool callbacks
        //
        static void CALLBACK ThreadPoolWorkItemCatchUpCopyAndReplication(
            __inout PTP_CALLBACK_INSTANCE Instance,
            __inout_opt PVOID Context,
            __inout PTP_WORK Work);

        static void CALLBACK ThreadPoolWorkItemReplication(
            __inout PTP_CALLBACK_INSTANCE Instance,
            __inout_opt PVOID Context,
            __inout PTP_WORK Work);

        static void CALLBACK ThreadPoolWorkItemReportMetrics(
            __inout PTP_CALLBACK_INSTANCE Instance,
            __inout_opt PVOID Context,
            __inout PTP_WORK Work);

        //
        // Workload methods
        //
        HRESULT CatchUpCopyProcessing();
        HRESULT ReplicationProcessing();
        HRESULT CatchUpReplicationProcessing();

        bool FinishReplicateState(AsyncOperationSPtr const & asyncOperation);
        bool FinishReplicateAtomicGroupOperation(AsyncOperationSPtr const & asyncOperation, bool expectCompletedSynchronously);
        bool CommitAtomicGroup(FABRIC_ATOMIC_GROUP_ID groupId, IFabricOperationData* data);
        bool FinishReplicateAtomicGroupCommit(AsyncOperationSPtr const & asyncOperation, bool expectCompletedSynchronously);
        bool ReplicateAtomicGroup();
        bool ReplicaNextState();
        bool FinishProcessingCatchupOperation(AsyncOperationSPtr const & asyncOperation, bool& isLast);
        bool ProcessNextCatchupOperation(Common::ComPointer<IFabricOperationStream> const & stream, bool isCopy);

        void PrimaryReportMetricsRandom();

        //
        // Additional methods
        //
        HRESULT CreateWorkload(
            __in FABRIC_REPLICA_ROLE newRole);
        HRESULT StopWorkload();
        void ResetReplicateStatus() { replicateEmptyOperationData_ = true; }

    private:
        NodeId nodeId_;
        std::wstring serviceName_;
        std::wstring serviceType_;
        FABRIC_PARTITION_ID partitionId_;
        FABRIC_REPLICA_ID replicaId_;
        FABRIC_REPLICA_ROLE currentReplicaRole_;
        FABRIC_REPLICA_ROLE majorReplicaRole_; // used for validation

        ComPointer<IFabricStatefulServicePartition> statefulPartition_;
        ComPointer<IFabricStateReplicator> stateReplicator_;

        ULONGLONG state_;
        FABRIC_SEQUENCE_NUMBER currentSequenceNumber_;
        ULONG operationCount_;
        Common::RwLock lock_;

        PTP_WORK workItemReplication_;
        Common::ComponentRootSPtr selfReportMetrics_;
        PTP_WORK workItemReportMetrics_;
        Common::ComponentRootSPtr selfReplication_;
        PTP_WORK workItemCatchUpCopy_;
        Common::ComponentRootSPtr selfCatchUpCopy_;

        Common::ManualResetEvent catchupDone_;
        Common::ManualResetEvent replicationDone_;

        Common::ComPointer<IFabricOperationStream> copyStream_;
        Common::ComPointer<IFabricOperationStream> replicationStream_;

        bool returnEmptyCopyContext_;
        bool returnEmptyCopyState_;
        bool returnNullCopyContext_;
        bool returnNullCopyState_;
        bool emptyOperationDataSeen_;
        bool replicateEmptyOperationData_;

        bool isStopping_;

        bool isGetCopyStateCalled_;

        Common::Random random_;

        class SGCopyContextReceiver;

        int rollbackCount_;
        FABRIC_SEQUENCE_NUMBER previousEpochLastLsn_;

        std::unique_ptr<TestCommon::CommandLineParser> settings_;

        static Common::StringLiteral const TraceSource;
    };
};
