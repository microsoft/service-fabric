// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.Replicator
{
    using System.Fabric;
    using System.Threading;
    using System.Threading.Tasks;

    public interface IAtomicGroupStateReplicatorEx
    {
        long ReserveSequenceNumber();

        bool CancelSequenceNumber(long sequenceNumber);

        Task<long> ReplicateOperationAsync(
            IOperationData serviceMetadata,
            IOperationData redoInfo,
            CancellationToken cancellationToken,
            ref long operationSequenceNumber);

        Task<long> CreateAtomicGroupAsync(
            CancellationToken cancellationToken,
            ref long atomicGroupId);

        Task<long> ReplicateAtomicGroupOperationAsync(
            long atomicGroupId,
            IOperationData serviceMetadata,
            IOperationData redoInfo,
            IOperationData undoInfo,
            CancellationToken cancellationToken,
            ref long operationSequenceNumber);

        Task<long> ReplicateAtomicGroupCommitAsync(
            long atomicGroupId,
            CancellationToken cancellationToken,
            ref long commitOperationSequenceNumber);

        Task<long> ReplicateAtomicGroupRollbackAsync(
            long atomicGroupId,
            CancellationToken cancellationToken);

        IOperationStreamEx GetCopyStream();

        IOperationStreamEx GetReplicationStream();

        IOperationStreamEx GetRecoveryStream();

        void UpdateReplicatorSettings(ReplicatorSettings settings);
    }
}