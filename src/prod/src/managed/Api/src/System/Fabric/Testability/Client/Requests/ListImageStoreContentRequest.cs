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
    using System.Fabric.Testability.Common;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;
   

    public class ListImageStoreContentRequest : FabricRequest
    {
        public ListImageStoreContentRequest(
            IFabricClient fabricClient,
            string remoteLocation,
            string imageStoreConnectionString,
            TimeSpan timeout)
           : base(fabricClient, timeout)
        {
            this.RemoteLocation = remoteLocation;
            this.ImageStoreConnectionString = imageStoreConnectionString;
        }

        public string RemoteLocation
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
                "Get image store content {0} with timeout {1}",
                this.RemoteLocation,
                this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            
            this.OperationResult = await this.FabricClient.ListImageStoreContentAsync(
                this.RemoteLocation,
                this.ImageStoreConnectionString,
                this.Timeout,
                cancellationToken);
        }
    }
}

#pragma warning restore 1591