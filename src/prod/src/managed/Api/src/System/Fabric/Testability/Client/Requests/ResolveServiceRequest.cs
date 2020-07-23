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

    public class ResolveServiceRequest : FabricRequest
    {
        public ResolveServiceRequest(IFabricClient fabricClient, Uri name, object partitionKey, TestServicePartitionInfo previousResult, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            ThrowIf.Null(name, "name");

            this.Name = name;
            this.PartitionKey = partitionKey;
            this.PreviousResult = previousResult;

            this.ConfigureErrorCodes();
        }

        public Uri Name
        {
            get;
            private set;
        }

        public object PartitionKey
        {
            get;
            private set;
        }

        public TestServicePartitionInfo PreviousResult
        {
            get;
            set;
        }

        public override string ToString()
        {
            // Todo: long version = (this.PreviousResult == null) ? -1 : this.PreviousResult.FMVersion;
            return string.Format(CultureInfo.InvariantCulture, "Resolve service (Name: {0} PartitionKey: {1} PreviousResult: {2} and Timeout {3})", this.Name, this.PartitionKey, 0, this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.ResolveServiceAsync(this.Name, this.PartitionKey, this.PreviousResult, this.Timeout, cancellationToken);
        }

        private void ConfigureErrorCodes()
        {
            this.RetryErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_SERVICE_OFFLINE);
        }
    }
}


#pragma warning restore 1591