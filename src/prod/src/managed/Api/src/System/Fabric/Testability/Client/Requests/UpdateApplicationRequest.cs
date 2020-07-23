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
    using System.Fabric.Description;

    public class UpdateApplicationRequest : FabricRequest
    {
        public UpdateApplicationRequest(
            IFabricClient fabricClient,
            ApplicationUpdateDescription updateDescription,
            TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            ThrowIf.Null(updateDescription, "ApplicationUpdateDescription");

            this.UpdateDescription = updateDescription;

            this.ConfigureErrorCodes();
        }

        public ApplicationUpdateDescription UpdateDescription
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "Update Application {0} with timeout {1}", this.UpdateDescription, this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.UpdateApplicationAsync(this.UpdateDescription, this.Timeout, cancellationToken);
        }

        private void ConfigureErrorCodes()
        {
            this.RetryErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_APPLICATION_UPDATE_IN_PROGRESS);
        }
    }
}


#pragma warning restore 1591