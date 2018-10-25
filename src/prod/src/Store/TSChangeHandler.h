// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    typedef Data::TStore::IDictionaryChangeHandler<KString::SPtr, KBuffer::SPtr> IKvsChangeHandler; 
    typedef Data::KeyValuePair<KString::SPtr, IKvsGetEntry> IKvsRebuildEntry;
    typedef std::map<std::wstring, IKvsRebuildEntry> RebuildBufferKeyEntries;
    typedef std::map<std::wstring, RebuildBufferKeyEntries> RebuildBuffer;

    // V1 stack:
    // In "blocking" mode, operations are only persisted and acked after the notification callback returns.
    //
    // V2 stack:
    // In "blocking" mode, operations are persisted before firing the notification callback and acked
    // after the notification callback returns. Therefore, blocking notification callbacks only
    // guarantees that operations commit after a quorum of replicas have finished processing the notification.
    // However, the existence of an operation on disk does not guarantee that any other replica has seen
    // the operation yet.
    // 
    // Furthermore, notifications are serialized for conflicting keys, but fired in parallel for non-conflicting keys.
    //
    // This has implications specifically for the Image Store Service. While processing a copy notification, 
    // a database entry for a file in the Available_V1/Committed state only guarantees that at least one replica has the 
    // actual file contents in its filesystem as long as the partition is not in quorum loss. The replica with
    // the file contents is not guaranteed to be the primary replica either. This means that a building replica must
    // look for the file from all available replicas in the partition. This further implies that processing of
    // the copy completion notification must not block Open or ChangeRole(Idle) - otherwise a restarting partition
    // can deadlock since ChangeRole(Idle) must complete in order for replicas to publish their endpoints.
    //

    class TSReplicatedStore;
    class TSChangeHandler 
        : public TxnReplicator::IStateManagerChangeHandler
        , public IKvsChangeHandler
        , public KObject<TSChangeHandler>
        , public KShared<TSChangeHandler>
        , public PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ReplicatedStore>
        , public TSComponent
    {
        K_FORCE_SHARED(TSChangeHandler)
        K_SHARED_INTERFACE_IMP(IStateManagerChangeHandler)
        K_SHARED_INTERFACE_IMP(IDictionaryChangeHandler)

    private:
        TSChangeHandler(
            __in TSReplicatedStore &,
            Api::ISecondaryEventHandlerPtr &&);

    public:

        static NTSTATUS Create(
            __in TSReplicatedStore &,
            Api::ISecondaryEventHandlerPtr &&,
            __out KSharedPtr<TSChangeHandler> &);

        void TryUnregister();
        
    public:
        //
        // IStateManagerChangeHandler
        //

        virtual void OnRebuilt(
            __in TxnReplicator::ITransactionalReplicator & source,
            __in Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, KSharedPtr<TxnReplicator::IStateProvider2>>> & stateProviders) override;

        virtual void OnAdded(
            __in TxnReplicator::ITransactionalReplicator & source,
            __in TxnReplicator::ITransaction const & transaction,
            __in KUri const & stateProviderName,
            __in TxnReplicator::IStateProvider2 & stateProvider) override;

        virtual void OnRemoved(
            __in TxnReplicator::ITransactionalReplicator & source,
            __in TxnReplicator::ITransaction const & transaction,
            __in KUri const & stateProviderName,
            __in TxnReplicator::IStateProvider2 & stateProvider) override;

    public:

        //
        // IDictionaryChangeHandler
        //

        ktl::Awaitable<void> OnAddedAsync(
            __in TxnReplicator::TransactionBase const& replicatorTransaction,
            __in KString::SPtr key,
            __in KBuffer::SPtr value,
            __in LONG64 sequenceNumber,
            __in bool isPrimary) noexcept override;
        
        ktl::Awaitable<void> OnUpdatedAsync(
            __in TxnReplicator::TransactionBase const& replicatorTransaction,
            __in KString::SPtr key,
            __in KBuffer::SPtr value,
            __in LONG64 sequenceNumber,
            __in bool isPrimary) noexcept override;

        ktl::Awaitable<void> OnRemovedAsync(
            __in TxnReplicator::TransactionBase const& replicatorTransaction,                
            __in KString::SPtr key,
            __in LONG64 sequenceNumber,
            __in bool isPrimary) noexcept override;

        ktl::Awaitable<void> OnRebuiltAsync(
            __in Data::Utilities::IAsyncEnumerator<IKvsRebuildEntry> &) noexcept override;

    protected:

        //
        // TSComponent
        //

        Common::StringLiteral const & GetTraceComponent() const override;
        std::wstring const & GetTraceId() const override;

    private:
        class StoreItemMetadata;
        class StoreItem;
        class StoreItemMetadataEnumerator;
        class StoreItemEnumerator;
        class StoreEnumerator;
        class StoreNotificationEnumerator;
        class RebuildBufferEnumerator;
        class NotificationEntry;

        void TryRegister(__in TxnReplicator::IStateProvider2 &);
        void DispatchRebuildBufferIfNeeded(std::wstring const & type, std::wstring const & key);
        void DispatchRebuildBuffer();
        void DispatchCopyCompletion(__in Data::Utilities::IAsyncEnumerator<IKvsRebuildEntry> &);
        void DispatchReplication(std::shared_ptr<NotificationEntry> &&);

        bool ShouldIgnore();
        bool ShouldBuffer();

        TSReplicatedStore & replicatedStore_;
        IKvs::SPtr innerStore_;
        Api::ISecondaryEventHandlerPtr secondaryEventHandler_;
        RebuildBuffer rebuildBuffer_;
        Common::RwLock rebuildBufferLock_;

        Common::atomic_long copyDispatchState_;
        Common::ManualResetEvent copyDispatchEvent_;
    };
}
