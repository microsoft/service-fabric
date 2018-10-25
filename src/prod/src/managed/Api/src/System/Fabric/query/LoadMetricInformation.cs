// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Collections.Generic;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;
    using System.Text;

    /// <summary>
    /// <para>Represents the load metric information.</para>
    /// </summary>
    public sealed class LoadMetricInformation
    {
        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.Query.LoadMetricInformation"/>.</para>
        /// </summary>
        public LoadMetricInformation()
        {
        }

        /// <summary>
        /// <para>Gets the name of the metric.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the metric.</para>
        /// </value>
        public string Name { get; internal set; }

        /// <summary>
        /// <para>Gets a value that indicates whether the metrics is balanced or not before resource balancer run.</para>
        /// </summary>
        /// <value>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if the metrics is balanced or not before resource balancer run; 
        /// otherwise, <languageKeyword>false</languageKeyword>.</para>
        /// </value>
        public bool IsBalancedBefore { get; internal set; }

        /// <summary>
        /// <para>Gets a value that indicates whether the metrics is balanced or not after resource balancer run.</para>
        /// </summary>
        /// <value>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if the metrics is balanced or not after resource balancer run; 
        /// otherwise, <languageKeyword>false</languageKeyword>.</para>
        /// </value>
        public bool IsBalancedAfter { get; internal set; }

        /// <summary>
        /// <para>Gets the standard average deviation of the metrics before resource balancer run.</para>
        /// </summary>
        /// <value>
        /// <para>The standard average deviation of the metrics before resource balancer run.</para>
        /// </value>
        public double DeviationBefore { get; internal set; }

        /// <summary>
        /// <para>Gets the standard average deviation of the metrics after resource balancer run.</para>
        /// </summary>
        /// <value>
        /// <para>The standard average deviation of the metrics after resource balancer run.</para>
        /// </value>
        public double DeviationAfter { get; internal set; }

        /// <summary>
        /// <para>Gets he balancing threshold for a certain metric.</para>
        /// </summary>
        /// <value>
        /// <para>The balancing threshold for a certain metric.</para>
        /// </value>
        public double BalancingThreshold { get; internal set; }

        /// <summary>
        /// <para>Gets the current action being taken with regard to this metric. Examples could be placement, balancing, and constraint checks.</para>
        /// </summary>
        /// <value>
        /// <para>The current action being taken with regard to this metric.</para>
        /// </value>
        public string Action { get; internal set; }

        /// <summary>
        /// <para>Gets the Activity Threshold specified for this metric in the system Cluster Manifest.</para>
        /// </summary>
        /// <value>
        /// <para>The Activity Threshold specified for this metric in the system Cluster Manifest.</para>
        /// </value>
        public long ActivityThreshold { get; internal set; }

        /// <summary>
        /// <para>Gets the total cluster capacity for a given metric, determined by summing the capacity for the metric across all nodes.</para>
        /// </summary>
        /// <value>
        /// <para>The total cluster capacity for a given metric.</para>
        /// </value>
        public long ClusterCapacity { get; internal set; }

        /// <summary>
        /// <para>Gets the total cluster load for a specific metric within the cluster</para>
        /// <para>
        /// In future releases of Service Fabric this parameter will be deprecated in favor of
        /// <see cref="System.Fabric.Query.LoadMetricInformation.CurrentClusterLoad"/>.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The total cluster load.</para>
        /// </value>
        public long ClusterLoad { get; internal set; }

        /// <summary>
        /// <para>Gets the total cluster load for a specific metric within the cluster.</para>
        /// </summary>
        /// <value>
        /// <para>The total cluster load.</para>
        /// </value>
        public double CurrentClusterLoad { get; internal set; }

        /// <summary>
        /// <para>Gets remaining capacity for the metric in the cluster. Remaining capacity is defined as the current Cluster Capacity 
        /// minus the current Cluster Load.</para>
        /// <para>
        /// In future releases of Service Fabric this parameter will be deprecated in favor of
        /// <see cref="System.Fabric.Query.LoadMetricInformation.ClusterCapacityRemaining"/>.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The remaining capacity for the metric in the cluster.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.RemainingUnbufferedCapacity)]
        public long ClusterRemainingCapacity { get; internal set; }

        /// <summary>
        /// <para>Gets remaining capacity for the metric in the cluster. Remaining capacity is defined as the current Cluster Capacity 
        /// minus the current Cluster Load.</para>
        /// </summary>
        /// <value>
        /// <para>The remaining capacity for the metric in the cluster.</para>
        /// </value>
        public double ClusterCapacityRemaining { get; internal set; }

        /// <summary>
        /// <para>Gets whether the metric is currently over capacity in the cluster.</para>
        /// </summary>
        /// <value>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if the metric is currently over capacity in the cluster; 
        /// otherwise, <languageKeyword>false</languageKeyword>.</para>
        /// </value>
        public bool IsClusterCapacityViolation { get; internal set; }

        /// <summary>
        /// <para>
        /// Gets the reserved percentage of total node capacity for this metric
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The reserved percentage of total node capacity for this metric</para>
        /// </value>
        public double NodeBufferPercentage { get; internal set; }

        /// <summary>
        /// <para>
        /// The reserved percentage of cluster total capacity for this metric
        /// </para>
        /// <para>
        /// In future releases of Service Fabric this parameter will be deprecated in favor of
        /// <see cref="System.Fabric.Query.LoadMetricInformation.BufferedClusterCapacityRemaining"/>.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>ClusterBufferedCapacity property gets/sets the value of the long field, _clusterBufferedCapacity.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.BufferedCapacity)]
        public long ClusterBufferedCapacity { get; internal set; }

        /// <summary>
        /// <para>
        /// The reserved percentage of cluster total capacity for this metric
        /// </para>
        /// </summary>
        /// <value>
        /// <para>Remaining capacity in the cluster excluding the reserved space.</para>
        /// </value>
        public double BufferedClusterCapacityRemaining { get; internal set; }

        /// <summary>
        /// <para>
        /// Gets the remaining percentage of cluster total capacity for this metric
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The remaining percentage of cluster total capacity for this metric.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.RemainingBufferedCapacity)]
        public long ClusterRemainingBufferedCapacity { get; internal set; }

        /// <summary>
        /// <para>
        /// Gets the minimum load on any node for this metric.
        /// </para>
        /// <para>
        /// In future releases of Service Fabric this parameter will be deprecated in favor of
        /// <see cref="System.Fabric.Query.LoadMetricInformation.MinimumNodeLoad"/>.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The minimum load on any node for this metric.</para>
        /// </value>
        public long MinNodeLoadValue { get; internal set; }

        /// <summary>
        /// <para>
        /// Gets the minimum load on any node for this metric.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The minimum load on any node for this metric.</para>
        /// </value>
        public double MinimumNodeLoad { get; internal set; }

        /// <summary>
        /// <para>
        /// Gets the node id of the node with the minimum load for this metric
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The node id of the node with the minimum load for this metric.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.MinNodeLoadId)]
        public NodeId MinNodeLoadNodeId { get; internal set; }

        /// <summary>
        /// <para>
        /// Gets the maximum load on any node for this metric.
        /// </para>
        /// <para>
        /// In future releases of Service Fabric this parameter will be deprecated in favor of
        /// <see cref="System.Fabric.Query.LoadMetricInformation.MaximumNodeLoad"/>.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The maximum load on any node for this metric.</para>
        /// </value>
        public long MaxNodeLoadValue { get; internal set; }

        /// <summary>
        /// <para>
        /// Gets the maximum load on any node for this metric.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The maximum load on any node for this metric.</para>
        /// </value>
        public double MaximumNodeLoad { get; internal set; }

        /// <summary>
        /// <para>
        /// Gets the node id of the node with the maximum load for this metric
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The node id of the node with the maximum load for this metric.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.MaxNodeLoadId)]
        public NodeId MaxNodeLoadNodeId { get; internal set; }

        internal static unsafe LoadMetricInformation CreateFromNative(NativeTypes.FABRIC_LOAD_METRIC_INFORMATION nativeResultItem)
        {
            long activityThreshold = 0;
            long clusterCapacity = 0;
            long clusterLoad = 0;
            long clusterRemainingCapacity = 0;
            double nodeBufferPercentage = 0;
            long clusterBufferedCapacity = 0;
            long clusterRemainingBufferedCapacity = 0;
            bool isClusterCapacityViolation = false;
            long minNodeLoadValue = -1;
            NodeId minNodeLoadNodeId = null;
            long maxNodeLoadValue = -1;
            NodeId maxNodeLoadNodeId = null;
            double currentClusterLoad = 0;
            double bufferedClusterCapacityRemaining = 0;
            double clusterCapacityRemaining = 0;
            double maximumNodeLoad = -1;
            double minimumNodeLoad = -1;

            if (nativeResultItem.Reserved != IntPtr.Zero)
            {
                NativeTypes.FABRIC_LOAD_METRIC_INFORMATION_EX1* ext1 = (NativeTypes.FABRIC_LOAD_METRIC_INFORMATION_EX1*)nativeResultItem.Reserved;
                activityThreshold = ext1->ActivityThreshold;
                clusterCapacity = ext1->ClusterCapacity;
                clusterLoad = ext1->ClusterLoad;

                // calculate remaining capacity and isCapacityViolation flag
                if (clusterCapacity != 0)
                {
                    if (clusterCapacity < clusterLoad)
                    {
                        isClusterCapacityViolation = true;
                        clusterRemainingCapacity = 0;
                    }
                    else
                    {
                        clusterRemainingCapacity = clusterCapacity - clusterLoad;
                    }
                }

                if (ext1->Reserved != IntPtr.Zero)
                {
                    NativeTypes.FABRIC_LOAD_METRIC_INFORMATION_EX2* ext2 = (NativeTypes.FABRIC_LOAD_METRIC_INFORMATION_EX2*)ext1->Reserved;

                    clusterRemainingCapacity = ext2->RemainingUnbufferedCapacity;
                    nodeBufferPercentage = ext2->NodeBufferPercentage;
                    clusterBufferedCapacity = ext2->BufferedCapacity;
                    clusterRemainingBufferedCapacity = ext2->RemainingBufferedCapacity;
                    isClusterCapacityViolation = NativeTypes.FromBOOLEAN(ext2->IsClusterCapacityViolation);
                    minNodeLoadValue = ext2->MinNodeLoadValue;
                    minNodeLoadNodeId = NativeTypes.FromNativeNodeId(ext2->MinNodeLoadNodeId);
                    maxNodeLoadValue = ext2->MaxNodeLoadValue;
                    maxNodeLoadNodeId = NativeTypes.FromNativeNodeId(ext2->MaxNodeLoadNodeId);

                    if (ext2->Reserved != IntPtr.Zero)
                    {
                        NativeTypes.FABRIC_LOAD_METRIC_INFORMATION_EX3* ext3 = (NativeTypes.FABRIC_LOAD_METRIC_INFORMATION_EX3*)ext2->Reserved;
                        currentClusterLoad = ext3->CurrentClusterLoad;
                        bufferedClusterCapacityRemaining = ext3->BufferedClusterCapacityRemaining;
                        clusterCapacityRemaining = ext3->ClusterCapacityRemaining;
                        maximumNodeLoad = ext3->MaximumNodeLoad;
                        minimumNodeLoad = ext3->MinimumNodeLoad;
                    }
                }
            }

            return new LoadMetricInformation
            {
                Name = NativeTypes.FromNativeString(nativeResultItem.Name),
                IsBalancedBefore = NativeTypes.FromBOOLEAN(nativeResultItem.IsBalancedBefore),
                IsBalancedAfter = NativeTypes.FromBOOLEAN(nativeResultItem.IsBalancedAfter),
                DeviationBefore = nativeResultItem.DeviationBefore,
                DeviationAfter = nativeResultItem.DeviationAfter,
                BalancingThreshold = nativeResultItem.BalancingThreshold,
                Action = NativeTypes.FromNativeString(nativeResultItem.Action),
                ActivityThreshold = activityThreshold,
                ClusterCapacity = clusterCapacity,
                ClusterLoad = clusterLoad,
                ClusterRemainingCapacity = clusterRemainingCapacity,
                NodeBufferPercentage = nodeBufferPercentage,
                ClusterBufferedCapacity = clusterBufferedCapacity,
                ClusterRemainingBufferedCapacity = clusterRemainingBufferedCapacity,
                IsClusterCapacityViolation = isClusterCapacityViolation,
                MinNodeLoadValue = minNodeLoadValue,
                MinNodeLoadNodeId = minNodeLoadNodeId,
                MaxNodeLoadValue = maxNodeLoadValue,
                MaxNodeLoadNodeId = maxNodeLoadNodeId,
                CurrentClusterLoad = currentClusterLoad,
                BufferedClusterCapacityRemaining = bufferedClusterCapacityRemaining,
                ClusterCapacityRemaining = clusterCapacityRemaining,
                MaximumNodeLoad = maximumNodeLoad,
                MinimumNodeLoad = minimumNodeLoad
        };
        }

        internal static unsafe IList<LoadMetricInformation> CreateFromNativeList(NativeTypes.FABRIC_LOAD_METRIC_INFORMATION_LIST* nativeResultList)
        {
            var rv = new List<LoadMetricInformation>();

            var nativeArray = (NativeTypes.FABRIC_LOAD_METRIC_INFORMATION*)nativeResultList->Items;
            for (int i = 0; i < nativeResultList->Count; i++)
            {
                var nativeItem = *(nativeArray + i);
                rv.Add(LoadMetricInformation.CreateFromNative(nativeItem));
            }

            return rv;
        }

        /// <summary>
        /// <para>
        /// Pretty print out each field of <see cref="System.Fabric.Query.LoadMetricInformation" />.
        /// </para>
        /// </summary>
        /// <returns>
        /// <para>A string representing the load metric information.</para>
        /// </returns>
        /// <example>
        ///     LoadMetricName        : Metric1
        ///     IsBalancedBefore      : True
        ///     IsBalancedAfter       : True
        ///     DeviationBefore       : 2
        ///     DeviationAfter        : 2
        ///     BalancingThreshold    : 1
        ///     Action    : NoActionNeeded
        ///     ActivityThreshold     : 3
        ///     ClusterCapacity       : 100
        ///     CurrentClusterLoad    : 1.0
        ///     ClusterCapacityRemaining : 0.0
        ///     NodeBufferPercentage  : 0
        ///     ClusterBufferedCapacity : 0
        ///     BufferedClusterCapacityRemaining : 0.0
        ///     ClusterCapacityViolation : True
        ///     MaximumNodeLoad       : 1.0
        ///     MinNodeLoadNodeId     : 1ca9521d70301383417536df0c96f671
        ///     MinimumNodeLoad       : 0.0
        ///     MaxNodeLoadNodeId     : cf68563e16a44f808e86197a9cf83de5
        /// </example>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();

            sb.AppendFormat("Name: {0}\n", Name);
            sb.AppendFormat("IsBalancedBefore: {0}\n", IsBalancedBefore);
            sb.AppendFormat("IsBalancedAfter: {0}\n", IsBalancedAfter);
            sb.AppendFormat("DeviationBefore: {0}\n", DeviationBefore);
            sb.AppendFormat("DeviationAfter: {0}\n", DeviationAfter);
            sb.AppendFormat("BalancingThreshold: {0}\n", BalancingThreshold);
            sb.AppendFormat("Action: {0}\n", Action);
            sb.AppendFormat("ActivityThreshold: {0}\n", ActivityThreshold);
            sb.AppendFormat("ClusterCapacity: {0}\n", ClusterCapacity);
            sb.AppendFormat("CurrentClusterLoad: {0}\n", CurrentClusterLoad);
            sb.AppendFormat("ClusterCapacityRemaining: {0}\n", ClusterCapacityRemaining);
            sb.AppendFormat("NodeBufferPercentage: {0}\n", NodeBufferPercentage);
            sb.AppendFormat("ClusterBufferedCapacity: {0}\n", ClusterBufferedCapacity);
            sb.AppendFormat("BufferedClusterCapacityRemaining: {0}\n", BufferedClusterCapacityRemaining);
            sb.AppendFormat("IsClusterCapacityViolation: {0}\n", IsClusterCapacityViolation);
            sb.AppendFormat("MinimumNodeLoad: {0}\n", MinimumNodeLoad);
            sb.AppendFormat("MinNodeLoadNodeId: {0}\n", MinNodeLoadNodeId);
            sb.AppendFormat("MaximumNodeLoad: {0}\n", MaximumNodeLoad);
            sb.AppendFormat("MaxNodeLoadNodeId: {0}\n", MaxNodeLoadNodeId);

            return sb.ToString();
        }
    }
}