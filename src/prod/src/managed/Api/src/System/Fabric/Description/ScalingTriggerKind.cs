// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Enumerates the available triggers for scaling.</para>
    /// </summary>
    public enum ScalingTriggerKind
    {
        /// <summary>
        /// <para>All Service Fabric enumerations have a reserved Invalid flag.</para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_SCALING_TRIGGER_KIND.FABRIC_SCALING_TRIGGER_KIND_INVALID,

        /// <summary>
        /// <para>Indicates a trigger where scaling decisions are made based on average load of a partition.</para>
        /// </summary>
        AveragePartitionLoadTrigger = NativeTypes.FABRIC_SCALING_TRIGGER_KIND.FABRIC_SCALING_TRIGGER_KIND_AVERAGE_PARTITION_LOAD,

        /// <summary>
        /// <para>Indicates a trigger where scaling decisions are made based on average load of a service.</para>
        /// </summary>
        AverageServiceLoadTrigger = NativeTypes.FABRIC_SCALING_TRIGGER_KIND.FABRIC_SCALING_TRIGGER_KIND_AVERAGE_SERVICE_LOAD
    }

}