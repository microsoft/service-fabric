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

    public class UnprovisionApplicationRequest : FabricRequest
    {
        public UnprovisionApplicationRequest(IFabricClient fabricClient, string applicationTypeName, string applicationTypeVersion, TimeSpan timeout)
            : this(fabricClient, new UnprovisionApplicationTypeDescription(applicationTypeName, applicationTypeVersion), timeout)
        {
        }

        public UnprovisionApplicationRequest(IFabricClient fabricClient, UnprovisionApplicationTypeDescription description, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            ThrowIf.Null(description, "description");

            this.Description = description;
            this.ConfigureErrorCodes();
        }

        public UnprovisionApplicationTypeDescription Description
        {
            get;
            private set;
        }

        public string ApplicationTypeName
        {
            get { return this.Description.ApplicationTypeName; }
        }

        public string ApplicationTypeVersion
        {
            get { return this.Description.ApplicationTypeVersion; }
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "Unprovision application type (ApplicationTypeName: {0} ApplicationTypeVersion: {1} Async: {2} with timeout {3})", this.ApplicationTypeName, this.ApplicationTypeVersion, this.Description.Async, this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.UnprovisionApplicationAsync(this.Description, this.Timeout, cancellationToken);
        }

        private void ConfigureErrorCodes()
        {
            this.SucceedErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_APPLICATION_TYPE_NOT_FOUND);
            this.RetryErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_IMAGESTORE_IOERROR);
        }
    }
}


#pragma warning restore 1591