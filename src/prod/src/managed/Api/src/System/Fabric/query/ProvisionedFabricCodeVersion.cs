// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents a provisioned Service Fabric code (MSI) version retrieved by calling 
    /// <see cref="System.Fabric.FabricClient.QueryClient.GetProvisionedFabricCodeVersionListAsync(System.String)" />.</para>
    /// </summary>
    public class ProvisionedFabricCodeVersion
    {
        internal ProvisionedFabricCodeVersion(string codeVersion)
        {
            this.CodeVersion = codeVersion;
        }

        /// <summary>
        /// <para>Gets the product version of Service Fabric.</para>
        /// </summary>
        /// <value>
        /// <para>The product version of Service Fabric.</para>
        /// </value>
        public string CodeVersion { get; private set; }

        internal static unsafe ProvisionedFabricCodeVersion CreateFromNative(
            NativeTypes.FABRIC_PROVISIONED_CODE_VERSION_QUERY_RESULT_ITEM nativeResultItem)
        {
            return new ProvisionedFabricCodeVersion(
                NativeTypes.FromNativeString(nativeResultItem.CodeVersion));
        }
    }
}