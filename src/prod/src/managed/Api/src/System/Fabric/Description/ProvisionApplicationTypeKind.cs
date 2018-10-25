// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents the provision application type kind,
    /// which gives information about how the application package is provisioned to image store.</para>
    /// </summary>
    public enum ProvisionApplicationTypeKind
    {
        /// <summary>
        /// <para>Indicates that the provision kind is invalid. This value is default and should not be used.</para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_PROVISION_APPLICATION_TYPE_KIND.FABRIC_PROVISION_APPLICATION_TYPE_KIND_INVALID,

        /// <summary>
        /// <para>Indicates that the provision is for a package that was previously uploaded to the image store
        /// using <see cref="System.Fabric.FabricClient.ApplicationManagementClient.CopyApplicationPackage(string, string, string, System.Fabric.IImageStoreProgressHandler, System.TimeSpan)"/>.
        /// The upload operation copied the application package to a image store relative path that
        /// must be provided to the provision operation.
        /// </para>
        /// </summary>
        ImageStorePath = NativeTypes.FABRIC_PROVISION_APPLICATION_TYPE_KIND.FABRIC_PROVISION_APPLICATION_TYPE_KIND_IMAGE_STORE_PATH,
        
        /// <summary>
        /// <para>Indicates that the provision is for an application package that was previously uploaded to
        /// an external store.</para>
        /// </summary>
        ExternalStore = NativeTypes.FABRIC_PROVISION_APPLICATION_TYPE_KIND.FABRIC_PROVISION_APPLICATION_TYPE_KIND_EXTERNAL_STORE,
    }
}