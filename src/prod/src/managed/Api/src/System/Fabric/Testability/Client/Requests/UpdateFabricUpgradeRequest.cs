// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Requests
{
    using System;
    using System.Fabric.Description;
    using System.Fabric.Interop;
    using System.Fabric.Testability.Client.Structures;
    using System.Fabric.Testability.Common;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;

    public class UpdateFabricUpgradeRequest : FabricRequest
    {
        public UpdateFabricUpgradeRequest(IFabricClient fabricClient, FabricUpgradeUpdateDescription updateDescription, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            ThrowIf.Null(updateDescription, "updateDescription");

            this.UpdateDescription = updateDescription;
        }

        public FabricUpgradeUpdateDescription UpdateDescription
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "Update upgrade fabric {0} with timeout {1}", this.UpdateDescription, this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.UpdateFabricUpgradeAsync(this.UpdateDescription, this.Timeout, cancellationToken);
        }
    }
}


#pragma warning restore 1591