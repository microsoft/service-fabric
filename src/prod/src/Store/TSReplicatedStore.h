// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class TSReplicatedStore
        : public IReplicatedStore
        , public Api::IDataLossHandler
        , public Common::RootedObject
        , public PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ReplicatedStore>
        , protected TSComponent
    {
        DENY_COPY(TSReplicatedStore)

    public:

        static Common::Global<KUriView> TStoreProviderName;
        static Common::Global<KStringView> TStoreProviderType;

    public:

        TSReplicatedStore(
            Common::Guid const & partitionId,
            ::FABRIC_REPLICA_ID replicaId,
            Reliability::ReplicationComponent::ReplicatorSettingsUPtr &&,
            TSReplicatedStoreSettingsUPtr &&,
            Api::IStoreEventHandlerPtr const &,
            Api::ISecondaryEventHandlerPtr const &,
            Common::ComponentRoot const &);

        virtual ~TSReplicatedStore();

    public:

        ::FABRIC_REPLICA_ROLE GetReplicatorRole() const;

        KAllocator & GetAllocator();

        void WaitForInitialization();

        void TransientFault(std::wstring const & message, Common::ErrorCode const & error);

    public:

        //
        // IStoreBase
        //

        virtual ::FABRIC_TRANSACTION_ISOLATION_LEVEL GetDefaultIsolationLevel() override;

        virtual Common::ErrorCode CreateTransaction(__out TransactionSPtr &) override;

        virtual FILETIME GetStoreUtcFILETIME() override;

        virtual Common::ErrorCode CreateEnumerationByTypeAndKey(
            __in TransactionSPtr const & transaction,
            __in std::wstring const & type,
            __in std::wstring const & keyStart,
            __out EnumerationSPtr & enumerationSPtr) override;

        virtual Common::ErrorCode ReadExact(
            __in TransactionSPtr const & tx,
            __in std::wstring const & type,
            __in std::wstring const & keyStart,
            __out std::vector<byte> & value,
            __out __int64 & operationLsn) override;

        virtual Common::ErrorCode ReadExact(
            __in TransactionSPtr const & tx,
            __in std::wstring const & type,
            __in std::wstring const & keyStart,
            __out std::vector<byte> & value,
            __out __int64 & operationLsn,
            __out FILETIME & lastModified) override;

    public:

        //
        // IReplicatedStore
        //

        virtual bool GetIsActivePrimary() const override;
        virtual Common::ErrorCode GetLastCommittedSequenceNumber(__out FABRIC_SEQUENCE_NUMBER &) override;

        virtual Common::ErrorCode CreateSimpleTransaction(
            __out TransactionSPtr & transactionSPtr) override;

        virtual Common::ErrorCode Insert(
            __in TransactionSPtr const & transaction,
            __in std::wstring const & type,
            __in std::wstring const & key,
            __in void const * value,
            __in size_t valueSizeInBytes) override;

        virtual Common::ErrorCode Update(
            __in TransactionSPtr const & transaction,
            __in std::wstring const & type,
            __in std::wstring const & key,
            __in _int64 checkOperationNumber,
            __in std::wstring const & newKey,
            __in_opt void const * newValue,
            __in size_t valueSizeInBytes) override;

        virtual Common::ErrorCode Delete(
            __in TransactionSPtr const & transaction,
            __in std::wstring const & type,
            __in std::wstring const & key,
            __in _int64 checkOperationNumber = ILocalStore::SequenceNumberIgnore) override;

        virtual Common::ErrorCode SetThrottleCallback(ThrottleCallback const &) override;

        virtual Common::ErrorCode GetCurrentEpoch(__out FABRIC_EPOCH &) const override;
            
        virtual Common::ErrorCode UpdateReplicatorSettings(FABRIC_REPLICATOR_SETTINGS const &) override;

        virtual Common::ErrorCode UpdateReplicatorSettings(Reliability::ReplicationComponent::ReplicatorSettingsUPtr const &) override;

        Common::ErrorCode GetQueryStatus(__out Api::IStatefulServiceReplicaStatusResultPtr &) override;

        void SetQueryStatusDetails(std::wstring const &);
        void SetMigrationQueryResult(std::unique_ptr<MigrationQueryResult> &&) override;

        void Test_SetTestHookContext(TestHooks::TestHookContext const &) override;

    public:

        //
        // IStatefulServiceReplica
        //

        virtual Common::AsyncOperationSPtr BeginOpen(
            ::FABRIC_REPLICA_OPEN_MODE openMode,
            Common::ComPointer<::IFabricStatefulServicePartition> const & partition,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent) override;

        virtual Common::ErrorCode EndOpen(
            Common::AsyncOperationSPtr const & asyncOperation, 
            __out Common::ComPointer<::IFabricReplicator> & replicator) override;

        virtual Common::AsyncOperationSPtr BeginChangeRole(
            ::FABRIC_REPLICA_ROLE newRole,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent) override;    

        virtual Common::ErrorCode EndChangeRole(
            Common::AsyncOperationSPtr const & asyncOperation, 
            __out std::wstring & serviceLocation) override;

        virtual Common::AsyncOperationSPtr BeginClose(
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent) override;

        virtual Common::ErrorCode EndClose(
            Common::AsyncOperationSPtr const & asyncOperation) override;

        virtual void Abort() override;

    protected:

        //
        // IDataLossHandler
        //

        virtual Common::AsyncOperationSPtr BeginOnDataLoss(
            __in Common::AsyncCallback const & callback,
            __in Common::AsyncOperationSPtr const & parent) override;

        virtual Common::ErrorCode EndOnDataLoss(
            __in Common::AsyncOperationSPtr const & asyncOperation,
            __out bool & isStateChanged) override;

    protected:

        //
        // TSComponent
        //

        Common::StringLiteral const & GetTraceComponent() const override;
        std::wstring const & GetTraceId() const override;

    private:

        class ComTransactionalReplicatorRuntimeConfigurations;
        class ComStateProviderFactory;
        class OpenAsyncOperation;
        class ChangeRoleAsyncOperation;
        class CloseAsyncOperation;

        Common::ErrorCode InitializeReplicator(__out Common::ComPointer<IFabricReplicator> &);
        void RetryInitializeStore();
        void ScheduleInitializeStore(Common::TimeSpan const & delay);
        ktl::Task InitializeStoreTask(Common::ComponentRootSPtr const & root);
        ktl::Awaitable<Common::ErrorCode> InitializeStoreAsync(Common::ComponentRootSPtr const & root);
        void CancelInitializationTimer();

        Common::ErrorCode OnOpen(__out Common::ComPointer<IFabricReplicator> &);
        Common::ErrorCode OnChangeRole();

        //
        // FabricComponent (used by FM)
        //
        Common::ErrorCode OnClose() override;

        bool ApplicationCanReadWrite();
        bool HasReadStatus();
        bool HasWriteStatus();
        bool InternalHasReadStatus();
        bool InternalHasWriteStatus();
        Common::ComPointer<IFabricStatefulServicePartition> TryGetPartition();

        void TryUnregisterChangeHandler();

        IKvs::SPtr TSReplicatedStore::TryGetTStore() const;
        IKvsTransaction::SPtr TSReplicatedStore::GetTStoreTransaction(TransactionSPtr const &);
        Common::ErrorCode TryGetTxReplicator(__out TxnReplicator::ITransactionalReplicator::SPtr &) const;

        FABRIC_REPLICATOR_SETTINGS const * CreateReplicatorSettings(
            __in Common::ScopedHeap &, 
            Reliability::ReplicationComponent::ReplicatorSettingsUPtr const &);

        Common::ErrorCode CreateKey(std::wstring const & type, std::wstring const & key, __out KString::SPtr &);
        Common::ErrorCode CreateBuffer(void const * value, size_t valueSize, __out KBuffer::SPtr &);

        Reliability::ReplicationComponent::ReplicatorSettingsUPtr replicatorSettings_;
        TSReplicatedStoreSettingsUPtr storeSettings_;

        KtlSystem * ktlSystem_;

        Common::ComPointer<IFabricTransactionalReplicator> stateReplicator_;
        TxnReplicator::ITransactionalReplicator::SPtr transactionalReplicator_;
        mutable Common::RwLock txReplicatorLock_;
        IKvs::SPtr innerStore_;
        mutable Common::RwLock innerStoreLock_;

        Api::IStoreEventHandlerPtr storeEventHandler_;
        Api::ISecondaryEventHandlerPtr secondaryEventHandler_;
        KSharedPtr<TSChangeHandler> changeHandler_;

        Common::atomic_bool isInitializationActive_;
        Common::TimerSPtr initializationTimer_;
        Common::RwLock initializationTimerLock_;
        Common::ManualResetEvent initializedEvent_;

        Common::ComPointer<::IFabricStatefulServicePartition> partition_;
        Common::RwLock partitionLock_;
        FABRIC_REPLICA_ROLE replicaRole_;
        Common::atomic_bool isActive_;

        Common::RwLock queryStatusDetailsLock_;
        std::wstring queryStatusDetails_;
        MigrationQueryResultSPtr migrationDetails_;

        // Used for fault injection from FabricTest
        //
        TestHooks::TestHookContext testHookContext_;
    };
}
