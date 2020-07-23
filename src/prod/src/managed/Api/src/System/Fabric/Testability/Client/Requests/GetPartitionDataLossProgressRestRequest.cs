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

    public class GetPartitionDataLossProgressRestRequest : FabricRequest
    {
        public GetPartitionDataLossProgressRestRequest(IFabricClient fabricClient, Guid operationId, Uri serviceName, Guid partitionId, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            this.OperationId = operationId;
            this.ServiceName = serviceName;
            this.PartitionId = partitionId;

            this.RetryErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_NOT_READY);
            this.RetryErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_RECONFIGURATION_PENDING);
            this.RetryErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_KEY_NOT_FOUND);
        }

        public Guid OperationId
        {
            get;
            private set;
        }

        public Uri ServiceName
        {
            get;
            private set;
        }

        public Guid PartitionId
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "GetPartitionDataLossProgressRestRequest with OperationId={0}, timeout={1}", this.OperationId, this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.GetPartitionDataLossProgressRestAsync(this.OperationId, this.ServiceName, this.PartitionId, this.Timeout, cancellationToken);
        }
    }
}

#pragma warning restore 1591