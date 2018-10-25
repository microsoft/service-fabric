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

    /// <summary>
    /// <para>Replicates state for high availability and reliability. </para>
    /// </summary>
    /// <remarks>
    /// <para>Provides the default implementation of the <see cref="System.Fabric.IStateReplicator" />, <see cref="System.Fabric.IReplicator" />, and <see cref="System.Fabric.IPrimaryReplicator" /> interfaces, which user services can use, along with their implementation of the <see cref="System.Fabric.IStateProvider" /> interface.</para>
    /// <para>An instance of the <see cref="System.Fabric.FabricReplicator" /> class is obtained via the <see cref="System.Fabric.IStatefulServicePartition.CreateReplicator(System.Fabric.IStateProvider,System.Fabric.ReplicatorSettings)" /> method.</para>
    /// </remarks>
    public sealed class FabricReplicator : IReplicator, IReplicatorCatchupSpecificQuorum
    {
        /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
        public const long UnknownSequenceNumber = NativeTypes.FABRIC_INVALID_SEQUENCE_NUMBER;

        /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
        public const long InvalidAtomicGroupId = NativeTypes.FABRIC_INVALID_ATOMIC_GROUP_ID;

        private readonly IReplicator internalReplicator;
        
        private readonly StateReplicator stateReplicator;
        
        internal FabricReplicator(
            NativeRuntime.IFabricReplicator nativeReplicator, 
            NativeRuntime.IFabricStateReplicator nativeStateReplicator, 
            NativeRuntime.IOperationDataFactory operationDataFactory)
            : this(nativeReplicator)
        {
            this.stateReplicator = FabricReplicator.CreateStateReplicator(nativeStateReplicator, operationDataFactory);
        }

        internal FabricReplicator(
            NativeRuntime.IFabricReplicator nativeReplicator)
        {
            this.internalReplicator = new InternalFabricReplicator(nativeReplicator);
        }

        /// <summary>
        /// <para>Gets the <see cref="System.Fabric.IStateReplicator" /> which can be used to replicate state</para>
        /// </summary>
        /// <value>A value representing the <see cref="System.Fabric.IStateReplicator" /> which can be used to replicate state</value>
        public IStateReplicator StateReplicator
        {
            get
            {
                return this.stateReplicator;
            }
        }

        /// <summary>
        /// <para>Gets the <see cref="System.Fabric.IStateReplicator2" /> which can be used to replicate state</para>
        /// </summary>
        /// <value>A value representing the <see cref="System.Fabric.IStateReplicator2" /> which can be used to replicate state</value>
        public IStateReplicator2 StateReplicator2
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

        /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
        /// <param name="cancellationToken">
        /// <para>The <see cref="System.Threading.CancellationToken" /> object that the operation is observing. It can be used to send a notification that the operation should be canceled. Note that cancellation is advisory and that the operation might still be completed even if it is canceled.</para>
        /// </param>
        /// <returns>
        /// <para>A task that represents the asynchronous operation.</para>
        /// </returns>
        Task<string> IReplicator.OpenAsync(CancellationToken cancellationToken)
        {
            return this.internalReplicator.OpenAsync(cancellationToken);
        }

        /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
        /// <param name="epoch">
        /// <para>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</para>
        /// </param>
        /// <param name="role">
        /// <para>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</para>
        /// </param>
        /// <param name="cancellationToken">
        /// <para>The <see cref="System.Threading.CancellationToken" /> object that the operation is observing. It can be used to send a notification that the operation should be canceled. Note that cancellation is advisory and that the operation might still be completed even if it is canceled.</para>
        /// </param>
        /// <returns>
        /// <para>A task that represents the asynchronous operation.</para>
        /// </returns>
        Task IReplicator.ChangeRoleAsync(Epoch epoch, ReplicaRole role, CancellationToken cancellationToken)
        {
            return this.internalReplicator.ChangeRoleAsync(epoch, role, cancellationToken);
        }

        /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
        /// <param name="cancellationToken">
        /// <para>The <see cref="System.Threading.CancellationToken" /> object that the operation is observing. It can be used to send a notification that the operation should be canceled. Note that cancellation is advisory and that the operation might still be completed even if it is canceled.</para>
        /// </param>
        /// <returns>
        /// <para>A task that represents the asynchronous operation.</para>
        /// </returns>
        Task IReplicator.CloseAsync(CancellationToken cancellationToken)
        {
            return this.internalReplicator.CloseAsync(cancellationToken);
        }

        /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
        /// <remarks>
        /// </remarks>
        void IReplicator.Abort()
        {
            this.internalReplicator.Abort();
        }

        /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
        /// <returns>
        /// <para>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</para>
        /// </returns>
        long IReplicator.GetCurrentProgress()
        {
            return this.internalReplicator.GetCurrentProgress();
        }

        /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
        /// <returns>
        /// <para>For Internal Use Only.</para>
        /// </returns>
        long IReplicator.GetCatchUpCapability()
        {
            return this.internalReplicator.GetCatchUpCapability();
        }

        /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
        /// <param name="epoch">
        /// <para>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</para>
        /// </param>
        /// <param name="cancellationToken">
        /// <para>The <see cref="System.Threading.CancellationToken" /> object that the operation is observing. It can be used to send a notification that the operation should be canceled. Note that cancellation is advisory and that the operation might still be completed even if it is canceled.</para>
        /// </param>
        /// <returns>
        /// <para>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</para>
        /// </returns>
        Task IReplicator.UpdateEpochAsync(Epoch epoch, CancellationToken cancellationToken)
        {
            return this.internalReplicator.UpdateEpochAsync(epoch, cancellationToken);
        }
 
        /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
        /// <param name="cancellationToken">
        /// <para>The <see cref="System.Threading.CancellationToken" /> object that the operation is observing. It can be used to send a notification that the operation should be canceled. Note that cancellation is advisory and that the operation might still be completed even if it is canceled.</para>
        /// </param>
        /// <returns>
        /// <para>A task that represents the asynchronous operation.</para>
        /// </returns>
        Task<bool> IPrimaryReplicator.OnDataLossAsync(CancellationToken cancellationToken)
        {
            return this.internalReplicator.OnDataLossAsync(cancellationToken);
        }
 
        /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
        /// <param name="currentConfiguration">
        /// <para>For Internal Use Only.</para>
        /// </param>
        /// <param name="previousConfiguration">
        /// <para>For Internal Use Only.</para>
        /// </param>
        void IPrimaryReplicator.UpdateCatchUpReplicaSetConfiguration(ReplicaSetConfiguration currentConfiguration, ReplicaSetConfiguration previousConfiguration)
        {
            this.internalReplicator.UpdateCatchUpReplicaSetConfiguration(currentConfiguration, previousConfiguration);
        }

        /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
        /// <param name="quorumMode">
        /// <para>For Internal Use Only.</para>
        /// </param>
        /// <param name="cancellationToken">
        /// <para>The CancellationToken object that the operation is observing. It can be used to send a notification that the operation should be canceled.
        /// Note that cancellation is advisory and that the operation might still be completed even if it is canceled.</para>
        /// </param>
        /// <returns>
        /// <para>For Internal Use Only.</para>
        /// </returns>
        Task IPrimaryReplicator.WaitForCatchUpQuorumAsync(ReplicaSetQuorumMode quorumMode, CancellationToken cancellationToken)
        {
            return this.internalReplicator.WaitForCatchUpQuorumAsync(quorumMode, cancellationToken);
        }
 
        /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
        /// <param name="currentConfiguration">
        /// <para>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</para>
        /// </param>
        /// <remarks>
        /// <para>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</para>
        /// </remarks>
 
        void IPrimaryReplicator.UpdateCurrentReplicaSetConfiguration(ReplicaSetConfiguration currentConfiguration)
        {
            this.internalReplicator.UpdateCurrentReplicaSetConfiguration(currentConfiguration);
        }

        /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
        /// <param name="replicaInfo">
        /// <para>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</para>
        /// </param>
        /// <param name="cancellationToken">
        /// <para>The <see cref="System.Threading.CancellationToken" />  object that the operation is observing. It can be used to send a notification that the operation should be canceled. Note that cancellation is advisory and that the operation might still be completed even if it is canceled.</para>
        /// </param>
        /// <returns>
        /// <para>A task that represents the asynchronous operation.</para>
        /// </returns>
        Task IPrimaryReplicator.BuildReplicaAsync(ReplicaInformation replicaInfo, CancellationToken cancellationToken)
        {
            return this.internalReplicator.BuildReplicaAsync(replicaInfo, cancellationToken);
        }

        /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
        /// <param name="replicaId">
        /// <para>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</para>
        /// </param>
        /// <remarks>
        /// <para>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</para>
        /// </remarks>
        void IPrimaryReplicator.RemoveReplica(long replicaId)
        {
            this.internalReplicator.RemoveReplica(replicaId);
        }

        private static StateReplicator CreateStateReplicator(
            NativeRuntime.IFabricStateReplicator nativeStateReplicator, 
            NativeRuntime.IOperationDataFactory nativeOperationDataFactory)
        {
            Requires.Argument("nativeOperationDataFactory", nativeOperationDataFactory).NotNull();
            NativeRuntime.IFabricAtomicGroupStateReplicator nativeAtomicGroupStateReplicator = nativeStateReplicator as NativeRuntime.IFabricAtomicGroupStateReplicator;

            if (nativeAtomicGroupStateReplicator != null)
            {
                return new AtomicGroupStateReplicator(nativeAtomicGroupStateReplicator, nativeStateReplicator, nativeOperationDataFactory);
            }
            else
            {
                return new StateReplicator(nativeStateReplicator, nativeOperationDataFactory);
            }
        }
    }
}