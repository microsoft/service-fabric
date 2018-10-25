// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class ReplicatedStore::SecondaryPump 
        : public Common::ComponentRoot
        , public PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ReplicatedStore>
    {
        using PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ReplicatedStore>::TraceId;
        using PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ReplicatedStore>::get_TraceId;

        DENY_COPY(SecondaryPump);

    public:
        SecondaryPump(__in ReplicatedStore &, Common::ComPointer<IFabricStateReplicator> const &);
        ~SecondaryPump();

        void Start();
        void Cancel();
        void FaultReplica(
            FABRIC_FAULT_TYPE,
            Common::ErrorCode const &,
            std::wstring const & message,
            bool scheduleDrain);

        Common::ErrorCode ApplyLocalOperations(
            CopyOperation &&,
            ::FABRIC_SEQUENCE_NUMBER mockLSN);

        void SetLogicalCopyStreamComplete(bool);

    public:

        // Test Hooks
        void Test_FaultStreamAndStopSecondaryPump(FABRIC_FAULT_TYPE faultType);
        bool Test_GetCopyStreamComplete();
#if defined(PLATFORM_UNIX)
        bool Test_IsPumpStateClosed();
#endif

    private:
        class ComPumpAsyncOperation : public Common::ComProxyAsyncOperation
        {
            DENY_COPY(ComPumpAsyncOperation);

        public:
            ComPumpAsyncOperation(
                Common::ComPointer<IFabricOperationStream> const & operationStream,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & root);

            ~ComPumpAsyncOperation();

            static Common::ErrorCode End(
                Common::AsyncOperationSPtr const &, 
                __out Common::ComPointer<IFabricOperation> &);
            
        protected:
            HRESULT BeginComAsyncOperation(__in IFabricAsyncOperationCallback *, __out IFabricAsyncOperationContext **);
            HRESULT EndComAsyncOperation(__in IFabricAsyncOperationContext *);

        private:
            Common::ComPointer<IFabricOperationStream> operationStream_;
            Common::ComPointer<IFabricOperation> pumpedOperation_;
            bool doEndOfStreamOperationAck_;
        };

        class CommitAndAcknowledgeAsyncOperation
            : public Common::AsyncOperation
            , public PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ReplicatedStore>
        {
            DENY_COPY(CommitAndAcknowledgeAsyncOperation);

        public:
            CommitAndAcknowledgeAsyncOperation(
                ReplicatedStore::SecondaryPump & owner,
                IStoreBase::TransactionSPtr && transactionSPtr,
                Common::ComPointer<IFabricOperation> const &operationCPtr,
                FABRIC_SEQUENCE_NUMBER operationLsn,
                std::shared_ptr<std::vector<ReplicationOperation>> const &,
                size_t updatedTombstoneCount,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & root);

            ~CommitAndAcknowledgeAsyncOperation();
            
            __declspec(property(get=get_OperationLSN)) ::FABRIC_SEQUENCE_NUMBER OperationLSN;
            ::FABRIC_SEQUENCE_NUMBER get_OperationLSN() const { return operationLSN_; }

        protected:
            virtual void OnStart(Common::AsyncOperationSPtr const& proxySPtr);

        private:
            void OnCommitComplete(Common::AsyncOperationSPtr const& asyncOperation, bool expectedCompletedSynchronously);

            ReplicatedStore::SecondaryPump & owner_;
            IStoreBase::TransactionSPtr transactionSPtr_;
            Common::ComPointer<IFabricOperation> operationCPtr_;
            ::FABRIC_SEQUENCE_NUMBER operationLSN_;
            std::shared_ptr<std::vector<ReplicationOperation>> replicationOperations_;
            size_t updatedTombstoneCount_;
            Common::Stopwatch localCommitStopwatch_;
        };

        enum SecondaryPumpState { PumpNotStarted, PumpCopy, PumpReplication, PumpClosed };

        void ScheduleDrainOperations();
        void DrainOperations();
        Common::AsyncOperationSPtr PumpOperation();
        void OnPumpOperationComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        Common::ErrorCode ProcessEndOfCopyStream(
            Common::AsyncOperationSPtr const &, 
            Common::ComPointer<IFabricOperation> const & operationCPtr,
            __out std::wstring &);

        Common::ErrorCode InnerProcessEndOfCopyStream(
            Common::AsyncOperationSPtr const &, 
            Common::ComPointer<IFabricOperation> const & operationCPtr,
            __out std::wstring &);

        Common::ErrorCode ProcessEndOfReplicationStream(
            Common::AsyncOperationSPtr const &, 
            Common::ComPointer<IFabricOperation> const & operationCPtr,
            __out std::wstring &);

        Common::ErrorCode GetOperationBuffers(
            Common::ComPointer<IFabricOperation> const & operationCPtr,
            __out std::vector<Serialization::FabricIOBuffer> & buffers,
            __out ULONG & totalSize,
            __out std::wstring & errorMessage);

        Common::ErrorCode ProcessCopyOperation(
            Common::AsyncOperationSPtr const & pumpOperation,
            Common::ComPointer<IFabricOperation> const & operationCPtr,
            ::FABRIC_SEQUENCE_NUMBER operationLsn,
            __out std::vector<ReplicationOperation> & replicationOperations,
            __out ::FABRIC_SEQUENCE_NUMBER & lastQuorumAcked,
            __out std::wstring & errorMessage);

        Common::ErrorCode ProcessReplicationOperation(
            Common::AsyncOperationSPtr const & pumpOperation,
            Common::ComPointer<IFabricOperation> const & operationCPtr,
            ::FABRIC_SEQUENCE_NUMBER operationLsn,
            __out std::vector<ReplicationOperation> & replicationOperations,
            __out ::FABRIC_SEQUENCE_NUMBER & lastQuorumAcked,
            __out std::wstring & errorMessage);

        void RestartDrainOperationsIfNeeded(Common::AsyncOperationSPtr const &);

        Common::ErrorCode TryCleanupCopyLocalStore(Common::AsyncOperationSPtr const& pumpOperation);

        Common::ErrorCode ApplyFileStreamFullCopyOperation(
            CopyType::Enum,
            ::FABRIC_SEQUENCE_NUMBER, 
            std::unique_ptr<FileStreamCopyOperationData> const &,
            __out std::wstring &);

        Common::ErrorCode ApplyOperationsWithRetry(
            Common::ComPointer<IFabricOperation> const &,
            std::shared_ptr<std::vector<ReplicationOperation>> const &,
            ::FABRIC_SEQUENCE_NUMBER replicationLSN,
            ::FABRIC_SEQUENCE_NUMBER lastQuorumAcked,
            __out std::wstring & errorMessage);
        Common::ErrorCode ApplyOperations(
            TransactionSPtr const &,
            std::vector<ReplicationOperation> const &,
            ::FABRIC_SEQUENCE_NUMBER replicationLSN,
            ::FABRIC_SEQUENCE_NUMBER lastQuorumAcked,
            __out size_t & updatedTombstoneCount,
            __out std::wstring & errorMessage);
        Common::ErrorCode ApplyCopyOperation(
            TransactionSPtr const &,
            ReplicationOperation const &);
        Common::ErrorCode ProcessTombstone(
            TransactionSPtr const &, 
            ReplicationOperation const &);
        Common::ErrorCode ProcessTombstoneLowWatermark(
            TransactionSPtr const &, 
            ReplicationOperation const &,
            __out bool & shouldAccept);
        Common::ErrorCode ProcessEpochHistory(
            TransactionSPtr const &, 
            ReplicationOperation const &);
        Common::ErrorCode ProcessEpochUpdate(
            TransactionSPtr const &, 
            ReplicationOperation const &);

        bool IsRetryable(Common::ErrorCode const &);

        void OnCommitComplete(Common::AsyncOperationSPtr const & asyncOperation, bool expectedCompletedSynchronously);
#if defined(PLATFORM_UNIX)
        Common::ManualResetEvent commitCompleteEvent_;
#endif

        Common::ErrorCode InsertOrUpdate(
            TransactionSPtr const & txSPtr,
            std::wstring const & type, 
            std::wstring const & key, 
            std::wstring const & newKey, 
            __in void const * value,
            size_t valueSizeInBytes,
            ::FABRIC_SEQUENCE_NUMBER operationLSN,
            std::unique_ptr<FILETIME> && lastModifiedOnPrimaryUtcUPtr);

        Common::ErrorCode UpdateOrInsert(
            TransactionSPtr const & txSPtr,
            std::wstring const & type, 
            std::wstring const & key, 
            std::wstring const & newKey, 
            __in void const * value,
            size_t valueSizeInBytes,
            ::FABRIC_SEQUENCE_NUMBER operationLSN,
            std::unique_ptr<FILETIME> && lastModifiedOnPrimaryUtcUPtr);

        Common::ErrorCode DoUpdate(
            ILocalStoreUPtr const &,
            TransactionSPtr const &,
            std::wstring const & type, 
            std::wstring const & key, 
            std::wstring const & newKey, 
            __in void const * value,
            size_t valueSizeInBytes,
            ::FABRIC_SEQUENCE_NUMBER operationLSN,
            ::FABRIC_SEQUENCE_NUMBER currentOperationLSN,
            std::unique_ptr<FILETIME> && lastModifiedOnPrimaryUtcUPtr);
			
        Common::ErrorCode DeleteIfDataItemExists(
            TransactionSPtr const & txSPtr,
            std::wstring const & type,
            std::wstring const & key,
            FABRIC_SEQUENCE_NUMBER operationLsn);

        void AddPendingInsert(std::wstring const & type, std::wstring const & key, FABRIC_SEQUENCE_NUMBER lsn);
        void RemovePendingInserts(std::vector<ReplicationOperation> const &, FABRIC_SEQUENCE_NUMBER lsn);
        bool ContainsPendingInsert(std::wstring const & type, std::wstring const & key, FABRIC_SEQUENCE_NUMBER lsn, __out FABRIC_SEQUENCE_NUMBER & pendingLsn);
			
        bool ApproveNewOperationCreation();
		
        void OnPendingOperationCompletion();

        void TransientFaultReplica(
            Common::ErrorCode const & error,
            std::wstring const & message,
            bool scheduleDrain);
        bool TryFaultStreamOrCancel(
            bool scheduleDrain,
            FABRIC_FAULT_TYPE faultType = FABRIC_FAULT_TYPE_TRANSIENT);
        void CancelInternal();

        SecondaryPumpState get_PumpState();
        ILocalStoreUPtr const & GetCurrentLocalStore();
        void FlushCurrentLocalStore();

    private:
        class PendingInsertMetadata;

        Common::ComponentRootSPtr root_;
        ReplicatedStore & replicatedStore_;
        ILocalStoreUPtr copyLocalStore_;
        bool isFullCopy_;
        bool isLogicalCopyStreamComplete_;
        bool shouldNotifyCopyComplete_;
        ::FABRIC_SEQUENCE_NUMBER lastCopyLsn_;

        Common::ComPointer<IFabricOperationStream> operationStream_;
        Common::ComPointer<IFabricStateReplicator> stateReplicator_;
        SecondaryPumpState pumpState_;
        Common::Stopwatch stopwatch_;

        ::FABRIC_SEQUENCE_NUMBER lastLsnProcessed_;
        ::FABRIC_SEQUENCE_NUMBER lsnAtLastSyncCommit_;

        // The following are used to delay closing of the secondary pump until all
        // operations have completed, including async commit of transactions
        //
        RWLOCK(StoreSecondaryPump, thisLock_);
        bool isCanceled_;
        Common::atomic_bool isStreamFaulted_;
        mutable Common::atomic_long pendingOperationsCount_;

        std::unique_ptr<Common::File> fullCopyFileUPtr_;

        std::unordered_map<std::wstring, std::shared_ptr<PendingInsertMetadata>> pendingInserts_;
        Common::RwLock pendingInsertsLock_;
    };
}
