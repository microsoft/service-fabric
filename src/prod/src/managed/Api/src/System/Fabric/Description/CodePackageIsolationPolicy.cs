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
    public enum DllHostIsolationPolicy
    {
        /// <summary>
        /// <para>Reserved for future use.</para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_DLLHOST_ISOLATION_POLICY.FABRIC_DLLHOST_ISOLATION_POLICY_INVALID,
        
        /// <summary>
        /// <para>Reserved for future use.</para>
        /// </summary>
        SharedDomain = NativeTypes.FABRIC_DLLHOST_ISOLATION_POLICY.FABRIC_DLLHOST_ISOLATION_POLICY_SHARED_DOMAIN,
        
        /// <summary>
        /// <para>Reserved for future use.</para>
        /// </summary>
        DedicatedDomain = NativeTypes.FABRIC_DLLHOST_ISOLATION_POLICY.FABRIC_DLLHOST_ISOLATION_POLICY_DEDICATED_DOMAIN,
        
        /// <summary>
        /// <para>Reserved for future use.</para>
        /// </summary>
        DedicatedProcess = NativeTypes.FABRIC_DLLHOST_ISOLATION_POLICY.FABRIC_DLLHOST_ISOLATION_POLICY_DEDICATED_PROCESS
    }
}