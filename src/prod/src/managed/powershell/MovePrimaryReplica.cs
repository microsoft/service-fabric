// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric.Result;
    using System.Management.Automation;

    /// <summary>
    /// The move primary replica.
    /// </summary>
    [Cmdlet(VerbsCommon.Move, "ServiceFabricPrimaryReplica")]
    public class MovePrimaryReplica : PartitionSelectorOperationBase
    {
        public MovePrimaryReplica()
        {
            this.IgnoreConstraints = false;
        }

        /// <summary>
        /// Gets or sets the node name.
        /// </summary>
        [Parameter(Mandatory = false, ValueFromPipelineByPropertyName = true)]
        public string NodeName
        {
            get;
            set;
        }       

        [Parameter(Mandatory = false, ValueFromPipelineByPropertyName = true)]
        public bool IgnoreConstraints
        {
            get;
            set;
        }

        /// <summary>
        /// The get operation name for success trace.
        /// </summary>
        /// <returns>
        /// The <see cref="string"/>.
        /// </returns>
        internal string GetOperationNameForSuccessTrace()
        {
            return "MovePrimaryReplica";
        }

        /// <summary>
        /// The process record.
        /// </summary>
        /// <exception cref="InvalidOperationException"> invalid operation exception
        /// </exception>
        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();
            try
            {
                var partitionSelector = this.GetPartitionSelector();
                var movePrimaryResult = clusterConnection.MovePrimaryReplicaAsync(
                                            this.NodeName,
                                            partitionSelector,
                                            this.IgnoreConstraints,
                                            this.TimeoutSec,
                                            this.GetCancellationToken()).Result;

                this.WriteObject(this.FormatOutput(movePrimaryResult));
            }
            catch (AggregateException aggregateException)
            {
                this.ThrowTerminatingError(
                    aggregateException.InnerException,
                    Constants.MoveReplicaCommandErrorId,
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
            if (!(output is MovePrimaryResult))
            {
                return base.FormatOutput(output);
            }

            var item = output as MovePrimaryResult;

            var selectedPartitionPropertyObj = new PSObject(item.SelectedPartition);
            selectedPartitionPropertyObj.Members.Add(
                new PSCodeMethod(
                    Constants.ToStringMethodName,
                    typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));
            var itemPsObj = new PSObject(item);
            itemPsObj.Properties.Add(
                new PSNoteProperty(
                    Constants.SelectedPartitionPropertyName,
                    selectedPartitionPropertyObj));

            return itemPsObj;
        }
    }
}