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
    /// <remarks>The filter can be used in <see cref="System.Fabric.Description.PartitionHealthQueryDescription"/>
    /// to specify whether the health statistics should be returned as part of <see cref="System.Fabric.Health.PartitionHealth"/>.</remarks>
    public sealed class PartitionHealthStatisticsFilter
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Health.PartitionHealthStatisticsFilter" /> class.</para>
        /// </summary>
        public PartitionHealthStatisticsFilter()
        {
        }

        /// <summary>
        /// Gets or sets a flag that indicates whether the health statistics should be included in query result.
        /// </summary>
        /// <value>A flag that indicates whether the health statistics should be included in query result.</value>
        /// <remarks>
        /// <para>
        /// If ExcludeHealthStatistics is set to <languageKeyword>true</languageKeyword>, the health statistics are not returned as part of the query result.
        /// Otherwise, the query result includes the partition health statistics, which contains information about how many replicas are in
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
        /// Returns a string representation of the filter.
        /// </summary>
        /// <returns>A string representation of the filter.</returns>
        public override string ToString()
        {
            return string.Format(CultureInfo.CurrentCulture, "ExcludeHealthStatistics: {0}", this.ExcludeHealthStatistics);
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeFilter = new NativeTypes.FABRIC_PARTITION_HEALTH_STATISTICS_FILTER();
            nativeFilter.ExcludeHealthStatistics = NativeTypes.ToBOOLEAN(this.ExcludeHealthStatistics);
            return pinCollection.AddBlittable(nativeFilter);
        }
    }
}
