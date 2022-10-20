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

    public class AddUnreliableTransportBehaviorRequest : FabricRequest
    {
        public AddUnreliableTransportBehaviorRequest(IFabricClient fabricClient, string nodeName, string name, System.Fabric.Testability.UnreliableTransportBehavior behavior, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            ThrowIf.Null(nodeName, "nodeName");

            this.Name = name;
            this.NodeName = nodeName;
            this.Behavior = behavior;
        }

        public string Name
        {
            get;
            private set;
        }

        public string NodeName
        {
            get;
            private set;
        }

        public System.Fabric.Testability.UnreliableTransportBehavior Behavior
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "AddUnreliableTransportBehaviorRequest at node {0} with name {1} with timeout {2}", this.NodeName, this.Name, this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.AddUnreliableTransportBehaviorAsync(this.NodeName, this.Name, this.Behavior, this.Timeout, cancellationToken);
            
            // TODO: Wait for some time so that the unreliable transport behavior can be read from the files. 
            // Bug#2271465 - Unreliable transport through API should return only once the behavior has been successfully applied
            await Task.Delay(TimeSpan.FromSeconds(5));
        }
    }
}


#pragma warning restore 1591