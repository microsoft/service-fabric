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
    /// <para>Represents filter for <see cref="System.Fabric.Health.ServiceHealthState" /> objects.</para>
    /// </summary>
    /// <remarks>The filter can be used in <see cref="System.Fabric.Description.ApplicationHealthQueryDescription"/>
    /// to specify which service children should be returned as part of <see cref="System.Fabric.Health.ApplicationHealth"/>.</remarks>
    public sealed class ServiceHealthStatesFilter
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Health. ServiceHealthStatesFilter" /> class.</para>
        /// </summary>
        public ServiceHealthStatesFilter()
        {
        }

        /// <summary>
        /// Gets or sets the filter for the aggregated health state of the <see cref="System.Fabric.Health.ServiceHealthState"/> entries in the collection. 
        /// </summary>
        /// <value>The filter for the aggregated health state of the <see cref="System.Fabric.Health.ServiceHealthState"/> entries.</value>
        /// <remarks>The input collection is filtered to return only entries that respect the desired health state. 
        /// The filter represents a value obtained from members or bitwise combinations of members of <see cref="System.Fabric.Health.HealthStateFilter"/>.</remarks>
        public HealthStateFilter HealthStateFilterValue
        {
            get;
            set;
        }

        /// <summary>
        /// <para>DEPRECATED. Gets or sets the filter for the aggregated health state of the <see cref="System.Fabric.Health.ServiceHealthState" /> entries in the collection. </para>
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
        /// <returns>A string representation of the filter.</returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder("ServicesFilter: ");
            sb.Append(string.Format(CultureInfo.CurrentCulture, "{0}", this.HealthStateFilterValue));
            return sb.ToString();
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeServiceHealthStatesFilter = new NativeTypes.FABRIC_SERVICE_HEALTH_STATES_FILTER();

            nativeServiceHealthStatesFilter.HealthStateFilter = (UInt32)this.HealthStateFilterValue;

            return pinCollection.AddBlittable(nativeServiceHealthStatesFilter);
        }
    }
}
