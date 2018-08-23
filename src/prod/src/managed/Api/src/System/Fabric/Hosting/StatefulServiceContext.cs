// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;

    /// <summary>
    /// Represents the service context that the stateful service is operating under.
    /// </summary>
    public sealed class StatefulServiceContext : ServiceContext
    {
        /// <summary>
        /// Gets the stateful service replica ID.
        /// </summary>
        /// <value>The stateful service replica ID.</value>
        public long ReplicaId
        {
            get { return this.ReplicaOrInstanceId; }
        }

        // Hide default ctor from public use
        internal StatefulServiceContext()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="System.Fabric.StatefulServiceContext"/> class.
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
        /// <param name="replicaId">The replica ID.</param>
        public StatefulServiceContext(
            NodeContext nodeContext,
            ICodePackageActivationContext codePackageActivationContext,
            string serviceTypeName,
            Uri serviceName,
            byte[] initializationData,
            Guid partitionId,
            long replicaId)
            : base(nodeContext,
                codePackageActivationContext,
                serviceTypeName,
                serviceName,
                initializationData,
                partitionId,
                replicaId)
        {
        }
    }
}