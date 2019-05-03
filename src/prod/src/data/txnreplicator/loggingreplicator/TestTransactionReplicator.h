// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace LoggingReplicatorTests
{
    // Test Transaction Replicator
    class TestTransactionReplicator
        : public KObject<TestTransactionReplicator>
        , public KShared<TestTransactionReplicator>
        , public KWeakRefType<TestTransactionReplicator>
        , public TxnReplicator::ITransactionalReplicator
    {
        K_FORCE_SHARED(TestTransactionReplicator)
        K_SHARED_INTERFACE_IMP(ITransactionalReplicator)
        K_SHARED_INTERFACE_IMP(IStateProviderMap)
        K_SHARED_INTERFACE_IMP(ITransactionManager)
        K_SHARED_INTERFACE_IMP(ITransactionVersionManager)
        K_SHARED_INTERFACE_IMP(IVersionManager)
        K_WEAKREF_INTERFACE_IMP(ITransactionManager, TestTransactionReplicator)
        K_WEAKREF_INTERFACE_IMP(ITransactionalReplicator, TestTransactionReplicator)

    public:
        static SPtr TestTransactionReplicator::Create(__in KAllocator & allocator);

    public: // ITransactionalReplicator interface.
        bool get_IsReadable() const
        {
            throw ktl::Exception(STATUS_NOT_IMPLEMENTED);
        }

        __declspec(property(get = get_HasPersistedState)) bool HasPersistedState;
        bool get_HasPersistedState() const override
        {
            return true;
        }

        Data::Utilities::IStatefulPartition::SPtr get_StatefulPartition() const
        {
            CODING_ASSERT("STATUS_NOT_IMPLEMENTED");
        }

        NTSTATUS GetLastStableSequenceNumber(
            __out LONG64 & lsn) noexcept
        {
            UNREFERENCED_PARAMETER(lsn);

            CODING_ASSERT("STATUS_NOT_IMPLEMENTED");
        }

        NTSTATUS GetLastCommittedSequenceNumber(
            __out LONG64 & lsn) noexcept
        {
            UNREFERENCED_PARAMETER(lsn);

            CODING_ASSERT("STATUS_NOT_IMPLEMENTED");
        }

        NTSTATUS GetCurrentEpoch(
            __out FABRIC_EPOCH & epoch) noexcept
        {
            UNREFERENCED_PARAMETER(epoch);

            CODING_ASSERT("STATUS_NOT_IMPLEMENTED");
        }

        NTSTATUS RegisterTransactionChangeHandler(
            __in TxnReplicator::ITransactionChangeHandler & transactionChangeHandler) noexcept override;

        NTSTATUS UnRegisterTransactionChangeHandler() noexcept override;

        NTSTATUS RegisterStateManagerChangeHandler(
            __in TxnReplicator::IStateManagerChangeHandler & stateManagerChangeHandler) noexcept
        {
            UNREFERENCED_PARAMETER(stateManagerChangeHandler);
            CODING_ASSERT("STATUS_NOT_IMPLEMENTED");
        }

        NTSTATUS UnRegisterStateManagerChangeHandler() noexcept
        {
            CODING_ASSERT("STATUS_NOT_IMPLEMENTED");
        }

        ktl::Awaitable<NTSTATUS> BackupAsync(
            __in TxnReplicator::IBackupCallbackHandler & backupCallbackHandler,
            __out TxnReplicator::BackupInfo & backupInfo) noexcept
        {
            UNREFERENCED_PARAMETER(backupCallbackHandler);
            UNREFERENCED_PARAMETER(backupInfo);
            CODING_ASSERT("STATUS_NOT_IMPLEMENTED");
        }

        ktl::Awaitable<NTSTATUS> BackupAsync(
            __in TxnReplicator::IBackupCallbackHandler & backupCallbackHandler,
            __in FABRIC_BACKUP_OPTION backupOption,
            __in Common::TimeSpan const & timeout,
            __in ktl::CancellationToken const & cancellationToken,
            __out TxnReplicator::BackupInfo & backupInfo) noexcept
        {
            UNREFERENCED_PARAMETER(backupCallbackHandler);
            UNREFERENCED_PARAMETER(backupOption);
            UNREFERENCED_PARAMETER(timeout);
            UNREFERENCED_PARAMETER(cancellationToken);
            UNREFERENCED_PARAMETER(backupInfo);

            CODING_ASSERT("STATUS_NOT_IMPLEMENTED");
        }

        ktl::Awaitable<NTSTATUS> RestoreAsync(
            __in KString const & backupFolder) noexcept
        {
            UNREFERENCED_PARAMETER(backupFolder);

            CODING_ASSERT("STATUS_NOT_IMPLEMENTED");
        }

        ktl::Awaitable<NTSTATUS> RestoreAsync(
            __in KString const & backupFolder,
            __in FABRIC_RESTORE_POLICY restorePolicy,
            __in Common::TimeSpan const & timeout,
            __in ktl::CancellationToken const & cancellationToken) noexcept
        {
            UNREFERENCED_PARAMETER(backupFolder);
            UNREFERENCED_PARAMETER(restorePolicy);
            UNREFERENCED_PARAMETER(timeout);
            UNREFERENCED_PARAMETER(cancellationToken);

            CODING_ASSERT("STATUS_NOT_IMPLEMENTED");
        }

        public: // IStateProviderMap interface.
            NTSTATUS Get(
                __in KUriView const & stateProviderName,
                __out TxnReplicator::IStateProvider2::SPtr & stateProvider) noexcept
            {
                UNREFERENCED_PARAMETER(stateProviderName);
                UNREFERENCED_PARAMETER(stateProvider);

                return STATUS_NOT_IMPLEMENTED;
            }

            ktl::Awaitable<NTSTATUS> AddAsync(
                __in TxnReplicator::Transaction & transaction,
                __in KUriView const & stateProviderName,
                __in KStringView const & stateProvider,
                __in_opt Data::Utilities::OperationData const * const initializationParameters = nullptr,
                __in_opt Common::TimeSpan const & timeout = Common::TimeSpan::MaxValue,
                __in_opt ktl::CancellationToken const & cancellationToken = ktl::CancellationToken::None) noexcept
            {
                UNREFERENCED_PARAMETER(transaction);
                UNREFERENCED_PARAMETER(stateProvider);
                UNREFERENCED_PARAMETER(stateProviderName);
                UNREFERENCED_PARAMETER(initializationParameters);
                UNREFERENCED_PARAMETER(timeout);
                UNREFERENCED_PARAMETER(cancellationToken);

                co_return STATUS_NOT_IMPLEMENTED;
            }

            ktl::Awaitable<NTSTATUS> RemoveAsync(
                __in TxnReplicator::Transaction & transaction,
                __in KUriView const & stateProviderName,
                __in_opt Common::TimeSpan const & timeout = Common::TimeSpan::MaxValue,
                __in_opt ktl::CancellationToken const & cancellationToken = ktl::CancellationToken::None) noexcept
            {
                UNREFERENCED_PARAMETER(transaction);
                UNREFERENCED_PARAMETER(stateProviderName);
                UNREFERENCED_PARAMETER(timeout);
                UNREFERENCED_PARAMETER(cancellationToken);

                co_return STATUS_NOT_IMPLEMENTED;
            }

            NTSTATUS CreateEnumerator(
                __in bool parentsOnly,
                __out Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, TxnReplicator::IStateProvider2::SPtr>>::SPtr & outEnumerator) noexcept
            {
                UNREFERENCED_PARAMETER(parentsOnly);
                UNREFERENCED_PARAMETER(outEnumerator);

                return STATUS_NOT_IMPLEMENTED;
            }

            ktl::Awaitable<NTSTATUS> GetOrAddAsync(
                __in TxnReplicator::Transaction & transaction,
                __in KUriView const & stateProviderName,
                __in KStringView const & stateProviderTypeName,
                __out TxnReplicator::IStateProvider2::SPtr & stateProvider,
                __out bool & alreadyExist,
                __in_opt Data::Utilities::OperationData const * const initializationParameters,
                __in Common::TimeSpan const & timeout,
                __in ktl::CancellationToken const & cancellationToken) noexcept
            {
                UNREFERENCED_PARAMETER(transaction);
                UNREFERENCED_PARAMETER(stateProviderName);
                UNREFERENCED_PARAMETER(stateProviderTypeName);
                UNREFERENCED_PARAMETER(stateProvider);
                UNREFERENCED_PARAMETER(alreadyExist);
                UNREFERENCED_PARAMETER(initializationParameters);
                UNREFERENCED_PARAMETER(timeout);
                UNREFERENCED_PARAMETER(cancellationToken);

                co_return STATUS_NOT_IMPLEMENTED;
            }

        public: // IVersionManager
            ktl::Awaitable<NTSTATUS> TryRemoveCheckpointAsync(
                __in FABRIC_SEQUENCE_NUMBER checkpointLsnToBeRemoved,
                __in FABRIC_SEQUENCE_NUMBER nextCheckpointLsn) noexcept
            {
                UNREFERENCED_PARAMETER(checkpointLsnToBeRemoved);
                UNREFERENCED_PARAMETER(nextCheckpointLsn);
                
                co_return STATUS_NOT_IMPLEMENTED;
            }

            NTSTATUS TryRemoveVersion(
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in FABRIC_SEQUENCE_NUMBER commitLsn,
                __in FABRIC_SEQUENCE_NUMBER nextCommitLsn,
                __out TxnReplicator::TryRemoveVersionResult::SPtr & result) noexcept
            {
                UNREFERENCED_PARAMETER(stateProviderId);
                UNREFERENCED_PARAMETER(commitLsn);
                UNREFERENCED_PARAMETER(nextCommitLsn);
                UNREFERENCED_PARAMETER(result);

                return STATUS_NOT_IMPLEMENTED;
            }
            
            FABRIC_REPLICA_ROLE get_Role() const override
            {
                throw ktl::Exception(STATUS_NOT_IMPLEMENTED);
            }

            // Create Transaction
            NTSTATUS CreateTransaction(__out TxnReplicator::Transaction::SPtr & txn) noexcept
            {
                UNREFERENCED_PARAMETER(txn);
                return STATUS_NOT_IMPLEMENTED;
            }

            NTSTATUS CreateAtomicOperation(__out TxnReplicator::AtomicOperation::SPtr & txn) noexcept
            {
                UNREFERENCED_PARAMETER(txn);
                return STATUS_NOT_IMPLEMENTED;
            }

            NTSTATUS CreateAtomicRedoOperation(__out TxnReplicator::AtomicRedoOperation::SPtr & txn) noexcept
            {
                UNREFERENCED_PARAMETER(txn);
                return STATUS_NOT_IMPLEMENTED;
            }

            NTSTATUS SingleOperationTransactionAbortUnlock(
                __in LONG64 stateProviderId,
                __in TxnReplicator::OperationContext const & operationContext) noexcept
            {
                UNREFERENCED_PARAMETER(stateProviderId);
                UNREFERENCED_PARAMETER(operationContext);
                return STATUS_NOT_IMPLEMENTED;
            }

            NTSTATUS ErrorIfStateProviderIsNotRegistered(
                __in LONG64 stateProviderId,
                __in LONG64 transactionId) noexcept
            {
                UNREFERENCED_PARAMETER(stateProviderId);
                UNREFERENCED_PARAMETER(transactionId);
                return STATUS_NOT_IMPLEMENTED;
            }

            NTSTATUS BeginTransaction(
                __in TxnReplicator::Transaction & transaction,
                __in_opt Data::Utilities::OperationData const * const metaData,
                __in_opt Data::Utilities::OperationData const * const undo,
                __in_opt Data::Utilities::OperationData const * const redo,
                __in LONG64 stateProviderId,
                __in_opt TxnReplicator::OperationContext const * const operationContext) noexcept
            {
                UNREFERENCED_PARAMETER(transaction);
                UNREFERENCED_PARAMETER(metaData);
                UNREFERENCED_PARAMETER(undo);
                UNREFERENCED_PARAMETER(redo);
                UNREFERENCED_PARAMETER(stateProviderId);
                UNREFERENCED_PARAMETER(operationContext);
                
                return STATUS_NOT_IMPLEMENTED;
            }

            ktl::Awaitable<NTSTATUS> BeginTransactionAsync(
                __in TxnReplicator::Transaction & transaction,
                __in_opt Data::Utilities::OperationData const * const metaData,
                __in_opt Data::Utilities::OperationData const * const undo,
                __in_opt Data::Utilities::OperationData const * const redo,
                __in LONG64 stateProviderId,
                __in_opt TxnReplicator::OperationContext const * const operationContext,
                __out LONG64 & lsn) noexcept
            {
                UNREFERENCED_PARAMETER(transaction);
                UNREFERENCED_PARAMETER(metaData);
                UNREFERENCED_PARAMETER(undo);
                UNREFERENCED_PARAMETER(redo);
                UNREFERENCED_PARAMETER(stateProviderId);
                UNREFERENCED_PARAMETER(operationContext);
                UNREFERENCED_PARAMETER(lsn);

                co_return STATUS_NOT_IMPLEMENTED;
            }

            NTSTATUS AddOperation(
                __in TxnReplicator::Transaction & transaction,
                __in_opt Data::Utilities::OperationData const * const metaData,
                __in_opt Data::Utilities::OperationData const * const undo,
                __in_opt Data::Utilities::OperationData const * const redo,
                __in LONG64 stateProviderId,
                __in_opt TxnReplicator::OperationContext const * const operationContext) noexcept
            {
                UNREFERENCED_PARAMETER(transaction);
                UNREFERENCED_PARAMETER(metaData);
                UNREFERENCED_PARAMETER(undo);
                UNREFERENCED_PARAMETER(redo);
                UNREFERENCED_PARAMETER(stateProviderId);
                UNREFERENCED_PARAMETER(operationContext);

                return STATUS_NOT_IMPLEMENTED;
            }

            ktl::Awaitable<NTSTATUS> AddOperationAsync(
                __in TxnReplicator::AtomicOperation & atomicOperation,
                __in_opt Data::Utilities::OperationData const * const metaData,
                __in_opt Data::Utilities::OperationData const * const undo,
                __in_opt Data::Utilities::OperationData const * const redo,
                __in LONG64 stateProviderId,
                __in_opt TxnReplicator::OperationContext const * const operationContext,
                __out LONG64 & lsn) noexcept
            {
                UNREFERENCED_PARAMETER(atomicOperation);
                UNREFERENCED_PARAMETER(metaData);
                UNREFERENCED_PARAMETER(undo);
                UNREFERENCED_PARAMETER(redo);
                UNREFERENCED_PARAMETER(stateProviderId);
                UNREFERENCED_PARAMETER(operationContext);

                co_return STATUS_NOT_IMPLEMENTED;
            }

            ktl::Awaitable<NTSTATUS> AddOperationAsync(
                __in TxnReplicator::AtomicRedoOperation & atomicRedoOperation,
                __in_opt Data::Utilities::OperationData const * const metaData,
                __in_opt Data::Utilities::OperationData const * const redo,
                __in LONG64 stateProviderId,
                __in_opt TxnReplicator::OperationContext const * const operationContext,
                __out LONG64 & lsn) noexcept
            {
                UNREFERENCED_PARAMETER(atomicRedoOperation);
                UNREFERENCED_PARAMETER(metaData);
                UNREFERENCED_PARAMETER(redo);
                UNREFERENCED_PARAMETER(stateProviderId);
                UNREFERENCED_PARAMETER(operationContext);
                UNREFERENCED_PARAMETER(lsn);

                co_return STATUS_NOT_IMPLEMENTED;
            }

            ktl::Awaitable<NTSTATUS> CommitTransactionAsync(
                __in TxnReplicator::Transaction & transaction,
                __out LONG64 & commitLsn) noexcept
            {
                UNREFERENCED_PARAMETER(transaction);
                UNREFERENCED_PARAMETER(commitLsn);

                co_return STATUS_NOT_IMPLEMENTED;
            }

            ktl::Awaitable<NTSTATUS> AbortTransactionAsync(
                __in TxnReplicator::Transaction & transaction,
                __out LONG64 & abortLsn) noexcept
            {
                UNREFERENCED_PARAMETER(transaction);
                UNREFERENCED_PARAMETER(abortLsn);

                co_return STATUS_NOT_IMPLEMENTED;
            }

            ktl::Awaitable<NTSTATUS> RegisterAsync(
                __out FABRIC_SEQUENCE_NUMBER & LSN) noexcept
            {
                UNREFERENCED_PARAMETER(LSN);
                
                co_return STATUS_NOT_IMPLEMENTED;
            }

            NTSTATUS UnRegister(
                __in FABRIC_SEQUENCE_NUMBER visibilityVersionNumber) noexcept
            {
                UNREFERENCED_PARAMETER(visibilityVersionNumber);

                return STATUS_NOT_IMPLEMENTED;
            }
    };
}
