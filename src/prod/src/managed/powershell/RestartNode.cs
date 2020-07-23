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
    using System.Numerics;

    [Cmdlet(VerbsLifecycle.Restart, "ServiceFabricNode")]
    public sealed class RestartNode : ReplicaSelectorOperationBase
    {
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 0, ParameterSetName = "ByNodeName")]
        public string NodeName
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ValueFromPipelineByPropertyName = true, Position = 1, ParameterSetName = "ByNodeName")]
        public BigInteger? NodeInstanceId
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

        [Parameter(Mandatory = false)]
        public SwitchParameter CreateFabricDump
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();
            try
            {
                if (this.ParameterSetName == "ByNodeName")
                {
                    var restartNodeResult = clusterConnection.RestartNodeAsync(
                                                this.NodeName,
                                                this.NodeInstanceId ?? 0,
                                                this.CreateFabricDump,
                                                this.TimeoutSec,
                                                this.CommandCompletionMode ?? CompletionMode.Verify,
                                                this.GetCancellationToken()).Result;
                    this.WriteObject(this.FormatOutput(restartNodeResult));
                }
                else
                {
                    var replicaSelector = this.GetReplicaSelector();
                    var restartNodeResult = clusterConnection.RestartNodeAsync(
                                                replicaSelector,
                                                this.CreateFabricDump,
                                                this.TimeoutSec,
                                                this.CommandCompletionMode ?? CompletionMode.Verify,
                                                this.GetCancellationToken()).Result;
                    this.WriteObject(this.FormatOutput(restartNodeResult));
                }
            }
            catch (AggregateException aggregateException)
            {
                this.ThrowTerminatingError(
                    aggregateException.InnerException,
                    Constants.NodeOperationCommandErrorId,
                    clusterConnection);
            }
        }

        protected override object FormatOutput(object output)
        {
            if (!(output is RestartNodeResult))
            {
                return base.FormatOutput(output);
            }

            var item = output as RestartNodeResult;

            var parametersPropertyPsObj = new PSObject(item.NodeResult);
            parametersPropertyPsObj.Members.Add(
                new PSCodeMethod(
                    Constants.ToStringMethodName,
                    typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

            var selectedReplicaPropertyObj = new PSObject(item.SelectedReplica);
            selectedReplicaPropertyObj.Members.Add(
                new PSCodeMethod(
                    Constants.ToStringMethodName,
                    typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

            var itemPsObj = new PSObject(item);

            itemPsObj.Properties.Add(
                new PSNoteProperty(
                    Constants.NodeResult,
                    parametersPropertyPsObj));

            itemPsObj.Properties.Add(
                new PSNoteProperty(
                    Constants.SelectedReplicaPropertyName,
                    selectedReplicaPropertyObj));

            return itemPsObj;
        }
    }
}