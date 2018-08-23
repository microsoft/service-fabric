// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Health;
    using System.Fabric.Interop;
    using System.Numerics;


    /// <summary>
    /// A ImageStoreFileInfo contains information about the native image store file
    /// </summary>
    public sealed class ImageStoreFileInfo
    {
        /// <summary>
        /// Initializes a new instance of the <cref name="ImageStoreFileInfo"/> structure
        /// </summary>
        /// <param name="storeRelativePath">
        /// Get the relative path of image store file which is from the native image store root.
        /// This parameter cannot be empty, or consist only of whitespace
        /// </param>
        /// <param name="fileVersion">
        ///  Gets the image store file version
        /// </param>
        /// <param name="modifiedDate">
        /// Gets the date and time when the file was last modified
        /// </param>
        /// <param name="fileSize">
        /// Gets the size in byte of the native image store file
        /// </param>
        internal ImageStoreFileInfo(
            string storeRelativePath,
            ImageStoreFileVersion fileVersion,
            DateTime modifiedDate,
            Int64 fileSize)
        {
            this.StoreRelativePath = storeRelativePath;
            this.FileVersion = fileVersion;
            this.ModifiedDate = modifiedDate;
            this.FileSize = fileSize;
        }

        /// <summary>
        /// Initializes a new instance of the <cref name="ImageStoreFileInfo"/> structure
        /// </summary>
        private ImageStoreFileInfo() { }

        /// <summary>
        /// <para>Get the relative path of the file from the native image store root.</para>
        /// </summary>
        /// <value>
        /// <para>The relative path of the file from the native image store root.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.StoreRelativePath)]
        public string StoreRelativePath { get; private set; }

        /// <summary>
        /// <para>Gets the image store file version.</para>
        /// </summary>
        /// <value>
        /// <para>The image store file verison.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.FileVersion)]
        public ImageStoreFileVersion FileVersion { get; private set; }

        /// <summary>
        /// <para>Gets the date and time when the file was last modified.</para>
        /// </summary>
        /// <value>
        /// <para>The date and time when the file was last modified.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.ModifiedDate)]
        public DateTime ModifiedDate { get; private set; }

        /// <summary>
        /// <para>Gets the size in bytes of the image store file.</para>
        /// </summary>
        /// <value>
        /// <para>The size in bytes of the image store file.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.FileSize)]
        public Int64 FileSize { get; private set; }

    }
}