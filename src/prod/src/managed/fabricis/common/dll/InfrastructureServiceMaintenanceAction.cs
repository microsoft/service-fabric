// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    /// <summary>
    /// A wrapper around the maintenance or repair enumerations provided by Azure MR.
    /// </summary>
    internal enum InfrastructureServiceMaintenanceAction
    {
        /// <summary>
        /// Reboot the virtual machine of the role instance.
        /// </summary>
        Reboot,

        /// <summary>
        /// Reimages the virtual machine of the role instance.
        /// </summary>        
        ReimageOS,

        /// <summary>
        /// Excludes the virtual machine of the role instance from the cloud service.
        /// </summary>
        Exclude,

        /// <summary>
        /// Repaves (or formats) the data on the virtual machine of the role instance from the cloud service.         
        /// </summary>
        RepaveData,

        /// <summary>
        /// Reboots the host OS. This maps to a string request in the MR SDK.
        /// </summary>
        HostReboot,

        /// <summary>
        /// Repaves (or formats) the data on the host OS. This maps to a string request in the MR SDK.
        /// </summary>
        HostRepaveData,

        /// <summary>
        /// Service heal the role instance by relocating the role instance to a different virtual machine (on a different host node).
        /// </summary>
        Heal,
    }
}