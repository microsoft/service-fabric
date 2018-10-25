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
    using System.Runtime.Serialization;

    /// <summary>
    /// <para>Represents the base class for provision application type classes.
    /// The provision description can be used to provision application types using
    /// <see cref="System.Fabric.FabricClient.ApplicationManagementClient.ProvisionApplicationAsync(System.Fabric.Description.ProvisionApplicationTypeDescriptionBase, System.TimeSpan, System.Threading.CancellationToken)" />.
    /// </para>
    /// </summary>
    /// <remarks>
    /// <para>
    /// Supported types of provision operations are:
    ///�<list type="bullet">
    ///�<item><description><see cref="System.Fabric.Description.ProvisionApplicationTypeDescription"/></description></item>
    ///�<item><description><see cref="System.Fabric.Description.ExternalStoreProvisionApplicationTypeDescription"/></description></item>
    /// </list>
    /// </para>
    /// </remarks>
    [KnownType(typeof(ProvisionApplicationTypeDescription))]
    [KnownType(typeof(ExternalStoreProvisionApplicationTypeDescription))]
    public abstract class ProvisionApplicationTypeDescriptionBase
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="System.Fabric.Description.ProvisionApplicationTypeDescriptionBase"/> class.
        /// </summary>
        /// <param name="kind">The kind of the provision application type operation.</param>
        protected ProvisionApplicationTypeDescriptionBase(ProvisionApplicationTypeKind kind)
        {
            this.Kind = kind;
        }

        // Default order is -1. A lower value like -2 will cause this property to appear before others.
        /// <summary>
        /// <para>Gets the kind of the provision application type operation.</para>
        /// </summary>
        /// <value>The kind of the provision application type operation.</value>
        [JsonCustomization(AppearanceOrder = -2)]
        public ProvisionApplicationTypeKind Kind
        {
            get;
            private set;
        }

        /// <summary>
        /// <para>Gets or sets the flag indicating whether provisioning should occur asynchronously.</para>
        /// </summary>
        /// <value>
        /// <para>If this flag is false, then the behavior is equivalent to calling
        /// <see cref="System.Fabric.FabricClient.ApplicationManagementClient.ProvisionApplicationAsync(string, System.TimeSpan, System.Threading.CancellationToken)" />. The timeout argument is applied to the provision operation itself and the returned task completes only when the provision operation completes in the system.</para>
        /// <para>If this flag is true, then the timeout argument is only applied to message delivery
        /// and the returned task completes once the system has accepted the request.
        /// The system will process the provision operation without any timeout limit and its state can be queried
        /// using <see cref="System.Fabric.FabricClient.QueryClient.GetApplicationTypeListAsync()" />.
        /// The pending provision operation can be interrupted using <see cref="System.Fabric.FabricClient.ApplicationManagementClient.UnprovisionApplicationAsync(string, string)" />.</para>
        /// </value>
        public bool Async { get; set; }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var desc = new NativeTypes.FABRIC_PROVISION_APPLICATION_TYPE_DESCRIPTION_BASE();
            desc.Kind = (NativeTypes.FABRIC_PROVISION_APPLICATION_TYPE_KIND)this.Kind;
            desc.Value = this.ToNativeValue(pinCollection);

            return pinCollection.AddBlittable(desc);
        }

        internal abstract IntPtr ToNativeValue(PinCollection pinCollection);
    }
}
