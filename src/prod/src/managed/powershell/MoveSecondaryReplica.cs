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
    /// The move secondary replica.
    /// </summary>
    [Cmdlet(VerbsCommon.Move, "ServiceFabricSecondaryReplica")]
    public class MoveSecondaryReplica : PartitionSelectorOperationBase
    {
        public MoveSecondaryReplica()
        {
            this.IgnoreConstraints = false;
        }

        /// <summary>
        /// Gets or sets the current secondary node name.
        /// </summary>
        [Parameter(Mandatory = false)]
        public string CurrentSecondaryNodeName
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the new secondary node name.
        /// </summary>
        [Parameter(Mandatory = false)]
        public string NewSecondaryNodeName
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
            return "MoveSecondaryReplica";
        }

        /// <summary>
        /// The process record.
        /// </summary>
        /// <exception cref="InvalidOperationException"> Invalid operation exception
        /// </exception>
        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();
            try
            {
                var partitionSelector = this.GetPartitionSelector();
                var moveSecondaryResult = clusterConnection.MoveSecondaryReplicaAsync(
                                              this.CurrentSecondaryNodeName,
                                              this.NewSecondaryNodeName,
                                              partitionSelector,
                                              this.IgnoreConstraints,
                                              this.TimeoutSec,
                                              this.GetCancellationToken()).Result;

                this.WriteObject(this.FormatOutput(moveSecondaryResult));
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
            if (!(output is MoveSecondaryResult))
            {
                return base.FormatOutput(output);
            }

            var item = output as MoveSecondaryResult;

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