// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    interface IStateProvider2
    {
        K_SHARED_INTERFACE(IStateProvider2);

    public:
        virtual KUriView const GetName() const = 0;

        virtual KArray<StateProviderInfo> GetChildren(KUriView const & rootName) = 0;

    public: // State Provider life cycle operations
            /// <summary>
            /// Initializes state provider.
            /// </summary>
            /// <param name="transactionalReplicator">Transactional Replicator instance.</param>
            /// <param name="stateProviderWorkDirectory">Work directory for state provider checkpoint files.</param>
            /// <param name="childrenStateProviders">Children state providers.</param>
        virtual VOID Initialize(
            __in KWeakRef<ITransactionalReplicator> & transactionalReplicatorWRef,
            __in KStringView const & workFolder,
            __in_opt KSharedArray<SPtr> const * const children) = 0;

        /// <summary>
        /// Called when replica is being opened.
        /// </summary>
        /// <param name="cancellationToken">cancellation token.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        virtual ktl::Awaitable<void> OpenAsync(__in ktl::CancellationToken const & cancellationToken) = 0;

        /// <summary>
        /// ChangeRole is called whenever the replica role is being changed.
        /// </summary>
        /// <param name="newRole">new role to be transition to.</param>
        /// <param name="cancellationToken">cancellation token.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        virtual ktl::Awaitable<void> ChangeRoleAsync(
            __in FABRIC_REPLICA_ROLE newRole,
            __in ktl::CancellationToken const & cancellationToken
        ) = 0;

        /// <summary>
        /// Called when replica is being closed.
        /// </summary>
        /// <param name="cancellationToken">cancellation token.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        virtual ktl::Awaitable<void> CloseAsync(
            __in ktl::CancellationToken const & cancellationToken
        ) = 0;

        /// <summary>
        /// Called when replica is being aborted.
        /// </summary>
        virtual void Abort() noexcept = 0;

    public: // Checkpoint APIs
        virtual void PrepareCheckpoint(
            __in LONG64 checkpointLSN) = 0;
        virtual ktl::Awaitable<void> PerformCheckpointAsync(
            __in ktl::CancellationToken const & cancellationToken) = 0;
        virtual ktl::Awaitable<void> CompleteCheckpointAsync(
            __in ktl::CancellationToken const & cancellationToken) = 0;

        /// <summary>
        /// Called when replica is being opened.
        /// </summary>
        /// <returns>Task that represents the asynchronous operation.</returns>
        virtual ktl::Awaitable<void> RecoverCheckpointAsync(
            __in ktl::CancellationToken const & cancellationToken) = 0;

        /// <summary>
        /// Backup the existing checkpoint state on local disk (if any) to the given directory.
        /// </summary>
        /// <param name="backupDirectory">The directory where the checkpoint backup is to be stored.</param>
        /// <param name="cancellationToken">Request cancellation of the checkpoint backup.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        virtual ktl::Awaitable<void> BackupCheckpointAsync(
            __in KString const & backupDirectory,
            __in ktl::CancellationToken const & cancellationToken) = 0;

        /// <summary>
        /// Restore the checkpoint state from the given directory.
        /// </summary>
        /// <param name="backupDirectory">The directory where the checkpoint backup is stored.</param>
        /// <param name="cancellationToken">Request cancellation of the checkpoint restore.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        /// <remarks>
        /// The previously backed up checkpoint state becomes the current checkpoint state.  The
        /// checkpoint is not recovered from.
        /// </remarks>
        virtual ktl::Awaitable<void> RestoreCheckpointAsync(
            __in KString const & backupDirectory,
            __in ktl::CancellationToken const & cancellationToken) = 0;

        /// <summary>
        /// Remove state.
        /// </summary>
        /// <returns>Task that represents the asynchronous operation.</returns>
        virtual ktl::Awaitable<void> RemoveStateAsync(
            __in ktl::CancellationToken const & cancellationToken) = 0;

        /// <summary>
        /// Gets current copy state from primary.
        /// </summary>
        /// <returns>Operation data stream that contains the current stream.</returns>
        virtual OperationDataStream::SPtr GetCurrentState() = 0;

        /// <summary>
        /// Called at the start of set current state.
        /// </summary>
        virtual ktl::Awaitable<void> BeginSettingCurrentStateAsync() = 0;

        /// <summary>
        /// Sets the state on secondary.
        /// </summary>
        /// <param name="stateRecordNumber">record number.</param>
        /// <param name="data">data that needs to be copied to the secondary.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        virtual ktl::Awaitable<void> SetCurrentStateAsync(
            __in LONG64 stateRecordNumber,
            __in Data::Utilities::OperationData const & operationData,
            __in ktl::CancellationToken const& cancellationToken) = 0;

        /// <summary>
        /// Called at the end of set state. This API will not get called if copy process gets aborted in the middle.
        /// </summary>
        virtual ktl::Awaitable<void> EndSettingCurrentStateAsync(
            __in ktl::CancellationToken const& cancellationToken) = 0;

    public: // Remove State Provider operations.
        virtual ktl::Awaitable<void> PrepareForRemoveAsync(
            __in TransactionBase const & transactionBase,
            __in ktl::CancellationToken const & cancellationToken) = 0;

    public: // State Provider Operations
        virtual ktl::Awaitable<OperationContext::CSPtr> ApplyAsync(
            __in LONG64 logicalSequenceNumber,
            __in TransactionBase const & transactionBase,
            __in ApplyContext::Enum applyContext,
            __in_opt Data::Utilities::OperationData const * const metadataPtr,
            __in_opt Data::Utilities::OperationData const * const dataPtr) = 0;

        // TODO: Consider: Should operationContext be const?
        // Today we are passing in a constant operation context.
        // This is because we are taking it in as constant (promising user to not change it).
        // This makes it interesting since we do not allow the user to change their own object.
        // One workaround would be to const cast at SM just before dispatching.
        virtual void Unlock(__in OperationContext const & operationContext) = 0;
    };
}
