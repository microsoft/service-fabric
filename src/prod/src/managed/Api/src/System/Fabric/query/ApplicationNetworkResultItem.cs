// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System;    
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;    

    /// <summary>
    /// <para>Describes a container network that an application is a member of.</para>
    /// </summary>    
    public sealed class ApplicationNetwork
    {
        internal ApplicationNetwork()
        {            
        }

        /// <summary>
        /// <para>Gets the name of the network.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the network.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.NetworkName)]
        public string NetworkName { get; internal set; }

        internal static unsafe ApplicationNetwork CreateFromNative(NativeTypes.FABRIC_APPLICATION_NETWORK_QUERY_RESULT_ITEM nativeApplicationNetwork)
        {
            ApplicationNetwork applicationNetwork = new ApplicationNetwork();

            applicationNetwork.NetworkName = NativeTypes.FromNativeString(nativeApplicationNetwork.NetworkName);

            return applicationNetwork;
        }
    }
}
