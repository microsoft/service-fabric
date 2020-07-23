// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric;
    using System.Fabric.Result;
    using System.Management.Automation;
    using System.Threading;
    using System.Threading.Tasks;

    [Cmdlet(VerbsLifecycle.Restart, "ServiceFabricReplica")]
    public sealed class RestartReplica : ReplicaControlOperationBase
    {
        /// <summary>
        /// The invoke command async.
        /// </summary>
        /// <param name="clusterConnection">
        /// The cluster connection.
        /// </param>
        /// <param name="nodeName">
        /// The node name.
        /// </param>
        /// <param name="partitionId">
        /// The partition id.
        /// </param>
        /// <param name="replicaId">
        /// The replica id.
        /// </param>
        /// <param name="completionMode">
        /// The completion mode.
        /// </param>
        /// <param name="timeout">
        /// The timeout.
        /// </param>
        /// <param name="cancellationToken">
        /// The cancellation token.
        /// </param>
        /// <returns>
        /// The <see cref="Task"/>.
        /// </returns>
        internal override Task InvokeCommandAsync(
            IClusterConnection clusterConnection,
            string nodeName,
            Guid partitionId,
            long replicaId,
            CompletionMode completionMode,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return clusterConnection.RestartReplicaAsync(nodeName, partitionId, replicaId, completionMode, timeout, cancellationToken);
        }

        /// <summary>
        /// The invoke command async.
        /// </summary>
        /// <param name="clusterConnection">
        /// The cluster connection.
        /// </param>
        /// <param name="replicaSelector">
        /// The replica selector.
        /// </param>
        /// <param name="completionMode">
        /// The completion mode.
        /// </param>
        /// <param name="cancellationToken">
        /// The cancellation token.
        /// </param>
        /// <returns>
        /// The <see cref="Task"/>.
        /// </returns>
        internal override async Task<ReplicaResult> InvokeCommandAsync(
            IClusterConnection clusterConnection,
            ReplicaSelector replicaSelector,
            CompletionMode completionMode,
            CancellationToken cancellationToken)
        {
            return
                await
                Task.FromResult(
                    clusterConnection.RestartReplicaAsync(
                        replicaSelector,
                        this.TimeoutSec,
                        completionMode,
                        cancellationToken).Result);
        }

        /// <summary>
        /// The get operation name for success trace.
        /// </summary>
        /// <returns>
        /// The <see cref="string"/>.
        /// </returns>
        internal override string GetOperationNameForSuccessTrace()
        {
            return "RestartReplica";
        }
    }
}