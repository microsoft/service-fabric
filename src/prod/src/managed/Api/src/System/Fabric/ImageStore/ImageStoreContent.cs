// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.ImageStore
{
    using System.Fabric.Health;
    using System.Collections.Generic;
    using System.Fabric.Interop;
    using System.Fabric.Description;

    /// <summary>
    /// <para>Information about the image store content at the specified image store relative path.</para>
    /// </summary>
    internal sealed class ImageStoreContent
    {
        /// <summary>
        /// <para>The list of image store file info objects represents files under the given image store relative path.</para>
        /// </summary>
        public List<ImageStoreFile> Files
        {
            get;
            set;
        }

        /// <summary>
        /// <para>The list of image store folder info objects represents subfolders under the given image store relative path.</para>
        /// </summary>
        public List<ImageStoreFolder> Folders
        {
            get;
            set;
        }

        internal unsafe void CreateFromNative(NativeImageStore.FABRIC_IMAGE_STORE_CONTENT_QUERY_RESULT* result)
        {
            this.CreateFromNativeFileList((NativeImageStore.FABRIC_IMAGE_STORE_FILE_INFO_QUERY_RESULT_LIST*)result->Files);
            this.CreateFromNativeFolderList((NativeImageStore.FABRIC_IMAGE_STORE_FOLDER_INFO_QUERY_RESULT_LIST*)result->Folders);
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