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
    using System.Fabric.Testability.Common;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;

    public class ProvisionApplicationRequest : FabricRequest
    {
        public ProvisionApplicationRequest(IFabricClient fabricClient, string applicationBuildPath, TimeSpan timeout)
            : this(fabricClient, new ProvisionApplicationTypeDescription(applicationBuildPath), timeout)
        {
        }

        public ProvisionApplicationRequest(IFabricClient fabricClient, ProvisionApplicationTypeDescriptionBase description, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            ThrowIf.Null(description, "description");

            this.Description = description;
            this.ConfigureErrorCodes();
        }

        public ProvisionApplicationTypeDescriptionBase Description
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, 
                "Provision application type ({0} with timeout {1})", 
                this.Description,
                this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.ProvisionApplicationAsync(this.Description, this.Timeout, cancellationToken);
        }

        private void ConfigureErrorCodes()
        {
            this.SucceedErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_APPLICATION_TYPE_ALREADY_EXISTS);
            this.RetryErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_IMAGEBUILDER_TIMEOUT);
            this.RetryErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_IMAGESTORE_IOERROR);
        }
    }
}


#pragma warning restore 1591