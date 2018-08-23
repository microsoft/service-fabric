// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;

    /// <summary>
    /// Describes a Service Group to be created from a Service Group Template that is pre-defined in the current Application Manifest.
    /// </summary>
    public sealed class ServiceGroupFromTemplateDescription
    {
        /// <summary>
        /// Creates an instance of <see cref="System.Fabric.Description.ServiceGroupFromTemplateDescription"/> with specified parameter.
        /// </summary>
        /// <param name="applicationName">Application name for the Service Group.</param>
        /// <param name="serviceName">>Service name for the Service Group.</param>
        /// <param name="serviceTypeName">Service Type Name for the Service Group.</param>
        /// <param name="servicePackageActivationMode">
        /// <see cref="System.Fabric.Description.ServicePackageActivationMode"/> to use for service group creation.
        /// </param>
        /// <param name="initializationData">Initialization data that will be passed to Service Group.</param>
        public ServiceGroupFromTemplateDescription(
            Uri applicationName,
            Uri serviceName,
            string serviceTypeName,
            ServicePackageActivationMode servicePackageActivationMode,
            byte[] initializationData)
        {
            Requires.Argument("applicationName", applicationName).NotNull();
            Requires.Argument("serviceName", serviceName).NotNull();
            Requires.Argument("serviceTypeName", serviceTypeName).NotNullOrWhiteSpace();

            this.ApplicationName = applicationName;
            this.ServiceName = serviceName;
            this.ServiceTypeName = serviceTypeName;
            this.ServicePackageActivationMode = servicePackageActivationMode;
            this.InitializationData = initializationData;
        }

        /// <summary>
        /// Name of the application to which service group belongs.
        /// </summary>
        /// <value>
        /// The <see cref="System.Uri" /> representing Service Fabric application name.
        /// </value>
        public Uri ApplicationName { get; private set; }

        /// <summary>
        /// Name of the service group to create.
        /// </summary>
        /// <value>
        /// The <see cref="System.Uri" /> representing Service Fabric service group name.
        /// </value>
        public Uri ServiceName { get; private set; }

        /// <summary>
        /// Name of the service to create.
        /// </summary>
        /// <value>
        /// The <see cref="string" /> representing Service Fabric service type name.
        /// </value>
        public string ServiceTypeName { get; private set; }

        /// <summary>
        /// Gets or sets the <see cref="System.Fabric.Description.ServicePackageActivationMode"/> of service group.
        /// </summary>
        /// <value>
        /// A <see cref="System.Fabric.Description.ServicePackageActivationMode"/> enumeration.
        /// </value>
        [JsonCustomization(IsDefaultValueIgnored = true)]
        public ServicePackageActivationMode ServicePackageActivationMode { get; private set; }

        /// <summary>
        /// <para> Gets or sets the initialization data that will be passed to service instances or replicas when they are created. </para>
        /// </summary>
        /// <value>
        /// <para>Returns an array of <see cref="System.Byte" />.</para>
        /// </value>
        [JsonCustomization(IsDefaultValueIgnored = true)]
        public byte[] InitializationData { get; private set; }

        internal IntPtr ToNative(PinCollection pin)
        {
            var initializationData = NativeTypes.ToNativeBytes(pin, this.InitializationData);

            var desc = new NativeTypes.FABRIC_SERVICE_GROUP_FROM_TEMPLATE_DESCRIPTION
            {
                ApplicationName = pin.AddObject(this.ApplicationName),
                ServiceName = pin.AddObject(this.ServiceName),
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