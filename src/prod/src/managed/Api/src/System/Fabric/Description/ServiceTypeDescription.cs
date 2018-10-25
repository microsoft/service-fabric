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
    using System.Runtime.InteropServices;
    using System.Runtime.Serialization;

    /// <summary>
    /// <para>Describes the service type.</para>
    /// </summary>
    [KnownType(typeof(StatelessServiceTypeDescription))]
    [KnownType(typeof(StatefulServiceTypeDescription))]
    public abstract class ServiceTypeDescription
    {
        private readonly KeyedItemCollection<string, ServiceLoadMetricDescription> loadMetrics = new KeyedItemCollection<string, ServiceLoadMetricDescription>(e => e.Name);

        private Dictionary<string, string> extensions = new Dictionary<string, string>();

        /// <summary>
        /// <para>
        /// Instantiates a <see cref="System.Fabric.Description.ServiceTypeDescription" /> class with specified service description kind.
        /// </para>
        /// </summary>
        /// <param name="kind">
        /// <para>The service description kind.</para>
        /// </param>
        protected internal ServiceTypeDescription(ServiceDescriptionKind kind)
        {
            this.ServiceTypeKind = kind;

            // Needs to be initialized as empty list as native serializer does not work well with null.
            this.Policies = new List<ServicePlacementPolicyDescription>();
        }

        /// <summary>
        /// <para>
        /// Instantiates a <see cref="System.Fabric.Description.ServiceTypeDescription" /> class with parameters from another 
        /// <see cref="System.Fabric.Description.ServiceTypeDescription" /> object.
        /// </para>
        /// </summary>
        /// <param name="other">
        /// <para>The service type description from which parameters are copied.</para>
        /// </param>
        protected internal ServiceTypeDescription(ServiceTypeDescription other)
            : this(other.ServiceTypeKind)
        {
            this.CopyFrom(other);
        }

        private ServiceTypeDescription()
        {
        }

        /// <summary>
        /// <para>Gets or sets the name of the service type.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the service type.</para>
        /// </value>
        public string ServiceTypeName
        {
            get;
            set;
        }

        /// <summary>
        /// <para>Gets or sets the kind of service type.</para>
        /// </summary>
        /// <value>
        /// <para>The kind of service type.</para>
        /// </value>
        [JsonCustomization(IsIgnored = true)]
        public ServiceDescriptionKind ServiceTypeKind
        {
            get;
            set;
        }

        /// <summary>
        /// <para>Gets or sets the constraint to be used when instantiating this service in a Service Fabric cluster.</para>
        /// </summary>
        /// <value>
        /// <para>The constraint to be used when instantiating this service in a Service Fabric cluster.</para>
        /// </value>
        public string PlacementConstraints
        {
            get;
            set;
        }

        /// <summary>
        /// <para>Gets the type of load metric is reported by the service.</para>
        /// </summary>
        /// <value>
        /// <para>The type of load metric is reported by the service.</para>
        /// </value>
        public KeyedCollection<string, ServiceLoadMetricDescription> LoadMetrics
        {
            get
            {
                return this.loadMetrics;
            }
        }

        /// <summary>
        /// <para>Gets the extensions for the service type.</para>
        /// </summary>
        /// <value>
        /// <para>The extensions for the service type.</para>
        /// </value>
        [JsonCustomization(IsIgnored = true)]
        public IDictionary<string, string> Extensions
        {
            get
            {
                return this.extensions;
            }
        }


        // Native serializer convert dictionary to JsonArray like [{key: "k", value: "v"}]
        // rather than {key: value} so this is needed to serialize this as JsonArray.
        // ObjectCreationHandling.Replace will cause serializer to use setter while deserializing this property.

        /// <summary>
        /// INTERNAL USE ONLY. Wrapper of property "Extensions". Needed for serialization.
        /// </summary>
        /// <value>
        /// INTERNAL USE ONLY. Wrapper of property "Extensions". Needed for serialization.
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.Extensions, ReCreateMember = true)]
        protected internal IList<KeyValuePair<string, string>> Extensions_
        {
            get
            {
                return this.Extensions.ToList();
            }

            set
            {
                this.Extensions.Clear();
                this.Extensions.AddRange(value);
            }
        }

        /// <summary>
        /// <para>Gets the policies of the service type.</para>
        /// </summary>
        /// <value>
        /// <para>The policies of the service type.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.ServicePlacementPolicies)]
        public List<ServicePlacementPolicyDescription> Policies { get; set; }

        /// <summary>
        /// <para>
        /// Indicates whether the service is stateful.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>Flag indicating whether the service is stateful.</para>
        /// </value>
        /// <remarks>
        /// <para>Exposed by REST API and native code.</para>
        /// </remarks>
        protected internal bool IsStateful
        {
            get
            {
                return this.ServiceTypeKind == ServiceDescriptionKind.Stateful;
            }
        }

        private void CopyFrom(ServiceTypeDescription other)
        {
            // Copy basic types
            this.PlacementConstraints = other.PlacementConstraints;
            this.ServiceTypeName = other.ServiceTypeName;

            // Create copies of reference types
            this.Policies.AddRange(other.Policies.Select(p => p.GetCopy()));

            ServiceLoadMetricDescription.CopyFrom(other.LoadMetrics, this.LoadMetrics);

            this.extensions = other.Extensions == null ? this.extensions : new Dictionary<string, string>(other.Extensions);
        }

        internal static unsafe bool TryCreateFromNative(IntPtr descriptionPtr, out ServiceTypeDescription serviceTypeDescription)
        {
            serviceTypeDescription = null;

            var nativeDescription = (NativeTypes.FABRIC_SERVICE_TYPE_DESCRIPTION*)descriptionPtr;
            switch (nativeDescription->Kind)
            {
                case NativeTypes.FABRIC_SERVICE_KIND.FABRIC_SERVICE_KIND_STATELESS:
                    serviceTypeDescription = StatelessServiceTypeDescription.CreateFromNative(nativeDescription->Value);
                    break;

                case NativeTypes.FABRIC_SERVICE_KIND.FABRIC_SERVICE_KIND_STATEFUL:
                    serviceTypeDescription = StatefulServiceTypeDescription.CreateFromNative(nativeDescription->Value);
                    break;

                default:
                    AppTrace.TraceSource.WriteNoise(
                        "ServiceTypeDescription.TryCreateFromNative",
                        "Ignoring the result with unsupported ServiceKind value {0}",
                        (int)nativeDescription->Kind);
                    break;
            }

            return (serviceTypeDescription != null);
        }

        internal static unsafe ServiceTypeDescription CreateFromNative(IntPtr descriptionPtr)
        {
            ServiceTypeDescription serviceTypeDescription;
            if (!TryCreateFromNative(descriptionPtr, out serviceTypeDescription))
            {
                var nativeDescription = (NativeTypes.FABRIC_SERVICE_TYPE_DESCRIPTION*)descriptionPtr;
                ReleaseAssert.Failfast(string.Format(CultureInfo.CurrentCulture, StringResources.Error_ServiceKindInvalid_Formatted, nativeDescription->Kind));
                throw new ArgumentException(StringResources.Error_InvalidServiceType);
            }

            return serviceTypeDescription;
        }

        /// <summary>
        /// <para>Reads the properties of the service type.</para>
        /// </summary>
        /// <param name="serviceTypeName">
        /// <para>The name of the service type.</para>
        /// </param>
        /// <param name="placementConstraints">
        /// <para>The constraints to be used.</para>
        /// </param>
        /// <param name="loadMetricsList">
        /// <para>The type of load metric.</para>
        /// </param>
        /// <param name="descriptionExtensionList">
        /// <para>The description extension list.</para>
        /// </param>
        protected internal unsafe void ReadCommonProperties(
            IntPtr serviceTypeName,
            IntPtr placementConstraints,
            IntPtr loadMetricsList,
            IntPtr descriptionExtensionList)
        {
            this.ServiceTypeName = NativeTypes.FromNativeString(serviceTypeName);
            this.PlacementConstraints = NativeTypes.FromNativeString(placementConstraints);

            AppTrace.TraceSource.WriteNoise("ServiceTypeDescription.ReadCommonProperties", "Type {0}", this.ServiceTypeName);

            bool isStateful = (this.ServiceTypeKind == ServiceDescriptionKind.Stateful);

            if (loadMetricsList != IntPtr.Zero)
            {
                NativeTypes.FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION_LIST* nativeMetrics = (NativeTypes.FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION_LIST*)loadMetricsList;

                for (int i = 0; i < nativeMetrics->Count; i++)
                {
                    this.loadMetrics.Add(ServiceLoadMetricDescription.CreateFromNative(nativeMetrics->Items + (i * Marshal.SizeOf(typeof(NativeTypes.FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION))), isStateful));
                }
            }

            if (descriptionExtensionList != IntPtr.Zero)
            {
                NativeTypes.FABRIC_SERVICE_TYPE_DESCRIPTION_EXTENSION_LIST* nativeDescriptions = (NativeTypes.FABRIC_SERVICE_TYPE_DESCRIPTION_EXTENSION_LIST*)descriptionExtensionList;
                for (int i = 0; i < nativeDescriptions->Count; i++)
                {
                    Tuple<string, string> extension = DescriptionExtension.CreateFromNative(nativeDescriptions->Items + (i * Marshal.SizeOf(typeof(NativeTypes.FABRIC_SERVICE_TYPE_DESCRIPTION_EXTENSION))));
                    this.extensions.Add(extension.Item1, extension.Item2);
                }
            }
        }

        internal unsafe void ParsePlacementPolicies(uint count, IntPtr array)
        {
            this.Policies = new List<ServicePlacementPolicyDescription>();

            if (count == 0)
            {
                return;
            }

            ReleaseAssert.AssertIfNot(array != IntPtr.Zero, StringResources.Error_NullArrayWithNonZeroSize);

            var nativePolicies = (NativeTypes.FABRIC_SERVICE_PLACEMENT_POLICY_DESCRIPTION*)array;
            for (int i = 0; i < count; i++)
            {
                var item = ServicePlacementPolicyDescription.CreateFromNative((IntPtr)(nativePolicies + i));
                this.Policies.Add(item);
            }
        }

        [DerivedTypeResolverAttribute("IsStateful")]
        internal static Type ResolveDerivedClass(bool isStateful)
        {
            if (isStateful)
            {
                return typeof(StatefulServiceTypeDescription);
            }

            return typeof(StatelessServiceTypeDescription);
        }
    }
}