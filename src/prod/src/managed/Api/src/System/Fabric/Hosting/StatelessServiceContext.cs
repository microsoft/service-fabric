// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;

    /// <summary>
    /// The service context that the stateless service is operating under.
    /// </summary>
    public sealed class StatelessServiceContext : ServiceContext
    {
        /// <summary>
        /// Gets the stateless service instance ID.
        /// </summary>
        /// <value>The stateless service instance ID.</value>
        public long InstanceId
        {
            get { return this.ReplicaOrInstanceId; }
        }

        // Hide default ctor from public use
        internal StatelessServiceContext()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="System.Fabric.StatelessServiceContext"/> class.
        /// </summary>
        /// <param name="nodeContext">The node context, which contains information about the node
        /// where the stateless service instance is running.</param>
        /// <param name="codePackageActivationContext">The code package activation context,
        /// which contains information from the service manifest and the currently activated code package,
        /// like work directory, context ID etc.</param>
        /// <param name="serviceTypeName">The service type name.</param>
        /// <param name="serviceName">The service name.</param>
        /// <param name="initializationData">The service initialization data, 
        /// which represents custom initialization data provided by the creator of the service.</param>
        /// <param name="partitionId">The partition ID.</param>
        /// <param name="instanceId">The instance ID.</param>
        public StatelessServiceContext(
            NodeContext nodeContext,
            ICodePackageActivationContext codePackageActivationContext,
            string serviceTypeName,
            Uri serviceName,
            byte[] initializationData,
            Guid partitionId,
            long instanceId)
            : base(nodeContext,
                codePackageActivationContext,
                serviceTypeName,
                serviceName,
                initializationData,
                partitionId,
                instanceId)
        {
        }
    }
}