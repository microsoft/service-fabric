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

    public class RestartNodeRequest : NodeControlRequest
    {
        private readonly bool createFabricDump;

        public RestartNodeRequest(IFabricClient fabricClient, string nodeName, BigInteger nodeInstanceId, bool createFabricDump, CompletionMode completionMode, TimeSpan timeout)
            : base(fabricClient, nodeName, nodeInstanceId, completionMode, timeout)
        {
            this.createFabricDump = createFabricDump;
        }

        public RestartNodeRequest(IFabricClient fabricClient, string nodeName, BigInteger nodeInstanceId, bool createFabricDump, TimeSpan timeout)
            : base(fabricClient, nodeName, nodeInstanceId, CompletionMode.Invalid, timeout)
        {
            this.createFabricDump = createFabricDump;
        }

        public RestartNodeRequest(IFabricClient fabricClient, ReplicaSelector replicaSelector, bool createFabricDump, CompletionMode completionMode, TimeSpan timeout)
            : base(fabricClient, replicaSelector, completionMode, timeout)
        {
            this.createFabricDump = createFabricDump;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "Restart node {0}:{1} with timeout {2} and create crash dump set to {3}", this.NodeName, this.NodeInstanceId, this.Timeout, this.createFabricDump);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            if (this.ReplicaSelector != null)
            {
                this.OperationResult = await this.FabricClient.RestartNodeAsync(this.ReplicaSelector, this.createFabricDump, this.CompletionMode, this.Timeout, cancellationToken).ConfigureAwait(false);
            }
            else if (this.CompletionMode == CompletionMode.Invalid)
            {
                // this exists for REST version only, because REST does not support ReplicaSelector or CompletionMode
                this.OperationResult = await this.FabricClient.RestartNodeNativeAsync(this.NodeName, this.NodeInstanceId, this.createFabricDump, this.Timeout, cancellationToken).ConfigureAwait(false);
            }
            else
            {
                this.OperationResult = await this.FabricClient.RestartNodeAsync(this.NodeName, this.NodeInstanceId, this.createFabricDump, this.CompletionMode, this.Timeout, cancellationToken).ConfigureAwait(false);
            }
        }
    }
}


#pragma warning restore 1591