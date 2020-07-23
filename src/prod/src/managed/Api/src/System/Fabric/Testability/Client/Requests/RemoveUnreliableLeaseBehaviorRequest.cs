// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Requests
{
    using System;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Fabric.Testability.Client.Structures;
    using System.Fabric.Testability.Common;

    public class RemoveUnreliableLeaseBehaviorRequest : FabricRequest
    {
        public RemoveUnreliableLeaseBehaviorRequest(IFabricClient fabricClient, string nodeName, string alias, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            ThrowIf.Null(nodeName, "nodeName");

            this.Alias = alias;
            this.NodeName = nodeName;
        }

        public string Alias
        {
            get;
            private set;
        }

        public string NodeName
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "RemoveUnreliableLeaseBehaviorRequest node {0} with name {1} and with timeout {2}", this.NodeName, this.Alias, this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.RemoveUnreliableLeaseBehaviorAsync(this.NodeName, this.Alias, this.Timeout, cancellationToken);
        }
    }
}


#pragma warning restore 1591