// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class TSUnitTestStore : public IReplicatedStore
    {
    private:

        TSUnitTestStore(std::wstring const & workingDirectory);

    public:

        virtual ~TSUnitTestStore();

        static IReplicatedStoreUPtr Create(std::wstring const & workingDirectory);

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
        // FabricComponent
        //

        Common::ErrorCode OnClose() override;

    public:

        //
        // IReplicatedStore
        //

        virtual Common::ErrorCode InitializeLocalStoreForUnittests(bool databaseShouldExist) override;

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

    private:

        class CloseAsyncOperation;

        std::wstring workingDirectory_;
        Common::ComponentRootSPtr mockComponentRoot_;
        std::shared_ptr<KtlLogger::KtlLoggerNode> ktlLogger_;
        std::shared_ptr<TSLocalStore> localStore_;
    };
}
