// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Health;
    using System.Fabric.Interop;
    using System.Numerics;


    /// <summary>
    /// A ImageStoreContentResult contains the information of image store content returned by list content query
    /// </summary>
    public sealed class ImageStoreContentResult
    {
        /// <summary>
        ///  Initializes a new instance of the <cref name="ImageStoreContentResult"/> structure
        /// </summary>
        /// <param name="storeFiles">
        /// <para>The list of <see cref="System.Fabric.Query.ImageStoreFileInfo"/> objects.</para>
        /// </param>
        /// <param name="storeFolders">
        /// <para>The list of <see cref="System.Fabric.Query.ImageStoreFolderInfo"/> objects.</para>
        /// </param>
        internal ImageStoreContentResult(
            IList<ImageStoreFileInfo> storeFiles,
            IList<ImageStoreFolderInfo> storeFolders)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <cref name="ImageStoreContentResult"/> structure
        /// </summary>
        private ImageStoreContentResult() { }

        /// <summary>
        /// <para>Gets the list of <see cref="System.Fabric.Query.ImageStoreFileInfo"/> objects.</para>
        /// </summary>
        /// <value>
        /// <para>The List of <see cref="System.Fabric.Query.ImageStoreFileInfo"/> objects.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.StoreFiles)]
        public IList<ImageStoreFileInfo> StoreFiles { get; private set; }

        /// <summary>
        /// <para>Get the list of <see cref="System.Fabric.Query.ImageStoreFolderInfo"/> objects.</para>
        /// </summary>
        /// <value>
        /// <para>The list of <see cref="System.Fabric.Query.ImageStoreFolderInfo"/> objects.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.StoreFolders)]
        public IList<ImageStoreFolderInfo> StoreFolders { get; private set; }
    }
}