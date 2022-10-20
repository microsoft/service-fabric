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
    using System.Fabric.Testability.Common;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;

    public class GetPartitionListRequest : FabricRequest
    {
        public GetPartitionListRequest(Uri serviceName, IFabricClient fabricClient, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            ThrowIf.Null(serviceName, "serviceName");

            this.ServiceName = serviceName;

            this.ConfigureErrorCodes();
        }

        public Uri ServiceName
        {
            get;
            private set;
        }

        public string ContinuationToken
        {
            get;
            set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "GetPartitionList for service {0} with timeout {1}, continuation token \"{2}\"", this.ServiceName.OriginalString, this.Timeout, this.ContinuationToken);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.GetPartitionListAsync(this.ServiceName, this.ContinuationToken, this.Timeout, cancellationToken);
        }

        private void ConfigureErrorCodes()
        {
            this.RetryErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_SERVICE_DOES_NOT_EXIST);
        }
    }
}

#pragma warning restore 1591