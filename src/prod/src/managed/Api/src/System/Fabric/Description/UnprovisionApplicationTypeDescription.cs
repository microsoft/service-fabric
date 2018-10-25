// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Collections.Generic;
    using System.Collections.Specialized;
    using System.Globalization;
    using System.Fabric.Common;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;
    using System.Fabric.Strings;

    /// <summary>
    /// <para>Describes an application type to be unprovisioned by using 
    /// <see cref="System.Fabric.FabricClient.ApplicationManagementClient.UnprovisionApplicationAsync(System.Fabric.Description.UnprovisionApplicationTypeDescription, TimeSpan, System.Threading.CancellationToken)" />.</para>
    /// </summary>
    public sealed class UnprovisionApplicationTypeDescription
    {
        /// <summary>
        /// <para>Creates an instance of <see cref="System.Fabric.Description.UnprovisionApplicationTypeDescription" />
        /// used to unprovision an application type previously provisioned using
        /// <see cref="System.Fabric.FabricClient.ApplicationManagementClient.ProvisionApplicationAsync(System.Fabric.Description.ProvisionApplicationTypeDescriptionBase)"/>.</para>
        /// </summary>
        /// <param name="applicationTypeName">
        /// <para>The application type name to unprovision</para>
        /// </param>
        /// <param name="applicationTypeVersion">
        /// <para>The application type version to unprovision</para>
        /// </param>
        /// <remarks><para>
        /// The application type version and name are defined in the application manifest.
        /// They can be obtained using <see cref="System.Fabric.FabricClient.QueryClient.GetApplicationTypePagedListAsync()"/>.
        /// </para></remarks>
        public UnprovisionApplicationTypeDescription(string applicationTypeName, string applicationTypeVersion)
        {
            Requires.Argument<string>("applicationTypeName", applicationTypeName).NotNullOrWhiteSpace();
            Requires.Argument<string>("applicationTypeVersion", applicationTypeVersion).NotNullOrWhiteSpace();

            this.ApplicationTypeName = applicationTypeName;
            this.ApplicationTypeVersion = applicationTypeVersion;
            this.Async = false;
        }
        
        /// <summary>
        /// <para>Gets the application type name to unprovision.</para>
        /// </summary>
        /// <value>
        /// <para>The application type name.</para>
        /// </value>
        /// <remarks><para>
        /// The application type name represents the name of the application type found in the application manifest.
        /// </para></remarks>
        public string ApplicationTypeName { get; private set; }

        /// <summary>
        /// <para>Gets the application type version to unprovision.</para>
        /// </summary>
        /// <value>
        /// <para>The application type version.</para>
        /// </value>
        /// <remarks><para>
        /// The application type version represents the version of the application type found in the application manifest.
        /// </para></remarks>
        public string ApplicationTypeVersion { get; private set; }
         
        /// <summary>
        /// <para>Gets or sets the flag indicating whether unprovisioning should occur asynchronously.</para>
        /// </summary>
        /// <value>
        /// <para>If this flag is false, then the behavior is equivalent to calling <see cref="System.Fabric.FabricClient.ApplicationManagementClient.UnprovisionApplicationAsync(string, string, TimeSpan, System.Threading.CancellationToken)" />. The timeout argument is applied to the unprovision operation itself and the returned task completes only when the unprovision operation completes in the system.</para>
        /// <para>If this flag is true, then the timeout argument is only applied to message delivery and the returned task completes once the system has accepted the request. The system will process the unprovision operation without any timeout limit and its state can be queried using <see cref="System.Fabric.FabricClient.QueryClient.GetApplicationTypeListAsync()" />.</para>
        /// </value>
        public bool Async { get; set; }

        internal IntPtr ToNative(PinCollection pin)
        {          
            var desc = new NativeTypes.FABRIC_UNPROVISION_APPLICATION_TYPE_DESCRIPTION[1];
            desc[0].ApplicationTypeName = pin.AddBlittable(this.ApplicationTypeName);
            desc[0].ApplicationTypeVersion = pin.AddBlittable(this.ApplicationTypeVersion);
            desc[0].Async = NativeTypes.ToBOOLEAN(this.Async);
            desc[0].Reserved = IntPtr.Zero;
            
            return pin.AddBlittable(desc);
        }
    }
}

