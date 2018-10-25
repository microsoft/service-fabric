// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Common;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Health;
    using System.Fabric.Interop;
    using System.Runtime.Serialization;

    /// <summary>
    /// <para>Specifies the service kind.</para>
    /// </summary>
    public enum ServiceKind
    {
        /// <summary>
        /// <para>Invalid.</para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_SERVICE_KIND.FABRIC_SERVICE_KIND_INVALID,
        
        /// <summary>
        /// <para>Does not use Service Fabric to make its state highly available or reliable.</para>
        /// </summary>
        Stateless = NativeTypes.FABRIC_SERVICE_KIND.FABRIC_SERVICE_KIND_STATELESS,
        
        /// <summary>
        /// <para>Uses Service Fabric to make its state or part of its state highly available and reliable.</para>
        /// </summary>
        Stateful = NativeTypes.FABRIC_SERVICE_KIND.FABRIC_SERVICE_KIND_STATEFUL
    }

    /// <summary>
    /// <para>Represents a service.</para>
    /// </summary>
    [KnownType(typeof(StatelessService))]
    [KnownType(typeof(StatefulService))]
    public abstract class Service
    {
        internal Service(
            ServiceKind kind,
            Uri serviceName,
            string serviceTypeName,
            string serviceManifestVersion,
            HealthState healthState,
            ServiceStatus serviceStatus,
            bool isServiceGroup = false)
        {
            this.ServiceKind = kind;
            this.ServiceName = serviceName;
            this.ServiceTypeName = serviceTypeName;
            this.ServiceManifestVersion = serviceManifestVersion;
            this.HealthState = healthState;
            this.ServiceStatus = serviceStatus;
            this.IsServiceGroup = isServiceGroup;
        }

        // Should be first json property.
        /// <summary>
        /// <para>Gets the service kind.</para>
        /// </summary>
        /// <value>
        /// <para>The service kind.</para>
        /// </value>
        [JsonCustomization(AppearanceOrder = -2)]
        public ServiceKind ServiceKind { get; private set; }

        [JsonCustomization(PropertyName = JsonPropertyNames.Id, IsIgnored = true)]
        private string ServiceId { get; set; }

        /// <summary>
        /// <para>Gets the service name.</para>
        /// </summary>
        /// <value>
        /// <para>The service name.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.Name)]
        public Uri ServiceName { get; private set; }

        /// <summary>
        /// <para>Gets the service type name.</para>
        /// </summary>
        /// <value>
        /// <para>The service type name.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.TypeName)]
        public string ServiceTypeName { get; private set; }

        /// <summary>
        /// <para>Gets the service manifest version.</para>
        /// </summary>
        /// <value>
        /// <para>The service manifest version.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.ManifestVersion)]
        public string ServiceManifestVersion { get; private set; }

        /// <summary>
        /// <para>Gets the health state.</para>
        /// </summary>
        /// <value>
        /// <para>The health state.</para>
        /// </value>
        public HealthState HealthState { get; private set; }

        /// <summary>
        /// <para>Gets the status of the service.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Fabric.Query.ServiceStatus" />.</para>
        /// </value>
        public ServiceStatus ServiceStatus { get; private set; }

        /// <summary>
        /// <para>Flag indicates if this service is a regular service or a service group.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Boolean" />.</para>
        /// </value>
        public bool IsServiceGroup { get; private set; }

        internal static unsafe Service CreateFromNative(
           NativeTypes.FABRIC_SERVICE_QUERY_RESULT_ITEM nativeResultItem)
        {
            if ((ServiceKind)nativeResultItem.Kind == ServiceKind.Stateless)
            {
                NativeTypes.FABRIC_STATELESS_SERVICE_QUERY_RESULT_ITEM nativeStatelessServiceQueryResult =
                    *(NativeTypes.FABRIC_STATELESS_SERVICE_QUERY_RESULT_ITEM*)nativeResultItem.Value;

                return StatelessService.FromNative(nativeStatelessServiceQueryResult);
            }
            else if ((ServiceKind)nativeResultItem.Kind == ServiceKind.Stateful)
            {
                NativeTypes.FABRIC_STATEFUL_SERVICE_QUERY_RESULT_ITEM nativeStatefulServiceQueryResult =
                    *(NativeTypes.FABRIC_STATEFUL_SERVICE_QUERY_RESULT_ITEM*)nativeResultItem.Value;

                return StatefulService.FromNative(nativeStatefulServiceQueryResult);
            }
            else
            {
                AppTrace.TraceSource.WriteNoise(
                    "Service.CreateFromNative",
                    "Ignoring the result with unsupported ServiceKind value {0}",
                    (int)nativeResultItem.Kind);
                return null;
            }
        }

        // Method used by JsonSerializer to resolve derived type using json property "ServiceKind".
        // This method must be static with one parameter input which represented by given json property.
        // Provide name of the json property which will be used as parameter to this method.
        [DerivedTypeResolverAttribute(JsonPropertyNames.ServiceKind)]
        internal static Type ResolveDerivedClass(ServiceKind kind)
        {
            switch (kind)
            {
                case ServiceKind.Stateless:
                    return typeof(StatelessService);

                case ServiceKind.Stateful:
                    return typeof(StatefulService);

                default:
                    return null;
            }
        }
    }
}