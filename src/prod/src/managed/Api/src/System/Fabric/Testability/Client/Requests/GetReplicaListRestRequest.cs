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

    public class GetReplicaListRestRequest : FabricRequest
    {
        public GetReplicaListRestRequest(Uri applicationName, Uri serviceName, Guid partitionId, IFabricClient fabricClient, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            this.ApplicationName = applicationName;
            this.ServiceName = serviceName;
            this.PartitionId = partitionId;
        }

        public Guid PartitionId
        {
            get;
            private set;
        }

        public Uri ApplicationName
        {
            get;
            private set;
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
            return string.Format(CultureInfo.InvariantCulture, "GetReplicaListRest for partition {0} with timeout {1}, continution token {2}", this.PartitionId, this.Timeout, this.ContinuationToken);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.GetReplicaListRestAsync(this.ApplicationName, this.ServiceName, this.PartitionId, this.ContinuationToken, this.Timeout, cancellationToken);
        }
    }
}

#pragma warning restore 1591