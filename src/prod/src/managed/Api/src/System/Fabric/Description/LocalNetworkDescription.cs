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
    /// <para>Describes a local container network</para>
    /// </summary>
    public sealed class LocalNetworkDescription : NetworkDescription
    {
        /// <summary>
        /// <para>Instantiates an instance of <see cref="System.Fabric.Description.LocalNetworkDescription" />.</para>
        /// </summary>
        public LocalNetworkDescription() : this(null /* networkConfigurationDescription */)
        {
        }

        /// <summary>
        /// <para>Instantiates an instance of <see cref="System.Fabric.Description.LocalNetworkDescription" />.</para>
        /// </summary>
        public LocalNetworkDescription(LocalNetworkConfigurationDescription networkConfigurationDescription) :
            base(NetworkType.Local)
        {
            this.NetworkConfiguration = networkConfigurationDescription;
        }

        /// <summary>
        /// <para>Gets or sets configuration of the container network.</para>
        /// </summary>
        /// <value>
        /// <para>Configuration of the container network.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.NetworkConfiguration)]
        public LocalNetworkConfigurationDescription NetworkConfiguration { get; set; }

        internal static void Validate(LocalNetworkDescription description)
        {
            Requires.Argument<LocalNetworkConfigurationDescription>("NetworkConfiguration", description.NetworkConfiguration).NotNull();
            LocalNetworkConfigurationDescription.Validate(description.NetworkConfiguration);
        }

        internal override IntPtr ToNative(PinCollection pinCollection, out NativeTypes.FABRIC_NETWORK_TYPE networkType)
        {
            networkType = NativeTypes.FABRIC_NETWORK_TYPE.FABRIC_NETWORK_TYPE_LOCAL;

            var nativeDescription = new NativeTypes.FABRIC_LOCAL_NETWORK_DESCRIPTION();
            nativeDescription.NetworkConfiguration = this.NetworkConfiguration.ToNative(pinCollection);
            nativeDescription.Reserved = IntPtr.Zero;

            return pinCollection.AddBlittable(nativeDescription);
        }
    }
}
