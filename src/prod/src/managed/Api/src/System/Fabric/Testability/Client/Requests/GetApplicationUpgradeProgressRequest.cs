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

    public class GetApplicationUpgradeProgressRequest : UpgradeProgressRequest<ApplicationUpgradeProgress>
    {
        public GetApplicationUpgradeProgressRequest(IFabricClient fabricClient, Uri applicationName, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            ThrowIf.Null(applicationName, "applicationName");

            this.ApplicationName = applicationName;
            this.ConfigureErrorCodes();
        }

        public Uri ApplicationName
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "Get upgrade status for application {0} with timeout {1}", this.ApplicationName, this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.GetApplicationUpgradeProgressAsync(this.ApplicationName, this.Timeout, cancellationToken);
        }

        private void ConfigureErrorCodes()
        {
            this.SucceedErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.S_OK);
            this.RetryErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_TIMEOUT);
            this.RetryErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.E_ABORT);
        }
    }
}


#pragma warning restore 1591