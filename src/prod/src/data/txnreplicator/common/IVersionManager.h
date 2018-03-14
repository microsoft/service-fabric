// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    /// <summary>
    /// This is for internal use only.
    /// Interface for the Version Manager.
    /// Used to provide snapshot isolation.
    /// </summary>
    interface IVersionManager
    {
        K_SHARED_INTERFACE(IVersionManager)

    public:
        /// <summary>
        /// Called to determine whether relevant checkpoint file can be removed
        /// </summary>
        /// <param name="checkpointLsnToBeRemoved">Version of the checkpoint in question.</param>
        /// <param name="nextcheckpointLsn">Lsn of the next checkpoint after the checkpoint in question.</param>
        /// <returns>
        /// Task that represents when it can be removed.
        /// If immediately, null will be returned.
        /// Otherwise, the task will fire when the checkpoint can be removed.
        /// </returns>
        virtual ktl::Awaitable<NTSTATUS> TryRemoveCheckpointAsync(
            __in LONG64 checkpointLsnToBeRemoved,
            __in LONG64 nextCheckpointLsn) noexcept = 0;

        /// <summary>
        /// Called to determine whether relevant version is currently being referenced by a snapshot transaction.
        /// If so, notification tasks will be returned for visibility sequence number completions that state provider has not registered yet.
        /// </summary>
        /// <param name="stateProviderId"></param>
        /// <param name="commitSequenceNumber"></param>
        /// <param name="nextCommitSequenceNumber"></param>
        /// <returns></returns>
        virtual NTSTATUS TryRemoveVersion(
            __in LONG64 stateProviderId,
            __in LONG64 commitLsn,
            __in LONG64 nextCommitLsn,
            __out TryRemoveVersionResult::SPtr & result) noexcept = 0;
    };
}
