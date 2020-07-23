//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

namespace System.Fabric.ImageStore
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.IO;
    using System.Reflection;
    using System.Threading.Tasks;

    /// <summary>
    /// XStore image store provider. This class implements the IImageStoreProvider
    /// interface for xstore.
    /// </summary>
    internal class XStoreProxy : IImageStore
    {
        private static readonly string SystemFabricManagementAssemblyName = "System.Fabric.Management";
        private static readonly string XStoreTypeNamespace = "System.Fabric.Management.ImageStore.XStoreImageStore";
        private const string XstoreSchemaTag = @"xstore:";

        private string XStoreUri;
        private Type XStoreType;
        private object XStore;
      
        public XStoreProxy()
        {
            AppDomain.CurrentDomain.AssemblyResolve += new ResolveEventHandler(LoadFromFabricCodePath);
        }

        public static bool IsXStoreUri(string imageStoreUri)
        {
            return imageStoreUri.Trim().StartsWith(XstoreSchemaTag, StringComparison.OrdinalIgnoreCase);
        }

        private Assembly LoadFromFabricCodePath(object sender, ResolveEventArgs args)
        {
            try
            {
                string folderPath = FabricEnvironment.GetCodePath();
#if DotNetCoreClr
                folderPath = Path.Combine(folderPath, "NS");
#endif
                string assemblyPath = Path.Combine(folderPath, new AssemblyName(args.Name).Name + ".dll");
                if (File.Exists(assemblyPath))
                {
                    return Assembly.LoadFrom(assemblyPath);
                }
            }
            catch (Exception)
            {
                // Suppress any Exception so that we can continue to
                // load the assembly through other means
            }

            return null;
        }

        /// <summary>
        /// Gets the Root uri for image store
        /// </summary>
        public string RootUri
        {
            get { return this.XStoreUri; }
        }

        public XStoreProxy(string imageStoreUri, string localRoot)
        {
            if (imageStoreUri == null)
            {
                throw new ArgumentException(StringResources.ImageStoreError_InvalidParameterSpecified, "imageStoreUri");
            }

            this.XStoreUri = imageStoreUri;

            Assembly currentAssembly = typeof(XStoreProxy).GetTypeInfo().Assembly;
            AssemblyName systemFabricManagementAssemblyName = new AssemblyName();
            systemFabricManagementAssemblyName.Name = SystemFabricManagementAssemblyName;
            systemFabricManagementAssemblyName.Version = currentAssembly.GetName().Version;
#if !DotNetCoreClr
            systemFabricManagementAssemblyName.CultureInfo = currentAssembly.GetName().CultureInfo;
#endif
            systemFabricManagementAssemblyName.ProcessorArchitecture = currentAssembly.GetName().ProcessorArchitecture;
            systemFabricManagementAssemblyName.SetPublicKeyToken(currentAssembly.GetName().GetPublicKeyToken());

            string xstoreTypeName = Helpers.CreateQualifiedNameForAssembly(
                systemFabricManagementAssemblyName.FullName,
                XStoreTypeNamespace);

            this.XStoreType = Type.GetType(xstoreTypeName, true);
            object[] argImageStore = new object[] { imageStoreUri, localRoot };
            this.XStore = Activator.CreateInstance(this.XStoreType, argImageStore);
        }

        /// <summary>
        /// Uploads content of the local source to the image store at the remote destination.
        /// </summary>
        /// <param name="remoteDestination">Location relative to RootUri where the content needs to be uploaded.</param>
        /// <param name="localSource">Physical location of the source.</param>
        /// <param name="timeout">The timeout for performing the uploading operation.</param>
        /// <param name="copyFlag">The copying flag specifying copy type </param>
        /// <param name="acquireSourceReaderLock">Indicates whether to acquire reader lock.</param>
        public void UploadContent(string remoteDestination, string localSource, TimeSpan timeout, CopyFlag copyFlag, bool acquireSourceReaderLock)
        {
            CheckForReservedImageStoreFolders(remoteDestination);

            MethodInfo uploadContentMethodInfo = this.XStoreType.GetMethod(
                "UploadContent", 
                new[] { typeof(string), typeof(string), typeof(TimeSpan), typeof(CopyFlag), typeof(bool) });
            if (uploadContentMethodInfo == null)
            {
                throw new InvalidOperationException(String.Format(CultureInfo.CurrentCulture, StringResources.Error_MethodLookupFailed, "UploadContent"));
            }

            uploadContentMethodInfo.Invoke(
                this.XStore, 
                BindingFlags.InvokeMethod,
                Type.DefaultBinder,
                new object[] { remoteDestination, localSource, timeout, copyFlag, acquireSourceReaderLock },
                CultureInfo.CurrentCulture);
        }

        public void UploadContent(string remoteDestination, string localSource, IImageStoreProgressHandler handler, TimeSpan timeout, CopyFlag copyFlag, bool acquireSourceReaderLock)
        {
            CheckForReservedImageStoreFolders(remoteDestination);

            MethodInfo uploadContentMethodInfo = this.XStoreType.GetMethod(
                "UploadContent",
                new[] { typeof(string), typeof(string), typeof(IImageStoreProgressHandler), typeof(TimeSpan), typeof(CopyFlag), typeof(bool) });

            if (uploadContentMethodInfo == null)
            {
                throw new InvalidOperationException(string.Format(CultureInfo.CurrentCulture, StringResources.Error_MethodLookupFailed, "UploadContent"));
            }

            uploadContentMethodInfo.Invoke(
                this.XStore,
                BindingFlags.InvokeMethod,
                Type.DefaultBinder,
                new object[] { remoteDestination, localSource, handler, timeout, copyFlag, acquireSourceReaderLock },
                CultureInfo.CurrentCulture);
        }

        public Task UploadContentAsync(string remoteDestination, string localSource, TimeSpan timeout, CopyFlag copyFlag, bool acquireSourceReaderLock)
        {
            return Task.Run(() =>
            {
                this.UploadContent(remoteDestination, localSource, timeout, copyFlag, acquireSourceReaderLock);
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
            MethodInfo copyContentMethodInfo = this.XStoreType.GetMethod(
                "CopyContent",
                new[] { typeof(string), typeof(string), typeof(TimeSpan), typeof(string[]), typeof(CopyFlag), typeof(bool) });

            if (copyContentMethodInfo == null)
            {
                throw new InvalidOperationException(string.Format(CultureInfo.CurrentCulture, StringResources.Error_MethodLookupFailed, "CopyContent"));
            }

            copyContentMethodInfo.Invoke(
                this.XStore,
                BindingFlags.InvokeMethod,
                Type.DefaultBinder,
                new object[] { remoteSource, remoteDestination, timeout, skipFiles, copyFlag, checkMarkFile },
                CultureInfo.CurrentCulture);
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
        /// <param name="remoteSource">Location (relative to RootUri) from where to download the content.</param>
        /// <param name="localDestination">Physical location where to download the content.</param>
        /// <param name="timeout"> Download timeout in milliseconds.</param>
        /// <param name="copyFlag">The copying control information to specify how file can be overwritten.</param>
        public void DownloadContent(string remoteSource, string localDestination, TimeSpan timeout, CopyFlag copyFlag)
        {
            MethodInfo downloadContentMethodInfo = this.XStoreType.GetMethod(
                "DownloadContent",
                new[] { typeof(string), typeof(string), typeof(TimeSpan), typeof(CopyFlag) });

            if (downloadContentMethodInfo == null)
            {
                throw new InvalidOperationException(string.Format(CultureInfo.CurrentCulture, StringResources.Error_MethodLookupFailed, "DownloadContent"));
            }

            downloadContentMethodInfo.Invoke(
                this.XStore,
                BindingFlags.InvokeMethod,
                Type.DefaultBinder,
                new object[] { remoteSource, localDestination, timeout, copyFlag },
                CultureInfo.CurrentCulture);
        }

        /// <summary>
        /// Downloads content from the XStore to local destination.
        /// </summary>
        /// <param name="remoteSource">Location (relative to RootUri) from where to download the content.</param>
        /// <param name="localDestination">Physical location where to download the content.</param>
        /// <param name="handler">Defines the behavior to process progress information from the downloading operation.</param>
        /// <param name="timeout">The timeout for performing the downloading operation.</param>
        /// <param name="copyFlag">The copying control information to specify how file can be overwritten.</param>
        public void DownloadContent(string remoteSource, string localDestination, IImageStoreProgressHandler handler, TimeSpan timeout, CopyFlag copyFlag)
        {
            MethodInfo downloadContentMethodInfo = this.XStoreType.GetMethod(
               "DownloadContent",
               new[] { typeof(string), typeof(string), typeof(IImageStoreProgressHandler), typeof(TimeSpan), typeof(CopyFlag) });

            if (downloadContentMethodInfo == null)
            {
                throw new InvalidOperationException(string.Format(CultureInfo.CurrentCulture, StringResources.Error_MethodLookupFailed, "DownloadContent"));
            }

            downloadContentMethodInfo.Invoke(
                this.XStore,
                BindingFlags.InvokeMethod,
                Type.DefaultBinder,
                new object[] { remoteSource, localDestination, handler, timeout, copyFlag },
                CultureInfo.CurrentCulture);
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
        /// <param name="remoteLocation">Location (relative to RootUri)</param>
        /// <param name="timeout">The timeout for performing the listing operation.</param>
        /// <returns>All blobs within the input blob path</returns>
        public IEnumerable<string> ListContent(string remoteLocation, TimeSpan timeout)
        {
            MethodInfo listContentMethodInfo = this.XStoreType.GetMethod(
              "ListContent",
              new[] { typeof(string), typeof(TimeSpan) });

            if (listContentMethodInfo == null)
            {
                throw new InvalidOperationException(string.Format(CultureInfo.CurrentCulture, StringResources.Error_MethodLookupFailed, "ListContent"));
            }

            return (IEnumerable<string>)listContentMethodInfo.Invoke(
                this.XStore,
                BindingFlags.InvokeMethod,
                Type.DefaultBinder,
                new object[] { remoteLocation, timeout },
                CultureInfo.CurrentCulture);
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
        /// <param name="remoteLocation">Location (relative to RootUri)</param>
        /// <param name="isRecursive">flag list subhierarchy</param>
        /// <param name="timeout">The timeout for performing the listing operation</param>
        /// <returns>All blobs within the input blob path/sub blob path</returns>
        public ImageStoreContent ListContentWithDetails(string remoteLocation, bool isRecursive, TimeSpan timeout)
        {
            MethodInfo listContentMethodInfo = this.XStoreType.GetMethod(
              "ListContentWithDetails",
              new[] { typeof(string), typeof(bool), typeof(TimeSpan) });

            if (listContentMethodInfo == null)
            {
                throw new InvalidOperationException(string.Format(CultureInfo.CurrentCulture, StringResources.Error_MethodLookupFailed, "ListContentWithDetails"));
            }

            return (ImageStoreContent)listContentMethodInfo.Invoke(
                this.XStore,
                BindingFlags.InvokeMethod,
                Type.DefaultBinder,
                new object[] { remoteLocation, isRecursive, timeout },
                CultureInfo.CurrentCulture);
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
        /// <param name="remoteLocation"> Location (relative to RootUri) where to check the presence of the content. </param>
        /// <param name="timeout">The timeout for performing the checking existence operation</param>
        /// <returns> True if the content exists, false otherwise. </returns>
        public bool DoesContentExist(string remoteLocation, TimeSpan timeout)
        {
            MethodInfo doesContentExistMethodInfo = this.XStoreType.GetMethod(
              "DoesContentExist",
              new[] { typeof(string), typeof(TimeSpan) });

            if (doesContentExistMethodInfo == null)
            {
                throw new InvalidOperationException(string.Format(CultureInfo.CurrentCulture, StringResources.Error_MethodLookupFailed, "DoesContentExist"));
            }

            return (bool)doesContentExistMethodInfo.Invoke(
                this.XStore,
                BindingFlags.InvokeMethod,
                Type.DefaultBinder,
                new object[] { remoteLocation, timeout },
                CultureInfo.CurrentCulture);
        }

        /// <summary>
        ///  Check if content is present in the XStore.
        /// </summary>
        /// <param name="remoteLocation">Location (relative to RootUri) where to check the presence of the content.</param>
        /// <param name="timeout">The timeout for performing the checking existence operation</param>
        /// <returns></returns>
        public Task<bool> DoesContentExistAsync(string remoteLocation, TimeSpan timeout)
        {
            return Task.Run<bool>(() =>
            {
                return this.DoesContentExist(remoteLocation, timeout);
            });
        }

        /// <summary>
        /// Remove tag from store, clear data assosciate with tag.
        /// </summary>
        /// <param name="remoteLocation">Location (relative to RootUri) from where to delete the content.</param>
        ///<param name="timeout">The timeout for performing the deleting operation</param>
        public void DeleteContent(string remoteLocation, TimeSpan timeout)
        {
            MethodInfo deleteContentMethodInfo = this.XStoreType.GetMethod(
             "DeleteContent",
             new[] { typeof(string), typeof(TimeSpan) });

            if (deleteContentMethodInfo == null)
            {
                throw new InvalidOperationException(string.Format(CultureInfo.CurrentCulture, StringResources.Error_MethodLookupFailed, "DeleteContent"));
            }

            deleteContentMethodInfo.Invoke(
                this.XStore,
                BindingFlags.InvokeMethod,
                Type.DefaultBinder,
                new object[] { remoteLocation, timeout },
                CultureInfo.CurrentCulture);
        }

        /// <summary>
        /// Remove tag from store, clear data assosciate with tag.
        /// </summary>
        /// <param name="remoteLocation">Location (relative to RootUri) from where to delete the content.</param>
        ///<param name="timeout">The timeout for performing the deleting operation</param>
        public Task DeleteContentAsync(string remoteLocation, TimeSpan timeout)
        {
            return Task.Run(() =>
            {
                this.DeleteContent(remoteLocation, timeout);
            });
        }

        private void CheckForReservedImageStoreFolders(string remoteDestination)
        {
            remoteDestination = remoteDestination.TrimEnd('\\');
            if (string.Compare(remoteDestination, Constants.StoreFolder, true) == 0 ||
                string.Compare(remoteDestination, Constants.WindowsFabricStoreFolder, true) == 0)
            {
                throw new FabricImageBuilderReservedDirectoryException(string.Format(StringResources.Error_InvalidReservedImageStoreOperation, remoteDestination));
            }
        }
    }
}
