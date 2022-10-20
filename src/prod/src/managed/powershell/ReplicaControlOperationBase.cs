// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric;
    using System.Fabric.Result;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.Management.Automation;
    using System.Threading;
    using System.Threading.Tasks;

    public abstract class ReplicaControlOperationBase : CommonCmdletBase
    {
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 0, ParameterSetName = "ByNodeName")]
        public string NodeName
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 1, ParameterSetName = "ByNodeName")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "PartitionId")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "PartitionIdReplicaPrimary")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "PartitionIdReplicaRandomSecondary")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "PartitionIdReplicaId")]
        public Guid PartitionId
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "PartitionId")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "PartitionIdReplicaPrimary")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "PartitionIdReplicaRandomSecondary")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "PartitionIdReplicaId")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceName")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionSingleton")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionNamed")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionUniformedInt")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionSingletonReplicaRandomSecondary")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionNamedReplicaRandomSecondary")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionUniformedIntReplicaRandomSecondary")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionSingletonReplicaPrimary")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionNamedReplicaPrimary")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionUniformedIntReplicaPrimary")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionSingletonReplicaId")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionNamedReplicaId")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionUniformedIntReplicaId")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNameReplicaPrimary")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNameReplicaRandomSecondary")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNameReplicaId")]
        public Uri ServiceName
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionSingleton")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionSingletonReplicaRandomSecondary")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionSingletonReplicaPrimary")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionSingletonReplicaId")]
        public SwitchParameter PartitionKindSingleton
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionUniformedInt")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionUniformedIntReplicaRandomSecondary")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionUniformedIntReplicaPrimary")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionUniformedIntReplicaId")]
        public SwitchParameter PartitionKindUniformInt64
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionNamed")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionNamedReplicaRandomSecondary")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionNamedReplicaPrimary")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionNamedReplicaId")]
        public SwitchParameter PartitionKindNamed
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionNamed")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionNamedReplicaRandomSecondary")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionNamedReplicaPrimary")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionNamedReplicaId")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionUniformedInt")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionUniformedIntReplicaRandomSecondary")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionUniformedIntReplicaPrimary")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionUniformedIntReplicaId")]
        public string PartitionKey
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionSingletonReplicaPrimary")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionNamedReplicaPrimary")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionUniformedIntReplicaPrimary")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "PartitionIdReplicaPrimary")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNameReplicaPrimary")]
        public SwitchParameter ReplicaKindPrimary
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "PartitionIdReplicaRandomSecondary")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionSingletonReplicaRandomSecondary")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionNamedReplicaRandomSecondary")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionUniformedIntReplicaRandomSecondary")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNameReplicaRandomSecondary")]
        public SwitchParameter ReplicaKindRandomSecondary
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 2, ParameterSetName = "ByNodeName")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "PartitionIdReplicaId")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionSingletonReplicaId")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionNamedReplicaId")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionUniformedIntReplicaId")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNameReplicaId")]
        public long? ReplicaOrInstanceId
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public CompletionMode? CommandCompletionMode
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
        internal abstract Task InvokeCommandAsync(
            IClusterConnection clusterConnection,
            string nodeName,
            Guid partitionId,
            long replicaId,
            CompletionMode completionMode,
            TimeSpan timeout,
            CancellationToken cancellationToken);

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
        internal abstract Task<ReplicaResult> InvokeCommandAsync(
            IClusterConnection clusterConnection,
            ReplicaSelector replicaSelector,
            CompletionMode completionMode,
            CancellationToken cancellationToken);

        /// <summary>
        /// The get operation name for success trace.
        /// </summary>
        /// <returns>
        /// The <see cref="string"/>.
        /// </returns>
        internal abstract string GetOperationNameForSuccessTrace();

        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                if (this.ParameterSetName == "ByNodeName")
                {
                    this.InvokeCommandAsync(
                        clusterConnection,
                        this.NodeName,
                        this.PartitionId,
                        this.ReplicaOrInstanceId.GetValueOrDefault(),
                        this.CommandCompletionMode ?? CompletionMode.DoNotVerify,
                        this.GetTimeout(),
                        this.GetCancellationToken()).Wait();
                    this.WriteObject(string.Format(
                                         CultureInfo.CurrentCulture,
                                         StringResources.Info_ReplicaControlOperationSucceeded,
                                         this.GetOperationNameForSuccessTrace(),
                                         "scheduled",
                                         this.NodeName,
                                         this.ReplicaOrInstanceId,
                                         this.PartitionId));
                }
                else
                {
                    var replicaSelector = ReplicaSelectorOperationBase.GetReplicaSelector(
                                              this.ParameterSetName,
                                              this.PartitionId,
                                              this.ServiceName,
                                              this.PartitionKey,
                                              this.ReplicaOrInstanceId);
                    var returnResult = this.InvokeCommandAsync(
                                           clusterConnection,
                                           replicaSelector,
                                           this.CommandCompletionMode ?? CompletionMode.Verify,
                                           this.GetCancellationToken()).Result;
                    this.WriteObject(this.FormatOutput(returnResult));
                }
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.ReportFaultCommandErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        /// <summary>
        /// The format output.
        /// </summary>
        /// <param name="output">
        /// The output.
        /// </param>
        /// <returns>
        /// The <see cref="object"/>.
        /// </returns>
        protected override object FormatOutput(object output)
        {
            return base.FormatOutput(output);
        }
    }
}