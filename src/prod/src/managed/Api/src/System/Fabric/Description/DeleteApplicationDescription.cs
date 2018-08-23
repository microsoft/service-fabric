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
    /// <para>Describes an application to be deleted by using 
    /// <see cref="System.Fabric.FabricClient.ApplicationManagementClient.DeleteApplicationAsync(System.Fabric.Description.DeleteApplicationDescription, TimeSpan, System.Threading.CancellationToken)" />.</para>
    /// </summary>
    public sealed class DeleteApplicationDescription
    {
        /// <summary>
        /// <para>Instantiates an instance of <see cref="System.Fabric.Description.DeleteApplicationDescription" />. </para>
        /// </summary>
        /// <param name="applicationName">
        /// <para>URI of the application instance name.</para>
        /// </param>
        public DeleteApplicationDescription(Uri applicationName)
        {
            Requires.Argument<Uri>("applicationName", applicationName).NotNull();
            this.ApplicationName = applicationName;
            this.ForceDelete = false;
        }

        /// <summary>
        /// <para>Gets the URI name of the application instance.</para>
        /// </summary>
        /// <value>
        /// <para>The application name.</para>
        /// </value>
        public Uri ApplicationName { get; private set; }

        /// <summary>
        /// <para>Gets or sets the flag that specifies whether the application should be given a chance to gracefully clean up its state and close.</para>
        /// </summary>
        /// <value>
        /// <para>Flag that specifies whether the application should be given a chance to gracefully clean up its state and close.</para>
        /// <para>If the ForceDelete flag is set then the application won't be closed gracefully and stateful services in it may leak persisted state.</para>
        /// </value>
        public bool ForceDelete { get; set; }

        internal IntPtr ToNative(PinCollection pin)
        {          
            var desc = new NativeTypes.FABRIC_APPLICATION_DELETE_DESCRIPTION();
            desc.ApplicationName = pin.AddObject(ApplicationName);
            desc.ForceDelete = NativeTypes.ToBOOLEAN(ForceDelete);
            desc.Reserved = IntPtr.Zero;

            return pin.AddBlittable(desc);
        }
    }
}