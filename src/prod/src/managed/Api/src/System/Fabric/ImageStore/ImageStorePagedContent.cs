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
    /// Information about the image store content
    /// </summary>
    internal sealed class ImageStorePagedContent
    {
        /// <summary>
        /// The list of image store file info objects represents files found under the given image store relative path
        /// </summary>
        public List<ImageStoreFile> Files
        {
            get;
            set;
        }

        /// <summary>
        /// The list of image store folder info objects represents subfolders found under the given image store relative path
        /// </summary>
        public List<ImageStoreFolder> Folders
        {
            get;
            set;
        }

        /// <summary>
        ///Information used to obtain next set of image store content
        /// </summary>
        public string ContinuationToken
        {
            get;
            set;
        }

        internal unsafe void CreateFromNative(NativeImageStore.FABRIC_IMAGE_STORE_PAGED_CONTENT_QUERY_RESULT* result)
        {
            this.CreateFromNativeFileList((NativeImageStore.FABRIC_IMAGE_STORE_FILE_INFO_QUERY_RESULT_LIST*)result->Files);
            this.CreateFromNativeFolderList((NativeImageStore.FABRIC_IMAGE_STORE_FOLDER_INFO_QUERY_RESULT_LIST*)result->Folders);
            this.ContinuationToken = NativeTypes.FromNativeString(result->ContinuationToken);
        }


        internal unsafe void CreateFromNativeFileList(NativeImageStore.FABRIC_IMAGE_STORE_FILE_INFO_QUERY_RESULT_LIST* nativeList)
        {
            if (nativeList != null && nativeList->Count > 0)
            {
                if (this.Files == null)
                {
                    this.Files = new List<ImageStoreFile>();
                }
           
                var nativeItemArray = (NativeImageStore.FABRIC_IMAGE_STORE_FILE_INFO_QUERY_RESULT_ITEM*)nativeList->Items;
                for (int index = 0; index < nativeList->Count; ++index)
                {
                    var nativeItem = *(nativeItemArray + index);
                    this.Files.Add(ImageStoreFile.CreateFromNative(nativeItem));
                }
            }
        }

        internal unsafe void CreateFromNativeFolderList(NativeImageStore.FABRIC_IMAGE_STORE_FOLDER_INFO_QUERY_RESULT_LIST* nativeList)
        {
            if (nativeList != null && nativeList->Count > 0)
            {
                if (this.Folders == null)
                {
                    this.Folders = new List<ImageStoreFolder>();
                }

                var nativeItemArray = (NativeImageStore.FABRIC_IMAGE_STORE_FOLDER_INFO_QUERY_RESULT_ITEM*)nativeList->Items;
                for (int index = 0; index < nativeList->Count; ++index)
                {
                    var nativeItem = *(nativeItemArray + index);
                    this.Folders.Add(ImageStoreFolder.CreateFromNative(nativeItem));
                }
            }
        }
    }
}