// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Fabric.Common;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.Linq;
    using System.Runtime.Serialization;

    /// <summary>
    /// <para> A ServiceDescription contains all of the information necessary to create a service. </para>
    /// </summary>
    [KnownType(typeof(StatelessServiceDescription))]
    [KnownType(typeof(StatefulServiceDescription))]
    public abstract class ServiceDescription
    {
        private KeyedCollection<string, ServiceLoadMetricDescription> metrics
            = new KeyedItemCollection<string, ServiceLoadMetricDescription>(d => d.Name);

        private readonly List<ServiceCorrelationDescription> correlations = new List<ServiceCorrelationDescription>();

        private readonly List<ServicePlacementPolicyDescription> placementPolicies = new List<ServicePlacementPolicyDescription>();

        private MoveCost defaultMoveCost;
        private bool isDefaultMoveCostSpecified;

        private List<ScalingPolicyDescription> scalingPolicies = new List<ScalingPolicyDescription>();

        /// <summary>
        /// <para>Initialize an instance of <see cref="System.Fabric.Description.ServiceDescription" /> with service kind.</para>
        /// </summary>
        /// <param name="kind">
        /// <para>Describe the kind of service type.</para>
        /// </param>
        protected ServiceDescription(ServiceDescriptionKind kind)
        {
            this.Kind = kind;
            this.PlacementConstraints = string.Empty;
            this.isDefaultMoveCostSpecified = false;
            this.defaultMoveCost = MoveCost.Low;
            this.ServicePackageActivationMode = ServicePackageActivationMode.SharedProcess;
        }

        /// <summary>
        /// <para>
        /// Instantiates a <see cref="System.Fabric.Description.ServiceDescription" /> class with parameters from another 
        /// <see cref="System.Fabric.Description.ServiceDescription" /> object.
        /// </para>
        /// </summary>
        /// <param name="other">
        /// <para>The service description from which parameters are copied.</para>
        /// </param>
        protected ServiceDescription(ServiceDescription other)
            : this(other.Kind)
        {
            this.CopyFrom(other);
        }

        /// <summary>
        /// <para>Gets the service kind (for example, Stateful or Stateless) of this service.</para>
        /// </summary>
        /// <value>
        /// <para>The service kind.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.ServiceKind, AppearanceOrder = -2)]
        public ServiceDescriptionKind Kind { get; private set; }

        /// <summary>
        /// <para> Gets or sets the placement constraints for this service.</para>
        /// </summary>
        /// <value>
        /// <para> Returns the placement constraints.</para>
        /// </value>
        /// <remarks>
        /// <para>Placement constraints are Boolean statements which allow services to select for particular node properties (and the values of 
        /// those properties) in order to control where it is legal to place them.  Node properties are key value pairs that define some additional 
        /// metadata about a node, usually related to the hardware capabilities of the node.  Examples of hardware characteristics that could be exposed 
        /// as node properties are “HasDisk”, “MemorySize”, “StorageSize”, “NumberOfCores” etc.  When deploying a service, an administrator can define 
        /// the properties that the service cares about as well as simple Boolean operators which define requirements for the values of those 
        /// properties.  Ex: (HasDisk==true &amp;&amp; MemorySize&gt;=2048).  During runtime, Service Fabric load balancing will only place services 
        /// on nodes that have properties with values which match those required by the service.</para>
        /// </remarks>
        [JsonCustomization(PropertyName = JsonPropertyNames.PlacementConstraints)]
        public string PlacementConstraints { get; set; }

        /// <summary>
        /// <para>Gets or sets the service type name.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the service type.</para>
        /// </value>
        public string ServiceTypeName { get; set; }

        /// <summary>
        /// <para>Gets or sets the URI of the application.</para>
        /// </summary>
        /// <value>
        /// <para>The application name.</para>
        /// </value>
        /// <remarks>
        /// <para>This is the unique name of an application and is used to group services together for management. The scheme must 
        /// be "fabric:/" and the application name must be a prefix of the service name.</para>
        /// </remarks>
        public Uri ApplicationName { get; set; }

        /// <summary>
        /// <para>Gets or sets the URI of this service.</para>
        /// </summary>
        /// <value>
        /// <para>The service name.</para>
        /// </value>
        public Uri ServiceName { get; set; }

        /// <summary>
        /// <para>Gets or sets the partition scheme description to be used for this service.</para>
        /// </summary>
        /// <value>
        /// <para>The partition scheme to be used for the service.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.PartitionDescription)]
        public PartitionSchemeDescription PartitionSchemeDescription { get; set; }

        /// <summary>
        /// <para>Gets or sets the initialization data that will be passed to service instances or replicas when they are created.</para>
        /// </summary>
        /// <value>
        /// <para>Returns the initialization data.</para>
        /// </value>
        [JsonCustomization(IsDefaultValueIgnored = true)]
        public byte[] InitializationData { get; set; }

        /// <summary>
        /// <para>
        /// Gets or sets the keyed collection of <see cref="System.Fabric.Description.ServiceLoadMetricDescription" />s
        /// that describe the load metrics defined for this service.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>Returns the collection of load metric descriptions.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.ServiceLoadMetrics)]
        public KeyedCollection<string, ServiceLoadMetricDescription> Metrics
        {
            get
            {
                return this.metrics;
            }
            set
            {
                this.metrics = value;
            }

        }

        /// <summary>
        /// <para>
        /// Gets the list of <see cref="System.Fabric.Description.ServiceCorrelationDescription" />s that describe the correlations
        /// of this service with other services.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>Returns the list of correlation descriptions.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.CorrelationScheme)]
        public IList<ServiceCorrelationDescription> Correlations
        {
            get
            {
                return this.correlations;
            }

        }

        /// <summary>
        /// <para> 
        /// Gets the list of <see cref="System.Fabric.Description.ServicePlacementPolicyDescription" />s that describe the placement policies
        /// for this service.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>Returns the list of placement policy descriptions.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.ServicePlacementPolicies)]
        public IList<ServicePlacementPolicyDescription> PlacementPolicies
        {
            get
            {
                return this.placementPolicies;
            }

        }

        /// <summary>
        /// <para> 
        /// Gets whether a default <see cref="System.Fabric.MoveCost" /> is specified for the service.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>A flag indicating whether a default <see cref="System.Fabric.MoveCost" /> is specified for the service.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.IsDefaultMoveCostSpecified)]
        public bool IsDefaultMoveCostSpecified
        {
            get
            {
                return isDefaultMoveCostSpecified;
            }

        }

        /// <summary>
        /// <para> 
        /// Gets or sets the default <see cref="System.Fabric.MoveCost" /> value for the service.
        /// </para>
        /// </summary>
        /// <value>
        /// <para> The default <see cref="System.Fabric.MoveCost" /> value that should be set to for the service.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.DefaultMoveCost)]
        public MoveCost DefaultMoveCost
        {
            get
            {
                return defaultMoveCost;
            }
            set
            {
                defaultMoveCost = value;
                isDefaultMoveCostSpecified = true;
            }
        }

        /// <summary>
        /// <para>
        /// Gets or sets the service DNS name. If this is specified, then the service can be 
        /// accessed via its DNS name instead of <see cref="ServiceName"/>.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The DNS name of the service or <c>null</c> if the service doesn't have a DNS name specified.</para>
        /// </value>
        [JsonCustomization(IsDefaultValueIgnored = true)]
        public string ServiceDnsName { get; set; }


        /// <summary>
        /// <para>
        /// Gets or sets the list of <see cref="System.Fabric.Description.ScalingPolicyDescription" /> for this service.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The scaling policies associated with this service.</para>
        /// </value>
        public IList<ScalingPolicyDescription> ScalingPolicies
        {
            get
            {
                return this.scalingPolicies;
            }
        }

        /// <summary>
        /// <para> 
        /// Gets or sets the <see cref="System.Fabric.Description.ServicePackageActivationMode"/> of a service.
        /// </para>
        /// </summary>
        /// <value>
        /// <para> 
        /// A <see cref="System.Fabric.Description.ServicePackageActivationMode"/> enumeration that represents activation mode 
        /// of the service package that contains the <see cref="System.Fabric.Description.ServiceDescription.ServiceTypeName"/> and will be activated to host the
        /// replica(s) or instance(s) of the service described by this <see cref="System.Fabric.Description.ServiceDescription"/> object. Please
        /// see <see cref="System.Fabric.Description.ServicePackageActivationMode"/> for more details.
        /// </para>
        /// </value>
        public ServicePackageActivationMode ServicePackageActivationMode { get; set; }

        private void CopyFrom(ServiceDescription other)
        {
            // Copy basic types
            this.PlacementConstraints = other.PlacementConstraints;
            this.ServiceTypeName = other.ServiceTypeName;
            this.ApplicationName = other.ApplicationName;
            this.ServiceName = other.ServiceName;
            this.InitializationData = other.InitializationData == null ? null : other.InitializationData.ToArray();
            this.isDefaultMoveCostSpecified = other.IsDefaultMoveCostSpecified;
            this.defaultMoveCost = other.defaultMoveCost;
            this.ServiceDnsName = other.ServiceDnsName;
            this.ServicePackageActivationMode = other.ServicePackageActivationMode;

            ServiceLoadMetricDescription.CopyFrom(other.Metrics, this.Metrics);

            this.correlations.AddRange(other.Correlations.Select(c => new ServiceCorrelationDescription(c)));
            this.PartitionSchemeDescription = other.PartitionSchemeDescription.GetCopy();
            this.placementPolicies.AddRange(other.placementPolicies.Select(p => p.GetCopy()));
            this.scalingPolicies.AddRange(other.ScalingPolicies.Select(p => ScalingPolicyDescription.GetCopy(p)));
        }

        internal static void Validate(ServiceDescription serviceDescription)
        {
            Requires.Argument<Uri>("serviceDescription.ServiceName", serviceDescription.ServiceName).NotNullOrWhiteSpace();
            Requires.Argument<string>("serviceDescription.ServiceTypeName", serviceDescription.ServiceTypeName).NotNullOrWhiteSpace();
            Requires.Argument<PartitionSchemeDescription>("serviceDescription.PartitionSchemeDescription", serviceDescription.PartitionSchemeDescription).NotNull();

            foreach (var correlation in serviceDescription.Correlations)
            {
                ServiceCorrelationDescription.Validate(correlation);
            }

            foreach (var metric in serviceDescription.Metrics)
            {
                ServiceLoadMetricDescription.Validate(metric);
            }
        }

        internal static unsafe ServiceDescription CreateFromNative(IntPtr native)
        {
            ReleaseAssert.AssertIfNot(native != IntPtr.Zero, StringResources.Error_NullNativePointer);

            NativeTypes.FABRIC_SERVICE_DESCRIPTION* casted = (NativeTypes.FABRIC_SERVICE_DESCRIPTION*)native;
            switch (casted->Kind)
            {
                case NativeTypes.FABRIC_SERVICE_DESCRIPTION_KIND.FABRIC_SERVICE_DESCRIPTION_KIND_STATELESS:
                    return StatelessServiceDescription.CreateFromNative(casted->Value);
                case NativeTypes.FABRIC_SERVICE_DESCRIPTION_KIND.FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL:
                    return StatefulServiceDescription.CreateFromNative(casted->Value);
                default:
                    AppTrace.TraceSource.WriteError("ServiceDescription.CreateFromNative", "Unknown service description type {0}", casted->Kind);
                    ReleaseAssert.Failfast(string.Format(CultureInfo.CurrentCulture, StringResources.Error_UnknownServiceDescriptionType_Formatted, casted->Kind));
                    break;
            }

            return null;
        }

        internal Tuple<uint, IntPtr> ToNativeCorrelations(PinCollection pin)
        {
            if (this.Correlations.Count == 0)
            {
                return Tuple.Create((uint)0, IntPtr.Zero);
            }

            var correlationArray = new NativeTypes.FABRIC_SERVICE_CORRELATION_DESCRIPTION[this.Correlations.Count];
            for (int i = 0; i < this.Correlations.Count; i++)
            {
                this.Correlations[i].ToNative(pin, ref correlationArray[i]);
            }

            return Tuple.Create((uint)this.correlations.Count, pin.AddBlittable(correlationArray));
        }

        internal Tuple<uint, IntPtr> ToNativePolicies(PinCollection pin)
        {
            if (this.PlacementPolicies == null || this.PlacementPolicies.Count == 0)
            {
                return Tuple.Create((uint)0, IntPtr.Zero);
            }

            var policyArray = new NativeTypes.FABRIC_SERVICE_PLACEMENT_POLICY_DESCRIPTION[this.PlacementPolicies.Count];
            for (int i = 0; i < this.PlacementPolicies.Count; i++)
            {
                this.PlacementPolicies[i].ToNative(pin, ref policyArray[i]);
            }

            return Tuple.Create((uint)this.placementPolicies.Count, pin.AddBlittable(policyArray));
        }

        internal Tuple<uint, IntPtr> ToNativeScalingPolicies(PinCollection pin)
        {
            if (this.ScalingPolicies == null || this.ScalingPolicies.Count == 0)
            {
                return Tuple.Create((uint)0, IntPtr.Zero);
            }

            var scalingArray = new NativeTypes.FABRIC_SCALING_POLICY[this.ScalingPolicies.Count];
            for (int i = 0; i < this.ScalingPolicies.Count; i++)
            {
                this.ScalingPolicies[i].ToNative(pin, ref scalingArray[i]);
            }

            return Tuple.Create((uint)this.ScalingPolicies.Count, pin.AddBlittable(scalingArray));
        }

        internal NativeTypes.FABRIC_MOVE_COST ToNativeDefaultMoveCost()
        {
            NativeTypes.FABRIC_MOVE_COST nativeMoveCost = NativeTypes.FABRIC_MOVE_COST.FABRIC_MOVE_COST_LOW;

            if (this.IsDefaultMoveCostSpecified)
            {
                switch (DefaultMoveCost)
                {
                    case MoveCost.Zero:
                        nativeMoveCost = NativeTypes.FABRIC_MOVE_COST.FABRIC_MOVE_COST_ZERO;
                        break;
                    case MoveCost.Low:
                        nativeMoveCost = NativeTypes.FABRIC_MOVE_COST.FABRIC_MOVE_COST_LOW;
                        break;
                    case MoveCost.Medium:
                        nativeMoveCost = NativeTypes.FABRIC_MOVE_COST.FABRIC_MOVE_COST_MEDIUM;
                        break;
                    case MoveCost.High:
                        nativeMoveCost = NativeTypes.FABRIC_MOVE_COST.FABRIC_MOVE_COST_HIGH;
                        break;
                }
            }
            return nativeMoveCost;
        }
        
        internal unsafe void ParseLoadMetrics(uint count, IntPtr array)
        {
            if (count == 0)
            {
                return;
            }

            ReleaseAssert.AssertIfNot(array != IntPtr.Zero, StringResources.Error_NullArrayWithNonZeroSize);

            bool isStateful = (this.Kind == ServiceDescriptionKind.Stateful);

            var nativeMetrics = (NativeTypes.FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION*)array;
            for (int i = 0; i < count; i++)
            {
                var item = ServiceLoadMetricDescription.CreateFromNative((IntPtr)(nativeMetrics + i), isStateful);
                this.metrics.Add(item);
            }
        }

        internal unsafe void ParseCorrelations(uint count, IntPtr array)
        {
            if (count == 0)
            {
                return;
            }

            ReleaseAssert.AssertIfNot(array != IntPtr.Zero, StringResources.Error_NullArrayWithNonZeroSize);

            var nativeCorrelations = (NativeTypes.FABRIC_SERVICE_CORRELATION_DESCRIPTION*)array;
            for (int i = 0; i < count; i++)
            {
                var item = ServiceCorrelationDescription.CreateFromNative((IntPtr)(nativeCorrelations + i));
                this.correlations.Add(item);
            }
        }

        internal unsafe void ParsePlacementPolicies(uint count, IntPtr array)
        {
            if (count == 0)
            {
                return;
            }

            ReleaseAssert.AssertIfNot(array != IntPtr.Zero, StringResources.Error_NullArrayWithNonZeroSize);

            var nativePolicies = (NativeTypes.FABRIC_SERVICE_PLACEMENT_POLICY_DESCRIPTION*)array;
            for (int i = 0; i < count; i++)
            {
                var item = ServicePlacementPolicyDescription.CreateFromNative((IntPtr)(nativePolicies + i));
                this.placementPolicies.Add(item);
            }
        }

        internal unsafe void ParseDefaultMoveCost(NativeTypes.FABRIC_MOVE_COST defaultMoveCost)
        {
            switch (defaultMoveCost)
            {
                case NativeTypes.FABRIC_MOVE_COST.FABRIC_MOVE_COST_ZERO:
                    DefaultMoveCost = MoveCost.Zero;
                    break;
                case NativeTypes.FABRIC_MOVE_COST.FABRIC_MOVE_COST_LOW:
                    DefaultMoveCost = MoveCost.Low;
                    break;
                case NativeTypes.FABRIC_MOVE_COST.FABRIC_MOVE_COST_MEDIUM:
                    DefaultMoveCost = MoveCost.Medium;
                    break;
                case NativeTypes.FABRIC_MOVE_COST.FABRIC_MOVE_COST_HIGH:
                    DefaultMoveCost = MoveCost.High;
                    break;
            }
        }

        internal unsafe void ParseScalingPolicies(uint count, IntPtr array)
        {
            if (count == 0)
            {
                return;
            }
            var nativeScaling = (NativeTypes.FABRIC_SCALING_POLICY*)array;

            for (int i = 0; i < count; ++i)
            {
                var item = ScalingPolicyDescription.CreateFromNative((IntPtr)(nativeScaling + i));
                this.scalingPolicies.Add(item);
            }
        }
        
        internal unsafe IntPtr ToNative(PinCollection pin)
        {
            var nativeDescription = new NativeTypes.FABRIC_SERVICE_DESCRIPTION[1];

            NativeTypes.FABRIC_SERVICE_DESCRIPTION_KIND kind;
            nativeDescription[0].Value = this.ToNative(pin, out kind);
            nativeDescription[0].Kind = kind;

            return pin.AddBlittable(nativeDescription);
        }

        internal abstract IntPtr ToNative(PinCollection pin, out NativeTypes.FABRIC_SERVICE_DESCRIPTION_KIND kind);

        // Method used by JsonSerializer to resolve derived type using json property "ServiceKind".
        // The base class needs to have attributes [KnownType()]
        // This method must be static with one parameter input which represented by given json property.
        // Provide name of the json property which will be used as parameter to this method.
        [DerivedTypeResolverAttribute(JsonPropertyNames.ServiceKind)]
        internal static Type ResolveDerivedClass(ServiceDescriptionKind kind)
        {
            switch (kind)
            {
                case ServiceDescriptionKind.Stateless:
                    return typeof(StatelessServiceDescription);

                case ServiceDescriptionKind.Stateful:
                    return typeof(StatefulServiceDescription);

                default:
                    return null;
            }
        }
    }
}
