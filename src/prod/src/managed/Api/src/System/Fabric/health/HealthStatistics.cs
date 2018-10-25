// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using Runtime.InteropServices;
    using System;
    using System.Collections.Generic;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.Text;

    /// <summary>
    /// <para>
    /// Represents health statistics for an entity.
    /// It is returned as part of health queries results when the query descriptions
    /// are configured to include statistics.
    /// The statistics include health state counts for all children types of the current entity.
    /// For example, for cluster, the health statistics include health state counts for
    /// nodes, applications, services, partitions, replicas, deployed applications and deployed service packages.
    /// For partition, the health statistics include health counts for replicas.
    /// </para>
    /// </summary>
    public sealed class HealthStatistics
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Health.HealthStatistics" /> class.</para>
        /// </summary>
        internal HealthStatistics()
        {
            this.HealthStateCountList = new List<EntityKindHealthStateCount>();
        }

        /// <summary>
        /// Gets a list of <see cref="HealthStateCount"/> information for different entity kinds.
        /// </summary>
        /// <value>A list with <see cref="HealthStateCount"/> information for different entity kinds.</value>
        /// <remarks>
        /// <para>
        /// The list keeps track of how many children of the queried entity are in
        /// <see cref="System.Fabric.Health.HealthState.Ok"/>, <see cref="System.Fabric.Health.HealthState.Warning"/>, and <see cref="System.Fabric.Health.HealthState.Error"/> state.
        /// </para>
        /// </remarks>
        public IList<EntityKindHealthStateCount> HealthStateCountList
        {
            get;
            internal set;
        }

        /// <summary>
        /// Returns a string representation of the health state count.
        /// </summary>
        /// <returns>A string representation of the health state count.</returns>
        public override string ToString()
        {
            if (this.HealthStateCountList == null)
            {
                return string.Format(CultureInfo.CurrentCulture, "HealthStatistics: null");
            }
            
            var sb = new StringBuilder(string.Format(CultureInfo.CurrentCulture, "HealthStatistics: ["));
            bool isFirst = true;
            foreach (var entry in this.HealthStateCountList)
            {
                if (isFirst)
                {
                    isFirst = false;
                }
                else
                {
                    sb.Append(string.Format(CultureInfo.CurrentCulture, ", "));
                }

                sb.Append(string.Format(CultureInfo.CurrentCulture, "{0}", entry));
            }

            sb.Append(" ]");

            return sb.ToString();
        }

        internal static unsafe HealthStatistics CreateFromNative(IntPtr nativeHealthStatisticsPtr)
        {
            if (nativeHealthStatisticsPtr == IntPtr.Zero)
            {
                return null;
            }

            var nativeHealthStatistics = (NativeTypes.FABRIC_HEALTH_STATISTICS*)nativeHealthStatisticsPtr;
            var healthStatistics = new HealthStatistics();

            var nativeItemArray = (NativeTypes.FABRIC_ENTITY_KIND_HEALTH_STATE_COUNT*)nativeHealthStatistics->Items;

            for (int i = 0; i < nativeHealthStatistics->Count; i++)
            {
                var nativeItem = *(nativeItemArray + i);
                var item = EntityKindHealthStateCount.FromNative(nativeItem);
                if (item != null)
                {
                    healthStatistics.HealthStateCountList.Add(item);
                }
            }

            return healthStatistics;
        }
    }
}
