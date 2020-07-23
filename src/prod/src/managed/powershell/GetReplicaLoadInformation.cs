// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric.Query;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Get, "ServiceFabricReplicaLoadInformation")]
    public sealed class GetReplicaLoadInformation : CommonCmdletBase
    {
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 0)]
        public Guid PartitionId
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 1)]
        public long ReplicaOrInstanceId
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();
            try
            {
                var queryResult = clusterConnection.GetReplicaLoadInformationAsync(
                                      this.PartitionId,
                                      this.ReplicaOrInstanceId,
                                      this.GetTimeout(),
                                      this.GetCancellationToken()).Result;

                this.WriteObject(this.FormatOutput(queryResult));
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.GetPartitionErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected override object FormatOutput(object output)
        {
            if (output is ReplicaLoadInformation)
            {
                var item = output as ReplicaLoadInformation;

                var primaryLoadsPSObj = new PSObject(item.LoadMetricReports);
                primaryLoadsPSObj.Members.Add(
                    new PSCodeMethod(
                        Constants.ToStringMethodName,
                        typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                var itemPSObj = new PSObject(item);
                itemPSObj.Properties.Add(
                    new PSNoteProperty(
                        Constants.LoadMetricReportsPropertyName,
                        primaryLoadsPSObj));

                return itemPSObj;
            }
            else
            {
                return base.FormatOutput(output);
            }
        }
    }
}