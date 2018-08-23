// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Reserved for future use.</para>
    /// </summary>
    public enum DllHostHostedDllKind
    {
        /// <summary>
        /// <para>Reserved for future use.</para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_DLLHOST_HOSTED_DLL_KIND.FABRIC_DLLHOST_HOSTED_DLL_KIND_INVALID,
        
        /// <summary>
        /// <para>Reserved for future use.</para>
        /// </summary>
        Unmanaged = NativeTypes.FABRIC_DLLHOST_HOSTED_DLL_KIND.FABRIC_DLLHOST_HOSTED_DLL_KIND_UNMANAGED,
        
        /// <summary>
        /// <para>Reserved for future use.</para>
        /// </summary>
        Managed = NativeTypes.FABRIC_DLLHOST_HOSTED_DLL_KIND.FABRIC_DLLHOST_HOSTED_DLL_KIND_MANAGED
    }
}