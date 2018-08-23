// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>The type of host for main entry point of a code package as specified in service manifest.</para>
    /// </summary>
    public enum HostType
    {
        /// <summary>
        /// <para>The type of host is not known or invalid.</para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_HOST_TYPE.FABRIC_HOST_TYPE_INVALID,
        
        /// <summary>
        /// <para>The host is an executable.</para>
        /// </summary>
        ExeHost = NativeTypes.FABRIC_HOST_TYPE.FABRIC_HOST_TYPE_EXE_HOST,
        
        /// <summary>
        /// <para>The host is a container.</para>
        /// </summary>
        ContainerHost = NativeTypes.FABRIC_HOST_TYPE.FABRIC_HOST_TYPE_CONTAINER_HOST
    }
}