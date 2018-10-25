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
    /// <para>Describes the health of an application deployed on a node as returned by <see cref="System.Fabric.FabricClient.HealthClient.GetDeployedApplicationHealthAsync(Description.DeployedApplicationHealthQueryDescription)" />.</para>
    /// </summary>
    public sealed class DeployedApplicationHealth : EntityHealth
    {
        internal DeployedApplicationHealth()
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
        /// <para>Gets the node name for the node where the application is deployed.</para>
        /// </summary>
        /// <value>
        /// <para>The node name for the node where the application is deployed.</para>
        /// </value>
        public string NodeName
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the deployed service package health states for the current deployed application as found in the health store.</para>
        /// </summary>
        /// <value>
        /// <para>The deployed service packages of the current deployed application as found in the health store.</para>
        /// </value>
        /// <para>Only deployed service packages that respect the <see cref="System.Fabric.Description.DeployedApplicationHealthQueryDescription.DeployedServicePackagesFilter"/> (if specified) are returned. 
        /// If the input filter is not specified, all deployed service packages are returned.</para>
        /// <para>All deployed service packages are evaluated to determine the deployed application aggregated health.</para>
        public IList<DeployedServicePackageHealthState> DeployedServicePackageHealthStates
        {
            get;
            internal set;
        }

        /// <summary>
        /// Gets the deployed application health statistics, which contain information about how many deployed service packages are in
        /// <see cref="System.Fabric.Health.HealthState.Ok"/>, <see cref="System.Fabric.Health.HealthState.Warning"/>, and <see cref="System.Fabric.Health.HealthState.Error"/> state.
        /// </summary>
        /// <value>The deployed application health statistics.</value>
        /// <remarks>
        /// <para>
        /// The deployed application health statistics contain information about how many deployed service packages are in
        /// <see cref="System.Fabric.Health.HealthState.Ok"/>, <see cref="System.Fabric.Health.HealthState.Warning"/>, and <see cref="System.Fabric.Health.HealthState.Error"/> state.
        /// It can be null or empty if the query that returns the <see cref="System.Fabric.Health.DeployedApplicationHealth"/>
        /// specified <see cref="System.Fabric.Health.DeployedApplicationHealthStatisticsFilter"/> to exclude health statistics.
        /// </para>
        /// </remarks>
        [JsonCustomization(IsDefaultValueIgnored = true)]
        public HealthStatistics HealthStatistics
        {
            get;
            internal set;
        }

        /// <summary>
        /// Gets a string representation of the <see cref="System.Fabric.Health.DeployedApplicationHealth"/>.
        /// </summary>
        /// <returns>A string representation of the <see cref="System.Fabric.Health.DeployedApplicationHealth"/>.</returns>
        public override string ToString()
        {
            var sb = new StringBuilder();
            sb.Append(
                string.Format(
                    CultureInfo.CurrentCulture,
                    "DeployedApplicationHealth ApplicationName={0}, NodeName={1}: {2}, {3} deployed service packages, {4} events",
                    this.ApplicationName,
                    this.NodeName,
                    this.AggregatedHealthState,
                    this.DeployedServicePackageHealthStates.Count,
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

        internal static unsafe DeployedApplicationHealth FromNativeResult(NativeClient.IFabricDeployedApplicationHealthResult nativeResult)
        {
            var nativePtr = nativeResult.get_DeployedApplicationHealth();
            ReleaseAssert.AssertIf(nativePtr == IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "DeployedApplicationHealth"));

            var retval = FromNative((NativeTypes.FABRIC_DEPLOYED_APPLICATION_HEALTH*)nativePtr);

            GC.KeepAlive(nativeResult);
            return retval;
        }

        private static unsafe DeployedApplicationHealth FromNative(NativeTypes.FABRIC_DEPLOYED_APPLICATION_HEALTH* nativeHealth)
        {
            var managedHealth = new DeployedApplicationHealth();

            managedHealth.AggregatedHealthState = (HealthState)nativeHealth->AggregatedHealthState;
            managedHealth.HealthEvents = HealthEvent.FromNativeList(nativeHealth->HealthEvents);
            managedHealth.ApplicationName = NativeTypes.FromNativeUri(nativeHealth->ApplicationName);
            managedHealth.NodeName = NativeTypes.FromNativeString(nativeHealth->NodeName);
            managedHealth.DeployedServicePackageHealthStates = DeployedServicePackageHealthState.FromNativeList(nativeHealth->DeployedServicePackageHealthStates);

            if (nativeHealth->Reserved != IntPtr.Zero)
            {
                var nativeHealthEx1 = (NativeTypes.FABRIC_DEPLOYED_APPLICATION_HEALTH_EX1*)nativeHealth->Reserved;
                managedHealth.UnhealthyEvaluations = HealthEvaluation.FromNativeList(nativeHealthEx1->UnhealthyEvaluations);

                if (nativeHealthEx1->Reserved != IntPtr.Zero)
                {
                    var nativeHealthEx2 = (NativeTypes.FABRIC_DEPLOYED_APPLICATION_HEALTH_EX2*)nativeHealthEx1->Reserved;
                    managedHealth.HealthStatistics = HealthStatistics.CreateFromNative(nativeHealthEx2->HealthStatistics);
                }
            }

            return managedHealth;
        }
    }
}