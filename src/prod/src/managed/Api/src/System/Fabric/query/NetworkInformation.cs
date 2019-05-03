// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System;    
    using System.Fabric.Common.Serialization;
    using System.Fabric.Description;
    using System.Fabric.Interop;
    using System.Runtime.Serialization;

    /// <summary>
    /// <para>Describes the base class for network information.</para>
    /// </summary>
    [KnownType(typeof(LocalNetworkInformation))]
    public abstract class NetworkInformation
    {
        internal NetworkInformation(NetworkType networkType)
        {
            this.NetworkType = networkType;
        }

        /// <summary>
        /// <para>Gets type of the container network.</para>
        /// </summary>
        /// <value>
        /// <para>Type of the container network.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.NetworkType)]
        public NetworkType NetworkType { get; private set; }

        internal static unsafe NetworkInformation CreateFromNative(NativeTypes.FABRIC_NETWORK_INFORMATION nativeNetworkInformation)
        {
            if (nativeNetworkInformation.NetworkType == NativeTypes.FABRIC_NETWORK_TYPE.FABRIC_NETWORK_TYPE_LOCAL)
            {
                var nativeLocalNetworkInformation = (NativeTypes.FABRIC_LOCAL_NETWORK_INFORMATION*)nativeNetworkInformation.Value;

                var localNetworkInformation = new LocalNetworkInformation();
                localNetworkInformation.NetworkName = NativeTypes.FromNativeString(nativeLocalNetworkInformation->NetworkName);
                localNetworkInformation.NetworkStatus = (NetworkStatus)nativeLocalNetworkInformation->NetworkStatus;
                localNetworkInformation.NetworkConfiguration = LocalNetworkConfigurationDescription.FromNative(nativeLocalNetworkInformation->NetworkConfiguration);

                return localNetworkInformation;
            }

            return null;
        }
    }
}
