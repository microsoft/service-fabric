// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
    public enum ReplicaSetQuorumMode
    {
        /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
        Invalid = NativeTypes.FABRIC_REPLICA_SET_QUORUM_MODE.FABRIC_REPLICA_SET_QUORUM_INVALID,

        /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
        WriteQuorum = NativeTypes.FABRIC_REPLICA_SET_QUORUM_MODE.FABRIC_REPLICA_SET_WRITE_QUORUM,

        /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
        QuorumAll = NativeTypes.FABRIC_REPLICA_SET_QUORUM_MODE.FABRIC_REPLICA_SET_QUORUM_ALL
    }
}