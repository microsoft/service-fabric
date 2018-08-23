// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Fabric.Interop;
    using System.Threading;
    using System.Threading.Tasks;
    using BOOLEAN = System.SByte;

    internal class ReplicatorBroker : NativeRuntime.IFabricReplicator, NativeRuntime.IFabricPrimaryReplicator
    {
        private readonly IReplicator replicator;

        public ReplicatorBroker(IReplicator replicator)
        {
            this.replicator = replicator;
        }

        public NativeCommon.IFabricAsyncOperationContext BeginOpen(NativeCommon.IFabricAsyncOperationCallback callback)
        {
            return Utility.WrapNativeAsyncMethodImplementation((cancellationToken) => this.replicator.OpenAsync(cancellationToken), callback, "ReplicaBroker.Open");
        }

        public NativeCommon.IFabricStringResult EndOpen(NativeCommon.IFabricAsyncOperationContext context)
        {
            string result = AsyncTaskCallInAdapter.End<string>(context);
            return new StringResult(result);
        }

        public NativeCommon.IFabricAsyncOperationContext BeginChangeRole(IntPtr nativeEpoch, NativeTypes.FABRIC_REPLICA_ROLE nativeRole, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            return Utility.WrapNativeAsyncMethodImplementation((cancellationToken) => this.ChangeRoleAsync(nativeEpoch, nativeRole, cancellationToken), callback, "ReplicaBroker.ChangeRole");
        }

        public void EndChangeRole(NativeCommon.IFabricAsyncOperationContext context)
        {
            AsyncTaskCallInAdapter.End(context);
        }

        public NativeCommon.IFabricAsyncOperationContext BeginClose(NativeCommon.IFabricAsyncOperationCallback callback)
        {
            return Utility.WrapNativeAsyncMethodImplementation((cancellationToken) => this.replicator.CloseAsync(cancellationToken), callback, "ReplicaBroker.Close");
        }

        public void EndClose(NativeCommon.IFabricAsyncOperationContext context)
        {
            AsyncTaskCallInAdapter.End(context);
        }

        public void Abort()
        {
            Utility.WrapNativeSyncMethodImplementation(() => this.replicator.Abort(), "ReplicaBroker.Abort");
        }

        public void GetCurrentProgress(out long lastSequenceNumber)
        {
            lastSequenceNumber = this.replicator.GetCurrentProgress();
        }

        public void GetCatchUpCapability(out long fromSequenceNumber)
        {
            fromSequenceNumber = this.replicator.GetCatchUpCapability();
        }

        public NativeCommon.IFabricAsyncOperationContext BeginUpdateEpoch(IntPtr nativeEpoch, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            return Utility.WrapNativeAsyncMethodImplementation((cancellationToken) => this.UpdateEpochAsync(nativeEpoch, cancellationToken), callback, "ReplicatorBroker.UpdateEpoch");
        }

        public void EndUpdateEpoch(NativeCommon.IFabricAsyncOperationContext context)
        {
            AsyncTaskCallInAdapter.End(context);
        }

        public NativeCommon.IFabricAsyncOperationContext BeginOnDataLoss(NativeCommon.IFabricAsyncOperationCallback callback)
        {
            return Utility.WrapNativeAsyncMethodImplementation((cancellationToken) => this.replicator.OnDataLossAsync(cancellationToken), callback, "ReplicaBroker.OnDataLoss");
        }

        public BOOLEAN EndOnDataLoss(NativeCommon.IFabricAsyncOperationContext context)
        {
            return NativeTypes.ToBOOLEAN(AsyncTaskCallInAdapter.End<bool>(context));
        }

        public void UpdateCatchUpReplicaSetConfiguration(IntPtr nativeCurrentConfiguration, IntPtr nativePreviousConfiguration)
        {
            Utility.WrapNativeSyncMethodImplementation(() => this.UpdateCatchUpReplicaSetConfigurationHelper(nativeCurrentConfiguration, nativePreviousConfiguration), "ReplicaBroker.UpdateCatchUpReplicaSetConfiguration");
        }

        public NativeCommon.IFabricAsyncOperationContext BeginWaitForCatchUpQuorum(NativeTypes.FABRIC_REPLICA_SET_QUORUM_MODE catchUpMode, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            return Utility.WrapNativeAsyncMethodImplementation((cancellationToken) => this.BeginWaitForCatchupQuorumAsync(catchUpMode, cancellationToken), callback, "ReplicaBroker.WaitForCatchUpQuorum");
        }

        public void EndWaitForCatchUpQuorum(NativeCommon.IFabricAsyncOperationContext context)
        {
            AsyncTaskCallInAdapter.End(context);
        }

        public void UpdateCurrentReplicaSetConfiguration(IntPtr nativeCurrentConfiguration)
        {
            Utility.WrapNativeSyncMethodImplementation(() => this.UpdateCurrentReplicaSetConfigurationHelper(nativeCurrentConfiguration), "ReplicaBroker.UpdateCurrentReplicaSetConfiguration");
        }

        public NativeCommon.IFabricAsyncOperationContext BeginBuildReplica(IntPtr replica, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            return Utility.WrapNativeAsyncMethodImplementation((cancellationToken) => this.BeginBuildReplicaAsync(replica, cancellationToken), callback, "ReplicatorBroker.BeginBuildReplica");
        }

        public void EndBuildReplica(NativeCommon.IFabricAsyncOperationContext context)
        {
            AsyncTaskCallInAdapter.End(context);
        }

        public void RemoveReplica(long replicaId)
        {
            Utility.WrapNativeSyncMethodImplementation(() => this.RemoveReplicaHelper(replicaId), "ReplicaBroker.RemoveReplica");
        }

        private void UpdateCatchUpReplicaSetConfigurationHelper(IntPtr nativeCurrentConfiguration, IntPtr nativePreviousConfiguration)
        {
            ReplicaSetConfiguration currentConfiguration = ReplicaSetConfiguration.FromNative(nativeCurrentConfiguration);
            ReplicaSetConfiguration previousConfiguration = ReplicaSetConfiguration.FromNative(nativePreviousConfiguration);

            this.replicator.UpdateCatchUpReplicaSetConfiguration(currentConfiguration, previousConfiguration);
        }

        private void UpdateCurrentReplicaSetConfigurationHelper(IntPtr nativeCurrentConfiguration)
        {
            ReplicaSetConfiguration currentConfiguration = ReplicaSetConfiguration.FromNative(nativeCurrentConfiguration);

            this.replicator.UpdateCurrentReplicaSetConfiguration(currentConfiguration);
        }

        private Task ChangeRoleAsync(IntPtr nativeEpoch, NativeTypes.FABRIC_REPLICA_ROLE nativeRole, CancellationToken cancellationToken)
        {
            Epoch epoch = Epoch.FromNative(nativeEpoch);

            ReplicaRole replicaRole = (ReplicaRole)nativeRole;
            return this.replicator.ChangeRoleAsync(epoch, replicaRole, cancellationToken);
        }

        private Task UpdateEpochAsync(IntPtr nativeEpoch, CancellationToken cancellationToken)
        {
            Epoch epoch = Epoch.FromNative(nativeEpoch);

            return this.replicator.UpdateEpochAsync(epoch, cancellationToken);
        }

        private Task BeginWaitForCatchupQuorumAsync(NativeTypes.FABRIC_REPLICA_SET_QUORUM_MODE catchUpMode, CancellationToken cancellationToken)
        {
            ReplicaSetQuorumMode quorumMode = (ReplicaSetQuorumMode)catchUpMode;
            return this.replicator.WaitForCatchUpQuorumAsync(quorumMode, cancellationToken);
        }

        private Task BeginBuildReplicaAsync(IntPtr replica, CancellationToken cancellationToken)
        {
            ReplicaInformation replicaInfo = ReplicaInformation.FromNative(replica);
            return this.replicator.BuildReplicaAsync(replicaInfo, cancellationToken);
        }

        private void RemoveReplicaHelper(long replicaId)
        {
            this.replicator.RemoveReplica(replicaId);            
        }
    }
}