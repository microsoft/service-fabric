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
    using System.Runtime.Serialization;

    /// <summary>
    /// <para>Describes the base class for network descriptions.</para>
    /// </summary>
    [KnownType(typeof(LocalNetworkDescription))]
    public abstract class NetworkDescription
    {
        /// <summary>
        /// <para>Base class constructor used by specific network description classes.</para>
        /// </summary>
        protected NetworkDescription(NetworkType networkType)
        {
            this.NetworkType = networkType;
        }

        /// <summary>
        /// <para>Gets type of the container network.</para>
        /// </summary>
        /// <value>
        /// <para>Type of the container network.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.NetworkType, AppearanceOrder = -2)]
        public NetworkType NetworkType { get; private set; }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            NativeTypes.FABRIC_NETWORK_TYPE networkType;
            NativeTypes.FABRIC_NETWORK_DESCRIPTION networkDescription;

            networkDescription.Value = this.ToNative(pinCollection, out networkType);
            networkDescription.NetworkType = networkType;

            return pinCollection.AddBlittable(networkDescription);
        }

        internal abstract IntPtr ToNative(PinCollection pin, out NativeTypes.FABRIC_NETWORK_TYPE networkType);
        
        // Method used by JsonSerializer to resolve derived type using json property "NetworkType".
        // Provide name of the json property which will be used as parameter to call this method.
        [DerivedTypeResolverAttribute("NetworkType")]
        internal static Type ResolveDerivedClass(NetworkType networkType)
        {
            switch (networkType)
            {
                case NetworkType.Local:
                    return typeof(LocalNetworkDescription);

                default:
                    return null;
            }
        }

        internal static void Validate(NetworkDescription description)
        {
            switch (description.NetworkType)
            {
                case NetworkType.Local:
                    LocalNetworkDescription.Validate(description as LocalNetworkDescription);
                    return;

                default:
                    return;
            }
        }
    }
}
