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
    using Description;

    public class DeleteApplicationRequest : FabricRequest
    {
        public DeleteApplicationRequest(IFabricClient fabricClient, DeleteApplicationDescription deleteApplicationDescription, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            ThrowIf.Null(deleteApplicationDescription, "deleteApplicationDescription");

            this.DeleteApplicationDescription = deleteApplicationDescription;
            this.ConfigureErrorCodes();
        }

        public DeleteApplicationDescription DeleteApplicationDescription
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "Delete application {0} Force {1} with timeout {2}", this.DeleteApplicationDescription.ApplicationName, this.DeleteApplicationDescription.ForceDelete, this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.DeleteApplicationAsync(this.DeleteApplicationDescription, this.Timeout, cancellationToken);
        }

        private void ConfigureErrorCodes()
        {
            this.SucceedErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_APPLICATION_NOT_FOUND);
        }
    }
}

#pragma warning restore 1591