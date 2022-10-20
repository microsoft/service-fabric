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
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;

    public class UnprovisionFabricRequest : FabricRequest
    {
        public UnprovisionFabricRequest(IFabricClient fabricClient, string codeVersion, string configVersion, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            if (codeVersion == null && configVersion == null)
            {
                throw new ArgumentException("CodeVersion & ConfigVersion cannot be null at the same time");
            }

            this.CodeVersion = codeVersion;
            this.ConfigVersion = configVersion;
            this.ConfigureErrorCodes();
        }

        public string CodeVersion
        {
            get;
            private set;
        }

        public string ConfigVersion
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "Unprovision fabric (CodeVersion: {0} ConfigVersion: {1} with timeout {2})", this.CodeVersion, this.ConfigVersion, this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.UnprovisionFabricAsync(this.CodeVersion, this.ConfigVersion, this.Timeout, cancellationToken);
        }

        private void ConfigureErrorCodes()
        {
            this.SucceedErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_FABRIC_VERSION_NOT_FOUND);
            this.RetryErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_IMAGESTORE_IOERROR);
        }
    }
}


#pragma warning restore 1591