// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.Replicator
{
    using System;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Threading;
    using System.Threading.Tasks;

    public sealed class FabricReplicatorEx : IReplicator
    {
        public const long InvalidSequenceNumber = NativeTypes.FABRIC_INVALID_SEQUENCE_NUMBER;
        public const long InvalidAtomicGroupId = NativeTypes.FABRIC_INVALID_ATOMIC_GROUP_ID;

        private readonly NativeRuntime.IFabricAtomicGroupStateReplicatorEx nativeStateReplicator;
        private readonly AtomicGroupStateReplicatorEx stateReplicator;

        private readonly IReplicator internalReplicator;

        internal FabricReplicatorEx(NativeRuntime.IFabricReplicator nativeReplicator, NativeRuntime.IFabricAtomicGroupStateReplicatorEx nativeStateReplicator, NativeRuntime.IOperationDataFactory operationDataFactory)
        {
            this.internalReplicator = new InternalFabricReplicator(nativeReplicator);

            this.nativeStateReplicator = nativeStateReplicator;

            this.stateReplicator = FabricReplicatorEx.CreateStateReplicator(this.nativeStateReplicator, operationDataFactory);
        }

        public IAtomicGroupStateReplicatorEx StateReplicator
        {
            get
            {
                return this.stateReplicator;
            }
        }

        internal NativeRuntime.IFabricReplicator NativeReplicator
        {
            get
            {
                return ((InternalFabricReplicator)this.internalReplicator).NativeReplicator;
            }
        }

        Task<string> IReplicator.OpenAsync(CancellationToken cancellationToken)
        {
            return this.internalReplicator.OpenAsync(cancellationToken);
        }

        Task IReplicator.ChangeRoleAsync(Epoch epoch, ReplicaRole role, CancellationToken cancellationToken)
        {
            return this.internalReplicator.ChangeRoleAsync(epoch, role, cancellationToken);
        }

        Task IReplicator.CloseAsync(CancellationToken cancellationToken)
        {
            return this.internalReplicator.CloseAsync(cancellationToken);
        }

        void IReplicator.Abort()
        {
            this.internalReplicator.Abort();
        }

        long IReplicator.GetCurrentProgress()
        {
            return this.internalReplicator.GetCurrentProgress();
        }

        long IReplicator.GetCatchUpCapability()
        {
            return this.internalReplicator.GetCatchUpCapability();
        }

        Task IReplicator.UpdateEpochAsync(Epoch epoch, CancellationToken cancellationToken)
        {
            return this.internalReplicator.UpdateEpochAsync(epoch, cancellationToken);
        }

        Task<bool> IPrimaryReplicator.OnDataLossAsync(CancellationToken cancellationToken)
        {
            return this.internalReplicator.OnDataLossAsync(cancellationToken);
        }

        void IPrimaryReplicator.UpdateCatchUpReplicaSetConfiguration(ReplicaSetConfiguration currentConfiguration, ReplicaSetConfiguration previousConfiguration)
        {
            this.internalReplicator.UpdateCatchUpReplicaSetConfiguration(currentConfiguration, previousConfiguration);
        }

        Task IPrimaryReplicator.WaitForCatchUpQuorumAsync(ReplicaSetQuorumMode quorumMode, CancellationToken cancellationToken)
        {
            return this.internalReplicator.WaitForCatchUpQuorumAsync(quorumMode, cancellationToken);
        }

        void IPrimaryReplicator.UpdateCurrentReplicaSetConfiguration(ReplicaSetConfiguration currentConfiguration)
        {
            this.internalReplicator.UpdateCurrentReplicaSetConfiguration(currentConfiguration);
        }

        Task IPrimaryReplicator.BuildReplicaAsync(ReplicaInformation replicaInfo, CancellationToken cancellationToken)
        {
            return this.internalReplicator.BuildReplicaAsync(replicaInfo, cancellationToken);
        }

        void IPrimaryReplicator.RemoveReplica(long replicaId)
        {
            this.internalReplicator.RemoveReplica(replicaId);
        }

        private static AtomicGroupStateReplicatorEx CreateStateReplicator(NativeRuntime.IFabricAtomicGroupStateReplicatorEx nativeStateReplicator, NativeRuntime.IOperationDataFactory nativeOperationDataFactory)
        {
            Requires.Argument("nativeOperationDataFactory", nativeOperationDataFactory).NotNull();

            return new AtomicGroupStateReplicatorEx(nativeStateReplicator, nativeOperationDataFactory);
        }
    }
}