// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace StateManager
    {
        //
        // Manages the in-memory state of State Manager
        //
        // Keeps track of
        // - Current State Providers: Transient (Write-Set), Active (D&C State), SoftDeleted (Marked for delete)
        // - Locks: Locks per state provider.
        // - In-flight Locks: Locks kept by in-flight transactions (StoreTransaction)
        //
        // Terminology:
        // - Transient:     A state provider whose "Add" transaction is in-flight.
        // - Active:        A state provider whose "Add" transaction is committed.
        // - SoftDeleted:   A state provider whose "Remove" transaction is committed, but txn.Committed < SafeLSN
        // - Stale:         False Progressed.
        //
        class MetadataManager final : 
            public KObject<MetadataManager>,
            public KShared<MetadataManager>,
            public KWeakRefType<MetadataManager>,
            public Utilities::IDisposable,
            public Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::SM>
        {
            K_FORCE_SHARED(MetadataManager)

            K_SHARED_INTERFACE_IMP(IDisposable)

        public:
            //
            // Creates a StateProviderMetadataManager::SPtr.
            //
            // Port Note: Does not take ILoggingReplicator because it is not used.
            static NTSTATUS Create(
                __in Utilities::PartitionedReplicaId const & traceType, 
                __in KAllocator & allocator,
                __out SPtr & result) noexcept;

        public: // In-flight transaction related APIs
            bool TryRemoveInflightTransaction(
                __in LONG64 transactionId,
                __out Data::Utilities::KHashSet<KUri::CSPtr>::SPtr lockSet) noexcept;

            ULONG32 GetInflightTransactionCount() const noexcept;
             
        public:
            ktl::Awaitable<void> AcquireLockAndAddAsync(
                __in KUri const & key,
                __in Metadata & metadata,
                __in Common::TimeSpan const & timeout,
                __in ktl::CancellationToken const & cancellationToken);

            // Adds a new state provider metadata.
            // Note that the add can be Transient or Committed.
            bool TryAdd(
                __in KUri const & key, 
                __in Metadata & metadata);

            // Add a soft deleted state provider.
            bool AddDeleted(
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in Metadata & metadata);

            // Add a soft deleted state provider.
            bool TryRemoveDeleted(
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __out Metadata::SPtr & metadata);

            // Checks if any State Provider exists (Transient + Committed)
            bool IsEmpty() const;

            // Check if State Provider exists (Committed only)
            bool ContainsKey(
                __in KUri const & key,
                __in bool allowTransientState = false) const;

            // Change: In managed code the GetMetadata logic is unnecessarily duplicated.
            bool TryGetMetadata(
                __in KUri const & key, 
                __in bool allowTransientState,
                __out Metadata::SPtr & result) const;
            bool TryGetMetadata(
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __out Metadata::SPtr & result) const;

            bool TryGetDeletedMetadata(
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __out Metadata::SPtr & metadata) const;

            // Get the Enumerator of active state providers collection.
            // The returned enumerator includes every metadata in InMemoryState, call with GetMetadataMoveNext() and Current()
            // to get the active state providers.
            IEnumerator<KeyValuePair<KUri::CSPtr, Metadata::SPtr>>::SPtr GetMetadataCollection() const;

            NTSTATUS GetInMemoryMetadataArray(
                __in StateProviderFilterMode::FilterMode filterMode,
                __out KSharedArray<Metadata::CSPtr>::SPtr & outputArray) const noexcept;

            NTSTATUS GetDeletedMetadataArray(
                __out KSharedArray<Metadata::SPtr>::SPtr & outputArray) const noexcept;
            NTSTATUS GetDeletedConstMetadataArray(
                __out KSharedArray<Metadata::CSPtr>::SPtr & outputArray) const noexcept;

            NTSTATUS MarkAllDeletedStateProviders(
                __in MetadataMode::Enum metadataMode) noexcept;

            //
            // Checks whether the input state provider id is deleted or stale
            //
            bool IsStateProviderDeletedOrStale(
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in MetadataMode::Enum filterMode) const;

            //
            // Acquire read lock.
            //
            __checkReturn ktl::Awaitable<NTSTATUS> LockForReadAsync(
                __in KUri const & key,
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in TxnReplicator::TransactionBase const & transaction,
                __in Common::TimeSpan const & timeout,
                __in ktl::CancellationToken const & cancellationToken,
                __out StateManagerLockContext::SPtr & lockContextSPtr) noexcept;

            //
            // Acquire write lock.
            //
            __checkReturn ktl::Awaitable<NTSTATUS> LockForWriteAsync(
                __in KUri const & key,
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in TxnReplicator::TransactionBase const & transaction,
                __in Common::TimeSpan const & timeout,
                __in ktl::CancellationToken const & cancellationToken,
                __out StateManagerLockContext::SPtr & lockContextSPtr) noexcept;

            void MoveStateProvidersToDeletedList();

            ktl::Awaitable<Metadata::SPtr> ResurrectStateProviderAsync(
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in Common::TimeSpan const & timeout,
                __in ktl::CancellationToken const & cancellationToken);

            /// <summary>
            /// Deletes state provider key from the dictionary. 
            /// This is called as a part of a transaction only. 
            /// Hence lock has already been acquired and no lock is needed.
            /// </summary>
            void SoftDelete(
                __in KUri const & key, 
                __in MetadataMode::Enum metadataMode);

            //
            // Release all the resources associated with the transaction.
            //
            // Transaction::Unlock can be called as part of Abort or Commit code path.
            // If called during Abort code path, Apply could not have been called.
            //
            NTSTATUS Unlock(
                __in StateManagerTransactionContext const & transactionContext) noexcept;

            bool GetChildrenMetadata(
                __in FABRIC_STATE_PROVIDER_ID parentId,
                __out KSharedArray<Metadata::SPtr>::SPtr & childrenMetadata);

            bool GetChildren(
                __in FABRIC_STATE_PROVIDER_ID parentId,
                __out KSharedArray<TxnReplicator::IStateProvider2::SPtr>::SPtr & childrenMetadata);

            void AddChild(
                __in FABRIC_STATE_PROVIDER_ID parentStateProviderId,
                __in Metadata & childMetadata);

            bool TryRemoveParent(
                __in FABRIC_STATE_PROVIDER_ID parentStateProviderId, 
                __inout KSharedArray<Metadata::SPtr>::SPtr & childrenMetadata);

            void ResetTransientState(__in KUri const & stateProviderName);

            void Dispose() override;

            ULONG GetInMemoryStateProvidersCount() const;

            ULONG GetDeletedStateProvidersCount() const;

        public: // Test only methods
            // 
            // Query the Lock Manager.
            //
            bool TryGetLockContext(
                __in KUri const & stateProviderName, 
                __out StateManagerLockContext::SPtr & lockContextSPtr) const;

        private: // Constructor
            NOFAIL MetadataManager(
                __in Utilities::PartitionedReplicaId const & partitionedReplicId,
                __in Utilities::ConcurrentSkipList<FABRIC_STATE_PROVIDER_ID, Metadata::SPtr> & stateProviderIdMap,
                __in Utilities::ConcurrentSkipList<FABRIC_STATE_PROVIDER_ID, Metadata::SPtr> & deletedStateProviderIdMap,
                __in Data::Utilities::ConcurrentDictionary<KUri::CSPtr, Metadata::SPtr> & inMemoryStateMap,
                __in Data::Utilities::ConcurrentDictionary<KUri::CSPtr, StateManagerLockContext::SPtr> & keyLockMap,
                __in Data::Utilities::ConcurrentDictionary<LONG64, Data::Utilities::KHashSet<KUri::CSPtr>::SPtr> & inflightTransactionMap,
                __in Data::Utilities::ConcurrentDictionary<LONG64, KSharedArray<Metadata::SPtr>::SPtr> & parentChildrenMap) noexcept;

        private: // Private methods
            bool DoesTransactionContainKeyLock(
                __in LONG64 transactionId, 
                __in KUri const & key) const;

            // 
            // Remove all resources held for a given state provider.
            // This includes removing the state provider and its lock.
            // 
            NTSTATUS Remove(
                __in KUri const & stateProviderName,
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in LONG64 transactionId) noexcept;

            NTSTATUS RemoveLock(
                __in StateManagerLockContext & lockContext, 
                __in LONG64 transactionId) noexcept;

        private:
            // Maps state provider id to Metadata. Active only.
            // Note: The dictionary value changed to the Metadata::SPtr from IStateProvider2::SPtr to solve the AddOperation takes in
            // Name bug.
            Utilities::ConcurrentSkipList<FABRIC_STATE_PROVIDER_ID, Metadata::SPtr>::SPtr const stateProviderIdMapSPtr_;

            // Soft delete table: All state providers that have been soft deleted.
            Utilities::ConcurrentSkipList<FABRIC_STATE_PROVIDER_ID, Metadata::SPtr>::SPtr const deletedStateProvidersSPtr_;

            // Contains metadata about all in-memory state providers.
            // Contains Active as well as Transient.
            Utilities::ConcurrentDictionary<KUri::CSPtr, Metadata::SPtr>::SPtr const inMemoryStateSPtr_;

            // Lock Manager. Keeps a lock for a given state provider.
            // Note: Change the key lock dictionary using StateProvider Name as key from StateProviderID, because the StateProviderID will
            // be generated in the Add part. Using StateProviderID will incorrectly add lock to the dictionary with different ID even through
            // they are the same state provider
            Utilities::ConcurrentDictionary<KUri::CSPtr, StateManagerLockContext::SPtr>::SPtr const keyLocksSPtr_;

            // Table of in-flight transactionIds to State Providers they have touched.
            // Using KUri::CSPtr instead of KUriView to avoid access violation when obj destructed. Using shared pointer because
            // the object is destroyed and its memory deallocated only when the last remaining shared pointer owning the object is destroyed.
            Utilities::ConcurrentDictionary<LONG64, Utilities::KHashSet<KUri::CSPtr>::SPtr>::SPtr const inflightTransactionsSPtr_;
           
            // In-flight parent state providers to their children's metadata.
            Utilities::ConcurrentDictionary<FABRIC_STATE_PROVIDER_ID, KSharedArray<Metadata::SPtr>::SPtr>::SPtr const parentToChildrenStateProviderMapSPtr_;
        };
    }
}
