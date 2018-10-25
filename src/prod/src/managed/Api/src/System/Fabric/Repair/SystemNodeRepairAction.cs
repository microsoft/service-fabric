// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Repair
{
    /// <summary>
    /// <para>Specifies node repair actions for which the system has a built-in executor.</para>
    /// <para>This type supports the Service Fabric platform; it is not meant to be used directly from your code.</para>
    /// </summary>
    public enum SystemNodeRepairAction
    {
        /// <summary>
        /// <para>Reboots the OS of the target nodes.</para>
        /// </summary>
        Reboot,

        /// <summary>
        /// <para>Reimages the OS volume of the target nodes.</para>
        /// </summary>
        ReimageOS,

        /// <summary>
        /// <para>Reimages all disk volumes of the target nodes, destroying all data stored on the nodes.</para>
        /// </summary>
        FullReimage,
    }
}