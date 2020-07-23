// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

namespace System.Fabric.Testability.Client.Requests
{
    using System;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;

    public class GetChaosEventsRequest : FabricRequest
    {
        public GetChaosEventsRequest(
            IFabricClient fabricClient,
            DateTime startTime,
            DateTime endTime,
            string continuationToken,
            long maxResults,
            TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            this.StartTime = startTime;
            this.EndTime = endTime;
            this.ContinuationToken = continuationToken;
            this.MaxResults = maxResults;

            this.RetryErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_RECONFIGURATION_PENDING);
            this.RetryErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_KEY_NOT_FOUND);
        }

        public DateTime StartTime { get; private set; }

        public DateTime EndTime { get; private set; }

        public string ContinuationToken { get; private set; }

        public long MaxResults { get; private set; }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "GetChaosEventsRequest with timeout={0}, starttime={1}, endtime={2}, continuationtoken={3}, maxresults={4}",
                this.Timeout,
                this.StartTime,
                this.EndTime,
                this.ContinuationToken,
                this.MaxResults);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.GetChaosEventsAsync(
                this.StartTime,
                this.EndTime,
                this.ContinuationToken,
                this.MaxResults,
                this.Timeout,
                cancellationToken).ConfigureAwait(false);
        }
    }
}
#pragma warning restore 1591