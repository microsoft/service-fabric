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

    public class CreateServiceRequest : FabricRequest
    {
        public CreateServiceRequest(IFabricClient fabricClient, ServiceDescription serviceDescription, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            ThrowIf.Null(serviceDescription, "serviceDescription");

            this.ServiceDescription = serviceDescription;
            this.ConfigureErrorCodes();
        }

        public ServiceDescription ServiceDescription
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "Create service {0} with timeout {1}", this.ServiceDescription.ServiceName, this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.CreateServiceAsync(this.ServiceDescription, this.Timeout, cancellationToken);
        }

        private void ConfigureErrorCodes()
        {
            this.SucceedErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_SERVICE_ALREADY_EXISTS);
        }
    }
}


#pragma warning restore 1591