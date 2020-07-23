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
    using System.Fabric.Testability.Common;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;

    public class ActivateNodeRequest : FabricRequest
    {
        public ActivateNodeRequest(IFabricClient fabricClient, string nodeName, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            ThrowIf.Null(nodeName, "nodeName");

            this.NodeName = nodeName;
            this.ConfigureErrorCodes();
        }

        public string NodeName
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "Activate node {0} with timeout {1}", this.NodeName, this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.ActivateNodeAsync(this.NodeName, this.Timeout, cancellationToken);
        }

        private void ConfigureErrorCodes()
        {
            this.RetryErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_NODE_NOT_FOUND);
        }
    }
}


#pragma warning restore 1591