// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
    public enum ReplicaStatus
    {
        /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
        Invalid = NativeTypes.FABRIC_REPLICA_STATUS.FABRIC_REPLICA_STATUS_INVALID,

        /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
        Down = NativeTypes.FABRIC_REPLICA_STATUS.FABRIC_REPLICA_STATUS_DOWN,

        /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
        Up = NativeTypes.FABRIC_REPLICA_STATUS.FABRIC_REPLICA_STATUS_UP
    }
}