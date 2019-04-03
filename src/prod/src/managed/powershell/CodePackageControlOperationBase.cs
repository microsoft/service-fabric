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

    public abstract class CodePackageControlOperationBase : ReplicaSelectorOperationBase
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
        public Uri ApplicationName
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 2, ParameterSetName = "ByNodeName")]
        public string ServiceManifestName
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 3, ParameterSetName = "ByNodeName")]
        public string CodePackageName
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ValueFromPipelineByPropertyName = true, Position = 4, ParameterSetName = "ByNodeName")]
        public long? CodePackageInstanceId
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ValueFromPipelineByPropertyName = true, ParameterSetName = "ByNodeName")]
        public string ServicePackageActivationId
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

        internal abstract Task<RestartDeployedCodePackageResult> InvokeCommandAsync(
            IClusterConnection clusterConnection,
            Uri uri,
            ReplicaSelector replicaSelector);

        internal abstract Task<RestartDeployedCodePackageResult> InvokeCommandAsync(
            IClusterConnection clusterConnection,
            string nodeName,
            Uri applicationName,
            string serviceManifestName,
            string servicePackageActivationId,
            string codePackageName,
            long codePackageInstanceId,
            CancellationToken cancellationToken);

        internal abstract string GetOperationNameForSuccessTrace();

        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();
            try
            {
                if (this.ParameterSetName == "ByNodeName")
                {
                    var restartDeployedCodePackage = this.InvokeCommandAsync(
                                                         clusterConnection,
                                                         this.NodeName,
                                                         this.ApplicationName,
                                                         this.ServiceManifestName,
                                                         this.ServicePackageActivationId ?? string.Empty,
                                                         this.CodePackageName,
                                                         this.CodePackageInstanceId ?? 0,
                                                         this.GetCancellationToken()).Result;

                    this.WriteObject(this.FormatOutput(restartDeployedCodePackage));
                }
                else
                {
                    var restartDeployedCodePackage = this.InvokeCommandAsync(
                                                         clusterConnection,
                                                         this.ApplicationName,
                                                         this.GetReplicaSelector()).Result;
                    this.WriteObject(this.FormatOutput(restartDeployedCodePackage));
                }
            }
            catch (AggregateException aggregateException)
            {
                this.ThrowTerminatingError(
                    aggregateException.InnerException,
                    Constants.CodePackageOperationCommandErrorId,
                    clusterConnection);
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
            if (!(output is RestartDeployedCodePackageResult))
            {
                return base.FormatOutput(output);
            }

            var item = output as RestartDeployedCodePackageResult;

            var selectedParametersPropertyPsObj = new PSObject(item.SelectedReplica);
            selectedParametersPropertyPsObj.Members.Add(
                new PSCodeMethod(
                    Constants.ToStringMethodName,
                    typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

            var itemPsObj = new PSObject(item);
            itemPsObj.Properties.Add(
                new PSNoteProperty(
                    Constants.SelectedReplicaPropertyName,
                    selectedParametersPropertyPsObj));

            return itemPsObj;
        }
    }
}