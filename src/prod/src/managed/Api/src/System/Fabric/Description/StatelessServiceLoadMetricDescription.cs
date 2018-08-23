// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System.Fabric.Common;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.Text;

    /// <summary>
    /// <para>Specifies a metric for a stateless service.</para>
    /// </summary>
    public sealed class StatelessServiceLoadMetricDescription : ServiceLoadMetricDescription
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.StatelessServiceLoadMetricDescription" /> class.</para>
        /// </summary>
        public StatelessServiceLoadMetricDescription()
        {
        }

        internal StatelessServiceLoadMetricDescription(string name, int defaultLoad, ServiceLoadMetricWeight loadMetricWeight = 0)
            : base(name, loadMetricWeight)
        {
            this.DefaultLoad = defaultLoad;
        }

        // Copy ctor
        internal StatelessServiceLoadMetricDescription(StatelessServiceLoadMetricDescription other)
            : this(other.Name, other.DefaultLoad, other.Weight)
        {
        }

#pragma warning disable 618

        /// <summary>
        /// <para>Gets or sets the default amount of load that this service creates for this metric.</para>
        /// </summary>
        /// <value><para>The default amount of load that this service creates for this metric.</para></value>
        /// <remarks>
        /// <para>Specifying default load values for custom metrics enables the Service Fabric Cluster Resource Manager to efficiently place services when they are first created.
        /// If default load is not specified Service Fabric Cluster Resource Manager will assume zero load for this replica until the service reports its load.
        /// <see cref="System.Fabric.IServicePartition.ReportLoad(System.Collections.Generic.IEnumerable{System.Fabric.LoadMetric})" />.
        /// </para>
        /// </remarks>
        [JsonCustomization(PropertyName = JsonPropertyNames.DefaultLoad)]
        public int DefaultLoad
        {
            get
            {
                return base.PrimaryDefaultLoad;
            }
            set
            {
                base.PrimaryDefaultLoad = value;
            }
        }

#pragma warning restore 618

        /// <summary>
        /// Pretty print out details of <see cref="System.Fabric.Description.StatelessServiceLoadMetricDescription"/>.
        /// </summary>
        /// <returns>A string representation of the <see cref="System.Fabric.Description.StatelessServiceLoadMetricDescription"/>.</returns>
        /// <example>
        /// CPU,High,90
        /// </example>
        public override string ToString()
        {
            return base.ToString(false);
        }
    }
}