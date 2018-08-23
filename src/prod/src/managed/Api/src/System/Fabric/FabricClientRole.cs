// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>
    /// Represents the fabric client security role.
    /// </para>
    /// </summary>
    public enum FabricClientRole : int
    {
        /// <summary>
        /// Indicates unknown permissions.
        /// </summary>
        Unknown = NativeTypes.FABRIC_CLIENT_ROLE.FABRIC_CLIENT_ROLE_UNKNOWN,
        
        /// <summary>
        /// <para>
        /// Indicates user permissions.
        /// </para>
        /// </summary>
        User = NativeTypes.FABRIC_CLIENT_ROLE.FABRIC_CLIENT_ROLE_USER,

        /// <summary>
        /// <para>
        /// Indicates administrator permissions.
        /// </para>
        /// </summary>
        Admin = NativeTypes.FABRIC_CLIENT_ROLE.FABRIC_CLIENT_ROLE_ADMIN
    }
}