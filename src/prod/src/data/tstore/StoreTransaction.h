// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define STORETRANSACTION_TAG 'nxTS'

namespace Data
{
    namespace TStore
    {
        template <typename TKey, typename TValue>
        class StoreTransaction
            : public TxnReplicator::LockContext
            , public IStoreTransaction<TKey, TValue>
        {
            K_FORCE_SHARED_WITH_INHERITANCE(StoreTransaction)
            K_SHARED_INTERFACE_IMP(IStoreTransaction)

        public:
            typedef KDelegate<ULONG(const TKey & Key)> HashFunctionType;
            static NTSTATUS
                Create(
                    __in LONG64 id,
                    __in TxnReplicator::TransactionBase& replicatorTransaction,
                    __in LONG64 owner,
                    __in ConcurrentDictionary2<LONG64, KSharedPtr<StoreTransaction<TKey, TValue>>>& container,
                    __in IComparer<TKey> & keyComparer,
                    __in StoreTraceComponent & traceComponent,
                    __in KAllocator& allocator,
                    __out SPtr& result)
            {
                NTSTATUS status;
                SPtr output = _new(STORETRANSACTION_TAG, allocator) StoreTransaction(id, replicatorTransaction, owner, container, keyComparer, traceComponent);

                if (!output)
                {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    return status;
                }

                status = output->Status();
                if (!NT_SUCCESS(status))
                {
                    return status;
                }

                result = Ktl::Move(output);
                return STATUS_SUCCESS;
            }

            static NTSTATUS
                Create(
                    __in LONG64 id,
                    __in LONG64 owner,
                    __in IComparer<TKey> & keyComparer,
                    __in StoreTraceComponent & traceComponent,
                    __in KAllocator& allocator,
                    __out SPtr& result)
            {
                NTSTATUS status;
                SPtr output = _new(STORETRANSACTION_TAG, allocator) StoreTransaction(id, owner, keyComparer, traceComponent);

                if (!output)
                {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    return status;
                }

                status = output->Status();
                if (!NT_SUCCESS(status))
                {
                    return status;
                }

                result = Ktl::Move(output);
                return STATUS_SUCCESS;
            }

            __declspec(property(get = get_ReplicatorTransaction)) TxnReplicator::TransactionBase::SPtr ReplicatorTransaction;
            TxnReplicator::TransactionBase::SPtr get_ReplicatorTransaction() const
            {
                return replicatorTransactionSPtr_;
            }

            __declspec(property(get = get_Id)) LONG64 Id;
            LONG64 get_Id() const
            {
                return id_;
            }

            __declspec(property(get = get_IsCompleted)) bool IsCompleted;
            bool get_IsCompleted() const
            {
                return isCompleted_;
            }

            __declspec(property(get = get_IsClosed)) bool IsClosed;
            bool get_IsClosed() const
            {
                return isClosed_;
            }

            __declspec(property(get = get_IsWriteSetEmpty)) bool IsWriteSetEmpty;
            bool get_IsWriteSetEmpty() const
            {
                if (writeSetStoreComponentSPtr_ == nullptr)
                {
                    return true;
                }

                return false;
            }

            __declspec(property(get = get_ReadIsolationLevel, put = set_ReadIsolationLevel)) StoreTransactionReadIsolationLevel::Enum ReadIsolationLevel;
            StoreTransactionReadIsolationLevel::Enum get_ReadIsolationLevel() const override
            {
                return readIsolationLevel_;
            }

            void set_ReadIsolationLevel(__in StoreTransactionReadIsolationLevel::Enum value) override
            {
                readIsolationLevel_ = value;
            }

            __declspec(property(get = get_LockingHints, put = set_LockingHints)) LockingHints::Enum LockingHints;
            LockingHints::Enum get_LockingHints() const override
            {
                return lockingHints_;
            }

            void set_LockingHints(__in LockingHints::Enum value) override
            {
                lockingHints_ = value;
            }

            //
            // Results of this functions should be used only as an intremediary as a part of a store transaction component only.
            //
            KSharedPtr<WriteSetStoreComponent<TKey, TValue>> GetComponent(__in HashFunctionType func)
            {
                if (writeSetStoreComponentSPtr_ == nullptr)
                {
                    NTSTATUS status = WriteSetStoreComponent<TKey, TValue>::Create(func, *keyComparerSPtr_, this->GetThisAllocator(), writeSetStoreComponentSPtr_);

                    if (!NT_SUCCESS(status))
                    {
                        throw ktl::Exception(status);
                    }

                }

                KInvariant(writeSetStoreComponentSPtr_ != nullptr);
                return writeSetStoreComponentSPtr_;
            }

            ktl::Awaitable<ULONG32> AcquirePrimeLockAsync(
                __in LockManager & lockManager,
                __in LockMode::Enum lockMode,
                __in Common::TimeSpan lockTimeout,
                __in bool reacquirePrimeLock)
            {
                ASSERT_IFNOT(lockMode == LockMode::Enum::Shared || lockMode == LockMode::Enum::Exclusive, "Invalid lock mode={0}", static_cast<int>(lockMode));
                ASSERT_IFNOT(lockTimeout >= Common::TimeSpan::Zero, "Invalid timeout {0}", lockTimeout.TotalMilliseconds());

                LockManager::SPtr lockManagerSPtr(&lockManager);

                if (reacquirePrimeLock)
                {
                    isPrimeLockAcquired_ = false;
                }

                if (isPrimeLockAcquired_)
                {
                    K_LOCK_BLOCK(lock_)
                    {
                        if (isClosed_ == true)
                        {
                            throw ktl::Exception(SF_STATUS_TRANSACTION_ABORTED);
                        }
                    }
                    // Initialization was already performed.
                    co_return static_cast<ULONG32>(lockTimeout.TotalMilliseconds());
                }

                LONG64 timeoutLeftOver = lockTimeout.TotalMilliseconds();
                KDateTime start = KDateTime::Now();

                co_await lockManagerSPtr->AcquirePrimeLockAsync(lockMode, lockTimeout);

                bool release = false;
                K_LOCK_BLOCK(lock_)
                {
                    if (isClosed_ == true)
                    {
                        release = true;
                    }
                    else
                    {
                        PrimeLockRequest::SPtr primeLockRequestSPtr = nullptr;
                        NTSTATUS status = PrimeLockRequest::Create(*lockManagerSPtr, lockMode, GetThisAllocator(), primeLockRequestSPtr);
                        Diagnostics::Validate(status);
                        primeLockRequestsSPtr_->Append(primeLockRequestSPtr);
                    }
                }

                if (release)
                {
                    lockManagerSPtr->ReleasePrimeLock(lockMode);
                    throw ktl::Exception(SF_STATUS_TRANSACTION_ABORTED);
                }

                // Reset initializer.
                isPrimeLockAcquired_ = true;

                // Compute left-over timeout, if needed.
                if (lockTimeout != Common::TimeSpan::Zero && lockTimeout != Common::TimeSpan::MaxValue)
                {
                    KDateTime end = KDateTime::Now();
                    KDuration duration = end - start;

                    if (duration.Milliseconds() <= 0)
                    {
                        timeoutLeftOver = lockTimeout.TotalMilliseconds();
                    }
                    else if (lockTimeout.TotalMilliseconds() < duration.Milliseconds())
                    {
                        // Inform the caller that locks could not be acquired in the given timeout.
                        timeoutLeftOver = 0;
                    }
                    else
                    {
                        // Next use left-over timeout.
                        timeoutLeftOver = lockTimeout.TotalMilliseconds() - duration.Milliseconds();
                    }
                }

                // todo: calculate 
                co_return static_cast<ULONG32>(timeoutLeftOver);
            }

            ktl::Awaitable<void> AcquireKeyLockAsync(
                __in LockManager& lockManager,
                __in ULONG64 lockResourceNameHash,
                __in LockMode::Enum lockMode,
                __in Common::TimeSpan timeout)
            {
               ASSERT_IFNOT(timeout >= Common::TimeSpan::Zero, "Invalid timeout {0}", timeout.TotalMilliseconds());
               ASSERT_IFNOT(lockMode == LockMode::Enum::Shared || lockMode == LockMode::Enum::Exclusive || lockMode == LockMode::Enum::Update, "Invalid lock mode={0}", static_cast<int>(lockMode));

               LockManager::SPtr lockManagerSPtr = &lockManager;
               if (lockMode == LockMode::Enum::Free)
               {
                  throw ktl::Exception(STATUS_INVALID_PARAMETER_3);
               }

               if (isReadOnly_)
               {
                  if (!lockManagerSPtr->IsShared(lockMode))
                  {
                     throw ktl::Exception(STATUS_INVALID_PARAMETER_3);
                  }
               }

               auto acquiredLock = co_await lockManagerSPtr->AcquireLockAsync(id_, lockResourceNameHash, lockMode, timeout);
               KInvariant(acquiredLock != nullptr);

               if (acquiredLock->GetStatus() == LockStatus::Enum::Invalid)
               {
                  acquiredLock->Close();
                  throw ktl::Exception(SF_STATUS_TRANSACTION_ABORTED);
               }

               if (acquiredLock->GetStatus() == LockStatus::Enum::Timeout)
               {
                  acquiredLock->Close();
                  throw ktl::Exception(SF_STATUS_TIMEOUT);
               }

               KInvariant(LockStatus::Enum::Granted == acquiredLock->GetStatus());
               KInvariant(acquiredLock->GetLockMode() == lockMode);
               KInvariant(acquiredLock->GetOwner() == id_);
               KInvariant(acquiredLock->GetCount() > 0);
               KInvariant(acquiredLock->GetLockManager() != nullptr);

               bool release = false;

               // todo: Not needed since we do not have concurrent operation support.
               K_LOCK_BLOCK(lock_)
               {
                  // Store transaction acquire can race with close.
                  release = (isClosed_ == true);

                  if (acquiredLock->GetCount() == 1 && !release)
                  {
                     keyLockRequestsSPtr_->Append(acquiredLock);
                  }
               }

               if (release)
               {
                  lockManagerSPtr->ReleaseLock(*acquiredLock);
                  acquiredLock->Close();
                  throw ktl::Exception(SF_STATUS_TRANSACTION_ABORTED);
               }
            }

            void Unlock() override
            {
                // Release all locks.
                Close();
            }

            void Close() override
            {
                if (InterlockedIncrement64(&clearLocks_) == 1)
                {
                    ClearLocks();
                    if (containerSPtr_ != nullptr)
                    {
                        bool success = containerSPtr_->Remove(id_);
                        KInvariant(success);
                    }

                    // Set replicator transaction since there is a circular dependency with replicator transaction 
                    // holding onto store transaction for lock context.
                    replicatorTransactionSPtr_ = nullptr;
                }
            }

        protected:
            virtual void OnClearLocks() {}

        private:
            void ClearLocks()
            {
                K_LOCK_BLOCK(lock_)
                {
                    if (isClosed_ == true)
                    {
                        return;
                    }

                    // Once locks have been cleared, no new locks should be acquired
                    isClosed_ = true;
                }

                KFinally([&] {
                    // Reset prime locks.
                    primeLockRequestsSPtr_->Clear();
                    primeLockRequestsSPtr_ = nullptr;

                    // Reset key locks
                    keyLockRequestsSPtr_->Clear();
                    keyLockRequestsSPtr_ = nullptr;
                });

                // For subclasses such as Queue
                OnClearLocks();

                for (ULONG32 index = 0; index < primeLockRequestsSPtr_->Count(); index++)
                {
                    PrimeLockRequest::SPtr primeLockSPtr = (*primeLockRequestsSPtr_)[index];

                    try
                    {
                        primeLockSPtr->LockManagerSPtr->ReleasePrimeLock(primeLockSPtr->Mode);
                    }
                    catch (ktl::Exception const & e)
                    {
                        // Swallow exception since throwing here will interfere with replicator txn cleanup
                        TraceException(L"StoreTransaction::ClearLocks.ReleasePrimeLock", e);
                    }
                }

                for (ULONG32 index = 0; index < keyLockRequestsSPtr_->Count(); index++)
                {
                    LockControlBlock::SPtr keyLockSPtr = (*keyLockRequestsSPtr_)[index];
                    while (keyLockSPtr->GetCount() > 0)
                    {
                        try
                        {
                            keyLockSPtr->GetLockManager()->ReleaseLock(*keyLockSPtr);
                        }
                        catch (ktl::Exception const & e)
                        {
                            // Swallow exception since throwing here will interfere with replicator txn cleanup
                            TraceException(L"StoreTransaction::ClearLocks.ReleaseKeyLock", e);
                            break;
                        }
                    }

                    keyLockSPtr->Close();
                }
            }

            void TraceException(__in KStringView const & methodName, __in ktl::Exception const & exception)
            {
                KDynStringA stackString(this->GetThisAllocator());
                Diagnostics::GetExceptionStackTrace(exception, stackString);
                StoreEventSource::Events->StoreException(
                    traceComponent_->PartitionId, traceComponent_->TraceTag,
                    Data::Utilities::ToStringLiteral(methodName),
                    id_, 0,
                    exception.GetStatus(),
                    Data::Utilities::ToStringLiteral(stackString));
            }

        protected:
            StoreTransaction(
                __in LONG64 id,
                __in TxnReplicator::TransactionBase & transaction,
                __in LONG64 owner,
                __in ConcurrentDictionary2<LONG64, KSharedPtr<StoreTransaction<TKey, TValue>>>& container,
                __in IComparer<TKey> & keyComparer,
                __in StoreTraceComponent & traceComponent);

            StoreTransaction(
                __in LONG64 id,
                __in LONG64 owner,
                __in IComparer<TKey> & keyComparer,
                __in StoreTraceComponent & traceComponent);

        private:
            KSharedPtr<WriteSetStoreComponent<TKey, TValue>> writeSetStoreComponentSPtr_;
            long primaryOperationCount_;
            bool isPrimeLockAcquired_;
            bool isCompleted_;
            bool isClosed_;
            bool isReadOnly_;
            LONG64 id_;
            LONG64 owner_;
            KSpinLock lock_;
            KSharedArray<KSharedPtr<LockControlBlock>>::SPtr keyLockRequestsSPtr_;
            KSharedArray<KSharedPtr<PrimeLockRequest>>::SPtr primeLockRequestsSPtr_;
            bool status_;
            LONG64 clearLocks_;
            StoreTransactionReadIsolationLevel::Enum readIsolationLevel_;
            LockingHints::Enum lockingHints_;
            KSharedPtr<ConcurrentDictionary2<LONG64, KSharedPtr<StoreTransaction<TKey, TValue>>>> containerSPtr_;
            TxnReplicator::TransactionBase::SPtr replicatorTransactionSPtr_;
            KSharedPtr<IComparer<TKey>> keyComparerSPtr_;

            StoreTraceComponent::SPtr traceComponent_;
        };

        template <typename TKey, typename TValue>
        StoreTransaction<TKey, TValue>::StoreTransaction(
            __in LONG64 id,
            __in TxnReplicator::TransactionBase& transaction,
            __in LONG64 owner,
            __in ConcurrentDictionary2<LONG64, KSharedPtr<StoreTransaction<TKey, TValue>>>& container,
            __in IComparer<TKey> & keyComparer,
            __in StoreTraceComponent & traceComponent)
            : primaryOperationCount_(0),
            isPrimeLockAcquired_(false),
            isCompleted_(false),
            isClosed_(false),
            isReadOnly_(false),
            id_(id),
            owner_(owner),
            keyLockRequestsSPtr_(nullptr),
            primeLockRequestsSPtr_(nullptr),
            status_(true),
            clearLocks_(0),
            containerSPtr_(&container),
            replicatorTransactionSPtr_(&transaction),
            keyComparerSPtr_(&keyComparer),
            traceComponent_(&traceComponent)
        {
            keyLockRequestsSPtr_ = _new(STORETRANSACTION_TAG, this->GetThisAllocator()) KSharedArray<KSharedPtr<LockControlBlock>>();
            primeLockRequestsSPtr_ = _new(STORETRANSACTION_TAG, this->GetThisAllocator()) KSharedArray<KSharedPtr<PrimeLockRequest>>();

            readIsolationLevel_ = StoreTransactionReadIsolationLevel::Enum::Snapshot;
            lockingHints_ = LockingHints::Enum::None;
        }

        template <typename TKey, typename TValue>
        StoreTransaction<TKey, TValue>::StoreTransaction(
            __in LONG64 id,
            __in LONG64 owner,
            __in IComparer<TKey> & keyComparer,
            __in StoreTraceComponent & traceComponent)
            : primaryOperationCount_(0),
            isPrimeLockAcquired_(false),
            isCompleted_(false),
            isReadOnly_(false),
            isClosed_(false),
            id_(id),
            owner_(owner),
            keyLockRequestsSPtr_(nullptr),
            primeLockRequestsSPtr_(nullptr),
            status_(true),
            clearLocks_(0),
            keyComparerSPtr_(&keyComparer),
            traceComponent_(&traceComponent)
        {
            keyLockRequestsSPtr_ = _new(STORETRANSACTION_TAG, this->GetThisAllocator()) KSharedArray<KSharedPtr<LockControlBlock>>();
            primeLockRequestsSPtr_ = _new(STORETRANSACTION_TAG, this->GetThisAllocator()) KSharedArray<KSharedPtr<PrimeLockRequest>>();

            readIsolationLevel_ = StoreTransactionReadIsolationLevel::Enum::Snapshot;
            lockingHints_ = LockingHints::Enum::None;
        }

        template <typename TKey, typename TValue>
        StoreTransaction<TKey, TValue>::~StoreTransaction()
        {
        }
    }
}
