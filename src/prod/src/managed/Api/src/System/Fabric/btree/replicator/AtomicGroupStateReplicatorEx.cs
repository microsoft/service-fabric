// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.Replicator
{
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Threading;
    using System.Threading.Tasks;

    internal class AtomicGroupStateReplicatorEx : IAtomicGroupStateReplicatorEx
    {
        private NativeRuntime.IFabricAtomicGroupStateReplicatorEx nativeStateReplicator;
        private OperationDataFactoryWrapper operationDataFactory;
        
        public AtomicGroupStateReplicatorEx(NativeRuntime.IFabricAtomicGroupStateReplicatorEx nativeStateReplicator, NativeRuntime.IOperationDataFactory nativeOperationDataFactory)
        {
            Requires.Argument("nativeStateReplicator", nativeStateReplicator).NotNull();
            Requires.Argument("nativeOperationDataFactory", nativeOperationDataFactory).NotNull();

            this.nativeStateReplicator = nativeStateReplicator;
            
            this.operationDataFactory = new OperationDataFactoryWrapper(nativeOperationDataFactory);
        }

        public long ReserveSequenceNumber()
        {
            return Utility.WrapNativeSyncInvoke(() => this.nativeStateReplicator.ReserveSequenceNumber(), "AtomicGroupStateReplicatorEx.ReserveSequenceNumber");
        }

        public bool CancelSequenceNumber(long sequenceNumber)
        {
            return Utility.WrapNativeSyncInvoke(
                () => NativeTypes.FromBOOLEAN(this.nativeStateReplicator.CancelSequenceNumber(sequenceNumber)), 
                "AtomicGroupStateReplicatorEx.CancelSequenceNumberReservation");
        }

        public Task<long> ReplicateOperationAsync(IOperationData serviceMetadata, IOperationData redoInfo, CancellationToken cancellationToken, ref long sequenceNumber)
        {
            Requires.Argument("redoInfo", redoInfo).NotNull();

            long sequenceNumberOut = sequenceNumber;

            var task = Utility.WrapNativeAsyncInvoke<long>(
                (callback) =>
                {
                    NativeCommon.IFabricAsyncOperationContext context;
                    sequenceNumberOut = this.ReplicateBeginWrapper(serviceMetadata, redoInfo, sequenceNumberOut, callback, out context);
                    return context;
                },
                this.ReplicateEndWrapper,
                cancellationToken,
                "AtomicGroupStateReplicatorEx.ReplicateAsync");

            sequenceNumber = sequenceNumberOut;
            return task;
        }

        public Task<long> CreateAtomicGroupAsync(CancellationToken cancellationToken, ref long atomicGroupId)
        {
            long atomicGroupIdOut = atomicGroupId;

            var task = Utility.WrapNativeAsyncInvoke<long>(
                (callback) =>
                {
                    NativeCommon.IFabricAsyncOperationContext context;
                    atomicGroupIdOut = this.CreateAtomicGroupBeginWrapper(atomicGroupIdOut, callback, out context);
                    return context;
                },
                this.CreateAtomicGroupEndWrapper,
                cancellationToken,
                "AtomicGroupStateReplicatorEx.CreateAtomicGroupAsync");

            atomicGroupId = atomicGroupIdOut;
            return task;
        }

        public Task<long> ReplicateAtomicGroupOperationAsync(long atomicGroupId, IOperationData serviceMetadata, IOperationData redoInfo, IOperationData undoInfo, CancellationToken cancellationToken, ref long sequenceNumber)
        {
            Requires.Argument("redoInfo", redoInfo).NotNull();
            Requires.Argument("undoInfo", undoInfo).NotNull();

            long sequenceNumberOut = sequenceNumber;

            var task = Utility.WrapNativeAsyncInvoke<long>(
                (callback) =>
                {
                    NativeCommon.IFabricAsyncOperationContext context;
                    sequenceNumberOut = this.ReplicateAtomicGroupOperationBeginWrapper(atomicGroupId, serviceMetadata, redoInfo, undoInfo, sequenceNumberOut, callback, out context);
                    return context;
                },
                this.ReplicateAtomicGroupOperationEndWrapper,
                cancellationToken,
                "AtomicGroupStateReplicatorEx.ReplicateAtomicGroupOperationAsync");

            sequenceNumber = sequenceNumberOut;
            return task;
        }

        public Task<long> ReplicateAtomicGroupCommitAsync(long atomicGroupId, CancellationToken cancellationToken, ref long commitSequenceNumber)
        {
            long commitSequenceNumberOut = commitSequenceNumber;

            var task = Utility.WrapNativeAsyncInvoke<long>(
                (callback) =>
                {
                    NativeCommon.IFabricAsyncOperationContext context;
                    commitSequenceNumberOut = this.ReplicateAtomicGroupCommitBeginWrapper(atomicGroupId, commitSequenceNumberOut, callback, out context);
                    return context;
                },
                this.ReplicationAtomicGroupCommitEndWrapper,
                cancellationToken,
                "AtomicGroupStateReplicatorEx.ReplicateAtomicGroupCommitAsync");

            commitSequenceNumber = commitSequenceNumberOut;
            return task;
        }

        public Task<long> ReplicateAtomicGroupRollbackAsync(long atomicGroupId, CancellationToken cancellationToken)
        {
            var task = Utility.WrapNativeAsyncInvoke<long>(
                (callback) => this.ReplicateAtomicGroupRollbackBeginWrapper(atomicGroupId, callback),
                this.ReplicationAtomicGroupRollbackEndWrapper,
                cancellationToken,
                "AtomicGroupStateReplicatorEx.ReplicateAtomicGroupRollbackAsync");

            return task;
        }

        public IOperationStreamEx GetCopyStream()
        {
            return Utility.WrapNativeSyncInvoke(() => new OperationStreamEx(this.nativeStateReplicator.GetCopyStream()), "AtomicGroupStateReplicatorEx.GetCopyStream");
        }

        public IOperationStreamEx GetReplicationStream()
        {
            return Utility.WrapNativeSyncInvoke(() => new OperationStreamEx(this.nativeStateReplicator.GetReplicationStream()), "AtomicGroupStateReplicatorEx.GetReplicationStream");
        }

        public IOperationStreamEx GetRecoveryStream()
        {
            return Utility.WrapNativeSyncInvoke(() => new OperationStreamEx(this.nativeStateReplicator.GetRecoveryStream()), "AtomicGroupStateReplicatorEx.GetRecoveryStream");
        }

        public void UpdateReplicatorSettings(ReplicatorSettings settings)
        {
            Utility.WrapNativeSyncInvoke(() => this.UpdateReplicatorSettingsHelper(settings), "AtomicGroupStateReplicatorEx.UpdateReplicatorSettings");
        }

        private NativeRuntime.IFabricOperationData GetOrCreateOperationData(IOperationData operationData)
        {
            OperationData managed = operationData as OperationData;
            if (managed != null)
            {
                return this.operationDataFactory.CreateOperationData(managed);
            }

            ReadOnlyOperationData native = operationData as ReadOnlyOperationData;
            if (native != null)
            {
                return native.NativeOperationData;
            }

            return null;
        }

        private long ReplicateBeginWrapper(IOperationData serviceMetadata, IOperationData redoInfo, long sequenceNumberIn, NativeCommon.IFabricAsyncOperationCallback callback, out NativeCommon.IFabricAsyncOperationContext context)
        {
            long sequenceNumber = sequenceNumberIn;

            context = this.nativeStateReplicator.BeginReplicateOperation(
                this.GetOrCreateOperationData(serviceMetadata),
                this.GetOrCreateOperationData(redoInfo),
                callback,
                ref sequenceNumber);

            return sequenceNumber;
        }

        private long ReplicateEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            return this.nativeStateReplicator.EndReplicateOperation(context);
        }

        private long CreateAtomicGroupBeginWrapper(long atomicGroupId, NativeCommon.IFabricAsyncOperationCallback callback, out NativeCommon.IFabricAsyncOperationContext context)
        {
            long atomicGroupIdOut = atomicGroupId;

            context = this.nativeStateReplicator.BeginCreateAtomicGroup(callback, ref atomicGroupIdOut);

            return atomicGroupIdOut;
        }

        private long CreateAtomicGroupEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            return this.nativeStateReplicator.EndCreateAtomicGroup(context);
        }

        private long ReplicateAtomicGroupOperationBeginWrapper(long atomicGroupId, IOperationData serviceMetadata, IOperationData redoInfo, IOperationData undoInfo, long sequenceNumberIn, NativeCommon.IFabricAsyncOperationCallback callback, out NativeCommon.IFabricAsyncOperationContext context)
        {
            long sequenceNumberOut = sequenceNumberIn;

            context = this.nativeStateReplicator.BeginReplicateAtomicGroupOperation(
                atomicGroupId,
                this.GetOrCreateOperationData(serviceMetadata),
                this.GetOrCreateOperationData(redoInfo),
                this.GetOrCreateOperationData(undoInfo),
                callback,
                ref sequenceNumberOut);

            return sequenceNumberOut;
        }

        private long ReplicateAtomicGroupOperationEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            return this.nativeStateReplicator.EndReplicateAtomicGroupOperation(context);
        }

        private long ReplicateAtomicGroupCommitBeginWrapper(long atomicGroupId, long sequenceNumberIn, NativeCommon.IFabricAsyncOperationCallback callback, out NativeCommon.IFabricAsyncOperationContext context)
        {
            long operationSequenceNumberOut = sequenceNumberIn;

            context = this.nativeStateReplicator.BeginReplicateAtomicGroupCommit(atomicGroupId, callback, ref operationSequenceNumberOut);

            return operationSequenceNumberOut;
        }

        private long ReplicationAtomicGroupCommitEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            return this.nativeStateReplicator.EndReplicateAtomicGroupCommit(context);
        }

        private NativeCommon.IFabricAsyncOperationContext ReplicateAtomicGroupRollbackBeginWrapper(long atomicGroupId, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            return this.nativeStateReplicator.BeginReplicateAtomicGroupRollback(atomicGroupId, callback);
        }

        private long ReplicationAtomicGroupRollbackEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            return this.nativeStateReplicator.EndReplicateAtomicGroupRollback(context);
        }

        private void UpdateReplicatorSettingsHelper(ReplicatorSettings replicatorSettings)
        {
            // Initialize replicator
            if (replicatorSettings == null)
            {
                AppTrace.TraceSource.WriteNoise("AtomicGroupStateReplicatorEx.UpdateReplicatorSettings", "Using default replicator settings");
            }
            else
            {
                AppTrace.TraceSource.WriteNoise("AtomicGroupStateReplicatorEx.UpdateReplicatorSettings", "Replicator Settings - address {0}, listenAddress {1}, publishAddress {2}, retyrInterval {3}, ackInterval {4}, credentials provided {5}", replicatorSettings.ReplicatorAddress, replicatorSettings.ReplicatorListenAddress, replicatorSettings.ReplicatorPublishAddress, replicatorSettings.RetryInterval, replicatorSettings.BatchAcknowledgementInterval, replicatorSettings.SecurityCredentials != null);
            }

            using (var pin = new PinCollection())
            {
                unsafe
                {
                    if (replicatorSettings == null)
                    {
                        this.nativeStateReplicator.UpdateReplicatorSettings(IntPtr.Zero);
                    }
                    else
                    {
                        IntPtr nativeReplicatorSettings = replicatorSettings.ToNative(pin);
                        this.nativeStateReplicator.UpdateReplicatorSettings(nativeReplicatorSettings);
                    }
                }
            }
        }
    }
}