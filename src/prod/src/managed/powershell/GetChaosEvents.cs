// ----------------------------------------------------------------------
//  <copyright file="GetChaosEvents.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric.Chaos.DataStructures;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Get, "ServiceFabricChaosEvents")]
    public sealed class GetChaosEvents : CommonCmdletBase
    {
        [Parameter(Mandatory = false, ParameterSetName = "UseFilter")]
        public DateTime? StartTimeUtc
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "UseFilter")]
        public DateTime? EndTimeUtc
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "UseContinuationToken")]
        public string ContinuationToken
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        [ValidateRange(0, long.MaxValue)]
        public long? MaxResults
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                ChaosEventsSegment report = null;

                if (string.IsNullOrEmpty(this.ContinuationToken))
                {
                    report = clusterConnection.GetChaosEventsAsync(
                                 this.CreateChaosEventsSegmentFilter(),
                                 this.GetMaxResults(),
                                 this.GetTimeout(),
                                 this.GetCancellationToken()).Result;
                }
                else
                {
                    report = clusterConnection.GetChaosEventsAsync(
                                 this.ContinuationToken,
                                 this.GetMaxResults(),
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
                        Constants.GetChaosEventsCommandErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        private ChaosEventsSegmentFilter CreateChaosEventsSegmentFilter()
        {
            var filter = new ChaosEventsSegmentFilter(
                this.StartTimeUtc.HasValue ? this.StartTimeUtc.Value : DateTime.MinValue,
                this.EndTimeUtc.HasValue ? this.EndTimeUtc.Value : DateTime.MaxValue);

            return filter;
        }

        private long GetMaxResults()
        {
            return this.MaxResults.HasValue ? this.MaxResults.Value : 0;
        }
    }
}