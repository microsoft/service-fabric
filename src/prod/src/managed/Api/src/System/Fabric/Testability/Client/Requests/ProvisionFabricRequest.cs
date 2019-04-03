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

    public class ProvisionFabricRequest : FabricRequest
    {
        public ProvisionFabricRequest(IFabricClient fabricClient, string codePackagePath, string clusterManifestFilePath, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            if (codePackagePath == null && clusterManifestFilePath == null)
            {
                throw new ArgumentException("CodePackagePath & ClusterManifestFilePath cannot be null at the same time");
            }

            this.CodePackagePath = codePackagePath;
            this.ClusterManifestFilePath = clusterManifestFilePath;
            this.ConfigureErrorCodes();
        }

        public string CodePackagePath
        {
            get;
            private set;
        }

        public string ClusterManifestFilePath
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "Provision fabric (CodePackagePath: {0} ClusterManifestFilePath: {1} with timeout {2})",
                this.CodePackagePath,
                this.ClusterManifestFilePath,
                this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.ProvisionFabricAsync(this.CodePackagePath, this.ClusterManifestFilePath, this.Timeout, cancellationToken);
        }

        private void ConfigureErrorCodes()
        {
            this.SucceedErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_FABRIC_VERSION_ALREADY_EXISTS);

            this.RetryErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_IMAGESTORE_IOERROR);
        }
    }
}


#pragma warning restore 1591