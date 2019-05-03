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

    /// <summary>
    /// <para>Describes information of a local container network</para>
    /// </summary>
    public sealed class LocalNetworkInformation : NetworkInformation
    {
        internal LocalNetworkInformation() : this(null /* networkName */, null /* networkConfigurationDescription */, NetworkStatus.Invalid /* networkStatus */)
        {
        }

        internal LocalNetworkInformation(string networkName, LocalNetworkConfigurationDescription networkConfigurationDescription, NetworkStatus networkStatus) :
            base(NetworkType.Local)
        {
            this.NetworkName = networkName;
            this.NetworkConfiguration = networkConfigurationDescription;
            this.NetworkStatus = networkStatus;
        }

        /// <summary>
        /// <para>Gets name of the container network.</para>
        /// </summary>
        /// <value>
        /// <para>Name of the container network.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.NetworkName)]
        public string NetworkName { get; internal set; }

        /// <summary>
        /// <para>Gets configuration of the container network.</para>
        /// </summary>
        /// <value>
        /// <para>Configuration of the container network.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.NetworkConfiguration)]
        public LocalNetworkConfigurationDescription NetworkConfiguration { get; internal set; }

        /// <summary>
        /// <para>Gets status of the container network.</para>
        /// </summary>
        /// <value>
        /// <para>Status of the container network.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.NetworkStatus)]
        public NetworkStatus NetworkStatus { get; internal set; }
    }
}
