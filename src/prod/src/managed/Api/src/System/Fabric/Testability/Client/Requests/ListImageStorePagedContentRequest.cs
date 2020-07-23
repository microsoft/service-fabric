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
    using System.Fabric.ImageStore;
    using System.Fabric.Testability.Common;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Text;

    public class ListImageStorePagedContentRequest : FabricRequest
    {
        public ListImageStorePagedContentRequest(
            IFabricClient fabricClient, 
            string remoteLocation,
            string imageStoreConnectionString,
            string continuationToken,
            TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            this.RemoteLocation = remoteLocation;
            this.ImageStoreConnectionString = imageStoreConnectionString;
            this.ContinuationToken = continuationToken;
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

        public string ContinuationToken
        {
            get;
            private set;
        }

        public override string ToString()
        {
            StringBuilder builder = new StringBuilder();
            builder.Append("Get image store content ");
            builder.Append(this.RemoteLocation);
            builder.Append(" with timeout:");
            builder.Append(this.Timeout);
            if (!string.IsNullOrEmpty(ContinuationToken))
            {
                builder.Append(",ContinuationToken:");
                builder.Append(ContinuationToken);
            }

            return string.Format(
                CultureInfo.InvariantCulture,
                builder.ToString());
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.ListImageStorePagedContentAsync(
                new ImageStoreListDescription(this.RemoteLocation, this.ContinuationToken, false),
                this.ImageStoreConnectionString,
                this.Timeout,
                cancellationToken);
        }
    }
}

#pragma warning restore 1591