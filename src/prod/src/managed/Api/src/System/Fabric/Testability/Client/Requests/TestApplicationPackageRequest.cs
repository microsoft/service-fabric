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
    using System.Threading.Tasks;
    using System.Fabric.Testability.Client.Structures;
    using System.Fabric.Testability.Common;

    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1812:AvoidUninstantiatedInternalClasses", Justification = "Todo")]
    public class TestApplicationPackageRequest : FabricRequest
    {
        public TestApplicationPackageRequest(IFabricClient fabricClient, string applicationPackagePath, string imageStoreConnectionString)
            : base(fabricClient, TimeSpan.MaxValue/*TestApplicationPackage does not take a timeout*/)
        {
            ThrowIf.Null(applicationPackagePath, "applicationPackagePath");
            
            this.ApplicationPackagePath = applicationPackagePath;
            this.ImageStoreConnectionString = imageStoreConnectionString;
        }

        public string ApplicationPackagePath
        {
            get;
            private set;
        }

        public string ImageStoreConnectionString
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture, 
                "Test application package request ApplicationPackagePath: {0}, imagestoreConnectionString: {1}, timeout: {2})", 
                this.ApplicationPackagePath, 
                this.ImageStoreConnectionString != null ? this.ImageStoreConnectionString : "null",
                this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.TestApplicationPackageAsync(this.ApplicationPackagePath, this.ImageStoreConnectionString, cancellationToken);
        }
    }
}


#pragma warning restore 1591