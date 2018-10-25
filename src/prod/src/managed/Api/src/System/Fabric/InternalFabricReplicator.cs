// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Threading;
    using System.Threading.Tasks;

    internal sealed class InternalFabricReplicator : IReplicator
    {
        public const long UnknownSequenceNumber = NativeTypes.FABRIC_INVALID_SEQUENCE_NUMBER;
        public const long InvalidAtomicGroupId = NativeTypes.FABRIC_INVALID_ATOMIC_GROUP_ID;

        private readonly NativeRuntime.IFabricReplicator nativeReplicator;

        internal InternalFabricReplicator(NativeRuntime.IFabricReplicator nativeReplicator)
        {
            this.nativeReplicator = nativeReplicator;
        }

        internal NativeRuntime.IFabricReplicator NativeReplicator
        {
            get
            {
                return this.nativeReplicator;
            }
        }

        private NativeRuntime.IFabricPrimaryReplicator PrimaryReplicator
        {
            get
            {
                return (NativeRuntime.IFabricPrimaryReplicator)this.nativeReplicator;
            }
        }

        #region IFabricReplicator

        Task<string> IReplicator.OpenAsync(CancellationToken cancellationToken)
        {
            return this.OpenAsyncHelper(cancellationToken);
        }

        Task IReplicator.ChangeRoleAsync(Epoch epoch, ReplicaRole role, CancellationToken cancellationToken)
        {
            return this.ChangeRoleAsyncHelper(epoch, role, cancellationToken);   
        }

        Task IReplicator.UpdateEpochAsync(Epoch epoch, CancellationToken cancellationToken)
        {
            return this.UpdateEpochAsyncHelper(epoch, cancellationToken);
        }

        Task IReplicator.CloseAsync(CancellationToken cancellationToken)
        {
            return this.CloseAsyncHelper(cancellationToken);
        }

        void IReplicator.Abort()
        {
            Utility.WrapNativeSyncInvoke(() => this.nativeReplicator.Abort(), "FabricReplicator.Abort");
        }

        long IReplicator.GetCurrentProgress()
        {
            return Utility.WrapNativeSyncInvoke<long>(this.GetCurrentProgressHelper, "FabricReplicator.GetCurrentProgress");            
        }

        long IReplicator.GetCatchUpCapability()
        {
            return Utility.WrapNativeSyncInvoke<long>(this.GetCatchupCapabilityHelper, "FabricReplicator.GetCatchUpCapability");            
        }

        #endregion

        #region IFabricPrimaryReplicator

        Task<bool> IPrimaryReplicator.OnDataLossAsync(CancellationToken cancellationToken)
        {
            return this.OnDataLossAsyncHelper(cancellationToken);
        }

        void IPrimaryReplicator.UpdateCatchUpReplicaSetConfiguration(ReplicaSetConfiguration currentConfiguration, ReplicaSetConfiguration previousConfiguration)
        {
            Utility.WrapNativeSyncInvoke(() => this.UpdateCatchUpReplicaSetConfigurationHelper(currentConfiguration, previousConfiguration), "FabricReplicator.UpdateCatchUpReplicaSetConfiguration");
        }

        Task IPrimaryReplicator.WaitForCatchUpQuorumAsync(ReplicaSetQuorumMode quorumMode, CancellationToken cancellationToken)
        {
            return this.WaitForCatchUpQuorumAsyncHelper(quorumMode, cancellationToken);
        }

        void IPrimaryReplicator.UpdateCurrentReplicaSetConfiguration(ReplicaSetConfiguration currentConfiguration)
        {
            Utility.WrapNativeSyncInvoke(() => this.UpdateCurrentReplicaSetConfigurationHelper(currentConfiguration), "FabricReplicator.UpdateCurrentReplicaSetConfiguration");
        }

        Task IPrimaryReplicator.BuildReplicaAsync(ReplicaInformation replicaInfo, CancellationToken cancellationToken)
        {
            return this.BuildReplicaAsyncHelper(replicaInfo, cancellationToken);
        }

        void IPrimaryReplicator.RemoveReplica(long replicaId)
        {
            Utility.WrapNativeSyncInvoke(() => this.RemoveReplicaHelper(replicaId), "FabricReplicator.RemoveReplica");
        }

        #endregion

        #region Interop Code
        #region Open Async

        private Task<string> OpenAsyncHelper(CancellationToken cancellationToken)
        {
            return Utility.WrapNativeAsyncInvoke<string>(
                (callback) => this.OpenBeginWrapper(callback),
                this.OpenEndWrapper,
                cancellationToken,
                "FabricReplicator.Open");
        }

        private NativeCommon.IFabricAsyncOperationContext OpenBeginWrapper(NativeCommon.IFabricAsyncOperationCallback callback)
        {
            return this.nativeReplicator.BeginOpen(callback);
        }

        private string OpenEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            string resultString = NativeTypes.FromNativeString(this.nativeReplicator.EndOpen(context));
            AppTrace.TraceSource.WriteNoise("FabricReplicator.OpenEndWrapper", "Result: {0}", resultString);
            
            return resultString;
        }

        #endregion

        #region ChangeRole Async

        private Task ChangeRoleAsyncHelper(Epoch epoch, ReplicaRole role, CancellationToken cancellationToken)
        {
            return Utility.WrapNativeAsyncInvoke(
                (callback) => this.ChangeRoleBeginWrapper(epoch, role, callback),
                this.ChangeRoleEndWrapper,
                cancellationToken,
                "FabricReplicator.ChangeRole");
        }

        private NativeCommon.IFabricAsyncOperationContext ChangeRoleBeginWrapper(Epoch epoch, ReplicaRole role, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            NativeTypes.FABRIC_EPOCH nativeEpoch;

            epoch.ToNative(out nativeEpoch);

            unsafe
            {
                return this.nativeReplicator.BeginChangeRole((IntPtr)(&nativeEpoch), (NativeTypes.FABRIC_REPLICA_ROLE)role, callback);
            }
        }

        private void ChangeRoleEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            this.nativeReplicator.EndChangeRole(context);
        }

        #endregion

        #region Close Async

        private Task CloseAsyncHelper(CancellationToken cancellationToken)
        {
            return Utility.WrapNativeAsyncInvoke(
                (callback) => this.CloseBeginWrapper(callback),
                this.CloseEndWrapper,
                cancellationToken,
                "FabricReplicator.Close");
        }

        private NativeCommon.IFabricAsyncOperationContext CloseBeginWrapper(NativeCommon.IFabricAsyncOperationCallback callback)
        {
            return this.nativeReplicator.BeginClose(callback);
        }

        private void CloseEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            this.nativeReplicator.EndClose(context);
        }

        #endregion

        #region OnDataLoss Async

        private Task<bool> OnDataLossAsyncHelper(CancellationToken cancellationToken)
        {
            return Utility.WrapNativeAsyncInvoke<bool>(
                (callback) => this.OnDataLossBeginWrapper(callback),
                this.OnDataLossEndWrapper,
                cancellationToken,
                "FabricReplicator.OnDataLoss");
        }

        private NativeCommon.IFabricAsyncOperationContext OnDataLossBeginWrapper(NativeCommon.IFabricAsyncOperationCallback callback)
        {
            return this.PrimaryReplicator.BeginOnDataLoss(callback);
        }

        private bool OnDataLossEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            return NativeTypes.FromBOOLEAN(this.PrimaryReplicator.EndOnDataLoss(context));
        }

        #endregion

        #region WaitForQuorumCatchup Async

        private Task WaitForCatchUpQuorumAsyncHelper(ReplicaSetQuorumMode quorumMode, CancellationToken cancellationToken)
        {
            return Utility.WrapNativeAsyncInvoke(
                (callback) => this.WaitForCatchUpQuorumBeginWrapper(quorumMode, callback),
                this.WaitForCatchUpQuorumEndWrapper,
                cancellationToken,
                "FabricReplicator.WaitForCatchupQuorum");
        }

        private NativeCommon.IFabricAsyncOperationContext WaitForCatchUpQuorumBeginWrapper(ReplicaSetQuorumMode quorumMode, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            return this.PrimaryReplicator.BeginWaitForCatchUpQuorum((NativeTypes.FABRIC_REPLICA_SET_QUORUM_MODE)quorumMode, callback);
        }

        private void WaitForCatchUpQuorumEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            this.PrimaryReplicator.EndWaitForCatchUpQuorum(context);
        }   

        #endregion

        #region BuildReplica Async

        private Task BuildReplicaAsyncHelper(ReplicaInformation replicaInfo, CancellationToken cancellationToken)
        {
            return Utility.WrapNativeAsyncInvoke(
                (callback) => this.BuildReplicaBeginWrapper(replicaInfo, callback),
                this.BuildReplicaEndWrapper,
                cancellationToken,
                "FabricReplicator.BuildReplica");
        }

        private NativeCommon.IFabricAsyncOperationContext BuildReplicaBeginWrapper(ReplicaInformation replicaInfo, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            using (var pin = new PinCollection())
            {
                NativeTypes.FABRIC_REPLICA_INFORMATION nativeReplica;
                replicaInfo.ToNative(pin, out nativeReplica);

                unsafe
                {
                    return this.PrimaryReplicator.BeginBuildReplica((IntPtr)(&nativeReplica), callback);
                }
            }
        }

        private void BuildReplicaEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            this.PrimaryReplicator.EndBuildReplica(context);
        }

        #endregion

        #region UpdateEpoch Async

        private Task UpdateEpochAsyncHelper(Epoch epoch, CancellationToken cancellationToken)
        {
            return Utility.WrapNativeAsyncInvoke(
                (callback) => this.UpdateEpochBeginWrapper(epoch, callback),
                this.UpdateEpochEndWrapper,
                cancellationToken,
                "FabricReplicator.UpdateEpoch");
        }

        private NativeCommon.IFabricAsyncOperationContext UpdateEpochBeginWrapper(Epoch epoch, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            NativeTypes.FABRIC_EPOCH nativeEpoch;

            epoch.ToNative(out nativeEpoch);

            unsafe
            {
                return this.NativeReplicator.BeginUpdateEpoch((IntPtr)(&nativeEpoch), callback);
            }
        }

        private void UpdateEpochEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            this.NativeReplicator.EndUpdateEpoch(context);
        }

        #endregion

        private void UpdateCatchUpReplicaSetConfigurationHelper(ReplicaSetConfiguration current, ReplicaSetConfiguration previous)
        {
            using (var pin = new PinCollection())
            {
                NativeTypes.FABRIC_REPLICA_SET_CONFIGURATION nativeCurrentConfiguration;
                current.ToNative(pin, out nativeCurrentConfiguration);

                NativeTypes.FABRIC_REPLICA_SET_CONFIGURATION nativePreviousConfiguration;
                if (previous != null)
                {
                    previous.ToNative(pin, out nativePreviousConfiguration);
                }

                IntPtr cc = IntPtr.Zero;
                IntPtr pc = IntPtr.Zero;

                unsafe
                {
                    cc = (IntPtr)(&nativeCurrentConfiguration);
                    if (previous != null)
                    {
                        pc = (IntPtr)(&nativePreviousConfiguration);
                    }
                }

                this.PrimaryReplicator.UpdateCatchUpReplicaSetConfiguration(cc, pc);
            }
        }

        private void UpdateCurrentReplicaSetConfigurationHelper(ReplicaSetConfiguration current)
        {
            using (var pin = new PinCollection())
            {
                NativeTypes.FABRIC_REPLICA_SET_CONFIGURATION nativeCurrentConfiguration;
                current.ToNative(pin, out nativeCurrentConfiguration);

                IntPtr cc = IntPtr.Zero;

                unsafe
                {
                    cc = (IntPtr)(&nativeCurrentConfiguration);
                }

                this.PrimaryReplicator.UpdateCurrentReplicaSetConfiguration(cc);
            }
        }

        private long GetCurrentProgressHelper()
        {
            long seqNum = 0;
            this.nativeReplicator.GetCurrentProgress(out seqNum);
            return seqNum;
        }

        private long GetCatchupCapabilityHelper()
        {
            long fromSequenceNumber = 0;
            this.nativeReplicator.GetCatchUpCapability(out fromSequenceNumber);
            return fromSequenceNumber;
        }

        private void RemoveReplicaHelper(long replicaId)
        {
            this.PrimaryReplicator.RemoveReplica(replicaId);
        }

        #endregion
    }
}