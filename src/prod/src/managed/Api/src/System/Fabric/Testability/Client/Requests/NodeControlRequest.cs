// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Requests
{
    using System;
    using System.Numerics;

    /// <summary>
    /// This class derives FabricRequest class and may represent a request which is specific to a node.
    /// The derived classes needs to implement ToString() and PerformCoreAsync() methods defined in FabricRequest.
    /// </summary>
    public abstract class NodeControlRequest : FabricRequest
    {
        protected NodeControlRequest(IFabricClient fabricClient, string nodeName, BigInteger nodeInstanceId, TimeSpan timeout)
            : this(fabricClient, nodeName, nodeInstanceId, CompletionMode.Invalid, timeout)
        {
        }

        protected NodeControlRequest(IFabricClient fabricClient, ReplicaSelector replicaSelector, TimeSpan timeout)
            : this(fabricClient, replicaSelector, CompletionMode.Invalid, timeout)
        {
            this.ReplicaSelector = replicaSelector;
        }

        protected NodeControlRequest(IFabricClient fabricClient, string nodeName, BigInteger nodeInstanceId, CompletionMode completionMode, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            this.NodeName = nodeName;
            this.NodeInstanceId = nodeInstanceId;
            this.CompletionMode = completionMode;
        }

        protected NodeControlRequest(IFabricClient fabricClient, ReplicaSelector replicaSelector, CompletionMode completionMode, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            this.ReplicaSelector = replicaSelector;
            this.CompletionMode = completionMode;
        }

        public string NodeName { get; private set; }

        public BigInteger NodeInstanceId { get; private set; }

        public ReplicaSelector ReplicaSelector { get; private set; }

        public CompletionMode CompletionMode { get; private set; }
    }
}


#pragma warning restore 1591