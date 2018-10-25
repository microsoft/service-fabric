// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.Text;

    /// <summary>
    /// <para>Specifies a metric for a stateful service.</para>
    /// </summary>
    public sealed class StatefulServiceLoadMetricDescription : ServiceLoadMetricDescription
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.StatefulServiceLoadMetricDescription" /> class.</para>
        /// </summary>
        public StatefulServiceLoadMetricDescription()
        {
        }

        // secondaryDefaultLoad not needed for StatelessServices that's why is optional.
        internal StatefulServiceLoadMetricDescription(string name, int primaryDefaultLoad, int secondaryDefaultLoad, ServiceLoadMetricWeight loadMetricWeight = 0)
            : base(name, loadMetricWeight)
        {
            this.PrimaryDefaultLoad = primaryDefaultLoad;
            this.SecondaryDefaultLoad = secondaryDefaultLoad;
        }

        // Copy ctor
        internal StatefulServiceLoadMetricDescription(StatefulServiceLoadMetricDescription other)
            : this(other.Name, other.PrimaryDefaultLoad, other.SecondaryDefaultLoad, other.Weight)
        {
        }

#pragma warning disable 618

        /// <summary>       
        /// <para>Gets or sets the default amount of load that this service creates for this metric when it is a Primary replica.</para>       
        /// </summary>        
        /// <value><para>The default amount of load that this service creates for this metric when it is a Primary replica.</para></value>        
        /// <remarks>        
        /// <para>Specifying default load values for custom metrics enables the Service Fabric Cluster Resource Manager to efficiently place services when they are first created.        
        ///  If default load is not specified Service Fabric Cluster Resource Manager will assume zero load for this replica until the primary replica of this service reports its load.         
        /// <see cref="System.Fabric.IServicePartition.ReportLoad(System.Collections.Generic.IEnumerable{System.Fabric.LoadMetric})" />.       
        /// </para>        
        /// </remarks>
        public new int PrimaryDefaultLoad
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

        /// <summary>        
        /// <para>Gets or sets the default amount of load that this service creates for this metric when it is a Secondary replica.</para>        
        /// </summary>        
        /// <value><para>The default amount of load that this service creates for this metric when it is a Secondary replica.</para></value>        
        /// <remarks>        
        /// <para>Specifying default load values for custom metrics enables the Service Fabric Cluster Resource Manager to efficiently place services when they are first created.        
        ///  If default load is not specified Service Fabric Cluster Resource Manager will assume zero load for this replica until the secondary replica of this service reports its load.         
        /// <see cref="System.Fabric.IServicePartition.ReportLoad(System.Collections.Generic.IEnumerable{System.Fabric.LoadMetric})" />.        
        /// </para>        
        /// </remarks>
        public new int SecondaryDefaultLoad
        {
            get
            {
                return base.SecondaryDefaultLoad;
            }
            set
            {
                base.SecondaryDefaultLoad = value;
            }
        }

#pragma warning restore 618
        /// <summary>
        /// Pretty print out details of <see cref="System.Fabric.Description.StatefulServiceLoadMetricDescription"/>.
        /// </summary>
        /// <returns>A string representation of the <see cref="System.Fabric.Description.StatefulServiceLoadMetricDescription"/>.</returns>
        /// <example>
        /// CPU,High,90,10
        /// </example>
        public override string ToString()
        {
            return base.ToString(true);
        }
    }
}