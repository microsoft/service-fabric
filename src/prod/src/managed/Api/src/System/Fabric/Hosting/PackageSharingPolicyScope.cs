// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents the scope for the <see cref="System.Fabric.PackageSharingPolicy" />.</para>
    /// </summary>
    public enum PackageSharingPolicyScope
    {
        /// <summary>
        /// <para>No package sharing policy scope.</para>
        /// </summary>
        None = NativeTypes.FABRIC_PACKAGE_SHARING_POLICY_SCOPE.FABRIC_PACKAGE_SHARING_POLICY_SCOPE_NONE,
        
        /// <summary>
        /// <para>Share all code, config and data packages from corresponding service manifest.</para>
        /// </summary>
        All = NativeTypes.FABRIC_PACKAGE_SHARING_POLICY_SCOPE.FABRIC_PACKAGE_SHARING_POLICY_SCOPE_ALL,
        
        /// <summary>
        /// <para>Share all code packages from corresponding service manifest.</para>
        /// </summary>
        Code = NativeTypes.FABRIC_PACKAGE_SHARING_POLICY_SCOPE.FABRIC_PACKAGE_SHARING_POLICY_SCOPE_CODE,
        
        /// <summary>
        /// <para>Share all config packages from corresponding service manifest.</para>
        /// </summary>
        Config = NativeTypes.FABRIC_PACKAGE_SHARING_POLICY_SCOPE.FABRIC_PACKAGE_SHARING_POLICY_SCOPE_CONFIG,
        
        /// <summary>
        /// <para>Share all data packages from corresponding service manifest</para>
        /// </summary>
        Data = NativeTypes.FABRIC_PACKAGE_SHARING_POLICY_SCOPE.FABRIC_PACKAGE_SHARING_POLICY_SCOPE_DATA
    }
}