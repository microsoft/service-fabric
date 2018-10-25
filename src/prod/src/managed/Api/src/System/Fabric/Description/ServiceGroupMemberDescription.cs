// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Describes a service that belongs to a service group.  </para>
    /// </summary>
    /// <remarks>
    /// <para>A <see cref="System.Fabric.Description.ServiceGroupMemberDescription" /> contains a subset of a normal stateless or stateful service 
    /// description. These fields are relevant to the service inside the group. Other fields that are present in a normal service description, such as 
    /// partitioning information, become properties of the service group via its <see cref="System.Fabric.Description.ServiceGroupDescription" />.</para>
    /// </remarks>
    public sealed class ServiceGroupMemberDescription
    {
        private readonly List<ServiceLoadMetricDescription> metrics = new List<ServiceLoadMetricDescription>();

        /// <summary>
        /// <para>Creates a <see cref="System.Fabric.Description.ServiceGroupMemberDescription" /> object and initializes it with the specified parameters.</para>
        /// </summary>
        /// <param name="serviceTypeName">
        /// <para>The service type name of the service group member.</para>
        /// </param>
        /// <param name="serviceName">
        /// <para>The fully qualified name to set for the member. For example, if the group name is fabric:/G1 and the member is M1, then the full name 
        /// to specify is fabric:/G1#M1.</para>
        /// </param>
        /// <param name="initializationData">
        /// <para>The byte[] that is provided as the initialization data to the member’s factory.</para>
        /// </param>
        public ServiceGroupMemberDescription(string serviceTypeName, Uri serviceName, byte[] initializationData)
        {
            Requires.Argument<string>("serviceType", serviceTypeName).NotNullOrEmpty();
            Requires.Argument<Uri>("serviceName", serviceName).NotNull();

            this.ServiceTypeName = serviceTypeName;
            this.ServiceName = serviceName;
            this.InitializationData = initializationData;
        }

        /// <summary>
        /// <para>Creates an empty <see cref="System.Fabric.Description.ServiceGroupMemberDescription" /> object.</para>
        /// </summary>
        public ServiceGroupMemberDescription()
        {
        }

        /// <summary>
        /// <para>Gets or sets the service type of this service group member.</para>
        /// </summary>
        /// <value>
        /// <para>The service type name.</para>
        /// </value>
        public string ServiceTypeName
        {
            get;
            set;
        }

        /// <summary>
        /// <para>Gets or sets the name of the service within the service group.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Uri" /> representing the service name.</para>
        /// </value>
        /// <remarks>
        /// <para>Services are independently named inside the service group. This name is used as a portion of the stable fabric name to resolve 
        /// the service. For example, if the service group’s name is "fabric:/groupA" and the service name provided here is "svc1", then a client 
        /// should resolve the name “fabric:/groupA#svc1” to resolve this service.</para>
        /// </remarks>
        /// <seealso cref="System.Fabric.Description.ServiceDescription" />
        public Uri ServiceName
        {
            get;
            set;
        }

        /// <summary>
        /// <para>Gets or sets the initialization data for this service group member.</para>
        /// </summary>
        /// <value>
        /// <para>the initialization data for this service group member.</para>
        /// </value>
        /// <remarks>
        /// <para>This information is passed by the service group factory to the service factories that correspond to the object when it is created 
        /// as initialization data for instances of this service group member, similar to how initialization data is passed when normal stateless or 
        /// stateful service instances are created.</para>
        /// </remarks>
        /// <seealso cref="System.Fabric.IStatelessServiceFactory.CreateInstance(System.String,System.Uri,System.Byte[],System.Guid,System.Int64)" />
        /// <seealso cref="System.Fabric.IStatefulServiceFactory.CreateReplica(System.String,System.Uri,System.Byte[],System.Guid,System.Int64)" />
        /// <seealso cref="System.Fabric.Description.ServiceDescription.InitializationData" />
        [SuppressMessage(FxCop.Category.Performance, FxCop.Rule.PropertiesShouldNotReturnArrays, Justification = "Manipulation of byte[] does not have any effect on the ServiceDescription already registered")]
        [JsonCustomization(IsDefaultValueIgnored = true)]
        public byte[] InitializationData
        {
            get;
            set;
        }

        /// <summary>
        /// <para>Gets or sets the collection of <see cref="System.Fabric.Description.ServiceLoadMetricDescription" /> objects for this service. The metrics 
        /// collection contains the <see cref="System.Fabric.Description.ServiceLoadMetricDescription" /> objects that are relevant to this service.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Collections.Generic.IList{T}" /> of type <see cref="System.Fabric.Description.ServiceLoadMetricDescription" />.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.ServiceLoadMetrics)]
        public IList<ServiceLoadMetricDescription> Metrics
        {
            get
            {
                return this.metrics;
            }
        }

        internal static void Validate(ServiceGroupMemberDescription serviceGroupMemberDescription)
        {
            Requires.Argument<Uri>("serviceGroupMemberDescription.Name", serviceGroupMemberDescription.ServiceName).NotNullOrWhiteSpace();
            Requires.Argument<string>("serviceGroupMemberDescription.ServiceTypeName", serviceGroupMemberDescription.ServiceTypeName).NotNullOrWhiteSpace();

            foreach (var metric in serviceGroupMemberDescription.Metrics)
            {
                ServiceLoadMetricDescription.Validate(metric);
            }
        }

        internal static unsafe ServiceGroupMemberDescription CreateFromNative(IntPtr nativeDescriptionPtr, bool isStateful)
        {
            var nativeDescription = (NativeTypes.FABRIC_SERVICE_GROUP_MEMBER_DESCRIPTION*)nativeDescriptionPtr;

            var description = new ServiceGroupMemberDescription
            {
                ServiceName = NativeTypes.FromNativeUri(nativeDescription->ServiceName),
                ServiceTypeName = NativeTypes.FromNativeString(nativeDescription->ServiceTypeName),
                InitializationData = NativeTypes.FromNativeBytes(nativeDescription->InitializationData, nativeDescription->InitializationDataSize),
            };

            var nativeMetrics = (NativeTypes.FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION*)nativeDescription->Metrics;
            for (int i = 0; i < (int)nativeDescription->MetricCount; i++)
            {
                var item = ServiceLoadMetricDescription.CreateFromNative((IntPtr)(nativeMetrics + i), isStateful);
                description.Metrics.Add(item);
            }

            return description;
        }
    }
}