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
    /// Information about a image store file
    /// </summary>
    internal sealed class ImageStoreFile
    {
        internal ImageStoreFile(string storeRelativePath, string fileVersion, Int64 fileSize, DateTime modifiedDate) 
        {
            this.StoreRelativePath = storeRelativePath;
            this.FileVersion = fileVersion;
            this.FileSize = fileSize;
            this.ModifiedDate = modifiedDate;
            this.VersionInfo = new ImageStoreVersionInfo();
        }

        /// <summary>
        /// The file path relative to the image store root path
        /// </summary>
        public string StoreRelativePath
        {
            get;
            private set;
        }

        /// <summary>
        /// <para>Information about the version of image store file.</para>
        /// </summary>
        public string FileVersion
        {
            get;
            private set;
        }

        /// <summary>
        /// <para>The size of file in bytes.</para>
        /// </summary>
        public Int64 FileSize
        {
            get;
            private set;
        }

        /// <summary>
        /// <para>The date and time when the image store file was last modified.</para>
        /// </summary>
        public DateTime ModifiedDate
        {
            get;
            private set;
        }

        /// <summary>
        /// <para>Information about the name/version of service and application with which a image store file is associated.</para>
        /// </summary>
        public ImageStoreVersionInfo VersionInfo;

        internal static unsafe ImageStoreFile CreateFromNative(NativeImageStore.FABRIC_IMAGE_STORE_FILE_INFO_QUERY_RESULT_ITEM nativeResultItem)
        {
            //Managed code uses Int64 for CLS compliant and check if overflow before conversion
            Int64 fileSize;
            try
            {
                fileSize = Convert.ToInt64(NativeTypes.FromNativeString(nativeResultItem.FileSize));
            }
            catch (OverflowException)
            {
                fileSize = Int64.MaxValue;
            }

            return new ImageStoreFile(
                NativeTypes.FromNativeString(nativeResultItem.StoreRelativePath),
                NativeTypes.FromNativeString(nativeResultItem.FileVersion),
                fileSize,
                Convert.ToDateTime(NativeTypes.FromNativeString(nativeResultItem.ModifiedDate)));
        }

        /// <summary>
        /// Gets a string representation of the image store file.
        /// </summary>
        /// <returns>A string representation of the <see cref="System.Fabric.ImageStore.ImageStoreFile"/>.</returns>
        public override string ToString()
        {
            return string.Format("StoreRelativePath={0}, FileVersion={1}, FileSize={2}, ModifiedDate={3}", 
                this.StoreRelativePath, 
                this.FileVersion, 
                this.FileSize, 
                this.ModifiedDate.ToString());
        }
    }
}