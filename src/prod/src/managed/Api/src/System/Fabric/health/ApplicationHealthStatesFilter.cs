// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.Text;

    /// <summary>
    /// <para>Provides filter for <see cref="System.Fabric.Health.ApplicationHealthState" /> objects.</para>
    /// </summary>
    /// <remarks>The filter can be used in <see cref="System.Fabric.Description.ClusterHealthQueryDescription"/>
    /// to specify which application children of the cluster should be returned as part of <see cref="System.Fabric.Health.ClusterHealth"/>.</remarks>
    public sealed class ApplicationHealthStatesFilter
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Health.ApplicationHealthStatesFilter" /> class.</para>
        /// </summary>
        public ApplicationHealthStatesFilter()
        {
        }

        /// <summary>
        /// Gets or sets the filter for the aggregated health state of the <see cref="System.Fabric.Health.ApplicationHealthState"/> entries in the collection. 
        /// </summary>
        /// <value>The filter for the aggregated health state of the <see cref="System.Fabric.Health.ApplicationHealthState"/> entries.</value>
        /// <remarks>The input collection is filtered to return only entries that respect the desired health state. 
        /// The filter represents a value obtained from members or bitwise combinations of members of <see cref="System.Fabric.Health.HealthStateFilter"/>.</remarks>
        public HealthStateFilter HealthStateFilterValue
        {
            get;
            set;
        }

        /// <summary>
        /// <para>DEPRECATED. Gets or sets filter for the aggregated health state of the <see cref="System.Fabric.Health.ApplicationHealthState" /> entries in the collection. </para>
        /// </summary>
        /// <value>
        /// <para>The value of the filter. The value comes from members or bitwise combination of members of <see cref="System.Fabric.Health.HealthStateFilter" />.</para>
        /// </value>
        /// <remarks>This property is obsolete. Use <see cref="System.Fabric.Health.ApplicationHealthStatesFilter.HealthStateFilterValue"/> instead.</remarks>
        [Obsolete("This property is obsolete. Use HealthStateFilterValue instead.")]
        public long HealthStateFilter
        {
            get { return (long)this.HealthStateFilterValue; }
            set { this.HealthStateFilterValue = (HealthStateFilter)value; }
        }

        /// <summary>
        /// Returns a string representation of the filter.
        /// </summary>
        /// <returns>A string representation of the <see cref="System.Fabric.Health.ApplicationHealthStatesFilter"/>.</returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder("ApplicationsFilter: ");
            sb.Append(string.Format(CultureInfo.CurrentCulture, "{0}", this.HealthStateFilterValue));
            return sb.ToString();
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeApplicationHealthStatesFilter = new NativeTypes.FABRIC_APPLICATION_HEALTH_STATES_FILTER();

            nativeApplicationHealthStatesFilter.HealthStateFilter = (UInt32)this.HealthStateFilterValue;

            return pinCollection.AddBlittable(nativeApplicationHealthStatesFilter);
        }
    }
}
