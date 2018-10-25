// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Fabric.ImageStore;
    using System.Threading.Tasks;

    public partial class FabricClient
    {
        /// <summary>
        /// Represents the image store client for performing image store operations.
        /// </summary>
        internal sealed class ImageStoreClient
        {
            #region Fields           
            FabricClient fabricClient;
           
            public ImageStoreProviderType ImageStoreType
            {
                get;
                private set;
            }

            #endregion

            #region Constructor

            public ImageStoreClient(FabricClient fabricClient)
            {
                this.fabricClient = fabricClient;
            }

            #endregion

            #region API

            public void UploadContent(string imageStoreConnectionString, string remoteDestination, string localSource, TimeSpan timeout, CopyFlag imageCopyFlag, bool acquireSourceReaderLock)
            {
                this.UploadContent(imageStoreConnectionString, remoteDestination, localSource, null, timeout, imageCopyFlag, acquireSourceReaderLock);
            }

            public Task UploadContentAsync(string imageStoreConnectionString, string remoteDestination, string localSource, TimeSpan timeout, CopyFlag imageCopyFlag, bool acquireSourceReaderLock)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<string>("localSource", localSource).NotNullOrEmpty();
                Requires.Argument<string>("remoteDestination", remoteDestination).NotNullOrEmpty();
                Requires.Argument<string>("imageStoreConnectionString", imageStoreConnectionString).NotNullOrEmpty();

                IImageStore imageStore = this.fabricClient.GetImageStore(imageStoreConnectionString);
                return imageStore.UploadContentAsync(
                    remoteDestination,
                    localSource,
                    timeout,
                    imageCopyFlag,
                    acquireSourceReaderLock);
            }

            public void UploadContent(string imageStoreConnectionString, string remoteDestination, string localSource, IImageStoreProgressHandler progressHandler, TimeSpan timeout, CopyFlag imageCopyFlag, bool acquireSourceReaderLock)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<string>("localSource", localSource).NotNullOrEmpty();
                Requires.Argument<string>("remoteDestination", remoteDestination).NotNullOrEmpty();
                Requires.Argument<string>("imageStoreConnectionString", imageStoreConnectionString).NotNullOrEmpty();

                IImageStore imageStore = this.fabricClient.GetImageStore(imageStoreConnectionString);
                if (progressHandler == null)
                {
                    imageStore.UploadContent(
                        remoteDestination,
                        localSource,
                        timeout,
                        CopyFlag.AtomicCopy,
                        acquireSourceReaderLock);
                }
                else
                {
                    imageStore.UploadContent(
                        remoteDestination,
                        localSource,
                        progressHandler,
                        timeout,
                        imageCopyFlag,
                        acquireSourceReaderLock);
                }
            }

            public void DownloadContent(string imageStoreConnectionString, string remoteSource, string localDestination, TimeSpan timeout, CopyFlag imageCopyFlag)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<string>("remoteSource", remoteSource).NotNullOrEmpty();
                Requires.Argument<string>("localDestination", localDestination).NotNullOrEmpty();
                IImageStore imageStore = this.fabricClient.GetImageStore(imageStoreConnectionString);
                imageStore.DownloadContent(remoteSource, localDestination, timeout, imageCopyFlag);
            }

            public void DeleteContent(string imageStoreConnectionString, string remoteLocation, TimeSpan timeout)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<string>("remoteLocation", remoteLocation).NotNullOrEmpty();
                Requires.Argument<string>("imageStoreConnectionString", imageStoreConnectionString).NotNullOrEmpty();
                IImageStore imageStore = this.fabricClient.GetImageStore(imageStoreConnectionString);
                imageStore.DeleteContent(remoteLocation, timeout);

            }

            public Task DeleteContentAsync(string imageStoreConnectionString, string remoteLocation, TimeSpan timeout)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<string>("remoteLocation", remoteLocation).NotNullOrEmpty();
                Requires.Argument<string>("imageStoreConnectionString", imageStoreConnectionString).NotNullOrEmpty();
                IImageStore imageStore = this.fabricClient.GetImageStore(imageStoreConnectionString);
                return imageStore.DeleteContentAsync(remoteLocation, timeout);
            }

            public ImageStoreContent ListContent(string imageStoreConnectionString, string remoteLocation, bool isRecursive, TimeSpan timeout)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<string>("remoteLocation", remoteLocation).NotNullOrEmpty();
                Requires.Argument<string>("imageStoreConnectionString", imageStoreConnectionString).NotNullOrEmpty();
                IImageStore imageStore = this.fabricClient.GetImageStore(imageStoreConnectionString);
                return imageStore.ListContentWithDetails(remoteLocation, isRecursive, timeout);
            }

            public Task<ImageStoreContent> ListContentAsync(string imageStoreConnectionString, string remoteLocation, bool isRecursive, TimeSpan timeout)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<string>("remoteLocation", remoteLocation).NotNullOrEmpty();
                Requires.Argument<string>("imageStoreConnectionString", imageStoreConnectionString).NotNullOrEmpty();
                IImageStore imageStore = this.fabricClient.GetImageStore(imageStoreConnectionString);
                return imageStore.ListContentWithDetailsAsync(remoteLocation, isRecursive, timeout);
            }

            public Task<ImageStorePagedContent> ListPagedContentAsync(string imageStoreConnectionString, ImageStoreListDescription listDescription, TimeSpan timeout)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<string>("remoteLocation", listDescription.RemoteLocation).NotNullOrEmpty();
                Requires.Argument<string>("imageStoreConnectionString", imageStoreConnectionString).NotNullOrEmpty();
                IImageStore imageStore = this.fabricClient.GetImageStore(imageStoreConnectionString);
                return imageStore.ListPagedContentWithDetailsAsync(listDescription, timeout);
            }

            #endregion 
        }
    }
}