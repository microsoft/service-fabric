// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Enumerates the options for the cleanup of application package policy.</para>
    /// </summary>
    public enum ApplicationPackageCleanupPolicy
    {
        /// <summary>
        /// <para> Indicates that the cleanup policy provided is invalid. </para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_APPLICATION_PACKAGE_CLEANUP_POLICY.FABRIC_APPLICATION_PACKAGE_CLEANUP_POLICY_INVALID,

        /// <summary>
        /// <para>Indicates that the cleanup policy of application packages are based on the cluster setting "CleanupApplicationPackageOnProvisionSuccess".
        /// </para>
        /// </summary>
        Default = NativeTypes.FABRIC_APPLICATION_PACKAGE_CLEANUP_POLICY.FABRIC_APPLICATION_PACKAGE_CLEANUP_POLICY_DEFAULT,

        /// <summary>
        /// <para>Indicates that the service fabric runtime determines when the application package is cleaned up from the image store.By default cleanup is done after a successful provision.</para>
        /// </summary>
        Automatic = NativeTypes.FABRIC_APPLICATION_PACKAGE_CLEANUP_POLICY.FABRIC_APPLICATION_PACKAGE_CLEANUP_POLICY_AUTOMATIC,

        /// <summary>
        /// <para>Indicates that the user has to explicitly clean up the application package by using <see cref="System.Fabric.FabricClient.ApplicationManagementClient.RemoveApplicationPackage(string, string)"/>.</para>
        /// </summary>
        Manual = NativeTypes.FABRIC_APPLICATION_PACKAGE_CLEANUP_POLICY.FABRIC_APPLICATION_PACKAGE_CLEANUP_POLICY_MANUAL
    }
}
