// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System.Fabric.Interop;
    using System.Globalization;

    /// <summary>
    /// Represents a deployed application health state chunk, which contains basic health information about the deployed application.
    /// It is included as child of an application.
    /// </summary>
    public sealed class DeployedApplicationHealthStateChunk
    {
        /// <summary>
        /// Instantiates a <see cref="System.Fabric.Health.DeployedApplicationHealthStateChunk"/>.
        /// </summary>
        internal DeployedApplicationHealthStateChunk()
        {
        }

        /// <summary>
        /// Gets the node name.
        /// </summary>
        /// <value>The node name.</value>
        public string NodeName
        {
            get;
            internal set;
        }

        /// <summary>
        /// Gets the aggregated health state of the deployed application, computed based on all reported health events, the children and the application health policy.
        /// </summary>
        /// <value>The aggregated health state of the deployed application.</value>
        public HealthState HealthState
        {
            get;
            internal set;
        }

        /// <summary>
        /// Gets the list of the deployed service package health state chunks children that respect the input filters.
        /// </summary>
        /// <value>The list of the deployed service package  replica health state chunks children that respect the input filters.</value>
        /// <remarks>
        /// <para>By default, no children are included in results. Users can request to include
        /// some children based on desired health or other information. 
        /// For example, users can request to include all deployed service packages that have health state error.
        /// Regardless of filter value, all children are used to compute the entity aggregated health.</para>
        /// </remarks>
        public DeployedServicePackageHealthStateChunkList DeployedServicePackageHealthStateChunks
        {
            get;
            internal set;
        }

        /// <summary>
        /// Creates a string description of the health state chunk.
        /// </summary>
        /// <returns>String description of the health state chunk.</returns>
        public override string ToString()
        {
            return string.Format(
                CultureInfo.CurrentCulture,
                "{0}: {1}",
                this.NodeName,
                this.HealthState);
        }

        internal static unsafe DeployedApplicationHealthStateChunk FromNative(NativeTypes.FABRIC_DEPLOYED_APPLICATION_HEALTH_STATE_CHUNK nativeDeployedApplicationHealthStateChunk)
        {
            var managedDeployedApplicationHealthStateChunk = new DeployedApplicationHealthStateChunk();

            managedDeployedApplicationHealthStateChunk.NodeName = NativeTypes.FromNativeString(nativeDeployedApplicationHealthStateChunk.NodeName);
            managedDeployedApplicationHealthStateChunk.HealthState = (HealthState)nativeDeployedApplicationHealthStateChunk.HealthState;
            managedDeployedApplicationHealthStateChunk.DeployedServicePackageHealthStateChunks = DeployedServicePackageHealthStateChunkList.CreateFromNativeList(nativeDeployedApplicationHealthStateChunk.DeployedServicePackageHealthStateChunks);

           return managedDeployedApplicationHealthStateChunk;
        }
    }
}
