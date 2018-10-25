// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System;
    using System.Fabric.Interop;
    using System.Globalization;

    /// <summary>
    /// <para>Represents the filter for <see cref="System.Fabric.Health.HealthStatistics" />.</para>
    /// </summary>
    /// <remarks>The filter can be used in <see cref="System.Fabric.Description.ClusterHealthQueryDescription"/>
    /// to specify whether the health statistics should be returned as part of <see cref="System.Fabric.Health.ClusterHealth"/>.</remarks>
    public sealed class ClusterHealthStatisticsFilter
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Health.ClusterHealthStatisticsFilter" /> class.</para>
        /// </summary>
        public ClusterHealthStatisticsFilter()
        {
        }

        /// <summary>
        /// Gets or sets a flag that indicates whether the health statistics should be included in query result.
        /// </summary>
        /// <value>A flag that indicates whether the health statistics should be included in query result.</value>
        /// <remarks>
        /// <para>
        /// If ExcludeHealthStatistics is set to <languageKeyword>true</languageKeyword>, the health statistics are not returned as part of the query result.
        /// Otherwise, the query result includes the cluster health statistics.
        /// The statistics shows the number of entities in the cluster per entity type, with count for 
        /// <see cref="System.Fabric.Health.HealthState.Ok"/>, <see cref="System.Fabric.Health.HealthState.Warning"/>, and <see cref="System.Fabric.Health.HealthState.Error"/> state.
        /// Defaults to <languageKeyword>false</languageKeyword>.
        /// </para>
        /// </remarks>
        public bool ExcludeHealthStatistics
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets a flag that indicates whether the health statistics should include information for the fabric:/System application.
        /// </summary>
        /// <value>A flag that indicates whether the health statistics should include information for the fabric:/System application.</value>
        /// <remarks>
        /// <para>
        /// If IncludeSystemApplicationHealthStatistics is set to <languageKeyword>true</languageKeyword>, the health statistics 
        /// include the entities that belong to the fabric:/System application.
        /// Otherwise, the query result includes health statistics only for user applications.
        /// Defaults to <languageKeyword>false</languageKeyword>.
        /// </para>
        /// <para>In order to apply IncludeSystemApplicationHealthStatistics,
        /// ExcludeHealthStatistics must be set to <languageKeyword>false</languageKeyword> (default value).</para>
        /// </remarks>
        public bool IncludeSystemApplicationHealthStatistics
        {
            get;
            set;
        }

        /// <summary>
        /// Returns a string representation of the filter.
        /// </summary>
        /// <returns>A string representation of the filter.</returns>
        public override string ToString()
        {
            return string.Format(
                CultureInfo.CurrentCulture, 
                "ExcludeHealthStatistics: {0}, IncludeSystemApplicationHealthStatistics: {1}",
                this.ExcludeHealthStatistics,
                this.IncludeSystemApplicationHealthStatistics);
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeFilter = new NativeTypes.FABRIC_CLUSTER_HEALTH_STATISTICS_FILTER();
            nativeFilter.ExcludeHealthStatistics = NativeTypes.ToBOOLEAN(this.ExcludeHealthStatistics);
            nativeFilter.IncludeSystemApplicationHealthStatistics = NativeTypes.ToBOOLEAN(this.IncludeSystemApplicationHealthStatistics);
            return pinCollection.AddBlittable(nativeFilter);
        }
    }
}
