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
    /// <para>Specifies the configuration of a local container network described by <see cref="System.Fabric.Description.LocalNetworkDescription" /></para>
    /// </summary>
    public sealed class LocalNetworkConfigurationDescription
    {
        /// <summary>
        /// <para>Instantiates an instance of <see cref="System.Fabric.Description.LocalNetworkConfigurationDescription" />.</para>
        /// </summary>
        public LocalNetworkConfigurationDescription() : this(null /* networkAddressPrefix */)
        {
        }


        /// <summary>
        /// <para>Instantiates an instance of <see cref="System.Fabric.Description.LocalNetworkConfigurationDescription" /> with network address prefix.</para>
        /// </summary>
        /// <param name="networkAddressPrefix">
        /// <para>Address prefix of the cotnainer network.</para>
        /// </param>
        public LocalNetworkConfigurationDescription(string networkAddressPrefix)
        {
            this.NetworkAddressPrefix = networkAddressPrefix;
        }

        /// <summary>
        /// <para>Gets or sets the network address prefix.</para>
        /// </summary>
        /// <value>
        /// <para>Address prefix of the container network.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.NetworkAddressPrefix)]
        public string NetworkAddressPrefix { get; set; }

        internal static void Validate(LocalNetworkConfigurationDescription description)
        {
            Requires.Argument<string>("NetworkAddressPrefix", description.NetworkAddressPrefix).NotNullOrWhiteSpace();

            // TODO: validation of the CIDR notation
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeDescription = new NativeTypes.FABRIC_LOCAL_NETWORK_CONFIGURATION_DESCRIPTION();
            nativeDescription.NetworkAddressPrefix = pinCollection.AddObject(this.NetworkAddressPrefix);
            nativeDescription.Reserved = IntPtr.Zero;            
            return pinCollection.AddBlittable(nativeDescription);
        }

        internal static unsafe LocalNetworkConfigurationDescription FromNative(IntPtr descriptionPtr)
        {
            if (descriptionPtr == IntPtr.Zero) { return null; }

            var castedPtr = (NativeTypes.FABRIC_LOCAL_NETWORK_CONFIGURATION_DESCRIPTION*)descriptionPtr;

            var description = new LocalNetworkConfigurationDescription();

            description.NetworkAddressPrefix = NativeTypes.FromNativeString(castedPtr->NetworkAddressPrefix);

            return description;
        }
    }
}
