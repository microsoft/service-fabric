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

    public class EnableBackupProtectionRequest : FabricRequest
    {
        public EnableBackupProtectionRequest(IFabricClient fabricClient, string policyNameJson, string fabricUri, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            ThrowIf.Null(policyNameJson, "policyNameJson");

            this.PolicyNameJson = policyNameJson;
            this.FabricUri = fabricUri;
            this.ConfigureErrorCodes();
        }

        public string PolicyNameJson
        {
            get;
            private set;
        }

        public string FabricUri
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "Enable backup protection, policy {0}, fabricUri {1}", this.PolicyNameJson, this.FabricUri);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.EnableBackupProtectionAsync(this.PolicyNameJson, this.FabricUri, this.Timeout);
        }

        private void ConfigureErrorCodes()
        {
        }
    }
}


#pragma warning restore 1591