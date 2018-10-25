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
    /// <para>Represents the filter for <see cref="System.Fabric.Health.DeployedApplicationHealthState" /> objects.</para>
    /// </summary>
    /// <remarks>The filter can be used in <see cref="System.Fabric.Description.ApplicationHealthQueryDescription"/>
    /// to specify which deployed application children should be returned as part of <see cref="System.Fabric.Health.ApplicationHealth"/>.</remarks>
    public sealed class DeployedApplicationHealthStatesFilter
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Health.DeployedApplicationHealthStatesFilter" /> class.</para>
        /// </summary>
        public DeployedApplicationHealthStatesFilter()
        {
        }

        /// <summary>
        /// Gets or sets the filter for the aggregated health state of the <see cref="System.Fabric.Health.DeployedApplicationHealthState"/> entries in the collection. 
        /// </summary>
        /// <value>The filter for the aggregated health state of the <see cref="System.Fabric.Health.DeployedApplicationHealthState"/> entries.</value>
        /// <remarks>The input collection is filtered to return only entries that respect the desired health state. 
        /// The filter represents a value obtained from members or bitwise combinations of members of <see cref="System.Fabric.Health.HealthStateFilter"/>.</remarks>
        public HealthStateFilter HealthStateFilterValue
        {
            get;
            set;
        }

        /// <summary>
        /// <para>DEPRECATED. Gets or sets the filter for the aggregated health state of the <see cref="System.Fabric.Health.DeployedApplicationHealthState" /> entries 
        /// in the collection. Represents a value obtained from members or bitwise combinations of members of <see cref="System.Fabric.Health.HealthStateFilter" />.</para>
        /// </summary>
        /// <value>
        /// <para>The filter for the aggregated health state of the <see cref="System.Fabric.Health.DeployedApplicationHealthState" /> entries in the collection.</para>
        /// </value>
        /// <remarks>This property is obsolete. Use <see cref="System.Fabric.Health.DeployedApplicationHealthStatesFilter.HealthStateFilterValue"/> instead.</remarks>
        [Obsolete("This property is obsolete. Use HealthStateFilterValue instead.")]
        public long HealthStateFilter
        {
            get { return (long)this.HealthStateFilterValue; }
            set { this.HealthStateFilterValue = (HealthStateFilter)value; }
        }

        /// <summary>
        /// Returns a string representation of the filter.
        /// </summary>
        /// <returns>A string representation of the <see cref="System.Fabric.Health.DeployedApplicationHealthStatesFilter"/>.</returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder("DeployedApplicationsFilter: ");
            sb.Append(string.Format(CultureInfo.CurrentCulture, "{0}", this.HealthStateFilterValue));
            return sb.ToString();
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeDeployedApplicationHealthStatesFilter = new NativeTypes.FABRIC_DEPLOYED_APPLICATION_HEALTH_STATES_FILTER();

            nativeDeployedApplicationHealthStatesFilter.HealthStateFilter = (UInt32)this.HealthStateFilterValue;

            return pinCollection.AddBlittable(nativeDeployedApplicationHealthStatesFilter);
        }
    }
}
