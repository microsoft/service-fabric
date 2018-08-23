// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents a provisioned Service Fabric configuration (Cluster Manifest) version retrieved by calling 
    /// <see cref="System.Fabric.FabricClient.QueryClient.GetProvisionedFabricConfigVersionListAsync()" />.</para>
    /// </summary>
    public class ProvisionedFabricConfigVersion
    {
        internal ProvisionedFabricConfigVersion(string configVersion)
        {
            this.ConfigVersion = configVersion;
        }

        /// <summary>
        /// <para>Gets the configuration version.</para>
        /// </summary>
        /// <value>
        /// <para>The configuration version.</para>
        /// </value>
        public string ConfigVersion { get; private set; }

        internal static unsafe ProvisionedFabricConfigVersion CreateFromNative(
            NativeTypes.FABRIC_PROVISIONED_CONFIG_VERSION_QUERY_RESULT_ITEM nativeResultItem)
        {
            return new ProvisionedFabricConfigVersion(
                NativeTypes.FromNativeString(nativeResultItem.ConfigVersion));
        }
    }
}