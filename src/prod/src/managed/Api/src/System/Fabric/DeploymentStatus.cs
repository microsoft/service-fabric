// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Specifies the status of the package on the node.</para>
    /// </summary>
    public enum DeploymentStatus
    {
        /// <summary>
        /// <para>The status of the package is not known or invalid.</para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_DEPLOYMENT_STATUS.FABRIC_DEPLOYMENT_STATUS_INVALID,
        /// <summary>
        /// <para>The package is being downloaded to the node from the ImageStore.</para>
        /// </summary>
        Downloading = NativeTypes.FABRIC_DEPLOYMENT_STATUS.FABRIC_DEPLOYMENT_STATUS_DOWNLOADING,
        /// <summary>
        /// <para>The package is being activated.</para>
        /// </summary>
        Activating = NativeTypes.FABRIC_DEPLOYMENT_STATUS.FABRIC_DEPLOYMENT_STATUS_ACTIVATING,
        /// <summary>
        /// <para>The package is active.</para>
        /// </summary>
        Active = NativeTypes.FABRIC_DEPLOYMENT_STATUS.FABRIC_DEPLOYMENT_STATUS_ACTIVE,
        /// <summary>
        /// <para>The package is being upgraded.</para>
        /// </summary>
        Upgrading = NativeTypes.FABRIC_DEPLOYMENT_STATUS.FABRIC_DEPLOYMENT_STATUS_UPGRADING,
        /// <summary>
        /// <para>The package is being deactivated.</para>
        /// </summary>
        Deactivating = NativeTypes.FABRIC_DEPLOYMENT_STATUS.FABRIC_DEPLOYMENT_STATUS_DEACTIVATING
    }
}