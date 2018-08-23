// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.Text;

    /// <summary>
    /// <para>Describes the health of a deployed service package 
    /// as returned by <see cref="System.Fabric.FabricClient.HealthClient.GetDeployedServicePackageHealthAsync(Description.DeployedServicePackageHealthQueryDescription)" />.</para>
    /// </summary>
    public sealed class DeployedServicePackageHealth : EntityHealth
    {
        internal DeployedServicePackageHealth()
        {
            this.ServicePackageActivationId = string.Empty;
        }

        /// <summary>
        /// <para>Gets the application name, which uniquely identifies the application.</para>
        /// </summary>
        /// <value>
        /// <para>The application name.</para>
        /// </value>
        public Uri ApplicationName
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the service manifest name.</para>
        /// </summary>
        /// <value>
        /// <para>The service manifest name.</para>
        /// </value>
        public string ServiceManifestName
        {
            get;
            internal set;
        }

        /// <summary>
        /// Gets the ActivationId of deployed service package.
        /// </summary>
        /// <value>
        /// <para>
        /// A string representing ActivationId of deployed service package. 
        /// </para>
        /// <para>
        /// If <see cref="System.Fabric.Description.ServicePackageActivationMode"/> specified at the time of creating the service is 
        /// <see cref="System.Fabric.Description.ServicePackageActivationMode.SharedProcess"/> (or if it is not specified, in
        /// which case it defaults to <see cref="System.Fabric.Description.ServicePackageActivationMode.SharedProcess"/>), then value of 
        /// <see cref="System.Fabric.Health.DeployedServicePackageHealth.ServicePackageActivationId"/> is always an empty string.
        /// For more details please see <see cref="System.Fabric.Description.ServicePackageActivationMode"/>.
        /// </para>
        /// </value>
        public string ServicePackageActivationId
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the node name where the deployed service package is running.</para>
        /// </summary>
        /// <value>
        /// <para>The node name where the deployed service package is running.</para>
        /// </value>
        public string NodeName
        {
            get;
            internal set;
        }

        /// <summary>
        /// Gets a string representation of the <see cref="System.Fabric.Health.DeployedServicePackageHealth"/>.
        /// </summary>
        /// <returns>A string representation of the <see cref="System.Fabric.Health.DeployedServicePackageHealth"/>.</returns>
        public override string ToString()
        {
            var sb = new StringBuilder();
            sb.Append(
                string.Format(
                    CultureInfo.CurrentCulture,
                    "DeployedServicePackageHealth ApplicationName={0}, NodeName={1}, ServiceManifestName={2}: {3}, {4} events",
                    this.ApplicationName,
                    this.NodeName,
                    this.ServiceManifestName,
                    this.AggregatedHealthState,
                    this.HealthEvents.Count));

            if (this.UnhealthyEvaluations != null && this.UnhealthyEvaluations.Count > 0)
            {
                sb.Append(string.Format(CultureInfo.CurrentCulture, ", {0}", this.UnhealthyEvaluations));
            }

            return sb.ToString();
        }

        internal static unsafe DeployedServicePackageHealth FromNativeResult(NativeClient.IFabricDeployedServicePackageHealthResult nativeResult)
        {
            var nativePtr = nativeResult.get_DeployedServicePackageHealth();
            ReleaseAssert.AssertIf(nativePtr == IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "DeployedServicePackageHealth"));

            var retval = FromNative((NativeTypes.FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH*)nativePtr);

            GC.KeepAlive(nativeResult);
            return retval;
        }

        private static unsafe DeployedServicePackageHealth FromNative(NativeTypes.FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH* nativeHealth)
        {
            var managedHealth = new DeployedServicePackageHealth();

            managedHealth.AggregatedHealthState = (HealthState)nativeHealth->AggregatedHealthState;
            managedHealth.HealthEvents = HealthEvent.FromNativeList(nativeHealth->HealthEvents);
            managedHealth.ApplicationName = NativeTypes.FromNativeUri(nativeHealth->ApplicationName);
            managedHealth.ServiceManifestName = NativeTypes.FromNativeString(nativeHealth->ServiceManifestName);
            managedHealth.NodeName = NativeTypes.FromNativeString(nativeHealth->NodeName);

            if (nativeHealth->Reserved == IntPtr.Zero)
            {
                return managedHealth;
            }

            var nativeHealthEx1 = (NativeTypes.FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_EX1*)nativeHealth->Reserved;
            managedHealth.UnhealthyEvaluations = HealthEvaluation.FromNativeList(nativeHealthEx1->UnhealthyEvaluations);

            if (nativeHealthEx1->Reserved == IntPtr.Zero)
            {
                return managedHealth;
            }

            var nativeHealthEx2 = (NativeTypes.FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_EX2*)nativeHealthEx1->Reserved;
            managedHealth.ServicePackageActivationId = NativeTypes.FromNativeString(nativeHealthEx2->ServicePackageActivationId);

            return managedHealth;
        }
    }
}