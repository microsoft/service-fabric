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
    /// <para>Describes the health of an application as returned by
    /// <see cref="System.Fabric.FabricClient.HealthClient.GetApplicationHealthAsync(Description.ApplicationHealthQueryDescription)" />.</para>
    /// </summary>
    public sealed class ApplicationHealth : EntityHealth
    {
        internal ApplicationHealth()
        {
        }

        /// <summary>
        /// <para>Gets the application name, which uniquely identifies the application . </para>
        /// </summary>
        /// <value>
        /// <para>The application name.</para>
        /// </value>
        [JsonCustomization(PropertyName = "Name")]
        public Uri ApplicationName
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the service health states for the current application as found in the health store.</para>
        /// </summary>
        /// <value>
        /// <para>The services of the current application as found in the health store.</para>
        /// </value>
        /// <para>Only services that respect the <see cref="System.Fabric.Description.ApplicationHealthQueryDescription.ServicesFilter"/> (if specified) are returned. 
        /// If the input filter is not specified, all services are returned.</para>
        /// <para>All services are evaluated to determine the application aggregated health.</para>
        public IList<ServiceHealthState> ServiceHealthStates
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the deployed application health states for the current application as found in the health store.</para>
        /// </summary>
        /// <value>
        /// <para>deployed applications for the current application as found in the health store.</para>
        /// </value>
        /// <para>Only deployed applications that respect the <see cref="System.Fabric.Description.ApplicationHealthQueryDescription.DeployedApplicationsFilter"/> (if specified) are returned. 
        /// If the input filter is not specified, all deployed applications are returned.</para>
        /// <para>All deployed applications are evaluated to determine the application aggregated health.</para>
        public IList<DeployedApplicationHealthState> DeployedApplicationHealthStates
        {
            get;
            internal set;
        }

        /// <summary>
        /// Gets the application health statistics, which contain information about how many entities of the application are in
        /// <see cref="System.Fabric.Health.HealthState.Ok"/>, <see cref="System.Fabric.Health.HealthState.Warning"/>, and <see cref="System.Fabric.Health.HealthState.Error"/> state.
        /// </summary>
        /// <value>The application health statistics.</value>
        /// <remarks>
        /// <para>
        /// The application health statistics contain information about how many services, partitions, replicas, deployed applications, and deployed service packages are in
        /// <see cref="System.Fabric.Health.HealthState.Ok"/>, <see cref="System.Fabric.Health.HealthState.Warning"/>, and <see cref="System.Fabric.Health.HealthState.Error"/> state.
        /// It can be null or empty if the query that returns the <see cref="System.Fabric.Health.ApplicationHealth"/>
        /// specified <see cref="System.Fabric.Health.ApplicationHealthStatisticsFilter"/> to exclude health statistics.
        /// </para>
        /// </remarks>
        [JsonCustomization(IsDefaultValueIgnored = true)]
        public HealthStatistics HealthStatistics
        {
            get;
            internal set;
        }

        /// <summary>
        /// Gets a string representation of the <see cref="System.Fabric.Health.ApplicationHealth"/>.
        /// </summary>
        /// <returns>A string representation of the <see cref="System.Fabric.Health.ApplicationHealth"/>.</returns>
        public override string ToString()
        {
            var sb = new StringBuilder();
            sb.Append(
                string.Format(
                    CultureInfo.CurrentCulture,
                    "ApplicationHealth {0}: {1}, {2} services, {3} deployed applications, {4} events",
                    this.ApplicationName,
                    this.AggregatedHealthState,
                    this.ServiceHealthStates.Count,
                    this.DeployedApplicationHealthStates.Count,
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
        
        internal static unsafe ApplicationHealth FromNativeResult(NativeClient.IFabricApplicationHealthResult nativeResult)
        {
            var nativePtr = nativeResult.get_ApplicationHealth();
            ReleaseAssert.AssertIf(nativePtr == IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "ApplicationHealth"));

            var retval = FromNative((NativeTypes.FABRIC_APPLICATION_HEALTH*)nativePtr);

            GC.KeepAlive(nativeResult);
            return retval;
        }

        private static unsafe ApplicationHealth FromNative(NativeTypes.FABRIC_APPLICATION_HEALTH* nativeHealth)
        {
            var managedHealth = new ApplicationHealth();

            managedHealth.AggregatedHealthState = (HealthState)nativeHealth->AggregatedHealthState;
            managedHealth.HealthEvents = HealthEvent.FromNativeList(nativeHealth->HealthEvents);
            managedHealth.ApplicationName = NativeTypes.FromNativeUri(nativeHealth->ApplicationName);
            managedHealth.ServiceHealthStates = ServiceHealthState.FromNativeList(nativeHealth->ServiceHealthStates);
            managedHealth.DeployedApplicationHealthStates = DeployedApplicationHealthState.FromNativeList(nativeHealth->DeployedApplicationHealthStates);

            if (nativeHealth->Reserved != IntPtr.Zero)
            {
                var nativeHealthEx1 = (NativeTypes.FABRIC_APPLICATION_HEALTH_EX1*)nativeHealth->Reserved;
                managedHealth.UnhealthyEvaluations = HealthEvaluation.FromNativeList(nativeHealthEx1->UnhealthyEvaluations);

                if (nativeHealthEx1->Reserved != IntPtr.Zero)
                {
                    var nativeHealthEx2 = (NativeTypes.FABRIC_APPLICATION_HEALTH_EX2*)nativeHealthEx1->Reserved;
                    managedHealth.HealthStatistics = HealthStatistics.CreateFromNative(nativeHealthEx2->HealthStatistics);
                }
            }

            return managedHealth;
        }
    }
}