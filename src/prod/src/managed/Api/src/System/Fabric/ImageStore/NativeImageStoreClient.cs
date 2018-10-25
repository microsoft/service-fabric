// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.ImageStore
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Fabric.Interop;
    using System.Fabric.Query;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.IO;
    using System.Reflection;
    using System.Security.Cryptography.X509Certificates;
    using System.Threading.Tasks;

    /// <summary>
    /// <para>Implements a native image store client.</para>
    /// </summary>
    internal class NativeImageStoreClient : IImageStore
    {
        /// <summary>
        /// The tag for file image store scheme.
        /// </summary>
        public const string SchemeTag = @"fabric:ImageStore";        

        private NativeImageStore.IFabricNativeImageStoreClient3 nativeClient;
        
        /// <summary>
        /// Gets the Root uri of the store.
        /// </summary>
        public string RootUri
        {
            get { return SchemeTag; }
        }

        /// <summary>
        /// Checks whether the uri is of the native image store.
        /// </summary>
        /// <param name="imageStoreUri"></param>
        /// <returns></returns>
        public static bool IsNativeImageStoreUri(string imageStoreUri)
        {
            return imageStoreUri.Trim().StartsWith(NativeImageStoreClient.SchemeTag, StringComparison.OrdinalIgnoreCase);
        }

        /// <summary>
        /// Initializes a new instance of the native image store client class.
        /// </summary>
        /// <param name="isInternal">Indicates whether or not it's for internal use.</param>
        public NativeImageStoreClient(bool isInternal)
            : this(null, isInternal)
        {            
        }

        /// <summary>
        /// Initializes a new instance of the native image store client class.
        /// </summary>
        /// <param name="connectionStrings">Information about the native image store connection string.</param>
        /// <param name="isInternal">Indicates whether or not it's for internal use.</param>
        public NativeImageStoreClient(string[] connectionStrings, bool isInternal)
            : this(connectionStrings, isInternal, null, null)
        {
        }

        /// <summary>
        /// Initializes a new instance of the native image store client class.
        /// </summary>
        /// <param name="connectionStrings">Information about the native image store connection string.</param>
        /// <param name="isInternal">Indicates whether or not it's for internal use.</param>
        /// <param name="workingDirectory">Specify the working folder for native image store operations.</param>
        /// <param name="credentials">Information represent security credentials.</param>
        public NativeImageStoreClient(string[] connectionStrings, bool isInternal, string workingDirectory, SecurityCredentials credentials)
        {
            Utility.WrapNativeSyncInvokeInMTA(() => { this.CreateNativeClient(connectionStrings, isInternal, workingDirectory, credentials); }, "CreateNativeImageStoreClient");
        }

        /// <summary>
        /// Uploads content of the local source to the native image store at the remote destination.
        /// </summary>
        /// <param name="remoteDestination">Location relative to image store root where the content needs to be uploaded.</param>
        /// <param name="localSource">local location of the source.</param>
        /// <param name="progressHandler">Defines the behavior to process progress information from the uploading operation.</param>
        /// <param name="timeout">The timeout for performing the uploading operation.</param>
        /// <param name="imageCopyFlag">Copy control information to specify how file can be overwritten.</param>
        /// <param name="acquireSourceReaderLock">Indicates whether to acquire reader lock.</param>
        public void UploadContent(string remoteDestination, string localSource, IImageStoreProgressHandler progressHandler, TimeSpan timeout, CopyFlag imageCopyFlag, bool acquireSourceReaderLock)
        {
            Utility.WrapNativeSyncInvokeInMTA(() =>
            {
                this.UploadContentWrapper(remoteDestination, localSource, progressHandler, Utility.ToMilliseconds(timeout, "timeout"), imageCopyFlag);
            }, "UploadContent");
        }

        /// <summary>
        /// Uploads content of the local source to the native image store at the remote destination.
        /// </summary>
        /// <param name="remoteDestination">Location relative to RootUri where the content needs to be uploaded.</param>
        /// <param name="localSource">local location of the source.</param>
        /// <param name="timeout">The timeout for performing the uploading operation.</param>
        /// <param name="imageCopyFlag">The copying control information to specify how file can be overwritten.</param>
        /// <param name="acquireSourceReaderLock">Indicates whether to acquire reader lock.</param>
        public void UploadContent(string remoteDestination, string localSource, TimeSpan timeout, CopyFlag imageCopyFlag, bool acquireSourceReaderLock)
        {
            this.UploadContent(remoteDestination, localSource, null, timeout, imageCopyFlag, acquireSourceReaderLock);
        }

        /// <summary>
        /// Uploads content of the local source to the native image store at the remote destination.
        /// </summary>
        /// <param name="storeDestination">Location relative to RootUri where the content needs to be uploaded.</param>
        /// <param name="localSource">local location of the source.</param>
        /// <param name="timeout">The timeout for performing the uploading operation.</param>
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
        /// Copies content from a path in the ImageStore to the destination path in the ImageStore.
        /// </summary>
        /// <param name="remoteSource">The relative path of source file image store to be copied from.</param>
        /// <param name="remoteDestination">The relative path of destination image store content to be copied to.</param>
        /// <param name="timeout">The timeout for performing the copying operation.</param>
        /// <param name="skipFiles">The list of the files names to be skipped for copying.</param>
        /// <param name="imageCopyFlag">The copying control information to specify how file can be overwritten.</param>
        /// <param name="checkMarkFile">Indicats whether to check mark file during copying.</param>
        public void CopyContent(string remoteSource, string remoteDestination, TimeSpan timeout, string[] skipFiles, CopyFlag imageCopyFlag, bool checkMarkFile)
        {
            Utility.WrapNativeSyncInvokeInMTA(() =>
            {
                this.CopyContentWrapper(remoteSource, remoteDestination, Utility.ToMilliseconds(timeout, "timeout"), skipFiles, imageCopyFlag, checkMarkFile);
            }, "CopyContent");
        }


        /// <summary>
        /// Copies content from a path in the ImageStore to the destination path in the ImageStore.
        /// </summary>
        /// <param name="storeSource">The relative path of source file image store to be copied from.</param>
        /// <param name="storeDestination">The relative path of destination image store content to be copied to.</param>
        /// <param name="timeout">The timeout for performing the copying operation.</param>
        /// <param name="skipFiles">The list of the files names to be skipped for copying.</param>
        /// <param name="copyFlag">The copying control information to specify how file can be overwritten.</param>
        /// <param name="checkMarkFile">Indicats whether to check mark file during copying.</param>
        /// <returns></returns>
        public Task CopyContentAsync(string storeSource, string storeDestination, TimeSpan timeout, string[] skipFiles, CopyFlag copyFlag, bool checkMarkFile)
        {
            return Task.Run(() =>
            {
                this.CopyContent(storeSource, storeDestination, timeout, skipFiles, copyFlag, checkMarkFile);
            });
        }

        /// <summary>
        /// Downloads content from the image store to local destination.
        /// </summary>
        /// <param name="remoteSource">The relative path of source image store to be downloaded from.</param>
        /// <param name="localDestination">The local destination path to download the content.</param>
        /// <param name="timeout">The timeout for performing the downloading operation.</param>
        /// <param name="imageCopyFlag">The copying control information to specify how file can be overwritten.</param>
        public void DownloadContent(string remoteSource, string localDestination, TimeSpan timeout, CopyFlag imageCopyFlag)
        {
            this.DownloadContent(remoteSource, localDestination, null, timeout, imageCopyFlag);
        }

        /// <summary>
        /// Downloads content from the image store to local destination.
        /// </summary>
        /// <param name="remoteSource">The relative path of source image store to be downloaded from.</param>
        /// <param name="localDestination">The local destination path to download the content.</param>
        /// <param name="progressHandler">Defines the behavior to process progress information from the downloading operation.</param>
        /// <param name="timeout">The timeout for performing the downloading operation.</param>
        /// <param name="imageCopyFlag">The copying control information to specify how file can be overwritten.</param>
        public void DownloadContent(string remoteSource, string localDestination, IImageStoreProgressHandler progressHandler, TimeSpan timeout, CopyFlag imageCopyFlag)
        {
            Utility.WrapNativeSyncInvokeInMTA(() =>
            {
                this.DownloadContentWrapper(remoteSource, localDestination, progressHandler, Utility.ToMilliseconds(timeout, "timeout"), imageCopyFlag);
            }, "DownloadContent");
        }

        /// <summary>
        /// Downloads content from the image store to local destination.
        /// </summary>
        /// <param name="storeSource">The relative path of source image store to be downloaded from.</param>
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
        /// Gets the image store content information.
        /// Returns the List of image store relative paths under the given relative path.
        /// </summary>
        /// <param name="remoteLocation">The relative path to file or folder in the image store from its root.</param>
        /// <param name="timeout">The timeout for performing the listing operation.</param>
        /// <returns></returns>
        public IEnumerable<string> ListContent(string remoteLocation, TimeSpan timeout)
        {
            return Utility.WrapNativeSyncInvokeInMTA(() =>
            {
                return this.ListContentWrapper(remoteLocation, Utility.ToMilliseconds(timeout, "timeout"));
            }, "ListContent");
        }

        /// <summary>
        /// Gets paged image store relative paths under the given relative path.
        /// The purpose of paging is to prevent from the oversized message which might be happened when listing a large number of files.
        /// </summary>
        ///<param name="listDescription">The information on how to get image store content.</param>
        ///<param name="timeout">The timeout for performing the listing operation.</param>
        /// <returns></returns>
        public ImageStorePagedRelativePath ListPagedContent(ImageStoreListDescription listDescription, TimeSpan timeout)
        {
            return Utility.WrapNativeSyncInvokeInMTA(() =>
            {
                return this.ListPagedContentWrapper(listDescription, Utility.ToMilliseconds(timeout, "timeout"));
            }, "ListContent");
        }

        /// <summary>
        /// Get the image store content information.
        /// Returns the List of image store relative paths under the given relative path.
        /// </summary>
        /// <param name="storeSource">The relative path to file or folder in the image store from its root.</param>
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
        /// Gets paged image store relative paths under the given relative path.
        /// The purpose of paging is to prevent from the oversized message which might be happened when listing a large number of files.
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
        /// Gets the image store content information.
        /// Returns the information about the image store content under the given relative path.
        /// </summary>
        ///<param name="remoteLocation">The relative path to file or folder in the image store from its root.</param>
        ///<param name="isRecursive">flag list subhierarchy.</param>
        ///<param name="timeout">The timeout for performing the listing operation.</param>
        /// <returns></returns>
        public ImageStoreContent ListContentWithDetails(string remoteLocation, bool isRecursive, TimeSpan timeout)
        {
            return Utility.WrapNativeSyncInvokeInMTA(() =>
            {
                return this.ListContentWithDetailsWrapper(remoteLocation, isRecursive, Utility.ToMilliseconds(timeout, "timeout"));
            }, "ListContent");
        }

        /// <summary>
        /// Gets paged image store content under the given relative path.
        /// The purpose of paging is to prevent from the oversized message which might be happend when listing a large number of files.
        /// </summary>
        /// <param name="listDescription">The information on how to get image store content.</param>
        /// <param name="timeout">The timeout for performing the listing operation.</param>
        /// <returns></returns>
        public ImageStorePagedContent ListPagedContentWithDetails(ImageStoreListDescription listDescription, TimeSpan timeout)
        {
            //return new ImageStoreContent();

            return Utility.WrapNativeSyncInvokeInMTA(() =>
            {
                return this.ListPagedContentWithDetailsWrapper(listDescription, Utility.ToMilliseconds(timeout, "timeout"));
            }, "ListContent");
        }

        /// <summary>
        /// Gets the image store content information.
        /// Returns the information about the image store content under the given relative path.
        /// </summary>
        /// <param name="storeSource">The relative path to file or folder in the image store from its root.</param>
        /// <param name="isRecursive">flag list subhierarchy.</param>
        /// <param name="timeout">The timeout for performing the listing operation.</param>
        /// <returns></returns>
        public async Task<ImageStoreContent> ListContentWithDetailsAsync(string storeSource, bool isRecursive, TimeSpan timeout)
        {
            return await Task.Run(() =>
            {
                return this.ListContentWithDetails(storeSource, isRecursive, timeout);
            });
        }

        /// <summary>
        /// Gets paged image store content under the given relative path.
        /// The purpose of paging is to prevent from the oversized message which might be happend when listing a large number of files.
        /// </summary>
        /// <param name="listDescription">The information on how to get image store content.</param>
        /// <param name="timeout">The timeout for performing the listing operation.</param>
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
        /// <param name="remoteLocation">Location (relative to image store root) where to check the presence of the content.</param>
        /// <param name="timeout">The timeout for performing the checking existence operation.</param>
        /// <returns></returns>
        public bool DoesContentExist(string remoteLocation, TimeSpan timeout)
        {
            return Utility.WrapNativeSyncInvokeInMTA(() =>
            {
                return this.DoesContentExistWrapper(remoteLocation, Utility.ToMilliseconds(timeout, "timeout"));
            }, "DoesContentExist");
        }

        /// <summary>
        /// Check if content is present in the store.
        /// </summary>
        /// <param name="tag">Location (relative to image store root) where to check the presence of the content.</param>
        /// <param name="timeout">The timeout for performing the checking existence operation.</param>
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
        /// <param name="remoteLocation">Location (relative to RootUri) from where to delete the content.</param>
        /// <param name="timeout">The timeout for performing the deleting operation.</param>
        public void DeleteContent(string remoteLocation, TimeSpan timeout)
        {
            Utility.WrapNativeSyncInvokeInMTA(() =>
            {
                this.DeleteContentWrapper(remoteLocation, Utility.ToMilliseconds(timeout, "timeout"));
            }, "DeleteContent");
        }

        /// <summary>
        /// Remove tag from store, clear data assosciate with tag.
        /// </summary>
        /// <param name="tag">Location (relative to RootUri) from where to delete the content.</param>
        /// <param name="timeout">The timeout for performing the deleting operation.</param>
        /// <returns></returns>
        public Task DeleteContentAsync(string tag, TimeSpan timeout)
        {
            return Task.Run(() =>
            {
                this.DeleteContent(tag, timeout);
            });
        }

        private void CreateNativeClient(IEnumerable<string> connectionStrings, bool isInternal, string workingDirectory, SecurityCredentials credentials)
        {
            Guid iid = typeof(NativeImageStore.IFabricNativeImageStoreClient).GetTypeInfo().GUID;

            if (connectionStrings == null)
            {
                using (var pinColl = new PinCollection())
                {
                    this.nativeClient = (NativeImageStore.IFabricNativeImageStoreClient3)NativeImageStore.FabricCreateLocalNativeImageStoreClient(
                        isInternal,
                        (workingDirectory == null ? IntPtr.Zero : pinColl.AddObject(workingDirectory)),
                        ref iid);
                }
            }
            else
            {
                using (var pinColl = new PinCollection())
                {
                    using (var pinArray = new PinArray(connectionStrings, PinBlittable.Create))
                    {
                        this.nativeClient = (NativeImageStore.IFabricNativeImageStoreClient3)NativeImageStore.FabricCreateNativeImageStoreClient(
                            isInternal,
                            (workingDirectory == null ? IntPtr.Zero : pinColl.AddObject(workingDirectory)),
                            (ushort)pinArray.Count,
                            pinArray.AddrOfPinnedObject(),
                            ref iid);
                    }
                }
            }

            if (credentials != null)
            {
                this.SetSecurityCredentialsWrapper(credentials);
            }
        }


        internal void SetSecurityCredentialsWrapper(SecurityCredentials credentials)
        {
            using (var pin = new PinCollection())
            {
                this.nativeClient.SetSecurityCredentials(credentials.ToNative(pin));
            }
        }

        private void UploadContentWrapper(string remoteDestination, string localSource, IImageStoreProgressHandler progressHandler, UInt32 timeoutMilliseconds, CopyFlag imageCopyFlag)
        {
            using (var pin = new PinCollection())
            {
                if (progressHandler == null)
                {
                    this.nativeClient.UploadContent(
                        pin.AddObject(remoteDestination),
                        pin.AddObject(localSource),
                        timeoutMilliseconds,
                        ToNative(imageCopyFlag));
                }
                else
                {
                    this.nativeClient.UploadContent2(
                        pin.AddObject(remoteDestination),
                        pin.AddObject(localSource),
                        new NativeImageStoreProgressEventHandlerBroker(progressHandler),
                        timeoutMilliseconds,
                        ToNative(imageCopyFlag));
                }
            }
        }

        private void CopyContentWrapper(string remoteSource, string remoteDestination, UInt32 timeoutMilliseconds, string[] skipFiles, CopyFlag imageCopyFlag, bool checkMarkFile)
        {
            using (var pin = new PinCollection())
            {
                this.nativeClient.CopyContent(
                    pin.AddObject(remoteSource),
                    pin.AddObject(remoteDestination),
                    timeoutMilliseconds,
                    NativeTypes.ToNativeStringList(pin, skipFiles),
                    ToNative(imageCopyFlag),
                    NativeTypes.ToBOOLEAN(checkMarkFile));
            }
        }

        private void DownloadContentWrapper(string remoteSource, string localDestination,  IImageStoreProgressHandler progressHandler, UInt32 timeoutMilliseconds, CopyFlag imageCopyFlag)
        {
            using (var pin = new PinCollection())
            {
                this.nativeClient.DownloadContent2(
                    pin.AddObject(remoteSource),
                    pin.AddObject(localDestination),
                    progressHandler == null ? null : new NativeImageStoreProgressEventHandlerBroker(progressHandler),
                    timeoutMilliseconds,
                    ToNative(imageCopyFlag));
            }
        }

        private IEnumerable<string> ListContentWrapper(string remoteLocation, UInt32 timeoutMilliseconds)
        {
            using (var pin = new PinCollection())
            {
                var nativceList = this.nativeClient.ListContent(
                    pin.AddObject(remoteLocation),
                    timeoutMilliseconds);

                var results = StringCollectionResult.FromNative(nativceList);
                string[] arrayResult = new string[results.Count];
                results.CopyTo(arrayResult, 0);
                return arrayResult;
            }
        }

        private ImageStorePagedRelativePath ListPagedContentWrapper(ImageStoreListDescription listDescription, UInt32 timeoutMilliseconds)
        {
            using (var pin = new PinCollection())
            {
                var nativeDescription = new NativeImageStore.FABRIC_IMAGE_STORE_LIST_DESCRIPTION();
                listDescription.ToNative(pin, out nativeDescription);

                var nativeFiles = this.nativeClient.ListPagedContent(
                    pin.AddBlittable(nativeDescription),
                    timeoutMilliseconds);

                return FromNativeFiles(nativeFiles);
            }
        }

        private ImageStoreContent ListContentWithDetailsWrapper(string remoteLocation, bool isRecursive, UInt32 timeoutMilliseconds)
        {
            using (var pin = new PinCollection())
            {
                var nativePtr = this.nativeClient.ListContentWithDetails(
                    pin.AddObject(remoteLocation),
                    NativeTypes.ToBOOLEAN(isRecursive),
                    timeoutMilliseconds);

                return FromNativeContent(nativePtr);
            }
        }

        private ImageStorePagedContent ListPagedContentWithDetailsWrapper(ImageStoreListDescription listDescription, UInt32 timeoutMilliseconds)
        {
            using (var pin = new PinCollection())
            {
                var nativeDescription = new NativeImageStore.FABRIC_IMAGE_STORE_LIST_DESCRIPTION();
                listDescription.ToNative(pin, out nativeDescription);

                var nativePtr = this.nativeClient.ListPagedContentWithDetails(
                    pin.AddBlittable(nativeDescription),
                    timeoutMilliseconds);

                return FromNativePagedContent(nativePtr);
            }
        }

        private static unsafe ImageStorePagedRelativePath FromNativeFiles(IntPtr nativePtr)
        {
            NativeImageStore.FABRIC_IMAGE_STORE_PAGED_RELATIVEPATH_QUERY_RESULT* casted = (NativeImageStore.FABRIC_IMAGE_STORE_PAGED_RELATIVEPATH_QUERY_RESULT*)nativePtr;
            var managed = new ImageStorePagedRelativePath();
            managed.CreateFromNative(casted);
            return managed;
        }

        private static unsafe ImageStoreContent FromNativeContent(IntPtr nativePtr)
        {
            NativeImageStore.FABRIC_IMAGE_STORE_CONTENT_QUERY_RESULT* casted = (NativeImageStore.FABRIC_IMAGE_STORE_CONTENT_QUERY_RESULT*)nativePtr;
            var managed = new ImageStoreContent();
            managed.CreateFromNative(casted);
            return managed;
        }

        private static unsafe ImageStorePagedContent FromNativePagedContent(IntPtr nativePtr)
        {
            NativeImageStore.FABRIC_IMAGE_STORE_PAGED_CONTENT_QUERY_RESULT* casted = (NativeImageStore.FABRIC_IMAGE_STORE_PAGED_CONTENT_QUERY_RESULT*)nativePtr;
            var managed = new ImageStorePagedContent();
            managed.CreateFromNative(casted);
            return managed;
        }

        private bool DoesContentExistWrapper(string remoteLocation, UInt32 timeoutMilliseconds)
        {
            using (var pin = new PinCollection())
            {
                var nativeBool = this.nativeClient.DoesContentExist(
                    pin.AddObject(remoteLocation),
                    timeoutMilliseconds);

                return NativeTypes.FromBOOLEAN(nativeBool);
            }
        }

        private void DeleteContentWrapper(string remoteLocation, UInt32 timeoutMilliseconds)
        {
            using (var pin = new PinCollection())
            {
                this.nativeClient.DeleteContent(
                    pin.AddObject(remoteLocation),
                    timeoutMilliseconds);
            }
        }

        private NativeImageStore.FABRIC_IMAGE_STORE_COPY_FLAG ToNative(CopyFlag flag)
        {
            switch (flag)
            {
            case CopyFlag.CopyIfDifferent:
                return NativeImageStore.FABRIC_IMAGE_STORE_COPY_FLAG.FABRIC_IMAGE_STORE_COPY_FLAG_IF_DIFFERENT;

            case CopyFlag.AtomicCopy:
                return NativeImageStore.FABRIC_IMAGE_STORE_COPY_FLAG.FABRIC_IMAGE_STORE_COPY_FLAG_ATOMIC;

            case CopyFlag.AtomicCopySkipIfExists:
                return NativeImageStore.FABRIC_IMAGE_STORE_COPY_FLAG.FABRIC_IMAGE_STORE_COPY_FLAG_ATOMIC_SKIP_IF_EXISTS;

            default:
                return NativeImageStore.FABRIC_IMAGE_STORE_COPY_FLAG.FABRIC_IMAGE_STORE_COPY_FLAG_INVALID;
            }
        }        
    }
}