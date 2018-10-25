// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// The enum passed into the RestartPartition API to specify what replicas need to be restarted
    /// </summary>
    [Serializable]
    public enum RestartPartitionMode
    {
        /// <summary>
        /// Reserved.  Do not pass into API.
        /// </summary>
        Invalid = NativeTypes.FABRIC_RESTART_PARTITION_MODE.FABRIC_RESTART_PARTITION_MODE_INVALID,
        
        /// <summary>
        /// All replicas or instances in the partition are restarted at once
        /// </summary>
        AllReplicasOrInstances = NativeTypes.FABRIC_RESTART_PARTITION_MODE.FABRIC_RESTART_PARTITION_MODE_ALL_REPLICAS_OR_INSTANCES,
        
        /// <summary>
        /// Only the secondary replicas are restarted. This option can only be used for stateful services and avoids data loss
        /// </summary>
        OnlyActiveSecondaries = NativeTypes.FABRIC_RESTART_PARTITION_MODE.FABRIC_RESTART_PARTITION_MODE_ONLY_ACTIVE_SECONDARIES
    }
}