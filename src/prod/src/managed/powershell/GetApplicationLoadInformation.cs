// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric.Query;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Get, "ServiceFabricApplicationLoadInformation")]
    public sealed class GetApplicationLoadInformation : CommonCmdletBase
    {
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 0)]
        public string ApplicationName
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();
            try
            {
                var queryResult = clusterConnection.GetApplicationLoadInformationAsync(
                                      this.ApplicationName,
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
                        Constants.GetApplicationLoadInformationErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected override object FormatOutput(object output)
        {
            if (output is ApplicationLoadInformation)
            {
                var item = output as ApplicationLoadInformation;

                var metricValuesPSObj = new PSObject(item.ApplicationLoadMetricInformation);
                metricValuesPSObj.Members.Add(
                    new PSCodeMethod(
                        Constants.ToStringMethodName,
                        typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                var itemPSObj = new PSObject(item);
                itemPSObj.Properties.Add(
                    new PSNoteProperty(
                        Constants.ApplicationLoadMetricPropertyApplicationLoad,
                        metricValuesPSObj));
                return itemPSObj;
            }
            else
            {
                return base.FormatOutput(output);
            }
        }
    }
}