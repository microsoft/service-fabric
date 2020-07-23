// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.ImageStore;
    using System.Globalization;
    using System.IO;
    using System.Fabric.Strings;
    using System.Threading.Tasks;

    internal class ImageStoreWrapper
    {
        private static readonly string TraceType = "ImageStoreWrapper";
        private readonly string localImageBuilderRoot;
        public const string xstoreSchemaTag = @"xstore:";

        public ImageStoreWrapper(IImageStore imageStore, string localRoot)
        {
            this.ImageStore = imageStore;
            this.localImageBuilderRoot = localRoot;
        }

        public IImageStore ImageStore { get; private set; }

        public bool DoesContentExists(string contentFolder, TimeSpan timeout)
        {
            return this.ImageStore.DoesContentExist(contentFolder, timeout);
        }

        public bool DownloadIfContentExists(string source, string destination, TimeSpan timeout)
        {
            return this.DownloadIfContentExists(source, destination, null, timeout);
        }

        public bool DownloadIfContentExists(string source, string destination, IImageStoreProgressHandler progressHandler, TimeSpan timeout)
        {
            TimeoutHelper timeoutHelper = new TimeoutHelper(timeout);
            if (this.ImageStore.DoesContentExist(source, timeoutHelper.GetRemainingTime()))
            {
                this.ImageStore.DownloadContent(source, destination, progressHandler, timeoutHelper.GetRemainingTime(), CopyFlag.AtomicCopy);
                ImageBuilderUtility.RemoveReadOnlyFlag(destination);
                return true;
            }
            else
            {
                return false;
            }
        }

        public void UploadContent(string destination, string source, TimeSpan timeout)
        {
            this.ImageStore.UploadContent(destination, source, timeout, CopyFlag.AtomicCopy, true);
        }

        public async Task UploadContentAsync(string destination, string source, TimeSpan timeout)
        {
            await this.ImageStore.UploadContentAsync(destination, source, timeout, CopyFlag.AtomicCopy, false);
        }

        public void CopyContent(string destination, string source, TimeSpan timeout, CopyFlag copyFlag = CopyFlag.AtomicCopy, bool checkFolderMarkFile = false)
        {
            this.ImageStore.CopyContent(source, destination, timeout, ManifestValidatorUtility.FabricAssemblies, copyFlag, checkFolderMarkFile);
        }

        public async Task CopyContentAsync(string destination, string source, TimeSpan timeout, CopyFlag copyFlag = CopyFlag.AtomicCopy, bool checkFolderMarkFile = false)
        {
            await this.ImageStore.CopyContentAsync(source, destination, timeout, ManifestValidatorUtility.FabricAssemblies, copyFlag, checkFolderMarkFile);
        }

        public string TryGetFromStore(string tag, out bool exists, TimeSpan timeout)
        {
            TimeoutHelper timeoutHelper = new TimeoutHelper(timeout);
            ImageBuilder.TraceSource.WriteInfo(
                TraceType,
                "Downloading {0} from store.",
                tag);

            exists = false;
            string value = null;            

            string tempFileName = ImageBuilderUtility.GetTempFileName(this.localImageBuilderRoot);

            try
            {
                if (this.ImageStore.DoesContentExist(tag, timeoutHelper.GetRemainingTime()))
                {
                    this.ImageStore.DownloadContent(tag, tempFileName, timeoutHelper.GetRemainingTime(), CopyFlag.AtomicCopy);

                    ImageBuilderUtility.RemoveReadOnlyFlag(tempFileName);                    
                    value = ImageBuilderUtility.ReadStringFromFile(tempFileName);
                    exists = true;
                }        
            }
            finally
            {
                ImageBuilderUtility.DeleteTempLocation(tempFileName);
            }

            return value;
        }

        public async Task<Tuple<string, bool>> TryGetFromStoreAsync(string tag, TimeSpan timeout)
        {
            TimeoutHelper timeoutHelper = new TimeoutHelper(timeout);
            ImageBuilder.TraceSource.WriteInfo(
                TraceType,
                "Downloading {0} from store.",
                tag);

            bool exists = false;
            string value = null;

            string tempFileName = ImageBuilderUtility.GetTempFileName(this.localImageBuilderRoot);

            try
            {
                bool doesExsit = await this.ImageStore.DoesContentExistAsync(tag, timeoutHelper.GetRemainingTime());
                if (doesExsit)
                {
                    await this.ImageStore.DownloadContentAsync(tag, tempFileName, timeoutHelper.GetRemainingTime(), CopyFlag.AtomicCopy);

                    ImageBuilderUtility.RemoveReadOnlyFlag(tempFileName);
                    value = ImageBuilderUtility.ReadStringFromFile(tempFileName);
                    exists = true;
                }
            }
            finally
            {
                ImageBuilderUtility.DeleteTempLocation(tempFileName);
            }

            return new Tuple<string, bool>(value, exists);
        }

        public T TryGetFromStore<T>(string tag, out bool exists, TimeSpan timeout)
        {
            TimeoutHelper timeoutHelper = new TimeoutHelper(timeout);
            ImageBuilder.TraceSource.WriteInfo(
                TraceType,
                "Downloading {0} from store.",
                tag);

            exists = false;
            T value = default(T);            

            string tempFileName = ImageBuilderUtility.GetTempFileName(this.localImageBuilderRoot);
            try
            {
                if (this.ImageStore.DoesContentExist(tag, timeoutHelper.GetRemainingTime()))
                {
                    this.ImageStore.DownloadContent(tag, tempFileName, timeoutHelper.GetRemainingTime(), CopyFlag.AtomicCopy);

                    ImageBuilderUtility.RemoveReadOnlyFlag(tempFileName);
                    value = ImageBuilderUtility.ReadXml<T>(tempFileName);
                    exists = true;
                }                
            }
            finally
            {
                ImageBuilderUtility.DeleteTempLocation(tempFileName);
            }            

            return value;
        }

        public async Task<Tuple<T,bool>> TryGetFromStoreAsync<T>(string tag, TimeSpan timeout)
        {
            TimeoutHelper timeoutHelper = new TimeoutHelper(timeout);
            ImageBuilder.TraceSource.WriteInfo(
                TraceType,
                "Downloading {0} from store.",
                tag);

            bool exists = false;
            T value = default(T);

            string tempFileName = ImageBuilderUtility.GetTempFileName(this.localImageBuilderRoot);
            try
            {
                if (await this.ImageStore.DoesContentExistAsync(tag, timeoutHelper.GetRemainingTime()))
                {
                    await this.ImageStore.DownloadContentAsync(tag, tempFileName, timeoutHelper.GetRemainingTime(), CopyFlag.AtomicCopy);

                    ImageBuilderUtility.RemoveReadOnlyFlag(tempFileName);
                    value = ImageBuilderUtility.ReadXml<T>(tempFileName);
                    exists = true;
                }
            }
            finally
            {
                ImageBuilderUtility.DeleteTempLocation(tempFileName);
            }

            return new Tuple<T,bool>(value, exists);
        }

        public T GetFromStore<T>(string tag, TimeSpan timeout)
        {
            TimeoutHelper timeoutHelper = new TimeoutHelper(timeout);
            ImageBuilder.TraceSource.WriteInfo(
                TraceType,
                "Downloading {0} from store.",
                tag);

            string tempFileName = ImageBuilderUtility.GetTempFileName(this.localImageBuilderRoot);            
            try
            {
                if (this.ImageStore.DoesContentExist(tag, timeoutHelper.GetRemainingTime()))
                {
                    this.ImageStore.DownloadContent(tag, tempFileName, timeoutHelper.GetRemainingTime(), CopyFlag.AtomicCopy);

                    ImageBuilderUtility.RemoveReadOnlyFlag(tempFileName);
                    return ImageBuilderUtility.ReadXml<T>(tempFileName);
                }
                else
                {
                    throw new FileNotFoundException(
                        string.Format(CultureInfo.CurrentCulture, StringResources.ImageStoreError_FileNotFound, tag));
                }
            }
            finally
            {
                ImageBuilderUtility.DeleteTempLocation(tempFileName);
            }            
        }

        public string GetFileNameFromStore(string tag, TimeSpan timeout)
        {
            TimeoutHelper timeoutHelper = new TimeoutHelper(timeout);
            ImageBuilder.TraceSource.WriteInfo(
                TraceType,
                "Downloading {0} from store.",
                tag);

            string tempFileName = ImageBuilderUtility.GetTempFileName(this.localImageBuilderRoot);
            if (this.ImageStore.DoesContentExist(tag, timeoutHelper.GetRemainingTime()))
            {
                this.ImageStore.DownloadContent(tag, tempFileName, timeoutHelper.GetRemainingTime(), CopyFlag.AtomicCopy);

                ImageBuilderUtility.RemoveReadOnlyFlag(tempFileName);
                return tempFileName;
            }
            else
            {
                throw new FileNotFoundException(
                    string.Format(CultureInfo.CurrentCulture, StringResources.ImageStoreError_FileNotFound, tag));
            }
        }

        public async Task<T> GetFromStoreAsync<T>(string tag, TimeSpan timeout)
        {
            TimeoutHelper timeoutHelper = new TimeoutHelper(timeout);
            ImageBuilder.TraceSource.WriteInfo(
               TraceType,
               "Downloading {0} from store async.",
               tag);

            string tempFileName = ImageBuilderUtility.GetTempFileName(this.localImageBuilderRoot);
            try
            {
                bool doesExist = await this.ImageStore.DoesContentExistAsync(tag, timeoutHelper.GetRemainingTime());
                if (doesExist)
                {
                    await this.ImageStore.DownloadContentAsync(tag, tempFileName, timeoutHelper.GetRemainingTime(), CopyFlag.AtomicCopy);

                    ImageBuilderUtility.RemoveReadOnlyFlag(tempFileName);
                    return ImageBuilderUtility.ReadXml<T>(tempFileName);
                }
                else
                {
                    throw new FileNotFoundException(
                        string.Format(CultureInfo.CurrentCulture, StringResources.ImageStoreError_FileNotFound, tag));
                }
            }
            finally
            {
                ImageBuilderUtility.DeleteTempLocation(tempFileName);
            }
        }

        public void SetToStore<T>(string tag, T value, TimeSpan timeout, bool shouldOverwrite = true)
        {
            TimeoutHelper timeoutHelper = new TimeoutHelper(timeout);
            ImageBuilder.TraceSource.WriteInfo(
                TraceType,
                "Uploading {0} to store. ShouldOverwrite: {1}.",
                tag,
                shouldOverwrite);

            if (!shouldOverwrite && this.ImageStore.DoesContentExist(tag, timeoutHelper.GetRemainingTime()))
            {
                return;
            }

            string tempFileName = ImageBuilderUtility.GetTempFileName(this.localImageBuilderRoot);
            try
            {
                ImageBuilderUtility.WriteXml<T>(tempFileName, value);
                this.ImageStore.UploadContent(tag, tempFileName, timeoutHelper.GetRemainingTime(), CopyFlag.AtomicCopy, true);                
            }
            finally
            {
                ImageBuilderUtility.DeleteTempLocation(tempFileName);
            }
        }

        public async Task SetToStoreAsync<T>(string tag, T value, TimeSpan timeout, bool shouldOverwrite = true)
        {
            TimeoutHelper timeoutHelper = new TimeoutHelper(timeout);
            ImageBuilder.TraceSource.WriteInfo(
                TraceType,
                "Uploading {0} to store. ShouldOverwrite: {1}.",
                tag,
                shouldOverwrite);

            if (!shouldOverwrite &&
                await this.ImageStore.DoesContentExistAsync(tag, timeoutHelper.GetRemainingTime()))
            {
                return;
            }

            string tempFileName = ImageBuilderUtility.GetTempFileName(this.localImageBuilderRoot);
            try
            {
                ImageBuilderUtility.WriteXml<T>(tempFileName, value);
                await this.ImageStore.UploadContentAsync(tag, tempFileName, timeoutHelper.GetRemainingTime(), CopyFlag.AtomicCopy, false /*acquireSourceLock*/);
            }
            finally
            {
                ImageBuilderUtility.DeleteTempLocation(tempFileName);
            }
        }

        public void DeleteContent(string tag, TimeSpan timeout)
        {
            this.ImageStore.DeleteContent(tag, timeout);
        }

        public async Task DeleteContentAsync(string tag, TimeSpan timeout)
        {
            await this.ImageStore.DeleteContentAsync(tag, timeout);
        }

        public bool IsXStoreBasedImageStore()
        {
            return this.ImageStore.RootUri.Trim().StartsWith(xstoreSchemaTag, StringComparison.OrdinalIgnoreCase);
        }
    }
}