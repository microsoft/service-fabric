// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Common.ImageModel;
    using System.Fabric.Description;
    using System.Fabric.ImageStore;
    using System.Fabric.Management.ImageBuilder;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Strings;
    using System.IO;
    using System.Linq;
    using System.Management.Automation;
    using System.Security.Cryptography.X509Certificates;

    [Cmdlet(VerbsCommon.Get, "ServiceFabricImageStoreContent")]
    public sealed class GetImageStoreContent : ImageStoreCmdletBase
    {
        private const string ImaageStorePathPrefix = "Store";
        private const string ChecksumFileExtension = "checksum";
     
        public GetImageStoreContent()
        {
            this.CertStoreLocation = StoreLocation.LocalMachine;
        }

        [Parameter(Mandatory = true, ParameterSetName = "Application")]
        public string ApplicationTypeName
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "Application")]
        public string ApplicationTypeVersion
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "Path", HelpMessage = @"Use empty string to get all locations")]
        [AllowEmptyString]
        public string RemoteRelativePath
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string ImageStoreConnectionString
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
            TimeSpan timeout = this.GetTimeout();
            string workingImageStoreConnectionString = this.TryFetchImageStoreConnectionString(this.ImageStoreConnectionString, this.CertStoreLocation);
            var imageStore = this.CreateImageStore(
                workingImageStoreConnectionString,
                System.IO.Path.GetTempPath());

            switch (this.ParameterSetName)
            {
                case "Path":
                    this.RemoteRelativePath = string.IsNullOrEmpty(this.RemoteRelativePath) ? " " : this.RemoteRelativePath.Trim(new char[] { System.IO.Path.DirectorySeparatorChar, System.IO.Path.AltDirectorySeparatorChar, ' ', '\t' });
                    if (workingImageStoreConnectionString.StartsWith(Constants.ImageStoreConnectionFabricType, StringComparison.OrdinalIgnoreCase))
                    {
                        this.GetImageStorePagedContentByLocation(this.RemoteRelativePath, imageStore, timeout);
                    }
                    else
                    {
                        this.GetImageStoreContentByLocation(this.RemoteRelativePath, imageStore, timeout);
                    }

                    break;
                case "Application":
                    if (string.IsNullOrEmpty(this.ApplicationTypeName))
                    {
                        this.WriteObject(StringResources.Error_ArgumentInvalid);
                        return;
                    }

                    TimeoutHelper helper = timeout == TimeSpan.MaxValue ? null : new TimeoutHelper(timeout);
                    var storeLayoutSpecification = StoreLayoutSpecification.Create();
                    if (storeLayoutSpecification == null)
                    {
                        throw new ArgumentException(StringResources.Error_storeLayoutSpecification_Init_Failed);
                    }

                    var clusterConnection = this.GetClusterConnection();
                    List<string> versions;
                    if (this.ApplicationTypeVersion == null)
                    {
                        versions = this.GetAppVersions(clusterConnection, this.ApplicationTypeName);
                        if (!versions.Any())
                        {
                            throw new ArgumentException(StringResources.Error_ApplicationTypeNotFound);
                        }
                    }
                    else
                    {
                        if (!this.VerifyAppVersion(clusterConnection, this.ApplicationTypeName, this.ApplicationTypeVersion))
                        {
                            throw new ArgumentException(StringResources.Error_ApplicationTypeNotFound);
                        }
                        else
                        {
                            versions = new List<string>()
                            {
                                this.ApplicationTypeVersion
                            };
                        }
                    }

                    if (helper != null)
                    {
                        helper.ThrowIfExpired();
                    }

                    if (workingImageStoreConnectionString.StartsWith(Constants.ImageStoreConnectionFabricType, StringComparison.OrdinalIgnoreCase))
                    {
                        bool morePages = true;
                        string currentContinuationToken = null;
                        while (morePages)
                        {
                            var content = imageStore.ListPagedContentWithDetailsAsync(
                                new ImageStoreListDescription(
                                    System.IO.Path.Combine(ImaageStorePathPrefix, this.ApplicationTypeName),
                                    currentContinuationToken,
                                    false),
                                helper == null ? timeout : helper.GetRemainingTime()).Result;

                            if (content.Files == null && content.Folders == null && string.IsNullOrEmpty(currentContinuationToken))
                            {
                                throw new ArgumentException(StringResources.ImageStoreError_DoesNotExistError);
                            }

                            versions.ForEach(version =>
                            {
                                this.OutputImageStoreContent(
                                 content.Files,
                                 content.Folders,
                                 version,
                                 imageStore,
                                 storeLayoutSpecification,
                                 clusterConnection,
                                 helper);
                            });

                            morePages = Helpers.ResultHasMorePages(this, content.ContinuationToken, out currentContinuationToken);
                            if (morePages)
                            {
                                this.WriteVerbose(string.Format("Continuation Token:{0}", currentContinuationToken));
                            }
                        }
                    }
                    else
                    {
                        var content = imageStore.ListContentWithDetails(System.IO.Path.Combine(ImaageStorePathPrefix, this.ApplicationTypeName), false, helper == null ? timeout : helper.GetRemainingTime());

                        if (content.Files == null && content.Folders == null)
                        {
                            throw new ArgumentException(StringResources.ImageStoreError_DoesNotExistError);
                        }

                        versions.ForEach(version =>
                        {
                            this.OutputImageStoreContent(
                                content.Files,
                                content.Folders,
                                version,
                                imageStore,
                                storeLayoutSpecification,
                                clusterConnection,
                                helper);
                        });
                    }

                    break;
            }
        }

        private void OutputImageStoreContent(
            List<ImageStoreFile> sourceFiles,
            List<ImageStoreFolder> sourceFolders,
            string version,
            IImageStore imageStore,
            StoreLayoutSpecification storeLayoutSpecification,
            IClusterConnection clusterConnection,
            TimeoutHelper helper)
        {
            var targetFiles = new List<ImageStoreFile>();
            var targetFolders = new List<ImageStoreFolder>();
            this.GetImageStoreContentByAppVersionImpl(
                                sourceFiles,
                                sourceFolders,
                                targetFiles,
                                targetFolders,
                                version,
                                imageStore,
                                storeLayoutSpecification,
                                clusterConnection,
                                helper);

            targetFiles.ForEach(file => { base.OutputStoreFileInfo(file); });
            targetFolders.ForEach(folder => { base.OutputStoreFolderInfo(folder); });
        }

        private void GetImageStoreContentByLocation(string location, IImageStore imageStore, TimeSpan timeout)
        {
            var content = imageStore.ListContentWithDetails(location, false, timeout);
            bool isEmpty = true;
            if (content.Files != null)
            {
                content.Files.ForEach(file => { base.OutputStoreFileInfo(file); });
                isEmpty = content.Files.Count == 0;
            }

            if (content.Folders != null)
            {
                content.Folders.ForEach(folder => { base.OutputStoreFolderInfo(folder); });
                isEmpty = content.Folders.Count == 0;
            }

            if (isEmpty && !string.IsNullOrEmpty(this.RemoteRelativePath))
            {
                this.WriteObject(StringResources.Error_ImageStoreRelativePathNotExist);
            }
        }

        private void GetImageStorePagedContentByLocation(string location, IImageStore imageStore, TimeSpan timeout)
        {
            bool morePages = true;
            string currentContinuationToken = string.Empty;

            while (morePages)
            {
                var content = imageStore.ListPagedContentWithDetailsAsync(
                    new ImageStoreListDescription(location, currentContinuationToken, false),
                    timeout).Result;

                bool isEmpty = true;
                if (content.Files != null)
                {
                    content.Files.ForEach(file => { base.OutputStoreFileInfo(file); });
                    isEmpty = content.Files.Count == 0;
                }

                if (content.Folders != null)
                {
                    content.Folders.ForEach(folder => { base.OutputStoreFolderInfo(folder); });
                    isEmpty = content.Folders.Count == 0;
                }

                morePages = Helpers.ResultHasMorePages(this, content.ContinuationToken, out currentContinuationToken);

                if (morePages)
                {
                    this.WriteVerbose(string.Format("Continuation Token:{0}", currentContinuationToken));
                }

                if (isEmpty && !string.IsNullOrEmpty(this.RemoteRelativePath) && string.IsNullOrEmpty(currentContinuationToken))
                {
                    this.WriteObject(StringResources.Error_ImageStoreRelativePathNotExist);
                }
            }
        }

        private void GetImageStoreContentByAppVersionImpl(
            List<ImageStoreFile> sourceFiles,
            List<ImageStoreFolder> sourceFolders,
            List<ImageStoreFile> targetFiles,
            List<ImageStoreFolder> targetFolders,
            string version,
            IImageStore imageStore,
            StoreLayoutSpecification storeLayoutSpecification,
            IClusterConnection clusterConnection,
            TimeoutHelper helper)
        {
            Action<string, string, string, string, bool> updateStoreInfo = (string path, string serviceName, string serviceVersion, string appVersion, bool isFile) =>
            {
                if (isFile)
                {
                    var existingFile = (from file in targetFiles
                                        where string.Compare(file.StoreRelativePath, path, StringComparison.OrdinalIgnoreCase) == 0
                                        select file).FirstOrDefault();

                    if (existingFile != null)
                    {
                        existingFile.VersionInfo.ConfigVersionInfo(serviceName, serviceVersion, appVersion);
                    }
                    else
                    {
                        existingFile = (from file in sourceFiles
                                        where string.Compare(file.StoreRelativePath, path, StringComparison.OrdinalIgnoreCase) == 0
                                        select file).FirstOrDefault();

                        if (existingFile != null)
                        {
                            existingFile.VersionInfo.ConfigVersionInfo(serviceName, serviceVersion, appVersion);
                            targetFiles.Add(existingFile);
                        }
                        else
                        {
                            throw new ArgumentException(StringResources.Error_ImageStoreRelativePathNotExist);
                        }
                    }
                }
                else
                {
                    var existingFolder = (from folder in targetFolders
                                          where string.Compare(folder.StoreRelativePath, path, StringComparison.OrdinalIgnoreCase) == 0
                                          select folder).FirstOrDefault();

                    if (existingFolder != null)
                    {
                        existingFolder.VersionInfo.ConfigVersionInfo(serviceName, serviceVersion, appVersion);
                    }
                    else
                    {
                        existingFolder = (from folder in sourceFolders
                                          where string.Compare(folder.StoreRelativePath, path, StringComparison.OrdinalIgnoreCase) == 0
                                          select folder).FirstOrDefault();

                        if (existingFolder != null)
                        {
                            existingFolder.VersionInfo.ConfigVersionInfo(serviceName, serviceVersion, appVersion);
                            targetFolders.Add(existingFolder);
                        }
                        else
                        {
                            throw new ArgumentException(StringResources.Error_ImageStoreRelativePathNotExist);
                        }
                    }
                }
            };

            if (helper != null)
            {
                helper.ThrowIfExpired();
            }

            var foldersPath = new Dictionary<string, Tuple<string, string>>();
            string applicationManifestPath = storeLayoutSpecification.GetApplicationManifestFile(this.ApplicationTypeName, version);

            updateStoreInfo(applicationManifestPath, string.Empty, string.Empty, version, true);
            var serviceTypeList = clusterConnection.GetServiceTypeListAsync(this.ApplicationTypeName, version, null, this.GetTimeout(), this.GetCancellationToken()).Result;

            foreach (var serviceType in serviceTypeList)
            {
                if (helper != null)
                {
                    helper.ThrowIfExpired();
                }

                string serviceManifestPath = storeLayoutSpecification.GetServiceManifestFile(this.ApplicationTypeName, serviceType.ServiceManifestName, serviceType.ServiceManifestVersion);
                updateStoreInfo(serviceManifestPath, serviceType.ServiceManifestName, serviceType.ServiceManifestVersion, version, true);
                if (helper != null)
                {
                    helper.ThrowIfExpired();
                }

                string serviceManifestChecksumPath = storeLayoutSpecification.GetServiceManifestChecksumFile(this.ApplicationTypeName, serviceType.ServiceManifestName, serviceType.ServiceManifestVersion);
                updateStoreInfo(serviceManifestChecksumPath, serviceType.ServiceManifestName, serviceType.ServiceManifestVersion, version, true);
                string localPath = System.IO.Path.Combine(System.IO.Path.GetTempPath(), serviceManifestPath.Substring(serviceManifestPath.LastIndexOf(@"\") + 1));
                try
                {
                    if (helper != null)
                    {
                        helper.ThrowIfExpired();
                        imageStore.DownloadContent(serviceManifestPath, localPath, helper.GetRemainingTime(), CopyFlag.AtomicCopy);
                    }
                    else
                    {
                        imageStore.DownloadContent(serviceManifestPath, localPath, this.GetTimeout(), CopyFlag.AtomicCopy);
                    }

                    foreach (var packagePath in this.GetPackagesPath(localPath, storeLayoutSpecification))
                    {
                        updateStoreInfo(packagePath, serviceType.ServiceManifestName, serviceType.ServiceManifestVersion, version, false);
                        updateStoreInfo(string.Format("{0}.{1}", packagePath, ChecksumFileExtension), serviceType.ServiceManifestName, serviceType.ServiceManifestVersion, version, true);
                    }
                }
                catch (Exception exception)
                {
                    this.ThrowTerminatingError(exception, Constants.GetImageStoreContentErrorId, null);
                }
                finally
                {
                    File.Delete(localPath);
                }
            }
        }

        private List<string> GetPackagesPath(string localPath, StoreLayoutSpecification storeLayoutSpecification)
        {
            var packagesPath = new List<string>();
            var manifest = ImageBuilderUtility.ReadXml<ServiceManifestType>(localPath);
            if (manifest.CodePackage != null)
            {
                foreach (var codePackage in manifest.CodePackage)
                {
                    string packagePath = storeLayoutSpecification.GetCodePackageFolder(this.ApplicationTypeName, manifest.Name, codePackage.Name, codePackage.Version);
                    packagesPath.Add(packagePath);
                }
            }

            if (manifest.ConfigPackage != null)
            {
                foreach (var configPackage in manifest.ConfigPackage)
                {
                    string packagePath = storeLayoutSpecification.GetConfigPackageFolder(this.ApplicationTypeName, manifest.Name, configPackage.Name, configPackage.Version);
                    packagesPath.Add(packagePath);
                }
            }

            if (manifest.DataPackage != null)
            {
                foreach (var dataPackage in manifest.DataPackage)
                {
                    string packagePath = storeLayoutSpecification.GetDataPackageFolder(this.ApplicationTypeName, manifest.Name, dataPackage.Name, dataPackage.Version);
                    packagesPath.Add(packagePath);
                }
            }

            return packagesPath;
        }
    }
}