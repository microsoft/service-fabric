// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Requests
{
    using System;
    using System.Globalization;
    using System.Threading;
    using System.Fabric.Testability.Common;
    using System.Threading.Tasks;
    using System.Fabric.Testability.Client.Structures;

    public class GetApplicationManifestRequest : FabricRequest
    {
        public GetApplicationManifestRequest(IFabricClient fabricClient, string applicationTypeName, string applicationTypeVersion, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            ThrowIf.Null(applicationTypeName, "applicationTypeName");
            ThrowIf.Null(applicationTypeVersion, "applicationTypeVersion");

            this.ApplicationTypeName = applicationTypeName;
            this.ApplicationTypeVersion = applicationTypeVersion;
        }

        public string ApplicationTypeName
        {
            get;
            private set;
        }

        public string ApplicationTypeVersion
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "Get application manifest ApplicationTypeName {0}  ApplicationTypeVersion {1} with timeout {2}",
                this.ApplicationTypeName, 
                this.ApplicationTypeVersion,
                this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.GetApplicationManifestAsync(this.ApplicationTypeName, this.ApplicationTypeVersion, this.Timeout, cancellationToken);
        }
    }
}


#pragma warning restore 1591