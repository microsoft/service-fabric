// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Describes the weight of a metric.</para>
    /// </summary>
    public enum ServiceLoadMetricWeight
    {
        /// <summary>
        /// <para>Specifies the metric weight of the service load as Zero. Disable Service Fabric Load Balancing for this metric. Note that metrics 
        /// that are not balanced during runtime can still be used to control capacity on nodes and can still be reported via 
        /// <see cref="System.Fabric.IServicePartition.ReportLoad(System.Collections.Generic.IEnumerable{System.Fabric.LoadMetric})" />.</para>
        /// </summary>
        Zero = NativeTypes.FABRIC_SERVICE_LOAD_METRIC_WEIGHT.FABRIC_SERVICE_LOAD_METRIC_WEIGHT_ZERO,
        
        /// <summary>
        /// <para>Specifies the metric weight of the service load as Low.</para>
        /// </summary>
        Low = NativeTypes.FABRIC_SERVICE_LOAD_METRIC_WEIGHT.FABRIC_SERVICE_LOAD_METRIC_WEIGHT_LOW,
        
        /// <summary>
        /// <para>Specifies the metric weight of the service load as Medium.</para>
        /// </summary>
        Medium = NativeTypes.FABRIC_SERVICE_LOAD_METRIC_WEIGHT.FABRIC_SERVICE_LOAD_METRIC_WEIGHT_MEDIUM,
        
        /// <summary>
        /// <para>Specifies the metric weight of the service load as High.</para>
        /// </summary>
        High = NativeTypes.FABRIC_SERVICE_LOAD_METRIC_WEIGHT.FABRIC_SERVICE_LOAD_METRIC_WEIGHT_HIGH
    }
}