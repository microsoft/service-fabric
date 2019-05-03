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

    public class DeleteServiceRequest : FabricRequest
    {
        private readonly bool serviceDoesNotExist;

        public DeleteServiceRequest(IFabricClient fabricClient, DeleteServiceDescription deleteServiceDescription, TimeSpan timeout)
            : this(fabricClient, deleteServiceDescription, timeout, false)
        {
        }

        public DeleteServiceRequest(IFabricClient fabricClient, DeleteServiceDescription deleteServiceDescription, TimeSpan timeout, bool serviceDoesNotExist)
            : base(fabricClient, timeout)
        {
            ThrowIf.Null(deleteServiceDescription, "deleteServiceDescription");

            this.DeleteServiceDescription = deleteServiceDescription;
            this.serviceDoesNotExist = serviceDoesNotExist;
            this.ConfigureErrorCodes();
        }

        public DeleteServiceDescription DeleteServiceDescription
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "Delete service {0} Force {1} with timeout {2}", this.DeleteServiceDescription.ServiceName, this.DeleteServiceDescription.ForceDelete, this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.DeleteServiceAsync(this.DeleteServiceDescription, this.Timeout, cancellationToken);
        }

        private void ConfigureErrorCodes()
        {
            if (this.serviceDoesNotExist)
            {
                this.SucceedErrorCodes.Remove((uint)NativeTypes.FABRIC_ERROR_CODE.S_OK);
            }
            
            this.SucceedErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_SERVICE_DOES_NOT_EXIST);
        }
    }
}


#pragma warning restore 1591