// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using Query;
    using System.Fabric.Interop;
    using System.Fabric.Description;
    using System.Globalization;

    /// <summary>
    /// Represents a deployed service package health state chunk, which contains basic health information about the deployed service package.
    /// It is included as child of a deployed application.
    /// </summary>
    public sealed class DeployedServicePackageHealthStateChunk
    {
        /// <summary>
        /// Instantiates a <see cref="System.Fabric.Health.DeployedServicePackageHealthStateChunk"/>.
        /// </summary>
        internal DeployedServicePackageHealthStateChunk()
        {
            this.ServicePackageActivationId = string.Empty;
        }
               
        /// <summary>
        /// Gets the service manifest name, which is part of the deployed service package unique identifier, together with node name and application name.
        /// </summary>
        /// <value>The service manifest name.</value>
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
        /// Gets the aggregated health state of the deployed service package, computed based on all reported health events and the application health policy.
        /// </summary>
        /// <value>The aggregated health state of the deployed service package .</value>
        public HealthState HealthState
        {
            get;
            internal set;
        }

        /// <summary>
        /// Creates a string description of the deployed service package health state chunk.
        /// </summary>
        /// <returns>String description of the health state chunk.</returns>
        public override string ToString()
        {
            return string.Format(
                CultureInfo.CurrentCulture,
                "{0}+{1}: {2}",
                this.ServiceManifestName,
                this.ServicePackageActivationId,
                this.HealthState);
        }

        internal static unsafe DeployedServicePackageHealthStateChunk FromNative(NativeTypes.FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE_CHUNK nativeDeployedServicePackageHealthStateChunk)
        {
            var managedDeployedServicePackageHealthStateChunk = new DeployedServicePackageHealthStateChunk();

            managedDeployedServicePackageHealthStateChunk.ServiceManifestName = NativeTypes.FromNativeString(nativeDeployedServicePackageHealthStateChunk.ServiceManifestName);
            managedDeployedServicePackageHealthStateChunk.HealthState = (HealthState)nativeDeployedServicePackageHealthStateChunk.HealthState;

            if (nativeDeployedServicePackageHealthStateChunk.Reserved != IntPtr.Zero)
            {
                var nativeResultItemEx1 =
                    (NativeTypes.FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE_CHUNK_EX1*)nativeDeployedServicePackageHealthStateChunk.Reserved;

                managedDeployedServicePackageHealthStateChunk.ServicePackageActivationId = NativeTypes.FromNativeString(nativeResultItemEx1->ServicePackageActivationId);
            }
            
            return managedDeployedServicePackageHealthStateChunk;
        }
    }
}
