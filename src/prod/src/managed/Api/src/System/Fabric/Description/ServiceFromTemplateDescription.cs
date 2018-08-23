// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Runtime.Serialization;

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;

    /// <summary>
    /// Describes a Service Fabric service to be created from Service Template that is pre-defined in the current Application Manifest.
    /// </summary>
    public sealed class ServiceFromTemplateDescription
    {
        /// <summary>
        /// Creates an instance of <see cref="System.Fabric.Description.ServiceFromTemplateDescription"/> with specified parameter.
        /// </summary>
        /// <param name="applicationName">The Service Fabric Name of the application under which the service will be created.</param>
        /// <param name="serviceName">Name of the service to create.</param>
        /// <param name="serviceDnsName">DNS name of the service to create.</param>
        /// <param name="serviceTypeName">Name of ServiceType. This has to be same as the ServiceTypeName specified in the Service Manifest.</param>
        /// <param name="servicePackageActivationMode">
        /// <see cref="System.Fabric.Description.ServicePackageActivationMode"/> to use for service creation.
        /// </param>
        /// <param name="initializationData">Initialization data that will be passed to service.</param>
        public ServiceFromTemplateDescription(
            Uri applicationName,
            Uri serviceName,
            string serviceDnsName,
            string serviceTypeName,
            ServicePackageActivationMode servicePackageActivationMode,
            byte[] initializationData)
        {
            Requires.Argument("applicationName", applicationName).NotNull();
            Requires.Argument("serviceName", serviceName).NotNull();
            Requires.Argument("serviceTypeName", serviceTypeName).NotNull();

            this.ApplicationName = applicationName;
            this.ServiceName = serviceName;
            this.ServiceDnsName = serviceDnsName;
            this.ServiceTypeName = serviceTypeName;
            this.ServicePackageActivationMode = servicePackageActivationMode;
            this.InitializationData = initializationData;
        }

        /// <summary>
        /// The Service Fabric Name of the application under which the service will be created.
        /// </summary>
        /// <value>
        /// Returns <see cref="System.Uri" /> representing Service Fabric application name.
        /// </value>
        public Uri ApplicationName { get; private set; }

        /// <summary>
        /// Name of the service to create.
        /// </summary>
        /// <value>
        /// Returns <see cref="System.Uri" /> representing Service Fabric service name.
        /// </value>
        public Uri ServiceName { get; private set; }

        /// <summary>
        /// The DNS name of the service to create.
        /// </summary>
        /// <value>
        /// Returns <see cref="string" /> representing Service Fabric service DNS name.
        /// </value>
        public string ServiceDnsName { get; private set; }

        /// <summary>
        /// Name of the service type to create.
        /// </summary>
        /// <value>
        /// Returns <see cref="string" /> representing Service Fabric service type name.
        /// </value>
        public string ServiceTypeName { get; private set; }

        /// <summary>
        /// Gets or sets the <see cref="System.Fabric.Description.ServicePackageActivationMode"/> of service.
        /// </summary>
        /// <value>
        ///  An <see cref="System.Fabric.Description.ServicePackageActivationMode"/> enumeration.
        /// </value>
        public ServicePackageActivationMode ServicePackageActivationMode { get; private set; }

        /// <summary>
        /// Gets or sets the initialization data that will be passed to service group instances or replicas when they are created.
        /// </summary>
        /// <value>
        /// <para>Returns an array of <see cref="System.Byte" />.</para>
        /// </value>
        [JsonCustomization(IsDefaultValueIgnored = true)]
        public byte[] InitializationData { get; private set; }

        internal IntPtr ToNative(PinCollection pin)
        {
            var initializationData = NativeTypes.ToNativeBytes(pin, this.InitializationData);

            var serviceDnsName = "";
            if (!string.IsNullOrWhiteSpace(this.ServiceDnsName))
            {
                serviceDnsName = this.ServiceDnsName;
            }

            var desc = new NativeTypes.FABRIC_SERVICE_FROM_TEMPLATE_DESCRIPTION
            {
                ApplicationName = pin.AddObject(this.ApplicationName),
                ServiceName = pin.AddObject(this.ServiceName),
                ServiceDnsName = pin.AddBlittable(serviceDnsName),
                ServiceTypeName = pin.AddObject(this.ServiceTypeName),
                ServicePackageActivationMode = InteropHelpers.ToNativeServicePackageActivationMode(this.ServicePackageActivationMode),
                InitializationDataSize = initializationData.Item1,
                InitializationData = initializationData.Item2,
                Reserved = IntPtr.Zero
            };
            
            return pin.AddBlittable(desc);
        }
    }
}