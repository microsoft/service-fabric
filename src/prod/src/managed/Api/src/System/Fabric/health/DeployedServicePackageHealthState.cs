// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using Query;
    using System.Collections.Generic;
    using System.Fabric.Description;
    using System.Fabric.Interop;
    using System.Globalization;

    /// <summary>
    /// <para>Represents the health state of a deployed service package, containing the entity identifier and the aggregated health state.</para>
    /// </summary>
    public sealed class DeployedServicePackageHealthState : EntityHealthState
    {
        internal DeployedServicePackageHealthState()
        {
            this.ServicePackageActivationId = string.Empty;
        }

        /// <summary>
        /// <para>Gets the application name.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Uri" /> representing the application name.</para>
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
        /// <para>A <see cref="System.String" /> representing the service manifest name.</para>
        /// </value>
        public string ServiceManifestName
        {
            get;
            internal set;
        }

        /// <summary>
        /// Gets the ActivationId of service package.
        /// </summary>
        /// <value>
        /// <para>
        /// The <see cref="System.Fabric.Query.DeployedServicePackage.ServicePackageActivationId"/> of deployed service package. 
        /// This can be obtained by using  <see cref="System.Fabric.FabricClient.QueryClient.GetDeployedServicePackageListAsync(string, System.Uri)"/> query. 
        /// </para>
        /// <para>
        /// If <see cref="System.Fabric.Description.ServicePackageActivationMode"/>
        /// specified at the time of creating the service was <see cref="System.Fabric.Description.ServicePackageActivationMode.SharedProcess"/> (or if it was not specified, in
        /// which case it defaults to <see cref="System.Fabric.Description.ServicePackageActivationMode.SharedProcess"/>), then value of 
        /// <see cref="System.Fabric.Query.DeployedServicePackage.ServicePackageActivationId"/> is always an empty string.
        /// For more details please see <see cref="System.Fabric.Description.ServicePackageActivationMode"/>.
        /// </para>
        /// </value>
        public string ServicePackageActivationId
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the node name of the node where the deployed service package is running.</para>
        /// </summary>
        /// <value>
        /// <para>A <see cref="System.String" /> representing the node name.</para>
        /// </value>
        public string NodeName
        {
            get;
            internal set;
        }

        /// <summary>
        /// Creates a string description of the deployed service package health state, containing the identifier and the aggregated health state.
        /// </summary>
        /// <returns>String description of the <see cref="System.Fabric.Health.DeployedServicePackageHealthState"/>.</returns>
        public override string ToString()
        {
            return string.Format(
                CultureInfo.CurrentCulture,
                "{0}+{1}+{2}+{3}: {4}",
                this.ApplicationName,
                this.NodeName,
                this.ServiceManifestName,
                this.ServicePackageActivationId,
                this.AggregatedHealthState);
        }

        internal static unsafe IList<DeployedServicePackageHealthState> FromNativeList(IntPtr nativeListPtr)
        {
            if (nativeListPtr != IntPtr.Zero)
            {
                var nativeList = (NativeTypes.FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE_LIST*)nativeListPtr;
                return DeployedServicePackageHealthStateList.FromNativeList(nativeList);
            }
            else
            {
                return new DeployedServicePackageHealthStateList();
            }
        }

        internal static unsafe DeployedServicePackageHealthState FromNative(NativeTypes.FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE nativeState)
        {
            var deployedServicePackageHealthState = new DeployedServicePackageHealthState();
            deployedServicePackageHealthState.ApplicationName = NativeTypes.FromNativeUri(nativeState.ApplicationName);
            deployedServicePackageHealthState.ServiceManifestName = NativeTypes.FromNativeString(nativeState.ServiceManifestName);
            deployedServicePackageHealthState.NodeName = NativeTypes.FromNativeString(nativeState.NodeName);
            deployedServicePackageHealthState.AggregatedHealthState = (HealthState)nativeState.AggregatedHealthState;

            if (nativeState.Reserved != IntPtr.Zero)
            {
                var nativeStateEx1 =
                    (NativeTypes.FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE_EX1*)nativeState.Reserved;

                deployedServicePackageHealthState.ServicePackageActivationId = NativeTypes.FromNativeString(nativeStateEx1->ServicePackageActivationId);
            }
            
            return deployedServicePackageHealthState;
        }
    }
}