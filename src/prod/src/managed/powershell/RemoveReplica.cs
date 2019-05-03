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

    /// <summary>
    /// The remove replica.
    /// </summary>
    [Cmdlet(VerbsCommon.Remove, "ServiceFabricReplica")]
    public sealed class RemoveReplica : ReplicaControlOperationBase
    {
        /// <summary>
        /// Gets or sets the force remove.
        /// </summary>
        [Parameter(Mandatory = false)]
        public SwitchParameter ForceRemove
        {
            get;
            set;
        }

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
            return clusterConnection.RemoveReplicaAsync(
                       nodeName,
                       partitionId,
                       replicaId,
                       completionMode,
                       this.ForceRemove,
                       timeout,
                       cancellationToken);
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
                    clusterConnection.RemoveReplicaAsync(
                        replicaSelector,
                        this.TimeoutSec,
                        completionMode,
                        this.ForceRemove,
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
            return "RemoveReplica";
        }
    }
}