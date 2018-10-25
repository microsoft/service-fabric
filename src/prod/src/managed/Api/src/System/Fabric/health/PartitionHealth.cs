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
    /// <para>Describes health of a partition as returned by <see cref="System.Fabric.FabricClient.HealthClient.GetPartitionHealthAsync(Description.PartitionHealthQueryDescription)" />.</para>
    /// </summary>
    public sealed class PartitionHealth : EntityHealth
    {
        internal PartitionHealth()
        {
        }

        /// <summary>
        /// <para>Gets the partition ID, which uniquely identifies the partition.</para>
        /// </summary>
        /// <value>
        /// <para>The partition ID.</para>
        /// </value>
        public Guid PartitionId
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the replica health states for the current partition as found in the health store.</para>
        /// </summary>
        /// <value>
        /// <para>The replicas of the current partition as found in the health store.</para>
        /// </value>
        /// <para>Only replicas that respect the <see cref="System.Fabric.Description.PartitionHealthQueryDescription.ReplicasFilter"/> (if specified) are returned. 
        /// If the input filter is not specified, all replicas are returned.</para>
        /// <para>All replicas are evaluated to determine the partition aggregated health.</para>
        public IList<ReplicaHealthState> ReplicaHealthStates
        {
            get;
            internal set;
        }

        /// <summary>
        /// Gets the partition health statistics, which contain information about how many replicas are in
        /// <see cref="System.Fabric.Health.HealthState.Ok"/>, <see cref="System.Fabric.Health.HealthState.Warning"/>, and <see cref="System.Fabric.Health.HealthState.Error"/> state.
        /// </summary>
        /// <value>The partition health statistics.</value>
        /// <remarks>
        /// <para>
        /// The partition health statistics contain information about how many replicas are in
        /// <see cref="System.Fabric.Health.HealthState.Ok"/>, <see cref="System.Fabric.Health.HealthState.Warning"/>, and <see cref="System.Fabric.Health.HealthState.Error"/> state.
        /// It can be null or empty if the query that returns the <see cref="System.Fabric.Health.PartitionHealth"/>
        /// specified <see cref="System.Fabric.Health.PartitionHealthStatisticsFilter"/> to exclude health statistics.
        /// </para>
        /// </remarks>
        [JsonCustomization(IsDefaultValueIgnored = true)]
        public HealthStatistics HealthStatistics
        {
            get;
            internal set;
        }

        /// <summary>
        /// Gets a string representation of the <see cref="System.Fabric.Health.PartitionHealth"/>.
        /// </summary>
        /// <returns>A string representation of the <see cref="System.Fabric.Health.PartitionHealth"/>.</returns>
        public override string ToString()
        {
            var sb = new StringBuilder();
            sb.Append(
                string.Format(
                    CultureInfo.CurrentCulture,
                    "PartitionHealth {0}: {1}, {2} replicas, {3} events",
                    this.PartitionId,
                    this.AggregatedHealthState,
                    this.ReplicaHealthStates.Count,
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

        internal static unsafe PartitionHealth FromNativeResult(NativeClient.IFabricPartitionHealthResult nativeResult)
        {
            var nativePtr = nativeResult.get_PartitionHealth();
            ReleaseAssert.AssertIf(nativePtr == IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "PartitionHealth"));

            var retval = FromNative((NativeTypes.FABRIC_PARTITION_HEALTH*)nativePtr);

            GC.KeepAlive(nativeResult);
            return retval;
        }

        private static unsafe PartitionHealth FromNative(NativeTypes.FABRIC_PARTITION_HEALTH* nativeHealth)
        {
            var managedHealth = new PartitionHealth();

            managedHealth.AggregatedHealthState = (HealthState)nativeHealth->AggregatedHealthState;
            managedHealth.HealthEvents = HealthEvent.FromNativeList(nativeHealth->HealthEvents);
            managedHealth.PartitionId = nativeHealth->PartitionId;
            managedHealth.ReplicaHealthStates = ReplicaHealthState.FromNativeList(nativeHealth->ReplicaHealthStates);

            if (nativeHealth->Reserved != IntPtr.Zero)
            {
                var nativeHealthEx1 = (NativeTypes.FABRIC_PARTITION_HEALTH_EX1*)nativeHealth->Reserved;
                managedHealth.UnhealthyEvaluations = HealthEvaluation.FromNativeList(nativeHealthEx1->UnhealthyEvaluations);

                if (nativeHealthEx1->Reserved != IntPtr.Zero)
                {
                    var nativeHealthEx2 = (NativeTypes.FABRIC_PARTITION_HEALTH_EX2*)nativeHealthEx1->Reserved;
                    managedHealth.HealthStatistics = HealthStatistics.CreateFromNative(nativeHealthEx2->HealthStatistics);
                }
            }

            return managedHealth;
        }
    }
}