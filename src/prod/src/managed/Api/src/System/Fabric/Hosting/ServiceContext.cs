// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Globalization;

namespace System.Fabric
{
    using System;
    using System.Fabric.Common;

    /// <summary>
    /// The service context that the service is operating under.
    /// </summary>
    public abstract class ServiceContext
    {
        private readonly NodeContext nodeContext;
        private readonly ICodePackageActivationContext codePackageActivationContext;
        private readonly string serviceTypeName;
        private readonly Uri serviceName;
        private readonly byte[] initializationData;
        private readonly Guid partitionId;
        private readonly long replicaOrInstanceId;
        private readonly string traceId;

        // Hide default ctor from public use
        internal ServiceContext()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="System.Fabric.ServiceContext"/> class.
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
        /// <param name="replicaOrInstanceId">The replica or instance ID.</param>
        protected ServiceContext(
            NodeContext nodeContext,
            ICodePackageActivationContext codePackageActivationContext,
            string serviceTypeName,
            Uri serviceName,
            byte[] initializationData,
            Guid partitionId,
            long replicaOrInstanceId)
        {
            nodeContext.ThrowIfNull("nodeContext");
            codePackageActivationContext.ThrowIfNull("codePackageActivationContext");
            serviceTypeName.ThrowIfNull("serviceTypeName");
            serviceName.ThrowIfNull("serviceName");

            this.nodeContext = nodeContext;
            this.codePackageActivationContext = codePackageActivationContext;
            this.serviceTypeName = serviceTypeName;
            this.serviceName = serviceName;
            this.initializationData = initializationData;
            this.partitionId = partitionId;
            this.replicaOrInstanceId = replicaOrInstanceId;
            this.traceId = String.Concat(partitionId.ToString("B"), ":", replicaOrInstanceId.ToString(CultureInfo.InvariantCulture));

            this.SetServiceAddresses();
        }

        /// <summary>
        /// Gets the node context with information about the node where the service replica is instantiated.
        /// </summary>
        /// <value>The node context for the node where the service replica or instance is instantiated.</value>
        public NodeContext NodeContext
        {
            get { return this.nodeContext; }
        }

        /// <summary>
        /// <para>
        /// Gets the code package activation context,
        /// which contains information from the service manifest and the currently activated code package,
        /// like work directory, context ID etc.</para>
        /// </summary>
        /// <value>The code package activation context.</value>
        public ICodePackageActivationContext CodePackageActivationContext
        {
            get { return this.codePackageActivationContext; }
        }

        /// <summary>
        /// Gets the service type name.
        /// </summary>
        /// <value>The service type name.</value>
        public string ServiceTypeName
        {
            get { return this.serviceTypeName; }
        }

        /// <summary>
        /// Get the service name.
        /// </summary>
        /// <value>The service name.</value>
        public Uri ServiceName
        {
            get { return this.serviceName; }
        }

        /// <summary>
        /// Gets the initialization data of the service.
        /// </summary>
        /// <value>The initialization data.</value>
        public byte[] InitializationData
        {
            get { return this.initializationData; }
        }

        /// <summary>
        /// Gets the partition ID.
        /// </summary>
        /// <value>The partition ID.</value>
        public Guid PartitionId
        {
            get { return this.partitionId; }
        }

        /// <summary>
        /// Gets the stateful service replica ID or the stateless service instance ID.
        /// </summary>
        /// <value>The stateful service replica ID or the stateless service instance ID.</value>
        public long ReplicaOrInstanceId
        {
            get { return this.replicaOrInstanceId; }
        }

        /// <summary>
        /// Gets the trace ID of the service.
        /// </summary>
        /// <value>The trace ID of the service.</value>
        /// <remarks>The trace ID can be used as an identifier for generated traces.</remarks>
        public string TraceId
        {
            get { return this.traceId; }
        }

        /// <summary>
        /// <para>The address at which the service should start the communication listener.</para>
        /// </summary>
        /// <value>
        /// <para>The address at which the service should start the communication listener.</para>
        /// </value>
        public string ListenAddress
        {
            get;
            private set;
        }

        /// <summary>
        /// <para>The address which the service should publish as the listen address.</para>
        /// </summary>
        /// <value>
        /// <para>The address which the service should publish as the listen address.</para>
        /// </value>
        public string PublishAddress
        {
            get;
            private set;
        }

        private void SetServiceAddresses()
        {
            ICodePackageActivationContext2 codePackageActivationContext2 = this.codePackageActivationContext as ICodePackageActivationContext2;
            if(codePackageActivationContext2 != null)
            {
                this.ListenAddress = codePackageActivationContext2.ServiceListenAddress;
                this.PublishAddress = codePackageActivationContext2.ServicePublishAddress;
            }
            else
            {
                this.ListenAddress = this.nodeContext.IPAddressOrFQDN;
                this.PublishAddress = this.nodeContext.IPAddressOrFQDN;
            }

        }
    }
}