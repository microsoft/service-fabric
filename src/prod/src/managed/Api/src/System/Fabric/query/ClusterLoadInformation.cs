// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Interop;
    using System.Collections.Generic;
    using System.Fabric.Common.Serialization;
    using System.Text;

    /// <summary>
    /// <para>Represents the cluster load information.</para>
    /// </summary>
    public class ClusterLoadInformation
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Query.ClusterLoadInformation" /> class.</para>
        /// </summary>
        public ClusterLoadInformation()
        {
        }

        /// <summary>
        /// <para>Gets the starting time (in UTC) of last resource balancing run.</para>
        /// </summary>
        /// <value>
        /// <para>The starting time of last resource balancing run.</para>
        /// </value>
        public DateTime LastBalancingStartTimeUtc { get; internal set; }

        /// <summary>
        /// <para>Gets the end time (in UTC) of last resource balancing run.</para>
        /// </summary>
        /// <value>
        /// <para>The end time of last resource balancing run.</para>
        /// </value>
        public DateTime LastBalancingEndTimeUtc { get; internal set; }

        /// <summary>
        /// <para>Gets a list of load metrics information object. Each entry is for a certain metrics.</para>
        /// </summary>
        /// <value>
        /// <para>A list of load metrics information object.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.LoadMetricInformation)]
        public IList<LoadMetricInformation> LoadMetricInformationList { get; internal set; }

        internal static unsafe ClusterLoadInformation CreateFromNative(
            NativeClient.IFabricGetClusterLoadInformationResult nativeLoadInformation)
        {
            var retval = CreateFromNative(*(NativeTypes.FABRIC_CLUSTER_LOAD_INFORMATION*)nativeLoadInformation.get_ClusterLoadInformation());
            GC.KeepAlive(nativeLoadInformation);

            return retval;
        }

        internal static unsafe ClusterLoadInformation CreateFromNative(
            NativeTypes.FABRIC_CLUSTER_LOAD_INFORMATION nativeLoadInformation)
        {
            IList<LoadMetricInformation> loadMetricInformationList;

            if (nativeLoadInformation.LoadMetricInformationList == IntPtr.Zero)
            {
                loadMetricInformationList = new List<LoadMetricInformation>();
            }
            else
            {
                loadMetricInformationList = LoadMetricInformation.CreateFromNativeList(
                    (NativeTypes.FABRIC_LOAD_METRIC_INFORMATION_LIST*)nativeLoadInformation.LoadMetricInformationList);
            }

            return new ClusterLoadInformation
            {
                LastBalancingStartTimeUtc = NativeTypes.FromNativeFILETIME(nativeLoadInformation.LastBalancingStartTimeUtc),
                LastBalancingEndTimeUtc = NativeTypes.FromNativeFILETIME(nativeLoadInformation.LastBalancingEndTimeUtc),
                LoadMetricInformationList = loadMetricInformationList
            };
        }

        /// <summary>
        /// <para>
        /// Pretty print out details of <see cref="System.Fabric.Query.ClusterLoadInformation" />.
        /// </para>
        /// </summary>
        /// <returns>A string representation of the <see cref="System.Fabric.Query.ClusterLoadInformation"/>.</returns>
        /// <example>
        /// LastBalancingStartTimeUtc : 11/9/2015 8:40:35 PM
        /// LastBalancingEndTimeUtc   : 11/9/2015 8:40:35 PM
        /// LoadMetricInformation     :
        ///     LoadMetricName        : Metric1
        ///     IsBalancedBefore      : True
        ///     IsBalancedAfter       : True
        ///     DeviationBefore       : 2
        ///     DeviationAfter        : 2
        ///     BalancingThreshold    : 1
        ///     Action    : NoActionNeeded
        ///     ActivityThreshold     : 3
        ///     ClusterCapacity       : 100
        ///     ClusterLoad           : 1
        ///     ClusterRemainingCapacity : 0
        ///     NodeBufferPercentage  : 0
        ///     ClusterBufferedCapacity : 0
        ///     ClusterRemainingBufferedCapacity : 0
        ///     ClusterCapacityViolation : True
        ///     MinNodeLoadValue      : 0
        ///     MinNodeLoadNodeId     : 1ca9521d70301383417536df0c96f671
        ///     MaxNodeLoadValue      : 1
        ///     MaxNodeLoadNodeId     : cf68563e16a44f808e86197a9cf83de5
        /// </example>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();
            sb.AppendFormat("LastBalancingStartTimeUtc: {0}\n", LastBalancingStartTimeUtc);
            sb.AppendFormat("LastBalancingEndTimeUtc: {0}\n", LastBalancingEndTimeUtc);
            foreach (LoadMetricInformation loadMetricInfo in LoadMetricInformationList)
            {
                sb.AppendFormat("{0}\n", loadMetricInfo.ToString());
            }

            return sb.ToString();
        }
    }
}