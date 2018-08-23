// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
    public enum ReplicaOpenMode
    {
        /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
        Invalid = NativeTypes.FABRIC_REPLICA_OPEN_MODE.FABRIC_REPLICA_OPEN_MODE_INVALID,

        /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
        New = NativeTypes.FABRIC_REPLICA_OPEN_MODE.FABRIC_REPLICA_OPEN_MODE_NEW,

        /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
        Existing = NativeTypes.FABRIC_REPLICA_OPEN_MODE.FABRIC_REPLICA_OPEN_MODE_EXISTING
    }
}