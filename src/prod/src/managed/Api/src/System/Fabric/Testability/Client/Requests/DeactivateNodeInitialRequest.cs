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
    using System.Fabric.Testability.Client.Structures;
    using System.Fabric.Testability.Common;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;

    public class DeactivateNodeInitialRequest : FabricRequest
    {
        public DeactivateNodeInitialRequest(
            IFabricClient fabricClient,
            string nodeName,
            NodeDeactivationIntent deactivationIntent,
            TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            ThrowIf.Null(nodeName, "nodeName");

            this.NodeName = nodeName;
            this.DeactivationIntent = deactivationIntent;

            this.ConfigureErrorCodes();
        }

        public string NodeName
        {
            get;
            private set;
        }

        public NodeDeactivationIntent DeactivationIntent
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "Initial deactivate node {0} with intent {1} with timeout {2}", this.NodeName, this.DeactivationIntent, this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.DeactivateNodeAsync(this.NodeName, this.DeactivationIntent, this.Timeout, cancellationToken);
        }

        private void ConfigureErrorCodes()
        {
            this.RetryErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_OBJECT_CLOSED);
        }
    }
}


#pragma warning restore 1591