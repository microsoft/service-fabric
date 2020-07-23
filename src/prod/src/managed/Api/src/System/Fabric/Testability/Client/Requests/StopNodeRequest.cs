// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Requests.Internal
{
    using System;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Numerics;

    public class StopNodeRequest : NodeControlRequest
    {
        public StopNodeRequest(IFabricClient fabricClient, string nodeName, BigInteger nodeInstanceId, TimeSpan timeout)
            : base(fabricClient, nodeName, nodeInstanceId, timeout)
        {
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "Stop node {0}:{1} with timeout {2}", this.NodeName, this.NodeInstanceId, this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.StopNodeNativeAsync(this.NodeName, this.NodeInstanceId, this.Timeout, cancellationToken).ConfigureAwait(false);
        }
    }
}


#pragma warning restore 1591