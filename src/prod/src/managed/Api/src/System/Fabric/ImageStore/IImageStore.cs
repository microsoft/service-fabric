//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------


namespace System.Fabric.ImageStore
{    
    using System.Threading.Tasks;
    using System.Collections.Generic;
    using System.Fabric.Description;
    using System.Fabric.Query;

    /// <summary>
    /// The image store interface.
    /// </summary>
    internal interface IImageStore
    {
        /// <summary>
        /// Gets the root uri of the image store.
        /// </summary>
        string RootUri { get; }

        /// <summary>
        /// Uploads content of the local source to the image store at the remote destination.
        /// </summary>
        /// <param name="remoteDestination"> Location relative to image store root where the content needs to be uploaded.</param>
        /// <param name="localSource"> Physical location of the source.</param>
        /// <param name="imageCopyFlag"> Copy flag specifying copy type.</param>
        /// <param name="acquireSourceReaderLock"> Specifies whether or not to acquire reader lock for the source. This is because when the end user uploads build the lock does not make sense.</param>
        /// <param name="timeout">specifying timeout of upload operation</param>
        void UploadContent(string remoteDestination, string localSource, TimeSpan timeout, CopyFlag imageCopyFlag, bool acquireSourceReaderLock);

        /// <summary>
        /// Uploads content of the local source to the image store at the remote destination.
        /// </summary>
        /// <param name="remoteDestination"> Location relative to image store root where the content needs to be uploaded.</param>
        /// <param name="localSource"> Physical location of the source.</param>
        /// <param name="progressHandler">Defines the behavior to process progress information from the uploading operation.</param>
        /// <param name="timeout">The timeout for performing the uploading operation.</param>
        /// <param name="imageCopyFlag">The copying control information to specify how file can be overwritten.</param>
        /// <param name="acquireSourceReaderLock">Indicates whether to acquire reader lock.</param>
        void UploadContent(string remoteDestination, string localSource, IImageStoreProgressHandler progressHandler, TimeSpan timeout, CopyFlag imageCopyFlag, bool acquireSourceReaderLock);

        /// <summary>
        /// Uploads content of the local source to the image store at the remote destination.
        /// </summary>
        /// <param name="remoteDestination"> Location relative to image store root where the content needs to be uploaded.</param>
        /// <param name="localSource"> Physical location of the source.</param>
        /// <param name="timeout">The timeout for performing the uploading operation.</param>
        /// <param name="imageCopyFlag">The copying control information to specify how file can be overwritten.</param>
        /// <param name="acquireSourceReaderLock">Indicates whether to acquire reader lock.</param>
        /// <returns></returns>
        Task UploadContentAsync(string remoteDestination, string localSource, TimeSpan timeout, CopyFlag imageCopyFlag, bool acquireSourceReaderLock);

        /// <summary>
        /// Copies content from a path in the ImageStore to the destination path in the ImageStore.
        /// </summary>
        /// <param name="remoteSource">Location relative to image store root where the content to be copied will be found.</param>
        /// <param name="remoteDestination">Location relative to image store root where the content needs to be copied.</param>
        /// <param name="imageCopyFlag">Copy flag specifying copy type.</param>
        /// <param name="checkFolderMarkFile">Indicats whether to check mark file during copying.</param>
        /// <param name="skipFiles">The list of the files names to be skipped for copying.</param>
        /// <param name="timeout">The timeout for performing the copying operation.</param>
        void CopyContent(string remoteSource, string remoteDestination, TimeSpan timeout, string[] skipFiles, CopyFlag imageCopyFlag, bool checkFolderMarkFile);

        /// <summary>
        /// Copies content from a path in the ImageStore to the destination path in the ImageStore.
        /// </summary>
        /// <param name="remoteSource">The relative path of source image store content to be copied from.</param>
        /// <param name="remoteDestination">The relative path of destination image store content to be copied to.</param>
        /// <param name="timeout">The timeout for performing the copying operation.</param>
        /// <param name="skipFiles">The list of the files names to be skipped for copying.</param>
        /// <param name="imageCopyFlag">The copying control information to specify how file can be overwritten.</param>
        /// <param name="checkFolderMarkFile">Indicats whether to check mark file during copying.</param>
        /// <returns></returns>
        Task CopyContentAsync(string remoteSource, string remoteDestination, TimeSpan timeout, string[] skipFiles, CopyFlag imageCopyFlag, bool checkFolderMarkFile);

        /// <summary>
        /// Downloads content from the image store to local destination.
        /// </summary>
        /// <param name="remoteSource"> Location (relative to image store root) from where to download the content.</param>
        /// <param name="localDestination"> Physical location where to download the content.</param>
        /// <param name="imageCopyFlag">The copying flag specifying copy type.</param>
        /// <param name="timeout">specifying timeout of download operation.</param>
        void DownloadContent(string remoteSource, string localDestination, TimeSpan timeout, CopyFlag imageCopyFlag);

        /// <summary>
        /// Downloads content from the image store to local destination.
        /// </summary>
        /// <param name="remoteSource">The relative path of source image store to be downloaded from.</param>
        /// <param name="localDestination">The local destination path to download the content.</param>
        /// <param name="progressHandler">Defines the behavior to process progress information from the downloading operation.</param>
        /// <param name="timeout">The timeout for performing the downloading operation.</param>
        /// <param name="imageCopyFlag">Copy control information to specify how file can be overwritten.</param>
        void DownloadContent(string remoteSource, string localDestination, IImageStoreProgressHandler progressHandler, TimeSpan timeout, CopyFlag imageCopyFlag);

        /// <summary>
        /// Downloads content from the image store to local destination.
        /// </summary>
        /// <param name="remoteSource">The relative path of source image store to be downloaded from.</param>
        /// <param name="localDestination">The local destination path to download the content.</param>
        /// <param name="timeout">The timeout for performing the downloading operation.</param>
        /// <param name="imageCopyFlag">Copy control information to specify how file can be overwritten.</param>
        /// <returns></returns>
        Task DownloadContentAsync(string remoteSource, string localDestination, TimeSpan timeout, CopyFlag imageCopyFlag);

        /// <summary>
        /// Get the image store content information.
        /// Returns the List of image store relative paths under the given relative path.
        /// </summary>
        /// <param name="remoteLocation">Location (relative to image store root).</param>
        /// <returns>All relative path of content within the location.</returns>
        /// <param name="timeout">The timeout for performing the listing operation.</param>
        IEnumerable<string> ListContent(string remoteLocation, TimeSpan timeout);

        /// <summary>
        /// Gets the image store relative paths under the given relative path.
        /// </summary>
        /// <param name="remoteLocation">The relative path to file or folder from image store root.</param>
        /// <param name="timeout">The timeout for performing the listing operation.</param>
        /// <returns></returns>
        Task<IEnumerable<string>> ListContentAsync(string remoteLocation, TimeSpan timeout);

        /// <summary>
        /// Gets paged image store relative paths under the given relative path.
        /// The purpose of paging is to prevent from the oversized message which might be happened when listing a large number of files.
        /// </summary>
        ///<param name="listDescription">The information on how to get image store content</param>
        ///<param name="timeout">The timeout for performing the listing operation</param>
        /// <returns>Paged relative path of the content within the location</returns>
        ImageStorePagedRelativePath ListPagedContent(ImageStoreListDescription listDescription, TimeSpan timeout);

        /// <summary>
        /// Gets paged image store relative paths under the given relative path.
        /// The purpose of paging is to prevent from the oversized message which might be happened when listing a large number of files.
        /// </summary>
        /// <param name="listDescription">The information on how to get image store content.</param>
        /// <param name="timeout">The timeout for performing the listing operation.</param>
        /// <returns></returns>
        Task<ImageStorePagedRelativePath> ListPagedContentAsync(ImageStoreListDescription listDescription, TimeSpan timeout);

        /// <summary>
        /// Gets the image store content information under the given relative path.
        /// </summary>
        /// <param name="remoteLocation">The relative path to file or folder from image store root.</param>
        /// <param name="isRecursive">flag list subhierarchy.</param>
        /// <param name="timeout">specifying timeout of list operation.</param>
        /// <returns>All content within the location</returns>
        ImageStoreContent ListContentWithDetails(string remoteLocation, bool isRecursive, TimeSpan timeout);

        /// <summary>
        /// Gets the image store content information under the given relative path.
        /// </summary>
        /// <param name="remoteLocation">The relative path to file or folder from image store root.</param>
        /// <param name="isRecursive">flag list subhierarchy.</param>
        /// <param name="timeout">The timeout for performing the listing operation.</param>
        /// <returns></returns>
        Task<ImageStoreContent> ListContentWithDetailsAsync(string remoteLocation, bool isRecursive, TimeSpan timeout);

        /// <summary>
        /// Gets paged image store content under the given relative path.
        /// The purpose of paging is to prevent from the oversized message which might be happend when listing a large number of files.
        /// </summary>
        ///<param name="listDescription">The information on how to get image store content.</param>
        /// /// <param name="timeout">specifying timeout of list operation.</param>
        /// <returns>paged content within the location</returns>
        ImageStorePagedContent ListPagedContentWithDetails(ImageStoreListDescription listDescription, TimeSpan timeout);

        /// <summary>
        /// Gets paged image store content under the given relative path.
        /// The purpose of paging is to prevent from the oversized message which might be happend when listing a large number of files.
        /// </summary>
        /// <param name="listDescription">The information on how to get image store content.</param>
        /// <param name="timeout">The timeout for performing the listing operation.</param>
        /// <returns></returns>
        Task<ImageStorePagedContent> ListPagedContentWithDetailsAsync(ImageStoreListDescription listDescription, TimeSpan timeout);


        /// <summary>
        /// Check if content is present in the store.
        /// </summary>
        /// <param name="remoteLocation"> Location (relative to image store root) where to check the presence of the content. </param>
        /// <param name="timeout">Specifying timeout of check existence operation.</param>
        /// <returns> True if the content exists, false otherwise. </returns>
        bool DoesContentExist(string remoteLocation, TimeSpan timeout);

        /// <summary>
        /// Check if content is present in the store.
        /// </summary>
        /// <param name="remoteLocation">Location (relative to image store root) where to check the presence of the content</param>
        /// <param name="timeout">The timeout for performing the checking existence operation</param>
        /// <returns></returns>
        Task<bool> DoesContentExistAsync(string remoteLocation, TimeSpan timeout);

        /// <summary>
        /// Remove tag from store, clear data associated with tag.
        /// </summary>
        /// <param name="remoteLocation">Location (relative to image store root) from where to delete the content.</param>
        /// <param name="timeout">specifying timeout of delete operation</param>
        void DeleteContent(string remoteLocation, TimeSpan timeout);

        /// <summary>
        /// Remove tag from store, clear data associated with tag.
        /// </summary>
        /// <param name="remoteLocation">Location (relative to image store root) where to delete the content</param>
        /// <param name="timeout">The timeout for performing the deleting operation</param>
        /// <returns></returns>
        Task DeleteContentAsync(string remoteLocation, TimeSpan timeout);
    }
}

