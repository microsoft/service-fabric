// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{ 
    using System.Fabric.Common;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Provides information about how to get the image store content </para>
    /// </summary>
    public sealed class ImageStoreListDescription
    {
        /// <summary>
        /// <para>Creates an instance of the <see cref="System.Fabric.Description.ImageStoreListDescription"/> class.</para>
        /// </summary>
        /// <param name="remoteLocation">
        /// <para>Relative path to file or folder in the image store from its root.</para>
        /// </param>
        /// <param name="continuationToken">
        /// <para>The continuation token returned by a previous query.</para>
        /// </param>
        /// <param name="isRecursive">
        /// <para>Specifies if the listing should loop through all files in the given relative path.</para>
        /// </param>
        public ImageStoreListDescription(string remoteLocation, string continuationToken, bool isRecursive)
        {
            this.RemoteLocation = remoteLocation;
            this.ContinuationToken = continuationToken;
            this.IsRecursive = isRecursive;
        }

        /// <summary>
        /// <para>Gets the image store relative path to list.</para>
        /// </summary>
        /// <value>
        /// <para>Relative path to the file or folder in the image store from its root.</para>
        /// </value>
        public string RemoteLocation
        {
            get;
            private set;
        }

        /// <summary>
        /// <para>Gets the token that can be used by queries to get the next page.</para>
        /// </summary>
        /// <value>
        /// <para>The token that can be used by queries to get the next page.</para>
        /// </value>
        public string ContinuationToken
        {
            get;
            private set;
        }

        /// <summary>
        /// <para>Specifies if the listing should loop through all files in the given relative path.</para>
        /// </summary>
        /// <value>
        /// <para>The flag that indicates whether the listing should loop through all files in the given relative path.</para>
        /// </value>
        public bool IsRecursive
        {
            get;
            private set;
        }

        internal unsafe void ToNative(PinCollection pinCollection, out NativeImageStore.FABRIC_IMAGE_STORE_LIST_DESCRIPTION description)
        {
            description.RemoteLocation = pinCollection.AddObject(this.RemoteLocation);
            description.ContinuationToken = pinCollection.AddObject(this.ContinuationToken);
            description.IsRecursive = NativeTypes.ToBOOLEAN(IsRecursive);
            description.Reserved = IntPtr.Zero;
        }
    }
}