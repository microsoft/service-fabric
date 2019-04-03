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
    using System.Numerics;
    using System.Threading;
    using System.Threading.Tasks;

    public class StartNodeRequest : FabricRequest
    {
        public StartNodeRequest(
            IFabricClient fabricClient, 
            string nodeName,
            string ipAddressOrFQDN,
            int clusterConnectionPort,
            TimeSpan timeout)
            : this(fabricClient, nodeName, 0, ipAddressOrFQDN, clusterConnectionPort, timeout)
        {
        }

        public StartNodeRequest(
             IFabricClient fabricClient,
             string nodeName,
             BigInteger NodeInstanceId,
             string ipAddressOrFQDN,
             int clusterConnectionPort,
             TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            this.NodeName = nodeName;
            this.NodeInstanceId = NodeInstanceId;
            this.IPAddressOrFQDN = ipAddressOrFQDN;
            this.ClusterConnectionPort = clusterConnectionPort;
        }

        public string NodeName { get; private set; }

        public BigInteger NodeInstanceId { get; private set; }

        public string IPAddressOrFQDN { get; private set; }

        public int ClusterConnectionPort { get; private set; }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "Start node {0}:{1}:{2}:{3} with timeout {4}", this.NodeName, this.NodeInstanceId, this.IPAddressOrFQDN, this.ClusterConnectionPort, this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.StartNodeNativeAsync(this.NodeName, this.NodeInstanceId, this.IPAddressOrFQDN, this.ClusterConnectionPort, this.Timeout, cancellationToken);
        }
    }
}


#pragma warning restore 1591