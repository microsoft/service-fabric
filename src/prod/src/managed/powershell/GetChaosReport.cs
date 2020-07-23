// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric.Chaos.DataStructures;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Get, "ServiceFabricChaosReport")]
    public sealed class GetChaosReport : CommonCmdletBase
    {
        [Parameter(Mandatory = false)]
        public DateTime? StartTimeUtc
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public DateTime? EndTimeUtc
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string ContinuationToken
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                ChaosReport report = null;

                if (string.IsNullOrEmpty(this.ContinuationToken))
                {
                    report = clusterConnection.GetChaosReportAsync(
                                 this.CreateChaosReportFilter(),
                                 this.GetTimeout(),
                                 this.GetCancellationToken()).Result;
                }
                else
                {
                    report = clusterConnection.GetChaosReportAsync(
                                 this.ContinuationToken,
                                 this.GetTimeout(),
                                 this.GetCancellationToken()).Result;
                }

                this.WriteObject(this.FormatOutput(report));
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.GetChaosReportCommandErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        private ChaosReportFilter CreateChaosReportFilter()
        {
            var filter = new ChaosReportFilter(
                this.StartTimeUtc.HasValue ? this.StartTimeUtc.Value : DateTime.MinValue,
                this.EndTimeUtc.HasValue ? this.EndTimeUtc.Value : DateTime.MaxValue);

            return filter;
        }
    }
}