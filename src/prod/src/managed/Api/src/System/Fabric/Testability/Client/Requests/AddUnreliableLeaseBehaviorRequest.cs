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

    public class AddUnreliableLeaseBehaviorRequest : FabricRequest
    {
        public AddUnreliableLeaseBehaviorRequest(IFabricClient fabricClient, string nodeName, string behaviorString, string alias, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            ThrowIf.Null(nodeName, "nodeName");

            this.Alias = alias;
            this.NodeName = nodeName;
            this.BehaviorString = behaviorString;
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

        public string BehaviorString
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "AddUnreliableLeaseBehaviorRequest at node {0} with alias {1} with timeout {2}", this.NodeName, this.Alias, this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.AddUnreliableLeaseBehaviorAsync(this.NodeName, this.BehaviorString, this.Alias, this.Timeout, cancellationToken);            
        }
    }
}


#pragma warning restore 1591