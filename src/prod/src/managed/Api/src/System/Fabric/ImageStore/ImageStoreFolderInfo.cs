// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.ImageStore
{
    using System.Collections.Generic;
    using System.Fabric.Description;
    using System.Fabric.Health;
    using System.Fabric.Interop;

    /// <summary>
    /// Information about a image store folder including the number of files this folder contains and its image store relative path
    /// </summary>
    internal sealed class ImageStoreFolder
    {
        /// <summary>
        /// Intialize Image store folder inforamtion object 
        /// </summary>
        /// <param name="storeRelativePath">The remote location within image store which is relative to the image store root.</param>
        /// <param name="fileCount">The number of files from within the image store folder.</param>
        public ImageStoreFolder(string storeRelativePath, Int32 fileCount)
        {
            this.StoreRelativePath = storeRelativePath;
            this.FileCount = fileCount;
            this.VersionInfo = new ImageStoreVersionInfo();
        }

        /// <summary>
        /// The remote location within image store which is relative to the image store root
        /// </summary>
        public string StoreRelativePath
        {
            get;
            private set;
        }

        /// <summary>
        /// The number of files from within the image store folder
        /// </summary>
        public Int32 FileCount
        {
            get;
            set;
        }

        /// <summary>
        /// <para>Information about the name/version of service and application with which a image store folder is associated.</para>
        /// </summary>
        public ImageStoreVersionInfo VersionInfo;

        internal static unsafe ImageStoreFolder CreateFromNative(NativeImageStore.FABRIC_IMAGE_STORE_FOLDER_INFO_QUERY_RESULT_ITEM nativeResultItem)
        {
            //Managed code uses Int32 for CLS compliant and check if overflow before conversion
            Int32 fileCount;
            try
            {
                fileCount = Convert.ToInt32(NativeTypes.FromNativeString(nativeResultItem.FileCount));
            }
            catch (OverflowException)
            {
                fileCount = Int32.MaxValue;
            }

            return new ImageStoreFolder(NativeTypes.FromNativeString(nativeResultItem.StoreRelativePath), fileCount);
        }

        /// <summary>
        /// Gets a string representation of the folder in image store
        /// </summary>
        /// <returns>A string representation of the <see cref="System.Fabric.ImageStore.ImageStoreFolder"/>.</returns>
        public override string ToString()
        {
            return string.Format("StoreRelativePath={0}, FileCount={1}", this.StoreRelativePath, this.FileCount);
        }
    }

}