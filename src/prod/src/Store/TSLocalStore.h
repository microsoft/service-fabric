// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    // TStore is tightly coupled with TransactionReplicator, so there's no natural 
    // "local store" that can be pulled out. As such, we currently provide a 
    // "local store" by wrapping a single-replica instance of TSReplicatedStore.
    //
    class TSLocalStore 
        : public ILocalStore
        , public Common::ComponentRoot
        , public PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ReplicatedStore>
        , protected TSComponent
    {
    public:

        static Common::GlobalWString TraceNodeId;

    public:

        TSLocalStore(TSReplicatedStoreSettingsUPtr &&, KtlLogger::KtlLoggerNodeSPtr const &);
        virtual ~TSLocalStore();

        __declspec(property(get=get_TraceId)) std::wstring const & TraceId;
        std::wstring const & get_TraceId() const { return PartitionedReplicaTraceComponent::TraceId; }

        __declspec(property(get=get_Settings)) TSReplicatedStoreSettingsUPtr const & Settings;
        TSReplicatedStoreSettingsUPtr const & get_Settings() const { return storeSettings_; }

        TSReplicatedStore * Test_GetReplicatedStore() const;

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

    public:

        //
        // ILocalStore
        //

        virtual Common::ErrorCode Initialize(std::wstring const & instanceName) override;

        // Used by RA
        //
        virtual Common::ErrorCode Initialize(std::wstring const & instanceName, Federation::NodeId const &) override;

        // Used by KeyValueStoreMigrator
        //
        virtual Common::ErrorCode Initialize(
            std::wstring const & instanceName, 
            Common::Guid const & partitionId, 
            FABRIC_REPLICA_ID,
            FABRIC_EPOCH const &);

        virtual Common::ErrorCode MarkCleanupInTerminate() override;
        virtual Common::ErrorCode Terminate() override;

        virtual void Drain() override;

        virtual Common::ErrorCode Cleanup() override;

        Common::ErrorCode Insert(
            __in TransactionSPtr const & transaction,
            __in std::wstring const & type,
            __in std::wstring const & key,
            __in void const * value,
            __in size_t valueSizeInBytes,
            __in _int64 operationNumber = ILocalStore::OperationNumberUnspecified,
            __in FILETIME const * lastModifiedOnPrimaryUtc = nullptr) override;

        Common::ErrorCode Update(
            __in TransactionSPtr const & transaction,
            __in std::wstring const & type,
            __in std::wstring const & key,
            __in _int64 checkOperationNumber,
            __in std::wstring const & newKey,
            __in_opt void const * newValue,
            __in size_t valueSizeInBytes,
            __in _int64 operationNumber = ILocalStore::OperationNumberUnspecified,
            __in FILETIME const * lastModifiedOnPrimaryUtc = nullptr) override;

        Common::ErrorCode Delete(
            __in TransactionSPtr const & transaction,
            __in std::wstring const & type,
            __in std::wstring const & key,
            __in _int64 checkOperationNumber = ILocalStore::OperationNumberUnspecified) override;

        Common::ErrorCode UpdateOperationLSN(
            TransactionSPtr const & transaction,
            std::wstring const & type,
            std::wstring const & key,
            __in ::FABRIC_SEQUENCE_NUMBER operationLSN) override;

        Common::ErrorCode CreateEnumerationByOperationLSN(
            __in TransactionSPtr const & transaction,
            __in _int64 fromOperationNumber,
            __out EnumerationSPtr & enumerationSPtr) override;

        Common::ErrorCode GetLastChangeOperationLSN(
            __in TransactionSPtr const & transaction,
            __out ::FABRIC_SEQUENCE_NUMBER & operationLSN) override;

    public:

        //
        // Helper functions
        //

        Common::ErrorCode DeleteDatabaseFiles(std::wstring const & sharedLogFilePath);

    public:

#if defined(PLATFORM_UNIX)
        Common::ErrorCode Lock(
                TransactionSPtr const & transaction,
                std::wstring const & type,
                std::wstring const & key) override;

        Common::ErrorCode Flush() override;
#endif

    protected:

        //
        // TSComponent
        //

        Common::StringLiteral const & GetTraceComponent() const override;
        std::wstring const & GetTraceId() const override;

    private:

        class ComAsyncOperationCallbackHelper;
        class ComMockPartition;
        class InnerStoreRoot;

        std::shared_ptr<InnerStoreRoot> GetStoreRoot() const;

        Common::ErrorCode InnerInitialize(
            std::wstring const & instanceName, 
            Common::Guid const & partitionId, 
            FABRIC_REPLICA_ID,
            std::unique_ptr<FABRIC_EPOCH> const &);

        Common::ErrorCode InnerOpen();
        Common::ErrorCode InnerChangeRolePrimary(std::unique_ptr<FABRIC_EPOCH> const &);
        Common::ErrorCode InnerClose();
        Common::Guid ToGuid(Federation::NodeId const &);

        TSReplicatedStoreSettingsUPtr storeSettings_;

        // Maintain reference to KtlLoggerNode rather than
        // shared log settings on the KtlLoggerNode directly
        // since the settings are only available after the node
        // opens (not during creation).
        //
        KtlLogger::KtlLoggerNodeSPtr ktlLogger_;

        Common::Guid mockPartitionId_;
        FABRIC_REPLICA_ID mockReplicaId_;
        Common::ComPointer<IFabricStatefulServicePartition> partition_;
        std::shared_ptr<InnerStoreRoot> innerStore_;
        mutable Common::RwLock innerStoreLock_;
        Common::ComPointer<IFabricReplicator> replicator_;

        Common::atomic_bool isActive_;

        bool shouldCleanup_;
    };
}
