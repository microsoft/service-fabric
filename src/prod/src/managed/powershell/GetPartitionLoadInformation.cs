// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric.Query;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Get, "ServiceFabricPartitionLoadInformation")]
    public sealed class GetPartitionLoadInformation : CommonCmdletBase
    {
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 1)]
        public Guid PartitionId
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();
            try
            {
                var queryResult = clusterConnection.GetPartitionLoadInformationAsync(
                                      this.PartitionId,
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
            if (output is PartitionLoadInformation)
            {
                var item = output as PartitionLoadInformation;

                var primaryLoadsPSObj = new PSObject(item.PrimaryLoadMetricReports);
                primaryLoadsPSObj.Members.Add(
                    new PSCodeMethod(
                        Constants.ToStringMethodName,
                        typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                var itemPSObj = new PSObject(item);
                itemPSObj.Properties.Add(
                    new PSNoteProperty(
                        Constants.PrimaryLoadMetricReportsPropertyName,
                        primaryLoadsPSObj));

                var secondaryLoadPSObj = new PSObject(item.SecondaryLoadMetricReports);
                secondaryLoadPSObj.Members.Add(
                    new PSCodeMethod(
                        Constants.ToStringMethodName,
                        typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                itemPSObj.Properties.Add(
                    new PSNoteProperty(
                        Constants.SecondaryLoadMetricReportsPropertyName,
                        secondaryLoadPSObj));

                return itemPSObj;
            }
            else
            {
                return base.FormatOutput(output);
            }
        }
    }
}