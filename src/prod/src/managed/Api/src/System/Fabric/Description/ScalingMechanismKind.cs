// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Enumerates the available mechanisms for scaling.</para>
    /// </summary>
    public enum ScalingMechanismKind
    {
        /// <summary>
        /// <para>All Service Fabric enumerations have a reserved Invalid flag.</para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_SCALING_MECHANISM_KIND.FABRIC_SCALING_MECHANISM_KIND_INVALID,

        /// <summary>
        /// <para>Indicates a mechanism for scaling where new instances are added or removed from a partition.</para>
        /// </summary>
        ScalePartitionInstanceCount = NativeTypes.FABRIC_SCALING_MECHANISM_KIND.FABRIC_SCALING_MECHANISM_KIND_SCALE_PARTITION_INSTANCE_COUNT,

        /// <summary>
        /// <para>Indicates a mechanism for scaling where new named partitions are added or removed from a service.</para>
        /// </summary>
        AddRemoveIncrementalNamedPartition = NativeTypes.FABRIC_SCALING_MECHANISM_KIND.FABRIC_SCALING_MECHANISM_KIND_ADD_REMOVE_INCREMENTAL_NAMED_PARTITION
    }

}