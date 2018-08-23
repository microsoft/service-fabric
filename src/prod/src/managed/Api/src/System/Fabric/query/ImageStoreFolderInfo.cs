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
    /// A ImageStoreFolderInfo contains information about a native image store folder
    /// </summary>
    public sealed class ImageStoreFolderInfo
    {
        /// <summary>
        /// Initializes a new instance of the <cref name="ImageStoreFolderInfo"/> structure
        /// </summary>
        /// <param name="storeRelativePath">
        /// The relative path from the native image store root
        /// </param>
        /// <param name="fileCount">
        /// The count of files from the current folder and all folders
        /// </param>
        internal ImageStoreFolderInfo(
           string storeRelativePath,
           int fileCount)
        {
            this.StoreRelativePath = storeRelativePath;
            this.FileCount = fileCount;
        }

        /// <summary>
        /// Initializes a new instance of the <cref name="ImageStoreFolderInfo"/> structure
        /// </summary>
        private ImageStoreFolderInfo() { }

        /// <summary>
        /// <para>Gets the relative path from the native image store root.</para>
        /// </summary>
        /// <value>
        /// <para>The relative path from the native image store root.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.StoreRelativePath)]
        public string StoreRelativePath { get; private set; }

        /// <summary>
        /// <para>Get the count of files within the current folder and all subfolders.</para>
        /// </summary>
        /// <value>
        /// <para>The count of files within the current folder and all subfolders.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.FileCount)]
        public int FileCount { get; private set; }

    }
}