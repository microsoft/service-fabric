// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageStore
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Description;
    using System.Fabric.ImageStore;
    using System.Fabric.Query;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Reflection;
    using System.Threading.Tasks;
    using Microsoft.WindowsAzure.Storage;

    /// <summary>
    /// XStore image store provider. This class implements the IImageStoreProvider
    /// interface for xstore.
    /// </summary>
    internal class XStoreImageStore : IImageStore
    {
        /// <summary>Class name </summary>
        private const string ClassName = "XStoreImageStore";

        /// <summary>XStore params </summary>
        private readonly XStoreProviderParameters providerParams = null;
        private readonly string localRoot;

        /// <summary> Source for writing traces in XStoreImageStore. </summary>
        private FabricEvents.ExtensionsEvents traceSource = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.ImageStoreClient);

        static XStoreImageStore()
        {

            AppDomain.CurrentDomain.AssemblyResolve += new ResolveEventHandler(LoadFromFabricCodePath);
        }

        static Assembly LoadFromFabricCodePath(object sender, ResolveEventArgs args)
        {
            try
            {
                string folderPath = FabricEnvironment.GetCodePath();
#if DotNetCoreClr
                folderPath = Path.Combine(folderPath, "NS");
#endif
                string assemblyPath = Path.Combine(folderPath, new AssemblyName(args.Name).Name + ".dll");
                if (FabricFile.Exists(assemblyPath))
                {
                    return Assembly.LoadFrom(assemblyPath);
                }
            }
            catch (Exception)
            {
                // Supress any Exception so that we can continue to
                // load the assembly through other means
            }

            return null;
        }

        /// <summary>
        /// Initializes a new instance of the XStoreImageStore class.
        /// </summary>
        /// <param name="imageStoreUri">The image store uri.</param>
        /// <param name="localRoot">The local path of the working folder.</param>
        public XStoreImageStore(string imageStoreUri, string localRoot)
        {
            if (imageStoreUri == null)
            {
                throw new ArgumentException(StringResources.ImageStoreError_InvalidParameterSpecified, "imageStoreUri");
            }

            imageStoreUri = imageStoreUri.Replace("\\;", ";");
            imageStoreUri = imageStoreUri.Replace("\\=", "=");

            this.providerParams = new XStoreProviderParameters(imageStoreUri);
            if (!this.providerParams.IsValid())
            {
                throw new ArgumentException(StringResources.ImageStoreError_InvalidParameterSpecified, "imageStoreUri");
            }

            this.localRoot = localRoot;
        }

        /// <summary>
        /// Gets the Root uri for image store
        /// </summary>
        public string RootUri
        {
            get { return this.providerParams.RootUri; }
        }

        /// <summary>
        /// Checks whether the uri is of the xstore store.
        /// </summary>
        /// <param name="imageStoreUri">The uri of the image store.</param>
        /// <returns>True if the uri is of a xstore store and false otherwise.</returns>
        public static bool IsXStoreUri(string imageStoreUri)
        {
            return XStoreCommon.IsXStoreUri(imageStoreUri);
        }

        /// <summary>
        /// Uploads content of the local source to the image store at the remote destination.
        /// </summary>
        /// <param name="tag">Location relative to RootUri where the content needs to be uploaded.</param>
        /// <param name="sourceSoftLink">Physical location of the source.</param>
        /// <param name="timeout">The timeout for performing the uploading operation.</param>
        /// <param name="copyFlag">The copying flag specifying copy type </param>
        /// <param name="acquireSourceReaderLock">Indicates whether to acquire reader lock.</param>
        public void UploadContent(string tag, string sourceSoftLink, TimeSpan timeout, CopyFlag copyFlag, bool acquireSourceReaderLock)
        {
            this.UploadContent(tag, sourceSoftLink, null, timeout, copyFlag, acquireSourceReaderLock);
        }

        public void UploadContent(string tag, string sourceSoftLink, IImageStoreProgressHandler handler, TimeSpan timeout, CopyFlag copyFlag, bool acquireSourceReaderLock)
        {
            TimeoutHelper helper = timeout == TimeSpan.MaxValue ? null : new TimeoutHelper(timeout);
            sourceSoftLink = this.GetLocalPath(sourceSoftLink);
            if ((!FabricFile.Exists(sourceSoftLink)) &&
                (!FabricDirectory.Exists(sourceSoftLink)))
            {
                throw new IOException(
                    string.Format(
                        CultureInfo.CurrentCulture,
                        StringResources.ImageStoreError_DoesNotExistError,
                        sourceSoftLink));
            }

            try
            {
                using (XStoreFileOperations xstoreFileOperation =
                        new XStoreFileOperations(new XStoreParameters(this.providerParams.ConnectionString, this.providerParams.SecondaryConnectionString, this.providerParams.ContainerName)))
                {
                    tag = XStoreCommon.FormatXStoreUri(tag);
                    if (copyFlag == CopyFlag.AtomicCopy)
                    {
                        this.DeleteContent(tag, helper == null ? timeout : helper.GetRemainingTime());
                    }

                    if (FabricFile.Exists(sourceSoftLink))
                    {
                        // This is a file copy
                        xstoreFileOperation.CopyFile(
                            sourceSoftLink,
                            tag,
                            XStoreFileOperationType.CopyFromSMBToXStore,
                            helper);
                    }
                    else
                    {
                        // this is a folder copy
                        xstoreFileOperation.CopyFolder(
                            sourceSoftLink,
                            tag,
                            XStoreFileOperationType.CopyFromSMBToXStore,
                            copyFlag,
                            null,
                            null,
                            helper);
                    }
                }
            }
            catch (Exception ex)
            {
                this.HandleException(ex);
                throw;
            }
        }

        public Task UploadContentAsync(string storeDestination, string localSource, TimeSpan timeout, CopyFlag copyFlag, bool acquireSourceReaderLock)
        {
            return Task.Run(() =>
            {
                this.UploadContent(storeDestination, localSource, timeout, copyFlag, acquireSourceReaderLock);
            });
        }

        /// <summary>
        /// Copies content from a path in the XStore to the destination path in the XStore
        /// </summary>
        /// <param name="remoteSource"> Location relative to RootUri where the content to be copied will be found. </param>
        /// <param name="remoteDestination"> Location relative to RootUri where the content needs to be copied. </param>
        /// <param name="timeout">The timeout for performing the copying operation.</param>
        /// <param name="skipFiles">The list of the blobs to be skipped for copying </param>
        /// <param name="copyFlag">The copying control information to specify how blob can be overwritten.</param>
        /// <param name="checkMarkFile">Indicats whether to check mark file during copying.</param>
        public void CopyContent(string remoteSource, string remoteDestination, TimeSpan timeout, string[] skipFiles, CopyFlag copyFlag, bool checkMarkFile)
        {
            TimeoutHelper helper = timeout == TimeSpan.MaxValue ? null : new TimeoutHelper(timeout);
            try
            {
                using (
                    XStoreFileOperations xstoreFileOperation =
                        new XStoreFileOperations(new XStoreParameters(this.providerParams.ConnectionString, this.providerParams.SecondaryConnectionString, this.providerParams.ContainerName)))
                {
                    remoteSource = XStoreCommon.FormatXStoreUri(remoteSource);
                    remoteDestination = XStoreCommon.FormatXStoreUri(remoteDestination);

                    bool xstoreFileExist = xstoreFileOperation.XStoreFileExists(remoteSource, helper);
                    bool xstoreFolderExist = xstoreFileOperation.XStoreFolderExists(remoteSource, checkMarkFile, helper);

                    if ((!xstoreFileExist) && (!xstoreFolderExist))
                    {
                        throw new FileNotFoundException(string.Format(CultureInfo.InvariantCulture, StringResources.ImageStoreError_DoesNotExistError, remoteSource));
                    }

                    if (copyFlag == CopyFlag.AtomicCopy)
                    {
                        this.DeleteContent(remoteDestination, helper == null ? timeout : helper.GetRemainingTime());
                    }

                    if (xstoreFileExist)
                    {
                        // this is a single blob copy from xstore to xstore
                        xstoreFileOperation.CopyFile(
                            remoteSource,
                            remoteDestination,
                            XStoreFileOperationType.CopyFromXStoreToXStore,
                            helper);
                    }
                    else
                    {
                        xstoreFileOperation.CopyFolder(
                            remoteSource,
                            remoteDestination,
                            XStoreFileOperationType.CopyFromXStoreToXStore,
                            copyFlag,
                            null,
                            skipFiles,
                            helper);
                    }
                }
            }
            catch (Exception ex)
            {
                this.HandleException(ex);
                throw;
            }
        }

        /// <summary>
        /// Copies content from a path in the XStore to the destination path in the XStore
        /// </summary>
        /// <param name="storeSource">The relative path of source XStore content to be copied from.</param>
        /// <param name="storeDestination">The relative path of destination XStore content to be copied to.</param>
        /// <param name="timeout">The timeout for performing the copying operation.</param>
        /// <param name="skipFiles">The list of the files names to be skipped for copying.</param>
        /// <param name="copyFlag">The copying control information to specify how file can be overwritten.</param>
        /// <param name="checkMakeFile">Indicats whether to check mark file during copying.</param>
        /// <returns></returns>
        public Task CopyContentAsync(string storeSource, string storeDestination, TimeSpan timeout, string[] skipFiles, CopyFlag copyFlag, bool checkMakeFile)
        {
            return Task.Run(() =>
            {
                this.CopyContent(storeSource, storeDestination, timeout, skipFiles, copyFlag, checkMakeFile);
            });
        }

        /// <summary>
        /// Downloads content from the XStore to local destination.
        /// </summary>
        /// <param name="tag">Location (relative to RootUri) from where to download the content.</param>
        /// <param name="destinationSoftLink">Physical location where to download the content.</param>
        /// <param name="timeout"> Download timeout in milliseconds.</param>
        /// <param name="copyFlag">The copying control information to specify how file can be overwritten.</param>
        public void DownloadContent(string tag, string destinationSoftLink, TimeSpan timeout, CopyFlag copyFlag)
        {
            this.DownloadContent(tag, destinationSoftLink, null, timeout, copyFlag);
        }

        /// <summary>
        /// Downloads content from the XStore to local destination.
        /// </summary>
        /// <param name="tag">Location (relative to RootUri) from where to download the content.</param>
        /// <param name="destinationSoftLink">Physical location where to download the content.</param>
        /// <param name="handler">Defines the behavior to process progress information from the downloading operation.</param>
        /// <param name="timeout">The timeout for performing the downloading operation.</param>
        /// <param name="copyFlag">The copying control information to specify how file can be overwritten.</param>
        public void DownloadContent(string tag, string destinationSoftLink, IImageStoreProgressHandler handler, TimeSpan timeout, CopyFlag copyFlag)
        {
            TimeoutHelper helper = timeout == TimeSpan.MaxValue ? null : new TimeoutHelper(timeout);
            destinationSoftLink = this.GetLocalPath(destinationSoftLink);
            try
            {
                using (XStoreFileOperations xstoreFileOperation =
                        new XStoreFileOperations(new XStoreParameters(this.providerParams.ConnectionString, this.providerParams.SecondaryConnectionString, this.providerParams.ContainerName)))
                {
                    tag = XStoreCommon.FormatXStoreUri(tag);
                    bool xstoreFileExist = xstoreFileOperation.XStoreFileExists(tag, helper);
                    bool xstoreFolderExist = xstoreFileOperation.XStoreFolderExists(tag, true, helper);

                    if ((!xstoreFileExist) && (!xstoreFolderExist))
                    {
                        throw new FileNotFoundException(string.Format(CultureInfo.InvariantCulture, StringResources.ImageStoreError_DoesNotExistError, tag));
                    }

                    if (xstoreFileExist)
                    {
                        // this is a single blob copy from xstore to smb
                        xstoreFileOperation.CopyFile(
                            tag,
                            destinationSoftLink,
                            XStoreFileOperationType.CopyFromXStoreToSMB,
                            helper);
                    }
                    else
                    {
                        var operationId = Guid.NewGuid();

                        // this is a multiple blob (folder) copy from xstore to smb
                        xstoreFileOperation.CopyFolder(
                            tag,
                            destinationSoftLink,
                            XStoreFileOperationType.CopyFromXStoreToSMB,
                            copyFlag,
                            null,
                            null,
                            helper,
                            operationId);

                        var blobContentEntries = XStoreCommon.GetDownloadContentEntries(operationId);
                        if (blobContentEntries == null)
                        {
                            return;
                        }

                        var missingFiles = blobContentEntries.Where(entry => !File.Exists(entry.Item2));
                        if (missingFiles.Count() > 0)
                        {
                            //The missing file count will be traced out and there will be no retrying without remaining time.
                            //The following step to do will return proper error code after sync up with hosting and image builder.
                            if (helper != null && helper.GetRemainingTime() == TimeSpan.Zero)
                            {
                                this.traceSource.WriteWarning(
                                    ClassName,
                                    "{0} files missing after downloading, OperationID: {1}", missingFiles.Count(), operationId);
                                return;
                            }

                            this.traceSource.WriteWarning(
                                ClassName,
                                "Retry to download the {0} missing files, operationID: {1}", missingFiles.Count(), operationId);

                            foreach (var file in missingFiles)
                            {
                                xstoreFileOperation.CopyFile(
                                    file.Item1,
                                    file.Item2,
                                    XStoreFileOperationType.CopyFromXStoreToSMB,
                                    helper);
                            }

                        }
                    }
                }
            }
            catch (Exception ex)
            {
                this.HandleException(ex);
                throw;
            }
        }

        /// <summary>
        /// Downloads content from the XStore to local destination.
        /// </summary>
        /// <param name="storeSource">Location (relative to RootUri) from where to download the content.</param>
        /// <param name="localDestination">Physical location where to download the content.</param>
        /// <param name="timeout">The timeout for performing the downloading operation.</param>
        /// <param name="copyFlag">The copying control information to specify how file can be overwritten.</param>
        public Task DownloadContentAsync(string storeSource, string localDestination, TimeSpan timeout, CopyFlag copyFlag)
        {
            return Task.Run(() =>
            {
                this.DownloadContent(storeSource, localDestination, timeout, copyFlag);
            });
        }

        /// <summary>
        /// Lists all blobs under the the given relative blob path
        /// </summary>
        /// <param name="tag">Location (relative to RootUri)</param>
        /// <param name="timeout">The timeout for performing the listing operation.</param>
        /// <returns>All blobs within the input blob path</returns>
        public IEnumerable<string> ListContent(string tag, TimeSpan timeout)
        {
            var content = this.ListContentWithDetails(tag, true, timeout);
            if (content.Files != null && content.Files.Count > 0)
            {
                var relativePaths = from file in content.Files
                                    select file.StoreRelativePath;
                return relativePaths.ToArray();
            }

            return null;
        }

        /// <summary>
        /// Gets the paged XStore blob relative paths.
        /// Currently no implementation and has not been supported at the XStore.
        /// </summary>
        /// <param name="listDescription">The information on how to get image store content.</param>
        /// <param name="timeout">The timeout for performing the listing operation.</param>
        /// <returns></returns>
        //Messgae based communication doesn't exist in the XStore. Paging doesn't need to be implemented here.
        public ImageStorePagedRelativePath ListPagedContent(ImageStoreListDescription listDescription, TimeSpan timeout)
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// Lists all blobs under the the given relative blob path
        /// </summary>
        /// <param name="storeSource">Location (relative to RootUri)</param>
        /// <param name="timeout">he timeout for performing the listing operation.</param>
        /// <returns></returns>
        public async Task<IEnumerable<string>> ListContentAsync(string storeSource, TimeSpan timeout)
        {
            return await Task.Run(() =>
            {
                return this.ListContent(storeSource, timeout);
            });
        }

        /// <summary>
        /// Gets the paged XStore blob relative paths.
        /// Currently no implementation and has not been supported at the XStore.
        /// </summary>
        /// <param name="listDescription">The information on how to get image store content.</param>
        /// <param name="timeout">The timeout for performing the listing operation.</param>
        /// <returns></returns>
        public async Task<ImageStorePagedRelativePath> ListPagedContentAsync(ImageStoreListDescription listDescription, TimeSpan timeout)
        {
            return await Task.Run(() =>
            {
                return this.ListPagedContent(listDescription, timeout);
            });
        }

        /// <summary>
        /// List all blobs under the the given relative blob path/sub blob path
        /// </summary>
        /// <param name="tag">Location (relative to RootUri)</param>
        /// <param name="isRecursive">flag list subhierarchy</param>
        /// <param name="timeout">The timeout for performing the listing operation</param>
        /// <returns>All blobs within the input blob path/sub blob path</returns>
        public ImageStoreContent ListContentWithDetails(string tag, bool isRecursive, TimeSpan timeout)
        {
            ImageStoreContent content = null;
            TimeoutHelper helper = timeout == TimeSpan.MaxValue ? null : new TimeoutHelper(timeout);
            try
            {
                using (XStoreFileOperations xstoreFileOperation =
                            new XStoreFileOperations(new XStoreParameters(this.providerParams.ConnectionString, this.providerParams.SecondaryConnectionString, this.providerParams.ContainerName)))
                {
                    content = xstoreFileOperation.ListXStoreFile(XStoreCommon.FormatXStoreUri(tag), isRecursive, helper);
                }
            }
            catch (Exception ex)
            {
                this.HandleException(ex);
                throw;
            }

            return content;
        }


        /// <summary>
        /// Get the paged XStore content information.
        /// Currently no implementation and has not been supported at the XStore.
        /// </summary>
        /// <param name="listDescription">The information on how to get XStore content.</param>
        /// <param name="timeout">The timeout for performing the listing operation.</param>
        public ImageStorePagedContent ListPagedContentWithDetails(ImageStoreListDescription listDescription, TimeSpan timeout)
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// List all blobs under the the given relative blob path/sub blob path
        /// </summary>
        /// <param name="storeSource">Location (relative to RootUri)</param>
        /// <param name="isRecursive">flag list subhierarchy</param>
        /// <param name="timeout">The timeout for performing the listing operation</param>
        /// <returns>All blobs within the input blob path/sub blob path</returns>
        public async Task<ImageStoreContent> ListContentWithDetailsAsync(string storeSource, bool isRecursive, TimeSpan timeout)
        {
            return await Task.Run(() =>
            {
                return this.ListContentWithDetails(storeSource, isRecursive, timeout);
            });
        }

        /// <summary>
        /// Get the paged XStore content information.
        /// Currently no implementation and has not been supported at the XStore.
        /// </summary>
        /// <param name="listDescription">The information on how to get XStore content.</param>
        /// <param name="timeout">The timeout for performing the listing operation.</param>
        public async Task<ImageStorePagedContent> ListPagedContentWithDetailsAsync(ImageStoreListDescription listDescription, TimeSpan timeout)
        {
            return await Task.Run(() =>
            {
                return this.ListPagedContentWithDetails(listDescription, timeout);
            });
        }

        /// <summary>
        /// Check if content is present in the store.
        /// </summary>
        /// <param name="tag"> Location (relative to RootUri) where to check the presence of the content. </param>
        /// <param name="timeout">The timeout for performing the checking existence operation</param>
        /// <returns> True if the content exists, false otherwise. </returns>
        public bool DoesContentExist(string tag, TimeSpan timeout)
        {
            TimeoutHelper helper = timeout == TimeSpan.MaxValue ? null : new TimeoutHelper(timeout);
            bool isExist = false;
            try
            {
                using (
                    XStoreFileOperations xstoreFileOperation =
                        new XStoreFileOperations(new XStoreParameters(this.providerParams.ConnectionString, this.providerParams.SecondaryConnectionString, this.providerParams.ContainerName)))
                {
                    tag = XStoreCommon.FormatXStoreUri(tag);
                    isExist = xstoreFileOperation.XStoreFileExists(tag, helper) || xstoreFileOperation.XStoreFolderExists(tag, true, helper);
                }
            }
            catch (Exception ex)
            {
                this.HandleException(ex);
                throw;
            }

            return isExist;
        }

        /// <summary>
        ///  Check if content is present in the XStore.
        /// </summary>
        /// <param name="tag">Location (relative to RootUri) where to check the presence of the content.</param>
        /// <param name="timeout">The timeout for performing the checking existence operation</param>
        /// <returns></returns>
        public Task<bool> DoesContentExistAsync(string tag, TimeSpan timeout)
        {
            return Task.Run<bool>(() =>
            {
                return this.DoesContentExist(tag, timeout);
            });
        }

        /// <summary>
        /// Remove tag from store, clear data assosciate with tag.
        /// </summary>
        /// <param name="tag">Location (relative to RootUri) from where to delete the content.</param>
        ///<param name="timeout">The timeout for performing the deleting operation</param>
        public void DeleteContent(string tag, TimeSpan timeout)
        {
            TimeoutHelper helper = timeout == TimeSpan.MaxValue ? null : new TimeoutHelper(timeout);
            try
            {
                using (
                    XStoreFileOperations xstoreFileOperation =
                        new XStoreFileOperations(new XStoreParameters(this.providerParams.ConnectionString, this.providerParams.SecondaryConnectionString, this.providerParams.ContainerName)))
                {
                    tag = XStoreCommon.FormatXStoreUri(tag);

                    if (!xstoreFileOperation.XStoreContainerExists(helper))
                    {
                        return;
                    }

                    if (string.IsNullOrEmpty(tag))
                    {
                        // remove the container in this case
                        xstoreFileOperation.RemoveContainer(helper);
                    }
                    else if (xstoreFileOperation.XStoreFileExists(tag, helper))
                    {
                        xstoreFileOperation.RemoveXStoreFile(tag, helper);
                    }
                    else
                    {
                        xstoreFileOperation.RemoveXStoreFolder(tag, helper);
                    }
                }
            }
            catch (Exception ex)
            {
                this.HandleException(ex);
                throw;
            }
        }

        /// <summary>
        /// Remove tag from store, clear data assosciate with tag.
        /// </summary>
        /// <param name="tag">Location (relative to RootUri) from where to delete the content.</param>
        ///<param name="timeout">The timeout for performing the deleting operation</param>
        public Task DeleteContentAsync(string tag, TimeSpan timeout)
        {
            return Task.Run(() =>
            {
                this.DeleteContent(tag, timeout);
            });
        }

        private void HandleException(Exception ex)
        {
            if (ex.GetType() == typeof(IOException))
            {
                throw new FabricImageStoreIOException(ex);
            }

            StorageException stException = ex as StorageException;
            if (null != stException)
            {
                if (stException.InnerException is TimeoutException)
                {
                    throw new TimeoutException(stException.InnerException.Message);
                }
            }
        }

        private string GetLocalPath(string tag)
        {
            return string.IsNullOrEmpty(this.localRoot) ? tag : Path.Combine(this.localRoot, tag);
        }

        /// <summary>
        ///
        /// </summary>
        private class BlobNameEqualityComparer : IEqualityComparer<string>
        {
            private Func<string, string> blobNameFilter;

            public BlobNameEqualityComparer(Func<string, string> blobNameFilter)
            {
                this.blobNameFilter = blobNameFilter;
            }

            public bool Equals(string x, string y)
            {
                x = blobNameFilter(x);
                y = blobNameFilter(y);
                return string.Compare(x, y, true) == 0;
            }

            public int GetHashCode(string blobUri)
            {
                return blobUri.GetHashCode();
            }
        }

        /// <summary>
        /// Parameter class for xstore provider; which parse the
        /// imagestore uri and get information of blob storage
        /// account, key and imagestore container
        /// </summary>
        public class XStoreProviderParameters
        {
            /// <summary>XStore storage connection string </summary>
            private readonly string connectionString = null;

            /// <summary>XStore storage connection string </summary>
            private readonly string secondaryConnectionString = null;

            /// <summary>XStore blob storage endpoint </summary>
            private readonly string blobEndpoint = null;

            /// <summary>XStore blob storage container for image store </summary>
            private readonly string containerName = null;

            /// <summary>Image store uri </summary>
            private readonly string imageStoreUri = null;

            /// <summary>
            /// Initializes a new instance of the XStoreProviderParameters class.
            /// </summary>
            /// <param name="imageStoreUri">Image store uri: format = "xstore:{connection string};Container={2}"</param>
            public XStoreProviderParameters(string imageStoreUri)
            {
                string accountName, endpointSuffix;
                this.imageStoreUri = imageStoreUri;
                XStoreCommon.TryParseImageStoreUri(
                    imageStoreUri,
                    out this.connectionString,
                    out this.secondaryConnectionString,
                    out this.blobEndpoint,
                    out this.containerName,
                    out accountName,
                    out endpointSuffix);
            }

            /// <summary> Gets the xstore storage connection string </summary>
            public string ConnectionString
            {
                get { return this.connectionString; }
            }

            /// <summary> Gets the xstore storage secondary connection string </summary>
            public string SecondaryConnectionString
            {
                get { return this.secondaryConnectionString; }
            }

            /// <summary> Gets the xstore blobendpoint string </summary>
            public string BlobEndpoint
            {
                get { return this.blobEndpoint; }
            }

            /// <summary> Gets the xstore container name </summary>
            public string ContainerName
            {
                get { return this.containerName; }
            }

            /// <summary> Gets the Root URI of the image store. </summary>
            public string RootUri
            {
                get { return this.imageStoreUri; }
            }

            /// <summary>
            /// Function to check if this is a valid xstore parameter
            /// </summary>
            /// <returns>
            /// Returns true to indicate valid
            /// </returns>
            public bool IsValid()
            {
                return (!string.IsNullOrEmpty(this.connectionString)) &&
                       (!string.IsNullOrEmpty(this.blobEndpoint) &&
                       (!string.IsNullOrEmpty(this.containerName)));
            }
        }
    }
}

