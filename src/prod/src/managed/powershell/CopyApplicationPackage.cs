// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.ImageStore;
    using System.Fabric.Interop;
    using System.Fabric.Security;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.Management.Automation;
    using System.Security.Cryptography.X509Certificates;

    [Cmdlet(VerbsCommon.Copy, "ServiceFabricApplicationPackage")]
    public sealed class CopyApplicationPackage : ApplicationCmdletBase
    {
        private const int DefaultTimeoutInSec = 1800;

        public CopyApplicationPackage()
        {
            this.ImageStoreConnectionString = null;
            this.CertStoreLocation = StoreLocation.LocalMachine;
        }

        [Parameter(Mandatory = true, Position = 0)]
        public string ApplicationPackagePath
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, Position = 1)]
        public string ImageStoreConnectionString
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, Position = 2)]
        public string ApplicationPackagePathInImageStore
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string ApplicationPackageCopyPath
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public SwitchParameter ShowProgress
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public int ShowProgressIntervalMilliseconds
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public SwitchParameter CompressPackage
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public SwitchParameter UncompressPackage
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public SwitchParameter SkipCopy
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public SwitchParameter GenerateChecksums
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public StoreLocation CertStoreLocation
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            try
            {
                if (!this.TimeoutSec.HasValue)
                {
                    this.TimeoutSec = DefaultTimeoutInSec;
                }

                var imageStoreString = (this.SkipCopy && !this.GenerateChecksums)
                    ? string.Empty
                    : this.TryFetchImageStoreConnectionString(this.ImageStoreConnectionString, this.CertStoreLocation);

                var sourcePath = this.GetAbsolutePath(this.ApplicationPackagePath);

                using (var handler = new UploadProgressHandler(sourcePath, this.ShowProgressIntervalMilliseconds))
                {
                    var progressHandler = this.ShowProgress ? handler : null;

                    if (progressHandler != null)
                    {
                        progressHandler.Initialize();
                    }

                    sourcePath = this.CopyPackageIfNeeded(sourcePath, progressHandler);

                    this.CompressOrUncompressIfNeeded(sourcePath, progressHandler);

                    this.GenerateChecksumsIfNeeded(sourcePath, imageStoreString, progressHandler);

                    if (this.SkipCopy)
                    {
                        return;
                    }

                    this.UploadToImageStore(
                        imageStoreString,
                        sourcePath,
                        this.ApplicationPackagePathInImageStore,
                        progressHandler);
                }
            }
            catch (Exception exception)
            {
                this.ThrowTerminatingError(
                    exception,
                    Constants.CopyApplicationPackageErrorId,
                    null);
            }
        }

        private string CopyPackageIfNeeded(string sourcePath, UploadProgressHandler progressHandler)
        {
            if (!string.IsNullOrEmpty(this.ApplicationPackageCopyPath))
            {
                var destPath = this.GetAbsolutePath(this.ApplicationPackageCopyPath);

                if (progressHandler != null)
                {
                    progressHandler.SetStateCopying(destPath);
                }

                FabricDirectory.Copy(sourcePath, destPath, true);

                if (progressHandler != null)
                {
                    progressHandler.UpdateApplicationPackagePath(destPath);
                }

                sourcePath = destPath;
            }

            return sourcePath;
        }

        private void CompressOrUncompressIfNeeded(string sourcePath, UploadProgressHandler progressHandler)
        {
            if (this.CompressPackage)
            {
                if (progressHandler != null)
                {
                    progressHandler.SetStateCompressing();
                }

                ImageStoreUtility.ArchiveApplicationPackage(sourcePath, progressHandler);
            }
            else if (this.UncompressPackage)
            {
                if (progressHandler != null)
                {
                    progressHandler.SetStateUncompressing();
                }

                ImageStoreUtility.TryExtractApplicationPackage(sourcePath, progressHandler);
            }
        }

        private void GenerateChecksumsIfNeeded(string sourcePath, string imageStoreString, UploadProgressHandler progressHandler)
        {
            if (this.GenerateChecksums)
            {
                if (string.IsNullOrEmpty(imageStoreString))
                {
                    this.ThrowTerminatingError(
                        new ArgumentException(StringResources.Error_MissingImageStoreConnectionStringArgument),
                        Constants.CopyApplicationPackageErrorId,
                        null);
                }

                if (progressHandler != null)
                {
                    progressHandler.SetStateGeneratingChecksums();
                }

                ImageStoreUtility.GenerateApplicationPackageChecksumFiles(
                    sourcePath, 
                    progressHandler,
                    ImageStoreUtility.GetImageStoreProviderType(imageStoreString) == ImageStoreProviderType.ImageStoreService);
            }
        }
    }
}