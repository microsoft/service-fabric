// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Requests
{
    using System;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;

    public class GetChaosReportRequest : FabricRequest
    {
        public GetChaosReportRequest(
            IFabricClient fabricClient,
            DateTime startTime,
            DateTime endTime,
            string continuationToken,
            TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            this.StartTime = startTime;
            this.EndTime = endTime;
            this.ContinuationToken = continuationToken;

            this.RetryErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_RECONFIGURATION_PENDING);
            this.RetryErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_KEY_NOT_FOUND);
        }

        public DateTime StartTime { get; private set; }

        public DateTime EndTime { get; private set; }

        public string ContinuationToken { get; private set; }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "GetChaosReportRequest with timeout={0}, starttime={1}, endtime={2}, continuationtoke={3}",
                this.Timeout,
                this.StartTime,
                this.EndTime,
                this.ContinuationToken);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.GetChaosReportAsync(
                this.StartTime,
                this.EndTime,
                this.ContinuationToken,
                this.Timeout,
                cancellationToken).ConfigureAwait(false);
        }
    }
}

#pragma warning restore 1591