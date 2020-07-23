// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Requests
{
    using System;
    using System.Fabric.Interop;
    using System.Fabric.Testability.Common;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;

    public class UnregisterServiceNotificationFilterRequest : FabricRequest
    {
        public UnregisterServiceNotificationFilterRequest(IFabricClient fabricClient, long filterId, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            this.FilterId = filterId;
        }

        public long FilterId
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "Unregister service notification filter {0} with timeout {1}", this.FilterId, this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.UnregisterServiceNotificationFilterAsync(this.FilterId, this.Timeout, cancellationToken);
        }
    }
}