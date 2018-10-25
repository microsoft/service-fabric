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
    /// <para>Represents the query result from listing image store content</para>
    /// </summary>
    public sealed class ImageStorePagedContentResult
    {
        /// <summary>
        ///  Initializes a new instance of the <cref name="ImageStorePagedContentResult"/>.
        /// </summary>
        /// <param name="storeFiles">
        /// <para>The list of <see cref="System.Fabric.Query.ImageStoreFileInfo"/> objects.</para>
        /// </param>
        /// <param name="storeFolders">
        /// <para>The list of <see cref="System.Fabric.Query.ImageStoreFolderInfo"/> objects.</para>
        /// </param>
        /// <param name="continuationToken">
        /// The continuation token used to access the subsequent page
        /// </param>
        internal ImageStorePagedContentResult(
            IList<ImageStoreFileInfo> storeFiles,
            IList<ImageStoreFolderInfo> storeFolders,
            string continuationToken)
        {
            this.StoreFiles = storeFiles;
            this.StoreFolders = storeFolders;
            this.ContinuationToken = continuationToken;
        }

        /// <summary>
        /// Initializes a new instance of the <cref name="ImageStorePagedContentResult"/> structure
        /// </summary>
        private ImageStorePagedContentResult() { }

        /// <summary>
        /// <para>Gets the list of <see cref="System.Fabric.Query.ImageStoreFileInfo"/> objects.</para>
        /// </summary>
        /// <value>
        /// <para>The List of <see cref="System.Fabric.Query.ImageStoreFileInfo"/> objects.</para>
        /// </value>
        public IList<ImageStoreFileInfo> StoreFiles { get; set; }

        /// <summary>
        /// <para>Get the list of <see cref="System.Fabric.Query.ImageStoreFolderInfo"/> objects.</para>
        /// </summary>
        /// <value>
        /// <para>The list of <see cref="System.Fabric.Query.ImageStoreFolderInfo"/> objects.</para>
        /// </value>
        public IList<ImageStoreFolderInfo> StoreFolders { get; set; }

        /// <summary>
        /// <para>Gets the continuation token for query operation.</para>
        /// </summary>
        /// <value>
        /// <para>Gets the continuation token. It can be used in a subsequent call to return the next page of results.</para>
        /// </value>
        public string ContinuationToken { get; set; }
    }
}