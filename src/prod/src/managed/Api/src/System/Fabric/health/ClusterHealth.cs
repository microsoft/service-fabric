// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.Text;

    /// <summary>
    /// <para>Represents the health of the cluster, as returned by <see cref="System.Fabric.FabricClient.HealthClient.GetClusterHealthAsync(Description.ClusterHealthQueryDescription)" />.
    /// Contains the cluster aggregated health state, the application health states, the node health states as well as health events, health evaluation, and health statistics.</para>
    /// </summary>
    public sealed class ClusterHealth : EntityHealth
    {
        internal ClusterHealth()
        {
        }

        /// <summary>
        /// <para>Gets the cluster node health states as found in the health store.</para>
        /// </summary>
        /// <value>
        /// <para>The cluster nodes as found in the health store.</para>
        /// </value>
        /// <para>Only nodes that respect the <see cref="System.Fabric.Description.ClusterHealthQueryDescription.NodesFilter"/> (if specified) are returned. 
        /// If the input filter is not specified, all nodes are returned.</para>
        /// <para>All nodes are evaluated to determine the cluster aggregated health.</para>
        public IList<NodeHealthState> NodeHealthStates
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the cluster application health states as found in the health store.</para>
        /// </summary>
        /// <value>
        /// <para>The cluster applications as found in the health store.</para>
        /// </value>
        /// <para>Only applications that respect the <see cref="System.Fabric.Description.ClusterHealthQueryDescription.ApplicationsFilter"/> (if specified) are returned. 
        /// If the input filter is not specified, all applications are returned.</para>
        /// <para>All applications are evaluated to determine the cluster aggregated health.</para>
        public IList<ApplicationHealthState> ApplicationHealthStates
        {
            get;
            internal set;
        }

        /// <summary>
        /// Gets the cluster health statistics, which contain information about how many cluster entities are in
        /// <see cref="System.Fabric.Health.HealthState.Ok"/>, <see cref="System.Fabric.Health.HealthState.Warning"/>, and <see cref="System.Fabric.Health.HealthState.Error"/> state.
        /// </summary>
        /// <value>The cluster health statistics.</value>
        /// <remarks>
        /// <para>
        /// The cluster health statistics contain information about how many cluster entities are in
        /// <see cref="System.Fabric.Health.HealthState.Ok"/>, <see cref="System.Fabric.Health.HealthState.Warning"/>, and <see cref="System.Fabric.Health.HealthState.Error"/> state.
        /// It can be null or empty if the query that returns the <see cref="System.Fabric.Health.ClusterHealth"/>
        /// specified <see cref="System.Fabric.Health.ClusterHealthStatisticsFilter"/> to exclude health statistics.
        /// By default, the health query returns statistics that do not include statistics about the system application:
        /// the number of applications, services, partitions, replicas, deployed applications, and deployed service packages
        /// contain only user entities.
        /// </para>
        /// </remarks>
        [JsonCustomization(IsDefaultValueIgnored = true)]
        public HealthStatistics HealthStatistics
        {
            get;
            internal set;
        }

        /// <summary>
        /// Gets a string representation of the <see cref="System.Fabric.Health.ClusterHealth"/>.
        /// </summary>
        /// <returns>A string representation of the <see cref="System.Fabric.Health.ClusterHealth"/>.</returns>
        public override string ToString()
        {
            var sb = new StringBuilder();
            sb.Append(
                string.Format(
                    CultureInfo.CurrentCulture,
                    "ClusterHealth: {0}, {1} nodes, {2} applications, {3} events",
                    this.AggregatedHealthState,
                    this.ApplicationHealthStates.Count,
                    this.NodeHealthStates.Count,
                    this.HealthEvents.Count));

            if (this.HealthStatistics != null)
            {
                sb.Append(string.Format(CultureInfo.CurrentCulture, ", {0}", this.HealthStatistics));
            }
            
            if (this.UnhealthyEvaluations != null && this.UnhealthyEvaluations.Count > 0)
            {
                sb.Append(string.Format(CultureInfo.CurrentCulture, ", {0}", this.UnhealthyEvaluations));
            }

            return sb.ToString();
        }

        internal static unsafe ClusterHealth FromNativeResult(NativeClient.IFabricClusterHealthResult nativeResult)
        {
            var nativePtr = nativeResult.get_ClusterHealth();
            ReleaseAssert.AssertIf(nativePtr == IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "ClusterHealth"));

            var retval = FromNative((NativeTypes.FABRIC_CLUSTER_HEALTH*)nativePtr);

            GC.KeepAlive(nativeResult);
            return retval;
        }

        private static unsafe ClusterHealth FromNative(NativeTypes.FABRIC_CLUSTER_HEALTH* nativeClusterHealth)
        {
            var clusterHealth = new ClusterHealth();
            clusterHealth.AggregatedHealthState = (HealthState)nativeClusterHealth->AggregatedHealthState;

            if (nativeClusterHealth->Reserved != IntPtr.Zero)
            {
                NativeTypes.FABRIC_CLUSTER_HEALTH_EX1* nativeClusterHealthEx1 = (NativeTypes.FABRIC_CLUSTER_HEALTH_EX1*)nativeClusterHealth->Reserved;

                clusterHealth.NodeHealthStates = NodeHealthState.FromNativeList(nativeClusterHealthEx1->NodeHealthStates);
                clusterHealth.ApplicationHealthStates = ApplicationHealthState.FromNativeList(nativeClusterHealthEx1->ApplicationHealthStates);
                clusterHealth.HealthEvents = HealthEvent.FromNativeList(nativeClusterHealthEx1->HealthEvents);

                if (nativeClusterHealthEx1->Reserved != IntPtr.Zero)
                {
                    var nativeHealthEx2 = (NativeTypes.FABRIC_CLUSTER_HEALTH_EX2*)nativeClusterHealthEx1->Reserved;
                    clusterHealth.UnhealthyEvaluations = HealthEvaluation.FromNativeList(nativeHealthEx2->UnhealthyEvaluations);

                    if (nativeHealthEx2->Reserved != IntPtr.Zero)
                    {
                        var nativeHealthEx3 = (NativeTypes.FABRIC_CLUSTER_HEALTH_EX3*)nativeHealthEx2->Reserved;
                        clusterHealth.HealthStatistics = HealthStatistics.CreateFromNative(nativeHealthEx3->HealthStatistics);
                    }
                }
            }

            return clusterHealth;
        }
    }
}