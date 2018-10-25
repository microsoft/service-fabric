// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.ImageStore
{
    /// <summary>
    /// Describes the underlying storage provider for the cluster's Image Store as defined by the ImageStoreConnectionString setting in the Cluster Manifest.
    /// </summary>
    internal enum ImageStoreProviderType
    {
        /// <summary>
        /// An invalid or unrecognized provider
        /// </summary>
        Invalid,

        /// <summary>
        /// Azure Storage
        /// </summary>
        AzureStorage,

        /// <summary>
        /// Either a local filesystem directory or file share
        /// </summary>
        FileShare,
            
        /// <summary>
        /// Image Store system service running inside the cluster itself
        /// </summary>
        ImageStoreService
    }
}