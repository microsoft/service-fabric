// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
    public interface IPrimaryReplicator
    {
        /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
        /// <param name="cancellationToken">
        /// <para>The <see cref="System.Threading.CancellationToken" /> object that the operation is observing. It can be used to send a notification that the operation should be canceled. Note that cancellation is advisory and that the operation might still be completed even if it is canceled.</para>
        /// </param>
        /// <returns>
        /// <para>A task that represents the asynchronous operation.</para>
        /// </returns>
        Task<bool> OnDataLossAsync(CancellationToken cancellationToken);

        /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
        /// <param name="currentConfiguration">
        /// <para>For Internal Use Only.</para>
        /// </param>
        /// <param name="previousConfiguration">
        /// <para>For Internal Use Only.</para>
        /// </param>
        void UpdateCatchUpReplicaSetConfiguration(ReplicaSetConfiguration currentConfiguration, ReplicaSetConfiguration previousConfiguration);

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
        Task WaitForCatchUpQuorumAsync(ReplicaSetQuorumMode quorumMode, CancellationToken cancellationToken);

        /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
        /// <param name="currentConfiguration">
        /// <para>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</para>
        /// </param>
        /// <remarks>
        /// <para>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</para>
        /// </remarks>
        void UpdateCurrentReplicaSetConfiguration(ReplicaSetConfiguration currentConfiguration);

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
        Task BuildReplicaAsync(ReplicaInformation replicaInfo, CancellationToken cancellationToken);

        /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
        /// <param name="replicaId">
        /// <para>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</para>
        /// </param>
        /// <remarks>
        /// <para>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</para>
        /// </remarks>
        void RemoveReplica(long replicaId);
    }

    /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
    public interface IReplicator : IPrimaryReplicator
    {
        /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
        /// <param name="cancellationToken">
        /// <para>The <see cref="System.Threading.CancellationToken" /> object that the operation is observing. It can be used to send a notification that the operation should be canceled. Note that cancellation is advisory and that the operation might still be completed even if it is canceled.</para>
        /// </param>
        /// <returns>
        /// <para>A task that represents the asynchronous operation.</para>
        /// </returns>
        Task<string> OpenAsync(CancellationToken cancellationToken);

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
        Task ChangeRoleAsync(Epoch epoch, ReplicaRole role, CancellationToken cancellationToken);

        /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
        /// <param name="cancellationToken">
        /// <para>The <see cref="System.Threading.CancellationToken" /> object that the operation is observing. It can be used to send a notification that the operation should be canceled. Note that cancellation is advisory and that the operation might still be completed even if it is canceled.</para>
        /// </param>
        /// <returns>
        /// <para>A task that represents the asynchronous operation.</para>
        /// </returns>
        Task CloseAsync(CancellationToken cancellationToken);

        /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
        /// <remarks>
        /// </remarks>
        void Abort();

        /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
        /// <returns>
        /// <para>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</para>
        /// </returns>
        long GetCurrentProgress();

        /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
        /// <returns>
        /// <para>For Internal Use Only.</para>
        /// </returns>
        long GetCatchUpCapability();

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
        Task UpdateEpochAsync(Epoch epoch, CancellationToken cancellationToken);
    }
}