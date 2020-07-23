// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.ImageStore
{
    using System;
    using System.Text;
    using System.Linq;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Fabric.Query;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Security.AccessControl;
    using System.Security.Principal;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// Implements a file share based image store.
    /// </summary>
    internal class FileImageStore : IImageStore
    {
        /// <summary>
        /// The tag for file image store scheme.
        /// </summary>
        public const string SchemeTag = @"file:";

        internal static FabricEvents.ExtensionsEvents TraceSource
        {
            get
            {
                return traceSource;
            }
        }

        private static FabricEvents.ExtensionsEvents traceSource;
        private static readonly string TraceType = "FileImageStore";

        
        /// <summary>
        /// This store keeps the parameters for the store.
        /// </summary>
        private readonly FileProviderParameters storeParams = null;

        /// <summary>
        /// The root URI of the store.
        /// </summary>
        private readonly string rootUri = null;

        private readonly string localRoot;
        
        private ImageStoreAccessDescription accessDescription;
       
        static FileImageStore()
        {
            // Try reading config (this may require access to tracing which is why we setup something up above
            TraceConfig.InitializeFromConfigStore();

            // Create the trace source based on the config
            FileImageStore.traceSource = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.ImageStoreClient);
        }

        /// <summary>
        /// Initializes a new instance of the FileImageStore class.
        /// </summary>
        /// <param name="uri">The root URI (the root folder) of the image store.</param>
        /// <param name="localRoot">The default path for the file image store root.</param>
        /// <param name="accessDescription">Access rights to specific users or groups.</param>
        public FileImageStore(
            string uri, 
            string localRoot = null, 
            ImageStoreAccessDescription accessDescription = null)
        {
            if (uri == null)
            {
                throw new ArgumentException(StringResources.ImageStoreError_InvalidParameterSpecified, "uri");
            }

            this.localRoot = localRoot;
            this.accessDescription = accessDescription;
            this.storeParams = new FileProviderParameters(uri);

            if (!this.storeParams.RootUri.StartsWith(SchemeTag, StringComparison.OrdinalIgnoreCase))
            {
                throw new InvalidOperationException(string.Format(CultureInfo.CurrentCulture, StringResources.ImageStoreError_InvalidImageStoreUriScheme, this.storeParams.RootUri));
            }

            this.rootUri = this.storeParams.RootUri.TrimStart().Substring(SchemeTag.Length).Trim();

#if !DotNetCoreClr
            using (WindowsImpersonationContext impersonationContext = this.GetImpersonationContext())
#endif
            {
                if (!FabricDirectory.Exists(this.rootUri))
                {
	                throw new DirectoryNotFoundException(string.Format(CultureInfo.CurrentCulture, StringResources.ImageStoreError_UnaccessibleImageStore, this.rootUri));
                }
            }
        }

        /// <summary>
        /// Gets the root URI of the store.
        /// </summary>
        public string RootUri
        {
            get { return this.storeParams.RootUri; }
        }

        #region Public Methods
        /// <summary>
        /// Checks whether the URI is of the file store.
        /// </summary>
        /// <param name="imageStoreUri">The URI of the image store.</param>
        /// <returns>True if the URI is of a file store and false otherwise.</returns>
        internal static bool IsFileStoreUri(string imageStoreUri)
        {
            return imageStoreUri.Trim().StartsWith(FileImageStore.SchemeTag, StringComparison.OrdinalIgnoreCase);
        }

        /// <summary>
        /// Uploads the local source content to the image store at the remote destination.
        /// </summary>
        /// <param name="storeDestination"> Location relative to RootUri where the content needs to be uploaded. </param>
        /// <param name="localSource"> Physical location of the source. </param>
        /// <param name="timeout"> Upload timeout. </param>
        /// <param name="copyFlag">The copying control information to specify how file can be overwritten.</param>
        /// <param name="acquireSourceReaderLock">Indicates whether to acquire reader lock.</param>
        public void UploadContent(string storeDestination, string localSource, TimeSpan timeout, CopyFlag copyFlag, bool acquireSourceReaderLock)
        {
            this.UploadContent(storeDestination, localSource, null, timeout, copyFlag, acquireSourceReaderLock);
        }

        /// <summary>
        /// Uploads the local source content to the image store at the remote destination.
        /// </summary>
        /// <param name="storeDestination"> Location relative to RootUri where the content needs to be uploaded. </param>
        /// <param name="localSource"> Physical location of the source. </param>
        /// <param name="handler">The image store progress handler which is not supported at the file image store</param>
        /// <param name="timeout"> Upload timeout. </param>
        /// <param name="copyFlag">The copying control information to specify how file can be overwritten.</param>
        /// <param name="acquireSourceReaderLock">Indicates whether to acquire reader lock.</param>
        public void UploadContent(string storeDestination, string localSource, IImageStoreProgressHandler handler, TimeSpan timeout, CopyFlag copyFlag, bool acquireSourceReaderLock)
        {
            TimeoutHelper helper = timeout == TimeSpan.MaxValue ? null : new TimeoutHelper(timeout);

            storeDestination = storeDestination.TrimEnd('\\');
            if (string.Compare(storeDestination, Constants.StoreFolder, true) == 0 ||
                string.Compare(storeDestination, Constants.WindowsFabricStoreFolder, true) == 0)
            {
                throw new FabricImageBuilderReservedDirectoryException(string.Format(StringResources.Error_InvalidReservedImageStoreOperation, storeDestination));
            }

            localSource = this.GetLocalPath(localSource);
            if ((!FabricFile.Exists(localSource)) &&
                (!FabricDirectory.Exists(localSource)))
            {
                throw new IOException(string.Format(CultureInfo.CurrentCulture, StringResources.ImageStoreError_DoesNotExistError, localSource));
            }

            string tempDirectory = null;
            FileReaderLock readerLock = null;
            try
            {
                string smbTag = this.ConvertTagToSMBPath(storeDestination);
                if (acquireSourceReaderLock)
                {
                    readerLock = new FileReaderLock(localSource);
                    if (!readerLock.Acquire())
                    {
                        throw new FabricTransientException(StringResources.Error_ImageStoreAcquireFileLockFailed, FabricErrorCode.ImageStoreAcquireFileLockFailed);
                    }
                }

                if (helper != null)
                {
                    helper.ThrowIfExpired();
                }

                if (accessDescription != null && !accessDescription.HasReadAccess)
                {
                    tempDirectory = CreateAndAclDirectory();
                    string tempLocalResource = Path.Combine(tempDirectory, Path.GetRandomFileName());

                    // Copy the content from local source to a temp location outside ImpersonationContext
                    CopyCallerHoldsReaderLock(localSource, tempLocalResource, CopyFlag.AtomicCopy, helper);
                 
                    if (readerLock != null) { readerLock.Release(); }

#if !DotNetCoreClr
                    using (WindowsImpersonationContext impersonationContext = accessDescription.WindowsIdentity.Impersonate())
#endif
                    {
                        // Copy the content from the temp location to the remote Store in the ImpersonationContext
                        CopyCallerHoldsReaderLock(tempLocalResource, smbTag, copyFlag, helper);
                    }
                }

#if !DotNetCoreClr
                using (WindowsImpersonationContext impersonationContext = this.GetImpersonationContext())
#endif
                {
                    CopyCallerHoldsReaderLock(localSource, smbTag, copyFlag, helper);
                }
            }
            catch (IOException exception)
            {
                if (exception.GetType() == typeof(IOException))
                {
                    throw new FabricImageStoreException(StringResources.Error_ImageStoreIOException, exception);
                }
                else
                {
                    throw;
                }
            }
            finally
            {
                if (readerLock != null) { readerLock.Dispose(); }

                if (tempDirectory != null && FabricDirectory.Exists(tempDirectory))
                {
                    FabricDirectory.Delete(tempDirectory, recursive: true, deleteReadOnlyFiles: true);
                }
            }
        }

        /// <summary>
        /// Uploads the local source content to the image store at the remote destination.
        /// </summary>
        /// <param name="storeDestination">Location relative to RootUri where the content needs to be uploaded.</param>
        /// <param name="localSource">Local location of the source.</param>
        /// <param name="timeout"> The timeout for performing the uploading operation.</param>
        /// <param name="copyFlag">The copying control information to specify how file can be overwritten.</param>
        /// <param name="acquireSourceReaderLock">Indicates whether to acquire reader lock.</param>
        /// <returns></returns>
        public Task UploadContentAsync(string storeDestination, string localSource, TimeSpan timeout, CopyFlag copyFlag, bool acquireSourceReaderLock)
        {
            return Task.Run(() =>
                {
                    this.UploadContent(storeDestination, localSource, timeout, copyFlag, acquireSourceReaderLock);
                });
        }
        
        /// <summary>
        /// Uploads the local source content to the image store at the remote destination.
        /// </summary>
        /// <param name="storeSource"> Location (relative to RootUri) from where to download the content. </param>
        /// <param name="storeDestination"> Location relative to RootUri where the content needs to be uploaded. </param>
        /// <param name="timeout">The timeout for copy content operation.</param>
        /// <param name="skipFiles">Files that do not need to be copied.</param>
        /// <param name="copyFlag">The copying control information to specify how file can be overwritten</param>
        /// <param name="checkMarkFile">Flag the specified the checkmark file.</param>
        public void CopyContent(string storeSource, string storeDestination, TimeSpan timeout, string[] skipFiles, CopyFlag copyFlag, bool checkMarkFile)
        {
            TimeoutHelper helper = timeout == TimeSpan.MaxValue ? null : new TimeoutHelper(timeout);
            FileReaderLock readerLock = null;
            try
            {
                string smbSourcePath = this.ConvertTagToSMBPath(storeSource);
                string smbDestinationPath = this.ConvertTagToSMBPath(storeDestination);

                readerLock = new FileReaderLock(smbSourcePath);
                if (!readerLock.Acquire())
                {
                    throw new FabricTransientException(StringResources.Error_ImageStoreAcquireFileLockFailed, FabricErrorCode.ImageStoreAcquireFileLockFailed);
                }

                if (helper != null)
                {
                    helper.ThrowIfExpired();
                }

#if !DotNetCoreClr
                using (WindowsImpersonationContext impersonationContext = this.GetImpersonationContext())
#endif
                {
                    bool fabricDirectoryExists = FabricDirectory.Exists(smbSourcePath);
                    if (fabricDirectoryExists && skipFiles.Any())
                    {
                        string[] fullFileNames = FabricDirectory.GetFiles(smbSourcePath, "*", true, SearchOption.AllDirectories);
                        var relativeFilePath = from file in fullFileNames
                                               select file.Replace(smbSourcePath, "").TrimStart(new char[] { Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar });

                        string[] filtered = relativeFilePath.Where(file => !skipFiles.Contains<string>(Path.GetFileName(file))).ToArray<string>();

                        if (filtered.Count() < fullFileNames.Count())
                        {
                            foreach (string file in filtered)
                            {
                                CopyCallerHoldsReaderLock(Path.Combine(smbSourcePath, file), Path.Combine(smbDestinationPath, file), copyFlag, helper);
                            }
                        }
                        else
                        {
                            CopyCallerHoldsReaderLock(smbSourcePath, smbDestinationPath, copyFlag, helper);
                        }
                    }
                    else
                    {
                        bool fabricFileExist = FabricFile.Exists(smbSourcePath);
                        if ((!FabricFile.Exists(smbSourcePath)) &&
                            (!FabricDirectory.Exists(smbSourcePath)))
                        {
                            throw new IOException(string.Format(CultureInfo.CurrentCulture, StringResources.ImageStoreError_DoesNotExistError, smbSourcePath));
                        }

                        CopyCallerHoldsReaderLock(smbSourcePath, smbDestinationPath, copyFlag, helper);
                    }
                }
            }
            catch (IOException exception)
            {
                if (exception.GetType() == typeof(IOException))
                {
                    throw new FabricImageStoreIOException(exception);
                }
                else
                {
                    throw;
                }
            }
            finally
            {
                if (readerLock != null) { readerLock.Dispose(); }
            }
        }

        /// <summary>
        /// Copies file image store content internally
        /// </summary>
        /// <param name="storeSource">The relative path of source file image store to be copied from.</param>
        /// <param name="storeDestination">The relative path of destination file image store content to be copied to.</param>
        /// <param name="timeout">The timeout for performing the copying operation.</param>
        /// <param name="skipFiles">The list of the files names to be skipped for copying.</param>
        /// <param name="copyFlag">The copying control information to specify how file can be overwritten.</param>
        /// <param name="checkMakeFile">Indicates whether to check mark file during copying.</param>
        /// <returns></returns>
        public Task CopyContentAsync(string storeSource, string storeDestination, TimeSpan timeout, string[] skipFiles, CopyFlag copyFlag, bool checkMakeFile)
        {
            return Task.Run(() =>
                {
                    this.CopyContent(storeSource, storeDestination, timeout, skipFiles, copyFlag, checkMakeFile);
                });
        }

        /// <summary>
        /// Downloads content from the image store to local destination.
        /// </summary>
        /// <param name="storeSource"> Location (relative to RootUri) from where to download the content. </param>
        /// <param name="localDestination"> Physical location where to download the content. </param>
        /// <param name="copyFlag">Copy control information to specify how file can be overwritten</param>
        /// <param name="timeout">Download timeout</param>
        public void DownloadContent(string storeSource, string localDestination, TimeSpan timeout, CopyFlag copyFlag)
        {
            this.DownloadContent(storeSource, localDestination, null, timeout, copyFlag);
        }

        /// <summary>
        /// Downloads content from the file image store to local destination.
        /// </summary>
        /// <param name="storeSource">The relative path of source file image store to be downloaded from.</param>
        /// <param name="localDestination">The local destination path to download the content.</param>
        /// <param name="handler">The image store progress handler which is not supported at the file image store.</param>
        /// <param name="timeout">The timeout for performing the downloading operation.</param>
        /// <param name="copyFlag">The copying control information to specify how file can be overwritten.</param>
        public void DownloadContent(string storeSource, string localDestination, IImageStoreProgressHandler handler, TimeSpan timeout, CopyFlag copyFlag)
        {
            TimeoutHelper helper = timeout == TimeSpan.MaxValue ? null : new TimeoutHelper(timeout);
            localDestination = this.GetLocalPath(localDestination);
            string tempDirectory = null;
            FileReaderLock readerLock = null;
            try
            {
                string smbTag = this.ConvertTagToSMBPath(storeSource);
                if ((!FabricFile.Exists(smbTag)) &&
                    (!FabricDirectory.Exists(smbTag)))
                {
                    throw new IOException(string.Format(CultureInfo.CurrentCulture, StringResources.ImageStoreError_DoesNotExistError, smbTag));
                }

                readerLock = new FileReaderLock(smbTag);
                if (!readerLock.Acquire())
                {
                    throw new FabricTransientException(StringResources.Error_ImageStoreAcquireFileLockFailed, FabricErrorCode.ImageStoreAcquireFileLockFailed);
                }

                if (helper != null)
                {
                    helper.ThrowIfExpired();
                }

                if (accessDescription != null && !accessDescription.HasWriteAccess)
                {
                    // Copy the content from the remote Store to a temp location using the ImpersonationContext
                    string tempLocalResource = null;
#if !DotNetCoreClr
                    using (WindowsImpersonationContext impersonationContext = accessDescription.WindowsIdentity.Impersonate())
#endif
                    {
                        tempDirectory = CreateAndAclDirectory();
                        tempLocalResource = Path.Combine(tempDirectory, Path.GetRandomFileName());
                        CopyCallerHoldsReaderLock(smbTag, tempLocalResource, CopyFlag.AtomicCopy, helper);
                    }

                    readerLock.Release();

                    // Copy the content from the temp location to the local destination outside the ImpersonationContext
                    CopyCallerHoldsReaderLock(tempLocalResource, localDestination, copyFlag, helper);
                }

#if !DotNetCoreClr
                using (WindowsImpersonationContext impersonationContext = this.GetImpersonationContext())
#endif
                {
                    CopyCallerHoldsReaderLock(smbTag, localDestination, copyFlag, helper);
                }
            }
            catch (IOException exception)
            {
                if (exception.GetType() == typeof(IOException))
                {
                    throw new FabricImageStoreException(StringResources.Error_ImageStoreIOException, exception);
                }
                else
                {
                    throw;
                }
            }
            finally
            {
                if (readerLock != null) { readerLock.Dispose(); }

                if (tempDirectory != null && FabricDirectory.Exists(tempDirectory))
                {
                    FabricDirectory.Delete(tempDirectory, recursive: true, deleteReadOnlyFiles: true);
                }
            }
        }

        /// <summary>
        /// Downloads content from the file image store to local destination.
        /// </summary>
        /// <param name="storeSource">The relative path of source file image store to be downloaded from.</param>
        /// <param name="localDestination">The local destination path to download the content.</param>
        /// <param name="timeout">The timeout for performing the downloading operation.</param>
        /// <param name="copyFlag">The copying control information to specify how file can be overwritten.</param>
        /// <returns></returns>
        public Task DownloadContentAsync(string storeSource, string localDestination, TimeSpan timeout, CopyFlag copyFlag)
        {
            return Task.Run(() =>
                {
                    this.DownloadContent(storeSource, localDestination, timeout, copyFlag);
                });
        }

        /// <summary>
        /// All files at the given folder
        /// </summary>
        /// <param name="storeSource">Location (relative to root)</param>
        /// <param name="timeout">List timeout</param>
        /// <returns>File name of each files within the given location</returns>
        public IEnumerable<string> ListContent(string storeSource, TimeSpan timeout)
        {
            List<string> files = new List<string>();
            try
            {
                string smbTag = this.ConvertTagToSMBPath(storeSource);
                if (!FabricDirectory.Exists(smbTag))
                {
                    TraceSource.WriteWarning(
                        TraceType,
                        "Directory {0} doesn't exist",
                        smbTag);
                }
                else
                {
                    this.GetAllFileFullNames(smbTag, timeout).ForEach<string>(fullName => files.Add(fullName.Substring(this.rootUri.Length + 1)));
                }
            }
            catch (IOException exception)
            {
                if (exception.GetType() == typeof(IOException))
                {
                    throw new FabricImageStoreException(StringResources.Error_ImageStoreIOException, exception);
                }
                else
                {
                    throw;
                }
            }

            return files;
        }

        /// <summary>
        /// Gets the paged image store relative paths.
        /// Not currently supported for file image store.
        /// </summary>
        /// <param name="listDescription">The information on how to get image store content.</param>
        /// <param name="timeout">The timeout for performing the listing operation.</param>
        /// <returns></returns>
        public ImageStorePagedRelativePath ListPagedContent(ImageStoreListDescription listDescription, TimeSpan timeout)
        {
            // Message based communication doesn't exist in the file share image store. Paging doesn't need to be implemented here. 
            throw new NotImplementedException();
        }

        /// <summary>
        /// Get the paged image store relative paths.
        /// </summary>
        /// <param name="storeSource">The relative path to file or folder from image store root.</param>
        /// <param name="timeout">The timeout for performing the listing operation.</param>
        /// <returns></returns>
        public async Task<IEnumerable<string>> ListContentAsync(string storeSource, TimeSpan timeout)
        {
            return await Task.Run(() =>
            {
                return this.ListContent(storeSource, timeout);
            });
        }

        /// <summary>
        /// Get the image store content paged information which is not supported at the file image store
        /// </summary>
        /// <param name="listDescription">The information on how to get image store content</param>
        /// <param name="timeout">The timeout for performing the listing operation</param>
        /// <returns></returns>
        public async Task<ImageStorePagedRelativePath> ListPagedContentAsync(ImageStoreListDescription listDescription, TimeSpan timeout)
        {
            return await Task.Run(() =>
            {
                return this.ListPagedContentAsync(listDescription, timeout);
            });
        }

        /// <summary>
        /// Get the image store content information including all files and subfolders 
        /// </summary>
        /// <param name="storeSource">The relative path to file or folder from image store root</param>
        /// <param name="isRecursive">flag list subhierarchy</param>
        /// <param name="timeout">The timeout for performing the listing operation</param>
        /// <returns>ImageStoreContent object including files and subfolders</returns>
        public ImageStoreContent ListContentWithDetails(string storeSource, bool isRecursive, TimeSpan timeout)
        {
            ImageStoreContent content = null;
            try
            {
                string smbTag = this.ConvertTagToSMBPath(storeSource);
                if (!FabricDirectory.Exists(smbTag))
                {
                    if (FabricFile.Exists(smbTag))
                    {
                        var fullSourcePath = FabricPath.GetFullPath(storeSource);
                        content = new ImageStoreContent()
                        {
                            Files = new List<ImageStoreFile>() { this.ConvertToImageStoreFile(fullSourcePath, storeSource) }
                        };
                    }
                    else
                    {
                        TraceSource.WriteWarning(
                            TraceType,
                            "Directory/File {0} doesn't exist",
                            smbTag);
                    }
                }
                else
                {
                    content = isRecursive
                        ? new ImageStoreContent() { Files = this.GetAllFiles(smbTag, storeSource).ToList() }
                        : this.GetFilesAndSubFolders(smbTag, storeSource, timeout);
                }
            }
            catch (IOException exception)
            {
                if (exception.GetType() == typeof(IOException))
                {
                    throw new FabricImageStoreException(string.Format(CultureInfo.CurrentCulture, StringResources.Error_ImageStoreIOException, exception));
                }
                else
                {
                    throw;
                }
            }

            return content;
        }

        /// <summary>
        /// Get the paged image store content information 
        /// Currently no implementation and has not been supported at the file image store
        /// </summary>
        /// <param name="listDescription">The information how to get image store content</param>
        /// <param name="timeout">The timeout for performing the listing operation</param>
        /// <returns></returns>
        public ImageStorePagedContent ListPagedContentWithDetails(ImageStoreListDescription listDescription, TimeSpan timeout)
        {
            // Message based communication doesn't exist in the file share image store. Paging doesn't need to be implemented here. 
            throw new NotImplementedException();
        }

        /// <summary>
        /// Get the file image store content including all files and subfolders
        /// </summary>
        /// <param name="storeSource">Location (relative to root)</param>
        /// <param name="isRecursive">flag list subhierarchy</param>
        /// <param name="timeout">The timeout for performing the listing operation</param>
        /// <returns></returns>
        public async Task<ImageStoreContent> ListContentWithDetailsAsync(string storeSource, bool isRecursive, TimeSpan timeout)
        {
            return await Task.Run(() =>
            {
                return this.ListContentWithDetails(storeSource, isRecursive, timeout);
            });
        }

        /// <summary>
        /// Get the paged image store content information.
        /// Currently no implementation and has not been supported at the file image store
        /// </summary>
        /// <param name="listDescription">The information how to get image store content</param>
        /// <param name="timeout">The timeout for performing the listing operation</param>
        /// <returns></returns>
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
        /// <param name="timeout">Check existence timeout</param>
        /// <returns> True if the content exists, false otherwise. </returns>
        public bool DoesContentExist(string tag, TimeSpan timeout)
        {
            TimeoutHelper helper = timeout == TimeSpan.MaxValue ? null : new TimeoutHelper(timeout);
#if !DotNetCoreClr
            using (WindowsImpersonationContext impersonationContext = this.GetImpersonationContext())
#endif
            {
                string smbTag = this.ConvertTagToSMBPath(tag);
                if(FabricFile.Exists(smbTag))
                {
                    return !FileLock.DoesWriterLockExist(smbTag);
                }

                if(helper != null)
                {
                    helper.ThrowIfExpired();
                }

                if(FabricDirectory.Exists(smbTag))
                {
                    return !FileLock.DoesWriterLockExist(smbTag);
                }
            }

            return false;
        }

        /// <summary>
        /// Check if content is present in the store.
        /// </summary>
        /// <param name="tag"> Location (relative to RootUri) where to check the presence of the content. </param>
        /// <param name="timeout">Check existence timeout</param>
        /// <returns> True if the content exists, false otherwise. </returns>
        public Task<bool> DoesContentExistAsync(string tag, TimeSpan timeout)
        {
            return Task.Run<bool>(() =>
            {
                return this.DoesContentExist(tag, timeout);
            });
        }

        /// <summary>
        /// Remove tag from store and clear the data associated with tag.
        /// </summary>
        /// <param name="tag">Location (relative to RootUri) from where to delete the content.</param>
        /// <param name="timeout">The timeout for performing the delete operation.</param>
        public void DeleteContent(string tag, TimeSpan timeout)
        {
            TimeoutHelper helper = (timeout == TimeSpan.MaxValue) ? null : new TimeoutHelper(timeout);
            
#if !DotNetCoreClr
            using (WindowsImpersonationContext impersonationContext = this.GetImpersonationContext())
#endif
            {
                try
                {
                    string smbTag = this.ConvertTagToSMBPath(tag);
                    if ((!FabricFile.Exists(smbTag)) &&
                        (!FabricDirectory.Exists(smbTag)))
                    {
                        return;
                    }

                    using (FileWriterLock fileWriterLock = new FileWriterLock(smbTag))
                    {
                        if (!fileWriterLock.Acquire())
                        {
                            throw new FabricTransientException(StringResources.Error_ImageStoreAcquireFileLockFailed, FabricErrorCode.ImageStoreAcquireFileLockFailed);
                        }

                        if (helper != null)
                        {
                            helper.ThrowIfExpired();
                        }

                        if (FabricFile.Exists(smbTag))
                        {
                            FabricFile.Delete(smbTag, deleteReadonly: true);
                        }
                        else if (FabricDirectory.Exists(smbTag))
                        {
                            FabricDirectory.Delete(smbTag, recursive: true, deleteReadOnlyFiles: true);
                        }
                    }
                }
                catch (IOException exception)
                {
                    // HResult 0x80070020: THE PROCESS CANNOT ACCESS THE FILE BECAUSE IT IS BEING USED BY ANOTHER PROCESS
                    // This work-around is done here since when we read a file from a folder we only take a reader lock on the file and not the 
                    // entire folder. When we delete a folder, we only take writer lock on the folder and hence we might run into scenarios 
                    // where a folder is attempted to be deleted even when a file in it is being read. 
                    // Returning FabricImageStoreException, causes CM to retry the delete operation.
                    int hrError = Marshal.GetHRForException(exception);
                    TraceSource.WriteWarning(
                        TraceType,
                        "Delete of {0} failed because of {1}, hrError {2:X}.",
                        tag,
                        exception.Message,
                        hrError);

                    if (hrError == unchecked((int)0x80070020))
                    {
                        throw new FabricTransientException(StringResources.Error_ImageStoreAcquireFileLockFailed, exception, FabricErrorCode.ImageStoreAcquireFileLockFailed);
                    }

                    throw;
                }
            }
        }

        /// <summary>
        /// Remove tag from store and clear data associated with tag.
        /// </summary>
        /// <param name="tag">Location (relative to RootUri) from where to delete the content.</param>
        /// <param name="timeout">The timeout for performing the listing operation</param>
        /// <returns></returns>
        public Task DeleteContentAsync(string tag, TimeSpan timeout)
        {
            return Task.Run(() =>
                {
                    this.DeleteContent(tag, timeout);
                });
        }
#endregion Public Methods

#region Private Methods

        private void CopyCallerHoldsReaderLock(string source, string destination, CopyFlag copyFlag, TimeoutHelper helper)
        {
            string destinationDirectory = FabricPath.GetDirectoryName(destination);
            if (!string.IsNullOrEmpty(destinationDirectory) && !FabricDirectory.Exists(destinationDirectory))
            {
                FabricDirectory.CreateDirectory(destinationDirectory);
            }

            using (FileWriterLock writerLock = new FileWriterLock(destination))
            {
                if (!writerLock.Acquire())
                {
                    throw new FabricTransientException(StringResources.Error_ImageStoreAcquireFileLockFailed, FabricErrorCode.ImageStoreAcquireFileLockFailed);
                }

                if (helper != null)
                {
                    helper.ThrowIfExpired();
                }

                if (FabricFile.Exists(source))
                {
                    // This is a file copy
                    if (FabricFile.Exists(destination))
                    {
                        FabricFile.Delete(destination, deleteReadonly: true);
                    }

                    int retryCount = 0;
                    while (helper == null || !TimeoutHelper.HasExpired(helper))
                    {
                        try
                        {
                            bool shouldOverwrite = (copyFlag != CopyFlag.AtomicCopySkipIfExists);
                            FabricFile.Copy(source, destination, shouldOverwrite);
                            break;
                        }
                        catch (UnauthorizedAccessException)
                        {
                            TraceSource.WriteInfo(
                                TraceType,
                                "Uploading {0} to {1} caused UnauthorizedAccessException. RetryCount: {2}.",
                                source,
                                destination,
                                retryCount);

                            if (retryCount++ > 3)
                            {
                                throw;
                            }

                            // This could happen when a file is marked for delete and we try to delete
                            // it again or try to open the file. Retrying after sometime should fix the issue.
                            Thread.Sleep(TimeSpan.FromSeconds(retryCount));
                        }

                        if (helper != null)
                        {
                            helper.ThrowIfExpired();
                        }
                    }
                }
                else
                {
                    // This is a folder copy
                    using (FolderCopy fc = new FolderCopy(copyFlag, null))
                    {
                        fc.Copy(source, destination);
                    }
                }
            }            
        }

        private string CreateAndAclDirectory()
        {            
            string tempDirectory = Path.GetRandomFileName();

            if (FabricDirectory.Exists(tempDirectory))
            {
                throw new InvalidOperationException("Could not generate temporary local directory");
            }

            try
            {
                // DirectoryInfo does not support long path names. Since this is only used
                // when the ImageStoreConnectionString is for a file share and specifies
                // user account information (ImageStoreFactory.ParseFileImageStoreConnectionString),
                // we can leave this usage here until it's absolutely needed (file share is not
                // supported in production).
                //
                DirectoryInfo tempDirectoryInfo = new DirectoryInfo(tempDirectory);
                tempDirectoryInfo.Create();
#if !DotNetCoreClr
                DirectorySecurity directorySecurity = tempDirectoryInfo.GetAccessControl();
                directorySecurity.AddAccessRule(
                            new FileSystemAccessRule(accessDescription.WindowsIdentity.Name, FileSystemRights.Read | FileSystemRights.Write, AccessControlType.Allow));
                Directory.SetAccessControl(tempDirectory, directorySecurity);
#endif
                return tempDirectory;
            }
            catch (Exception)
            {
                if (FabricDirectory.Exists(tempDirectory))
                {
                    FabricDirectory.Delete(tempDirectory, recursive: true, deleteReadOnlyFiles: true);
                }

                throw;
            }
        }

#if !DotNetCoreClr
        private WindowsImpersonationContext GetImpersonationContext()
        {
            return (accessDescription != null) ? accessDescription.WindowsIdentity.Impersonate() : null;
        }
#endif

        /// <summary>
        /// Converts a tag to an SMB file path by combining the root with the tag.
        /// </summary>
        /// <param name="tag">The tag specifying the resource.</param>
        /// <returns>The path to the tag.</returns>
        private string ConvertTagToSMBPath(string tag)
        {
            // Fix for Bug#1318495
            // Path.Combine will return only tag if tag starts with '\' or '/'.
            return string.IsNullOrEmpty(tag)
              ? this.rootUri
              : Path.Combine(this.rootUri, tag.Trim(new char[] { Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar, ' ', '\t' }));
        }

        private string GetLocalPath(string tag)
        {            
            if (string.IsNullOrEmpty(this.localRoot))
            {
                return tag;
            }
            else
            {
                // Fix for Bug#1318495
                // Path.Combine will return only tag if tag starts with '\' or '/'.
                tag = tag.TrimStart(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
                return Path.Combine(this.localRoot, tag);
            }            
        }

        private string GetStoreRelativePathFromFullName(string fullName, string storeSource)
        {
            int index = fullName.IndexOf(string.IsNullOrWhiteSpace(storeSource) ? this.rootUri : storeSource);
            if (index >= 0)
            {
                return fullName.Substring(string.IsNullOrWhiteSpace(storeSource) ? this.rootUri.Length + 1 : index);
            }
            else
            {
                throw new IOException(string.Format(CultureInfo.CurrentCulture, StringResources.ImageStoreError_DoesNotExistError, fullName));
            }
        }

        private ImageStoreFile ConvertToImageStoreFile(string fullFilePath, string storeSource)
        {
            return new ImageStoreFile(
                this.GetStoreRelativePathFromFullName(fullFilePath, storeSource),
                string.Empty,
                FabricFile.GetSize(fullFilePath),
                FabricFile.GetLastWriteTime(fullFilePath));
        }

        private IEnumerable<ImageStoreFile> GetAllFiles(string directory, string storeSource)
        {
            var fileFullNames = FabricDirectory.GetFiles(directory, "*", true, SearchOption.AllDirectories);
            foreach (var fileFullName in fileFullNames)
            {
                yield return this.ConvertToImageStoreFile(fileFullName, storeSource);
            }
        }

        private IEnumerable<string> GetAllFileFullNames(string dir, TimeSpan timeout)
        {
            TimeoutHelper helper = timeout == TimeSpan.MaxValue ? null : new TimeoutHelper(timeout);

            var fileFullNames = FabricDirectory.GetFiles(dir, "*", true, SearchOption.AllDirectories);
            foreach (var fileFullName in fileFullNames)
            {
                yield return fileFullName;

                if (helper != null)
                {
                    helper.ThrowIfExpired();
                }
            }
        }

        private Int32 GetFileCount(string directory)
        {
            var fileNames = FabricDirectory.GetFiles(directory, "*", false, SearchOption.AllDirectories);
            return fileNames.Length;
        }

        private ImageStoreContent GetFilesAndSubFolders(string directory, string storeSource, TimeSpan timeout)
        {
            TimeoutHelper helper = timeout == TimeSpan.MaxValue ? null : new TimeoutHelper(timeout);

            var directoryFullNames = FabricDirectory.GetDirectories(directory);
            var fileFullNames = FabricDirectory.GetFiles(directory);

            var content = new ImageStoreContent();

            foreach (var fileFullName in fileFullNames)
            {
                if (content.Files == null)
                {
                    content.Files = new List<ImageStoreFile>();
                }

                content.Files.Add(this.ConvertToImageStoreFile(fileFullName, storeSource));

                if (helper != null)
                {
                    helper.ThrowIfExpired();
                }
            }

            foreach (var directoryFullName in directoryFullNames)
            {
                if (content.Folders == null)
                {
                    content.Folders = new List<ImageStoreFolder>();
                }

                content.Folders.Add(new ImageStoreFolder(
                    this.GetStoreRelativePathFromFullName(directoryFullName, storeSource),
                    this.GetFileCount(directoryFullName)));

                if (helper != null)
                {
                    helper.ThrowIfExpired();
                }
            }

            return content;
        }

#endregion Private Methods

        /// <summary>
        /// The parameters for the file share.
        /// </summary>
        public class FileProviderParameters
        {
            /// <summary>
            /// This is the root URI of the provider.
            /// </summary>
            private readonly string imageStoreRootUri = null;

            /// <summary>
            /// Initializes a new instance of the FileProviderParameters class.
            /// </summary>
            /// <param name="rootUri">The root URI of the file store.</param>
            public FileProviderParameters(string rootUri)
            {
                this.imageStoreRootUri = rootUri;
            }

            /// <summary>
            /// Gets the value of the root URI.
            /// </summary>
            public string RootUri
            {
                get { return this.imageStoreRootUri; }
            }
        }
    }
}