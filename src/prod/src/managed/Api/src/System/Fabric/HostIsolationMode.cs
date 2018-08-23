// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>
    /// Indicates isolation mode of main entry point of a code package when it's host type is 
    /// <see cref="System.Fabric.HostType.ContainerHost"/>. This is specified as part of container host policies
    /// in application manifest while importing service manifest.
    /// </para>
    /// <remarks>
    /// For host type other than <see cref="System.Fabric.HostType.ContainerHost"/>, its value is <see cref="System.Fabric.HostIsolationMode.None"/>.
    /// </remarks>
    /// </summary>
    public enum HostIsolationMode
    {
        /// <summary>
        /// <para>Indicates isolation mode is not applicable for given <see cref="System.Fabric.HostType"/>.</para>
        /// </summary>
        None = NativeTypes.FABRIC_HOST_ISOLATION_MODE.FABRIC_HOST_ISOLATION_MODE_NONE,

        /// <summary>
        /// <para>The default isolation mode for a <see cref="System.Fabric.HostType.ContainerHost"/>.</para>
        /// </summary>
        Process = NativeTypes.FABRIC_HOST_ISOLATION_MODE.FABRIC_HOST_ISOLATION_MODE_PROCESS,

        /// <summary>
        /// <para>
        /// Indicates the <see cref="System.Fabric.HostType.ContainerHost"/> is a Hyper-V container.
        /// This applies to only Windows containers.
        /// </para>
        /// </summary>
        HyperV = NativeTypes.FABRIC_HOST_ISOLATION_MODE.FABRIC_HOST_ISOLATION_MODE_HYPER_V
    }
}