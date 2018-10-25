// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.ImageStore
{
    using System.Collections.Generic;
    using System.Fabric.Interop;
    using System.Fabric.Description;

    /// <summary>
    /// <para>Represents the information of the relative path from image store root with the continuation token for obtaining next set of image store content.</para>
    /// </summary>
    internal sealed class ImageStorePagedRelativePath
    {
        /// <summary>
        /// <para>The relative path to file or folder from image store root.</para>
        /// </summary>
        public IList<string> RelativePath
        {
            get;
            set;
        }

        /// <summary>
        /// <para>Information used to obtain next set of image store content.</para>
        /// </summary>
        public string ContinuationToken
        {
            get;
            set;
        }

        internal unsafe void CreateFromNative(NativeImageStore.FABRIC_IMAGE_STORE_PAGED_RELATIVEPATH_QUERY_RESULT* result)
        {
            var nativeFiles = (NativeTypes.FABRIC_STRING_LIST*)result->Files;
            this.RelativePath = NativeTypes.FromNativeStringList(*nativeFiles);
            this.ContinuationToken = NativeTypes.FromNativeString(result->ContinuationToken);
        }
    }
}