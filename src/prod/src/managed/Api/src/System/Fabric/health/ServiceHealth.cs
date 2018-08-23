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
    /// <para>Describes the health of a service as returned by <see cref="System.Fabric.FabricClient.HealthClient.GetServiceHealthAsync(Description.ServiceHealthQueryDescription)" />.</para>
    /// </summary>
    public sealed class ServiceHealth : EntityHealth
    {
        internal ServiceHealth()
        {
        }

        /// <summary>
        /// <para>Gets the service name which uniquely identifies the service health entity.</para>
        /// </summary>
        /// <value>
        /// <para>The service name which uniquely identifies the service health entity.</para>
        /// </value>
        [JsonCustomization(PropertyName = "Name")]
        public Uri ServiceName
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets partition health states for the current service.</para>
        /// </summary>
        /// <value>
        /// <para>The partition health states for the current service.</para>
        /// </value>
        /// <remarks>
        /// <para>Only partitions that respect the <see cref="System.Fabric.Description.ServiceHealthQueryDescription.PartitionsFilter"/> (if specified) are returned. 
        /// If the input filter is not specified, all partitions are returned.</para>
        /// <para>All partitions are evaluated to determine the service aggregated health.</para>
        /// </remarks>
        public IList<PartitionHealthState> PartitionHealthStates
        {
            get;
            internal set;
        }

        /// <summary>
        /// Gets the service health statistics, which contain information about how many partitions and replicas are in
        /// <see cref="System.Fabric.Health.HealthState.Ok"/>, <see cref="System.Fabric.Health.HealthState.Warning"/>, and <see cref="System.Fabric.Health.HealthState.Error"/> state.
        /// </summary>
        /// <value>The service health statistics.</value>
        /// <remarks>
        /// <para>
        /// The service health statistics contain information about how many partitions and replicas are in
        /// <see cref="System.Fabric.Health.HealthState.Ok"/>, <see cref="System.Fabric.Health.HealthState.Warning"/>, and <see cref="System.Fabric.Health.HealthState.Error"/> state.
        /// It can be null or empty if the query that returns the <see cref="System.Fabric.Health.ServiceHealth"/>
        /// specified <see cref="System.Fabric.Health.ServiceHealthStatisticsFilter"/> to exclude health statistics.
        /// </para>
        /// </remarks>
        [JsonCustomization(IsDefaultValueIgnored = true)]
        public HealthStatistics HealthStatistics
        {
            get;
            internal set;
        }

        /// <summary>
        /// Gets a string representation of the <see cref="System.Fabric.Health.ServiceHealth"/>.
        /// </summary>
        /// <returns>A string representation of the <see cref="System.Fabric.Health.ServiceHealth"/>.</returns>
        public override string ToString()
        {
            var sb = new StringBuilder();
            sb.Append(
                string.Format(
                    CultureInfo.CurrentCulture,
                    "ServiceHealth {0}: {1}, {2} partitions, {3} events",
                    this.ServiceName,
                    this.AggregatedHealthState,
                    this.PartitionHealthStates.Count,
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

        internal static unsafe ServiceHealth FromNativeResult(NativeClient.IFabricServiceHealthResult nativeResult)
        {
            var nativePtr = nativeResult.get_ServiceHealth();
            ReleaseAssert.AssertIf(nativePtr == IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "ServiceHealth"));

            var retval = FromNative((NativeTypes.FABRIC_SERVICE_HEALTH*)nativePtr);

            GC.KeepAlive(nativeResult);
            return retval;
        }

        private static unsafe ServiceHealth FromNative(NativeTypes.FABRIC_SERVICE_HEALTH* nativeHealth)
        {
            var managedHealth = new ServiceHealth();

            managedHealth.AggregatedHealthState = (HealthState)nativeHealth->AggregatedHealthState;
            managedHealth.HealthEvents = HealthEvent.FromNativeList(nativeHealth->HealthEvents);
            managedHealth.ServiceName = NativeTypes.FromNativeUri(nativeHealth->ServiceName);
            managedHealth.PartitionHealthStates = PartitionHealthState.FromNativeList(nativeHealth->PartitionHealthStates);

            if (nativeHealth->Reserved != IntPtr.Zero)
            {
                var nativeHealthEx1 = (NativeTypes.FABRIC_SERVICE_HEALTH_EX1*)nativeHealth->Reserved;
                managedHealth.UnhealthyEvaluations = HealthEvaluation.FromNativeList(nativeHealthEx1->UnhealthyEvaluations);

                if (nativeHealthEx1->Reserved != IntPtr.Zero)
                {
                    var nativeHealthEx2 = (NativeTypes.FABRIC_SERVICE_HEALTH_EX2*)nativeHealthEx1->Reserved;
                    managedHealth.HealthStatistics = HealthStatistics.CreateFromNative(nativeHealthEx2->HealthStatistics);
                }
            }

            return managedHealth;
        }
    }
}