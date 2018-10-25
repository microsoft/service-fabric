// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;

    /// <summary>
    /// <para>Represents the name of a metric and a runtime value as a name-value pair that is reported to Service Fabric. The metric loads are used by Service Fabric to ensure that the cluster is evenly used and that nodes do not exceed their capacities for given metrics. <see cref="System.Fabric.LoadMetric" /> reports are provided to Service Fabric via <see cref="System.Fabric.IServicePartition.ReportLoad(System.Collections.Generic.IEnumerable{System.Fabric.LoadMetric})" />.</para>
    /// </summary>

    [Serializable]
    public sealed class LoadMetric
    {
        /// <summary>
        /// <para>Creates and initializes a <see cref="System.Fabric.LoadMetric" /> object with the specified name and load value.</para>
        /// </summary>
        /// <param name="name">
        /// <para>The name of the metric. This string must match the names of the metrics that are specified in the <see cref="System.Fabric.Description.ServiceDescription.Metrics" /> collection, or they will be ignored.</para>
        /// </param>
        /// <param name="value">
        /// <para>The current value of the metric as an integer.</para>
        /// </param>
        public LoadMetric(string name, int value)
        {
            this.Name = name;
            this.Value = value;
        }

        /// <summary>
        /// <para>Indicates the name of the metric that the service plans to report. </para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.String" />.</para>
        /// </value>
        public string Name { get; private set; }

        /// <summary>
        /// <para>Indicates the current load of the metric.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Int32" />.</para>
        /// </value>
        public int Value { get; private set; }
    }
}