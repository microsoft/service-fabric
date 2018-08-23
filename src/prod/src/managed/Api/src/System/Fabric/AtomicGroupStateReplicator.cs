// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Threading;
    using System.Threading.Tasks;

    internal sealed class AtomicGroupStateReplicator : StateReplicator, IAtomicGroupStateReplicator
    {
        private readonly NativeRuntime.IFabricAtomicGroupStateReplicator nativeAtomicGroupStateReplicator;

        public AtomicGroupStateReplicator(NativeRuntime.IFabricAtomicGroupStateReplicator nativeAtomicGroupStateReplicator, NativeRuntime.IFabricStateReplicator nativeStateReplicator, NativeRuntime.IOperationDataFactory nativeOperationDataFactory)
            : base(nativeStateReplicator, nativeOperationDataFactory)
        {
            Requires.Argument("nativeAtomicGroupStateReplicator", nativeAtomicGroupStateReplicator).NotNull();

            this.nativeAtomicGroupStateReplicator = nativeAtomicGroupStateReplicator;
        }

        public long CreateAtomicGroup()
        {
            return Utility.WrapNativeSyncInvoke<long>(this.nativeAtomicGroupStateReplicator.CreateAtomicGroup, "IFabricAtomicGroupStateReplicator.CreateAtomicGroup");
        }

        public Task<long> ReplicateAtomicGroupOperationAsync(long atomicGroupId, OperationData operationData, CancellationToken cancellationToken, out long sequenceNumber)
        {
            Requires.Argument("operationData", operationData).NotNull();
            long sequenceNumberOut = 0;

            var returnValue = Utility.WrapNativeAsyncInvoke<long>(
                (callback) =>
                {
                    NativeCommon.IFabricAsyncOperationContext context;
                    sequenceNumberOut = this.ReplicateBeginWrapper(atomicGroupId, operationData, callback, out context);
                    return context;
                },
                this.ReplicateEndWrapper,
                cancellationToken,
                "AtomicGroupStateReplicator.ReplicateAtomicGroupOperation");

            sequenceNumber = sequenceNumberOut;

            return returnValue;
        }

        public Task<long> ReplicateAtomicGroupCommitAsync(long atomicGroupId, CancellationToken cancellationToken, out long commitSequenceNumber)
        {
            long sequenceNumberOut = 0;

            var returnValue = Utility.WrapNativeAsyncInvoke<long>(
                (callback) =>
                {
                    NativeCommon.IFabricAsyncOperationContext context;
                    sequenceNumberOut = this.CommitBeginWrapper(atomicGroupId, callback, out context);
                    return context;
                },
                this.CommitEndWrapper,
                cancellationToken,
                "AtomicGroupStateReplicator.ReplicateAtomicGroupCommit");

            commitSequenceNumber = sequenceNumberOut;

            return returnValue;
        }

        public Task<long> ReplicateAtomicGroupRollbackAsync(long atomicGroupId, CancellationToken cancellationToken, out long rollbackSequenceNumber)
        {
            long sequenceNumberOut = 0;

            var returnValue = Utility.WrapNativeAsyncInvoke<long>(
                (callback) =>
                {
                    NativeCommon.IFabricAsyncOperationContext context;
                    sequenceNumberOut = this.RollbackBeginWrapper(atomicGroupId, callback, out context);
                    return context;
                },
                this.RollbackEndWrapper,
                cancellationToken,
                "AtomicGroupStateReplicator.ReplicateAtomicGroupRollback");

            rollbackSequenceNumber = sequenceNumberOut;

            return returnValue;
        }

        private long ReplicateBeginWrapper(long atomicGroupId, OperationData operationData, NativeCommon.IFabricAsyncOperationCallback callback, out NativeCommon.IFabricAsyncOperationContext context)
        {
            long sequenceNumber = 0;

            context = this.nativeAtomicGroupStateReplicator.BeginReplicateAtomicGroupOperation(
                atomicGroupId,
                this.OperationDataFactory.CreateOperationData(operationData),
                callback,
                out sequenceNumber);

            return sequenceNumber;
        }

        private long ReplicateEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            return this.nativeAtomicGroupStateReplicator.EndReplicateAtomicGroupOperation(context);
        }

        private long CommitBeginWrapper(long atomicGroupId, NativeCommon.IFabricAsyncOperationCallback callback, out NativeCommon.IFabricAsyncOperationContext context)
        {
            long sequenceNumber = 0;

            context = this.nativeAtomicGroupStateReplicator.BeginReplicateAtomicGroupCommit(
                atomicGroupId,
                callback,
                out sequenceNumber);

            return sequenceNumber;
        }

        private long CommitEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            return this.nativeAtomicGroupStateReplicator.EndReplicateAtomicGroupCommit(context);
        }

        private long RollbackBeginWrapper(long atomicGroupId, NativeCommon.IFabricAsyncOperationCallback callback, out NativeCommon.IFabricAsyncOperationContext context)
        {
            long sequenceNumber = 0;

            context = this.nativeAtomicGroupStateReplicator.BeginReplicateAtomicGroupRollback(
                atomicGroupId,
                callback,
                out sequenceNumber);

            return sequenceNumber;
        }

        private long RollbackEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            return this.nativeAtomicGroupStateReplicator.EndReplicateAtomicGroupRollback(context);
        }
    }
}