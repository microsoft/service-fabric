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
    /// Represents a service health state chunk, which contains basic health information about the service.
    /// </summary>
    public sealed class ServiceHealthStateChunk
    {
        /// <summary>
        /// Instantiates a <see cref="System.Fabric.Health.ServiceHealthStateChunk"/>.
        /// </summary>
        internal ServiceHealthStateChunk()
        {
        }

        /// <summary>
        /// Gets the service name, which uniquely identifies the service.
        /// </summary>
        /// <value>The service name.</value>
        public Uri ServiceName
        {
            get;
            internal set;
        }

        /// <summary>
        /// Gets the aggregated health state of the service, computed based on all reported health events, its children and the application health policy.
        /// </summary>
        /// <value>The aggregated health state of the service.</value>
        public HealthState HealthState
        {
            get;
            internal set;
        }

        /// <summary>
        /// Gets the list of the service partition health state chunks that respect the input filters.
        /// </summary>
        /// <value>The list of the partition health state chunks that respect the input filters.</value>
        /// <remarks>
        /// <para>By default, no children are included in results. Users can request to include
        /// some children based on desired health or other information. 
        /// For example, users can request to include all partitions that have health state error.
        /// Regardless of filter value, all children are used to compute service aggregated health.</para>
        /// </remarks>
        public PartitionHealthStateChunkList PartitionHealthStateChunks
        {
            get;
            internal set;
        }

        /// <summary>
        /// Creates a string description of the service health state chunk.
        /// </summary>
        /// <returns>String description of the health state chunk.</returns>
        public override string ToString()
        {
            return string.Format(
                CultureInfo.CurrentCulture,
                "{0}: {1}",
                this.ServiceName,
                this.HealthState);
        }

        internal static unsafe ServiceHealthStateChunk FromNative(NativeTypes.FABRIC_SERVICE_HEALTH_STATE_CHUNK nativeServiceHealthStateChunk)
        {
            var managedServiceHealthStateChunk = new ServiceHealthStateChunk();

            managedServiceHealthStateChunk.ServiceName = NativeTypes.FromNativeUri(nativeServiceHealthStateChunk.ServiceName);
            managedServiceHealthStateChunk.HealthState = (HealthState)nativeServiceHealthStateChunk.HealthState;
            managedServiceHealthStateChunk.PartitionHealthStateChunks = PartitionHealthStateChunkList.CreateFromNativeList(nativeServiceHealthStateChunk.PartitionHealthStateChunks);

           return managedServiceHealthStateChunk;
        }
    }
}
