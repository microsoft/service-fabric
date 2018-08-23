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
    /// <para>Describes an service to be deleted by using 
    /// <see cref="System.Fabric.FabricClient.ServiceManagementClient.DeleteServiceAsync(System.Fabric.Description.DeleteServiceDescription, TimeSpan, System.Threading.CancellationToken)" />.</para>
    /// </summary>
    public sealed class DeleteServiceDescription
    {
        /// <summary>
        /// <para>Instantiates an instance of <see cref="System.Fabric.Description.DeleteServiceDescription" />. </para>
        /// </summary>
        /// <param name="serviceName">
        /// <para>URI of the service instance name.</para>
        /// </param>
        public DeleteServiceDescription(Uri serviceName)
        {
            Requires.Argument<Uri>("serviceName", serviceName).NotNull();
            this.ServiceName = serviceName;
            this.ForceDelete = false;
        }

        /// <summary>
        /// <para>Gets the URI name of the service instance.</para>
        /// </summary>
        /// <value>
        /// <para>The service name.</para>
        /// </value>
        public Uri ServiceName { get; private set; }

        /// <summary>
        /// <para>Gets the flag that specifies whether the service should be given a chance to gracefully clean up its state and close.</para>
        /// </summary>
        /// <value>
        /// <para>Flag that specifies whether the service should be given a chance to gracefully clean up its state and close.</para>
        /// <para>If the ForceDelete flag is set then the service won't be closed gracefully and stateful services may leak persisted state.</para>
        /// </value>
        public bool ForceDelete { get; set; }

        internal IntPtr ToNative(PinCollection pin)
        {          
            var desc = new NativeTypes.FABRIC_SERVICE_DELETE_DESCRIPTION();
            desc.ServiceName = pin.AddObject(ServiceName);
            desc.ForceDelete = NativeTypes.ToBOOLEAN(ForceDelete);
            desc.Reserved = IntPtr.Zero;

            return pin.AddBlittable(desc);
        }
    }
}