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

    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1812:AvoidUninstantiatedInternalClasses", Justification = "Todo")]
    public class TestClusterConnectionRequest : FabricRequest
    {
        public TestClusterConnectionRequest(IFabricClient fabricClient)
            : base(fabricClient, TimeSpan.MaxValue/*API does not take timeout*/)
        {
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "Test cluster connection with timeout {0})", this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.TestClusterConnectionAsync(cancellationToken);
        }
    }
}


#pragma warning restore 1591