// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Requests
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;

    public class StartNodeTransitionRequest : FabricRequest
    {
        public StartNodeTransitionRequest(IFabricClient fabricClient, NodeTransitionDescription description, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            ThrowIf.Null(description, "description");

            this.Description = description;

            this.RetryErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_NOT_READY);
            this.RetryErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_RECONFIGURATION_PENDING);

            this.SucceedErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_TEST_COMMAND_OPERATION_ID_ALREADY_EXISTS);
        }

        public NodeTransitionDescription Description
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "StartNodeTransitionRequest with Description={0} timeout={1}",
                this.Description,
                this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.StartNodeTransitionAsync(this.Description, this.Timeout, cancellationToken).ConfigureAwait(false);
        }
    }
}

#pragma warning restore 1591