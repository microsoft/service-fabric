// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System;
    using System.Fabric.Interop;
    using System.Globalization;

    /// <summary>
    /// Represents an application health state chunk, which contains basic health information about the application.
    /// </summary>
    public sealed class ApplicationHealthStateChunk
    {
        /// <summary>
        /// Instantiates an <see cref="System.Fabric.Health.ApplicationHealthStateChunk"/>.
        /// </summary>
        internal ApplicationHealthStateChunk()
        {
        }

        /// <summary>
        /// Gets the application name.
        /// </summary>
        /// <value>The application name.</value>
        public Uri ApplicationName
        {
            get;
            internal set;
        }

        /// <summary>
        /// Gets the application type name.
        /// </summary>
        /// <value>The application type name.</value>
        public string ApplicationTypeName
        {
            get;
            internal set;
        }

        /// <summary>
        /// Gets the application aggregated health state, computed based on all reported health events, the children and the application health policy.
        /// </summary>
        /// <value>The application aggregated health state.</value>
        public HealthState HealthState
        {
            get;
            internal set;
        }

        /// <summary>
        /// Gets the list of the service health state chunks that respect the input filters.
        /// </summary>
        /// <value>The list of the service health state chunks that respect the input filters.</value>
        /// <remarks>
        /// <para>By default, no children are included in results. Users can request to include
        /// some children based on desired health or other information. 
        /// For example, users can request to include all services that have health state error.
        /// Regardless of filter value, all children are used to compute application aggregated health.</para>
        /// </remarks>
        public ServiceHealthStateChunkList ServiceHealthStateChunks
        {
            get;
            internal set;
        }

        /// <summary>
        /// Gets the list of the deployed application health state chunks that respect the input filters.
        /// </summary>
        /// <value>The list of the deployed application health state chunks that respect the input filters.</value>
        /// <remarks>
        /// <para>By default, no children are included in results. Users can request to include
        /// some children based on desired health or other information. 
        /// For example, users can request to include all deployed applications that have health state error.
        /// Regardless of filter value, all children are used to compute application aggregated health.</para>
        /// </remarks>
        public DeployedApplicationHealthStateChunkList DeployedApplicationHealthStateChunks
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
                "{0}, application type '{1}': {2}",
                this.ApplicationName,
                this.ApplicationTypeName,
                this.HealthState);
        }

        internal static unsafe ApplicationHealthStateChunk FromNative(NativeTypes.FABRIC_APPLICATION_HEALTH_STATE_CHUNK nativeApplicationHealthStateChunk)
        {
            var managedApplicationHealthStateChunk = new ApplicationHealthStateChunk();

            managedApplicationHealthStateChunk.ApplicationName = NativeTypes.FromNativeUri(nativeApplicationHealthStateChunk.ApplicationName);
            managedApplicationHealthStateChunk.HealthState = (HealthState)nativeApplicationHealthStateChunk.HealthState;
            managedApplicationHealthStateChunk.ServiceHealthStateChunks = ServiceHealthStateChunkList.CreateFromNativeList(nativeApplicationHealthStateChunk.ServiceHealthStateChunks);
            managedApplicationHealthStateChunk.DeployedApplicationHealthStateChunks = DeployedApplicationHealthStateChunkList.CreateFromNativeList(nativeApplicationHealthStateChunk.DeployedApplicationHealthStateChunks);

            if (nativeApplicationHealthStateChunk.Reserved != IntPtr.Zero)
            {
                var ex1 = (NativeTypes.FABRIC_APPLICATION_HEALTH_STATE_CHUNK_EX1*)nativeApplicationHealthStateChunk.Reserved;
                managedApplicationHealthStateChunk.ApplicationTypeName = NativeTypes.FromNativeString(ex1->ApplicationTypeName);
            }

            return managedApplicationHealthStateChunk;
        }
    }
}
