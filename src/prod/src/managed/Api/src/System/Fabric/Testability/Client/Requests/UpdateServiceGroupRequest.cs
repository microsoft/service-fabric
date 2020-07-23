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
    using System.Fabric.Query;

    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1812:AvoidUninstantiatedInternalClasses", Justification = "Todo")]
    public class UpdateServiceGroupRequest : FabricRequest
    {
        public UpdateServiceGroupRequest(IFabricClient fabricClient, ServiceKind serviceKind, Uri serviceGroupName, int targetReplicaSetSize, TimeSpan replicaRestartWaitDuration, TimeSpan quorumLossWaitDuration, int instanceCount, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            ThrowIf.Null(serviceGroupName, "serviceNameGroup");

            this.ServiceGroupName = serviceGroupName;
            this.ServiceKind = serviceKind;
            this.TargetReplicaSetSize = targetReplicaSetSize;
            this.ReplicaRestartWaitDuration = replicaRestartWaitDuration;
            this.QuorumLossWaitDuration = quorumLossWaitDuration;
            this.InstanceCount = instanceCount;
        }

        public ServiceKind ServiceKind
        {
            get;
            private set;
        }

        public Uri ServiceGroupName
        {
            get;
            private set;
        }

        public int TargetReplicaSetSize
        {
            get;
            private set;
        }

        public TimeSpan ReplicaRestartWaitDuration
        {
            get;
            private set;
        }

        public TimeSpan QuorumLossWaitDuration
        {
            get;
            private set;
        }

        public int InstanceCount
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "Update service group ServiceGroupName: {0} ServiceKind: {1} with timeout {2})", this.ServiceGroupName, this.ServiceKind, this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.UpdateServiceGroupAsync(this.ServiceKind, this.ServiceGroupName, this.TargetReplicaSetSize, this.ReplicaRestartWaitDuration, this.QuorumLossWaitDuration, this.InstanceCount, this.Timeout, cancellationToken);
        }
    }
}


#pragma warning restore 1591