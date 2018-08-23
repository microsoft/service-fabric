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
        // Represents the operations of a transaction or a single atomic/atomic-redo operation
        //
        class StateManagerLockContext 
            : public Utilities::IDisposable
            , public KObject<StateManagerLockContext>
            , public KShared<StateManagerLockContext>
        {
            K_FORCE_SHARED(StateManagerLockContext)
            K_SHARED_INTERFACE_IMP(IDisposable)

        public:
            //
            // Creates the StateManagerTransactionContext::SPtr.
            // 
            // Parameters:
            // key                      Id of the parent transaction.
            // stateProviderId          Information on the lock that was taken for the operation.
            // metadataManager          Type of the operation for which the lock was taken.
            //
            static NTSTATUS Create(
                __in KUri const & key,
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in MetadataManager & metadataManager,
                __in KAllocator & allocator,
                __out StateManagerLockContext::SPtr & result) noexcept;

            __declspec(property(get = get_Key)) KUri::CSPtr Key;
            KUri::CSPtr StateManagerLockContext::get_Key() const
            {
                return key_;
            }

            __declspec(property(get = get_StateProviderId)) FABRIC_STATE_PROVIDER_ID StateProviderId;
            FABRIC_STATE_PROVIDER_ID StateManagerLockContext::get_StateProviderId()
            {
                return stateProviderId_;
            }

            __declspec(property(get = get_GrantorCount, put = put_GrantorCount)) LONG GrantorCount;
            LONG StateManagerLockContext::get_GrantorCount()
            {
                return grantorCount_;
            }

            void StateManagerLockContext::put_GrantorCount(__in LONG value)
            {
                grantorCount_ = value;
            }

            __declspec(property(get = get_LockMode)) StateManagerLockMode::Enum LockMode;
            StateManagerLockMode::Enum StateManagerLockContext::get_LockMode()
            {
                return lockMode_;
            }

            //
            // Acquire read lock.
            //
            // Use: Read lock for the initial part of GetOrAdd.
            // Note that TryGetStateProvider is lock free.
            //
            ktl::Awaitable<NTSTATUS> AcquireReadLockAsync(
                __in Common::TimeSpan const & timeout, 
                __in ktl::CancellationToken const & cancellationToken) noexcept;

            //
            // Acquire write lock.
            //
            // Use: All write operations
            //
            ktl::Awaitable<NTSTATUS> AcquireWriteLockAsync(
                __in Common::TimeSpan const & timeout, 
                __in ktl::CancellationToken const & cancellationToken) noexcept;

            //
            // Release the lock outside of the Txn:LockContext.
            // Port Note: Release name is taken by KSharedBase, hence using ReleaseLock.
            //
            // Use: 
            // - Upgrading a read lock to write lock.
            // - Releasing not registered locks. E.g. 
            //      - Failure before Txn::AddLockContext accepts the lock context
            //      - Local-only operations (copy & recovery)
            //      - Upgrading a lock (Read to Write)
            //      - Unlocking during transaction unlock.
            //
            NTSTATUS ReleaseLock(__in LONG64 transactionId) noexcept;

            //
            // Simply forwards the Unlock call.
            //
            // Called when transaction unlocks (commit or abort).
            // 11908685: Remove LockContext from between TransactionContext and SPMM. Let TransactionContext call unlock directly.
            // 
            NTSTATUS Unlock(__in StateManagerTransactionContext & transactionContext) noexcept;

            //
            // Closes the lock. This is called when SM Metadata Manager needs to be destructed.
            //
            void Close();

            //
            // Used for deleting a lock.
            //
            void Dispose() override;

        private:

            // Key lock
            Utilities::ReaderWriterAsyncLock::SPtr keyLockSPtr_;

            // NB: Must only be accessed using INTERLOCKED operations.
            volatile LONG lockWaiterCount_;

            // Number of times the lock was granted.
            LONG grantorCount_;

            // Resource name which is locked.
            const KUri::CSPtr key_;

            // Type of the lock.
            StateManagerLockMode::Enum lockMode_;

            // State provider id that is locked.
            const FABRIC_STATE_PROVIDER_ID stateProviderId_;
            
            // Port Note: lockWaiterCount never used.
            KWeakRef<MetadataManager>::SPtr metadataManagerSPtr_;

            // Indicates deletion of a lock.
            bool isDisposed_;

            //
            // NOFAIL Constructor for StateManagerLockContext class.
            //
            // Parameters:
            // key                  Resource name being locked.
            // stateProviderId      StateProviderID that is locked..
            // metadataManager      Handle to the metadata manager.
            //
            NOFAIL StateManagerLockContext(
                __in KUri const & key,
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in MetadataManager & metadataManager);
        };
    }
}
