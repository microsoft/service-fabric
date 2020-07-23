// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.Replicator
{
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Threading;
    using System.Threading.Tasks;

    public interface IAtomicGroupStateProviderEx 
    {
        long GetLastCommittedSequenceNumber();

        Task UpdateEpochAsync(Epoch epoch, long previousEpochLastSequenceNumber, CancellationToken cancellationToken);

        [SuppressMessage(FxCop.Category.MSInternal, FxCop.Rule.UseApprovedGenericsForPrecompiledAssemblies, Justification = "Not precompiled assembly.")]
        Task<bool> OnDataLossAsync(CancellationToken cancellationToken);

        IOperationDataStreamEx GetCopyContext();

        [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1305:FieldNamesMustNotUseHungarianNotation", Justification = "Not Hungarian notation.")]
        IOperationDataStreamEx GetCopyState(long upToSequenceNumber, IOperationDataStreamEx copyContext);

        Task AtomicGroupCommitAsync(long atomicGroupId, long commitSequenceNumber, CancellationToken cancellationToken);

        Task AtomicGroupRollbackAsync(long atomicGroupId, IOperationStreamEx undoOperationStream, CancellationToken cancellationToken);

        Task AtomicGroupAbortAsync(long atomicGroupId, CancellationToken cancellationToken);

        Task UndoProgressAsync(long fromSequenceNumber, CancellationToken cancellationToken);

        Task CheckpointAsync(long sequenceNumber, CancellationToken cancellationToken);

        Task<long> OnOperationStableAsync(long sequenceNumber, CancellationToken cancellationToken);
    }
}