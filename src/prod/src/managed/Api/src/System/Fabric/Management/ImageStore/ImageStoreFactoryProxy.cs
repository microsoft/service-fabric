// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageStore
{
    using System;
    using System.Fabric.ImageStore;
    using System.Fabric.Strings;

    /// <summary>
    /// This is the factory proxy class to produce different types of image stores.
    /// </summary>
    internal class ImageStoreFactoryProxy
    {
        /// <summary>
        /// Create a new image store.
        /// </summary>
        /// <param name="imageStoreUri">The uri of the image store.</param>
        /// <param name="workingDirectory">The temporary working directory.</param>
        /// <param name="localRoot">The local path of the working folder.</param>
        /// <param name="isInternal">Indicates whether or not it's for internal use.</param>
        /// <returns>An object of Image Store pointing to the given uri.</returns>
        internal static IImageStore CreateImageStore(
            string imageStoreUri,
            string localRoot = null,
            string workingDirectory = null,
            bool isInternal = false)
        {
            if (imageStoreUri == null)
            {
                throw new ArgumentException(StringResources.Error_InvalidImageStoreConnectionString);
            }

            string[] connectionStrings = null;
            SecurityCredentials credentials = null;

            return CreateImageStore(imageStoreUri, localRoot, connectionStrings, credentials, workingDirectory, isInternal);
        }

        internal static IImageStore CreateImageStore(
            string imageStoreUri,
            string localRoot,
            string[] connectionStrings,
            SecurityCredentials credentials,
            string workingDirectory,
            bool isInternal)
        {
            if (imageStoreUri == null)
            {
                throw new ArgumentException(StringResources.Error_InvalidImageStoreConnectionString);
            }

            if (XStoreImageStore.IsXStoreUri(imageStoreUri))
            {
                return new XStoreImageStore(imageStoreUri, localRoot);
            }
            else
            {
                return ImageStoreFactory.CreateImageStore(imageStoreUri, localRoot, connectionStrings, credentials, workingDirectory, isInternal);
            }
        }
    }
}