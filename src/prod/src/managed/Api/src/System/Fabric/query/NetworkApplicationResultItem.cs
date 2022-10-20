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
    /// <para>Describes an application in a container network.</para>
    /// </summary>    
    public sealed class NetworkApplication
    {
        internal NetworkApplication()
        {            
        }

        /// <summary>
        /// <para>Gets the name of the application as a URI.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the application.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.Name)]
        public Uri ApplicationName { get; internal set; }

        internal static unsafe NetworkApplication CreateFromNative(NativeTypes.FABRIC_NETWORK_APPLICATION_QUERY_RESULT_ITEM nativeNetworkApplication)
        {
            NetworkApplication networkApplication = new NetworkApplication();

            networkApplication.ApplicationName = new Uri(NativeTypes.FromNativeString(nativeNetworkApplication.ApplicationName));

            return networkApplication;
        }
    }
}
