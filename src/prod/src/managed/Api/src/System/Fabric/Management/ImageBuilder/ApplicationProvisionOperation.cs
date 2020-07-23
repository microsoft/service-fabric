// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Common.ImageModel;
    using System.Fabric.ImageStore;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Threading.Tasks;
    using System.Xml;

    class ApplicationProvisionOperation
    {
        private static readonly string TraceType = "ApplicationProvisionOperation";

        private string localPath;        
        private XmlReaderSettings validatingXmlReaderSettings;
        private ImageStoreWrapper imageStoreWrapper;
        private IEnumerable<IApplicationValidator> validators;
        private BuildLayoutSpecification remoteBuildLayoutSpecification;
        private BuildLayoutSpecification localBuildLayoutSpecification;
        private StoreLayoutSpecification storeLayoutSpecification;
        private bool shouldSkipChecksumValidation;
        private bool imageStoreServiceEnabled;

        private volatile int completedUploadCount;
        private volatile int totalUploadCount;

        public ApplicationProvisionOperation(
            string buildPath,
            string localPath,
            XmlReaderSettings validatingXmlReaderSettings,
            ImageStoreWrapper imageStoreWrapper,
            IEnumerable<IApplicationValidator> validators,
            bool shouldSkipChecksumValidation)
        {
            this.localPath = localPath;
            this.validatingXmlReaderSettings = validatingXmlReaderSettings;
            this.imageStoreWrapper = imageStoreWrapper;
            this.validators = validators;
            this.shouldSkipChecksumValidation = shouldSkipChecksumValidation;
            this.imageStoreServiceEnabled = imageStoreWrapper.ImageStore.RootUri.StartsWith(NativeImageStoreClient.SchemeTag);

            this.localBuildLayoutSpecification = BuildLayoutSpecification.Create();
            this.localBuildLayoutSpecification.SetRoot(localPath);

            this.remoteBuildLayoutSpecification = BuildLayoutSpecification.Create();
            this.remoteBuildLayoutSpecification.SetRoot(buildPath);

            this.storeLayoutSpecification = StoreLayoutSpecification.Create();

            this.completedUploadCount = 0;
            this.totalUploadCount = 0;
        }

        public async Task<ApplicationTypeContext> ProvisionApplicationAsync(
            bool validateOnly, 
            bool disableServerSideCopy, 
            ApplicationProvisionProgressHandler progressHandler,
            TimeoutHelper timeoutHelper,
            bool isComposeDeployment = false,
            bool isSFVolumeDiskServiceEnabled = false)
        {
            ManifestValidatorUtility.RemoveFabricAssemblies(this.localPath);

            var applicationTypeContext = await this.ParseApplicationPackageAsync(progressHandler, timeoutHelper, isComposeDeployment, isSFVolumeDiskServiceEnabled);

            if (!validateOnly)
            {
                timeoutHelper.ThrowIfExpired();

                await this.ConvertAndUploadToStoreAsync(disableServerSideCopy, applicationTypeContext, progressHandler, timeoutHelper);
            }

            return applicationTypeContext;
        }

        private async Task<ApplicationTypeContext> ParseApplicationPackageAsync(
            ApplicationProvisionProgressHandler progressHandler, 
            TimeoutHelper timeoutHelper,
            bool isComposeDeployment,
            bool isSFVolumeDiskServiceEnabled)
        {
            if (progressHandler != null)
            {
                progressHandler.WriteStatusValidating();
            }

            // Read the ApplicationManifest from the local path
            string applicationManifestFile = this.localBuildLayoutSpecification.GetApplicationManifestFile();
            if (!FabricFile.Exists(applicationManifestFile))
            {
                ImageBuilderUtility.TraceAndThrowValidationError(
                    TraceType,
                    StringResources.ImageBuilderError_InvalidBuildLayout_MissingApplicationManifest,
                    this.localPath,
                    Path.GetFileName(applicationManifestFile));
            }

            var applicationManifestType = ImageBuilderUtility.ReadXml<ApplicationManifestType>(applicationManifestFile, this.validatingXmlReaderSettings);
            string applicationTypeName = applicationManifestType.ApplicationTypeName;

            var applicationTypeContext = new ApplicationTypeContext(applicationManifestType, this.localPath);
            
            ImageBuilderSorterUtility.SortApplicationManifestType(applicationTypeContext.ApplicationManifest);

            // Read the ServiceManifests imported in the ApplicationManifest
            DuplicateDetector serviceManifestImportDuplicateDetector = new DuplicateDetector("ServiceManifestRef", "ServiceManifestName", applicationManifestFile);
            List<Task<ServiceManifest>> parseServiceManifestTasks = new List<Task<ServiceManifest>>();
            TimeSpan remainingTime = timeoutHelper.GetRemainingTime();
            foreach (ApplicationManifestTypeServiceManifestImport serviceManifestImport in applicationManifestType.ServiceManifestImport)
            {                
                var parseServiceManifestTask = this.ParseServiceManifestAsync(
                    applicationTypeName, 
                    serviceManifestImport.ServiceManifestRef.ServiceManifestName, 
                    serviceManifestImport.ServiceManifestRef.ServiceManifestVersion,
                    serviceManifestImportDuplicateDetector,
                    remainingTime);
                parseServiceManifestTasks.Add(parseServiceManifestTask);
            }

            await Task.WhenAll(parseServiceManifestTasks);

            parseServiceManifestTasks.ForEach(task => applicationTypeContext.ServiceManifests.Add(task.Result));

            this.ValidateApplicationType(applicationTypeContext, isComposeDeployment, isSFVolumeDiskServiceEnabled); 

            return applicationTypeContext;
        }

        private async Task<ServiceManifest> ParseServiceManifestAsync(string applicationTypeName, string serviceManifestName, string serviceManifestVersion, DuplicateDetector serviceManifestImportDuplicateDetector, TimeSpan timeout)
        {
            TimeoutHelper timeoutHelper = new TimeoutHelper(timeout);
            serviceManifestImportDuplicateDetector.Add(serviceManifestName);

            // Get build layout information
            string serviceManifestFile = this.localBuildLayoutSpecification.GetServiceManifestFile(serviceManifestName);

            // Get store layout information
            
            string storeServiceManifestFile = this.storeLayoutSpecification.GetServiceManifestFile(applicationTypeName, serviceManifestName, serviceManifestVersion);
            var getServiceManifestTask = this.imageStoreWrapper.TryGetFromStoreAsync<ServiceManifestType>(storeServiceManifestFile, timeoutHelper.GetRemainingTime());

            string storeServiceManifestChecksumFile = this.storeLayoutSpecification.GetServiceManifestChecksumFile(applicationTypeName, serviceManifestName, serviceManifestVersion);
            var getServiceManifestChecksumTask = this.imageStoreWrapper.TryGetFromStoreAsync(storeServiceManifestChecksumFile, timeoutHelper.GetRemainingTime());

            await Task.WhenAll(getServiceManifestTask, getServiceManifestChecksumTask);

            ServiceManifestType storeServiceManifestType = getServiceManifestTask.Result.Item1;
            bool serviceManifestFoundInStore = getServiceManifestTask.Result.Item2;

            string storeServiceManifestChecksum = getServiceManifestChecksumTask.Result.Item1;
            bool serviceManifestChecksumFoundInStore = getServiceManifestChecksumTask.Result.Item2;

            if (FabricFile.Exists(serviceManifestFile))
            {
                var serviceManifestType = ImageBuilderUtility.ReadXml<ServiceManifestType>(serviceManifestFile, this.validatingXmlReaderSettings);                

                // Validates that the Name in the ServiceManifestImport of ApplicationManifest matches the Name in the ServiceManifest
                if (!ImageBuilderUtility.Equals(serviceManifestName, serviceManifestType.Name))
                {
                    ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                        TraceType,
                        this.remoteBuildLayoutSpecification.GetApplicationManifestFile(),
                        StringResources.ImageBuilderError_InvalidServiceManifestRef,
                        "Name",
                        serviceManifestName,
                        serviceManifestType.Name);
                }

                // Validates that the Version in the ServiceManifestImport of ApplicationManifest matches the Version in the ServiceManifest
                if (!ImageBuilderUtility.Equals(serviceManifestVersion, serviceManifestType.Version))
                {
                    ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                        TraceType,
                        this.remoteBuildLayoutSpecification.GetApplicationManifestFile(),
                        StringResources.ImageBuilderError_InvalidServiceManifestRef,
                        "Version",
                        serviceManifestVersion,
                        serviceManifestType.Version);
                }

                var serviceManifest = await this.CreateServiceManifestAsync(applicationTypeName, serviceManifestType, serviceManifestFile, timeoutHelper.GetRemainingTime());

                // Ensure that the hash file is present for the ServiceManifest file
                string serviceManifestChecksumFile = this.localBuildLayoutSpecification.GetServiceManifestChecksumFile(serviceManifestName);
                serviceManifest.Checksum = ImageBuilderUtility.EnsureHashFile(serviceManifestFile, serviceManifestChecksumFile, this.imageStoreServiceEnabled);

                // Check against previous versions
                if (serviceManifestFoundInStore)
                {
                    if (serviceManifestChecksumFoundInStore)
                    {
                        serviceManifest.ConflictingChecksum = storeServiceManifestChecksum;
                        if (!ImageBuilderUtility.Equals(serviceManifest.Checksum, serviceManifest.ConflictingChecksum))
                        {
                            string currentWorkingDir = Directory.GetCurrentDirectory();
                            var downloadedStoreServiceManifestFile = Path.Combine(currentWorkingDir, Guid.NewGuid().ToString());

                            if (!this.imageStoreWrapper.DownloadIfContentExists(storeServiceManifestFile, downloadedStoreServiceManifestFile, timeoutHelper.GetRemainingTime()))
                            {
                                throw new FileNotFoundException(StringResources.Error_FileNotFound, storeServiceManifestFile);
                            }

                            string localServiceManifestContent = File.ReadAllText(serviceManifestFile);
                            string storeServiceManifestContent = File.ReadAllText(downloadedStoreServiceManifestFile);

                            ImageBuilder.TraceSource.WriteInfo(
                                TraceType,
                                "Checksum mismatch in ServiceManifest with Name:{0} and Version:{1}: LocalManifestFile:{2} StoreManifestFile:{3} localFileChecksum:{4} storeFileChecksum:{5}",
                                serviceManifestName,
                                serviceManifestVersion,
                                localServiceManifestContent,
                                storeServiceManifestContent,
                                serviceManifest.Checksum,
                                serviceManifest.ConflictingChecksum
                                );
                                ImageBuilderUtility.DeleteTempLocation(downloadedStoreServiceManifestFile);

                            ImageBuilderUtility.TraceAndThrowValidationError(
                                TraceType,
                                StringResources.ImageBuilderError_ServiceManifestModfiedWithoutChangingVersion,
                                serviceManifestName,
                                serviceManifestVersion);
                        }
                    }
                    else
                    {
                        ImageBuilder.TraceSource.WriteInfo(
                            TraceType,
                            "ServiceManifest with Name:{0} and Version:{1} is found in the store without a checksum file. Comparing the content.",
                            serviceManifestName,
                            serviceManifestVersion);

                        this.ValidateAgainstPreviousVersion(serviceManifestType, storeServiceManifestType);
                    }
                }

                return serviceManifest;
            }
            else if (serviceManifestFoundInStore)
            {
                ImageBuilder.TraceSource.WriteInfo(
                    TraceType,
                    "ServiceManifest with Name:{0} and Version:{1} is not provided, get from store",
                    serviceManifestName,
                    serviceManifestVersion);

                var serviceManifest = await this.CreateServiceManifestAsync(applicationTypeName, storeServiceManifestType, null, timeoutHelper.GetRemainingTime());
                serviceManifest.Checksum = storeServiceManifestChecksum;

                return serviceManifest;
            }
            else
            {

                string remoteServiceManifestFile = this.remoteBuildLayoutSpecification.GetServiceManifestFile(serviceManifestName);

                // Service manifest is not provided and it's not in store either, validation fails
                ImageBuilderUtility.TraceAndThrowValidationError(
                    TraceType,
                    StringResources.ImageBuilderError_InvalidBuildLayout_MissingResourceForService,
                    localPath,
                    Path.GetFileName(remoteServiceManifestFile),
                    serviceManifestName);
                return null;
            }
        }

        private async Task<ServiceManifest> CreateServiceManifestAsync(string applicationTypeName, ServiceManifestType serviceManifestType, string serviceManifestFile, TimeSpan timeout)
        {
            if (serviceManifestFile != null)
            {
                // Check that there are no duplicate code, data and config packages with the same name.
                // If not checked before next loop, multiple tasks are started in parallel and the duplicate package
                // will fail with FileLoadException (since the duplicate packages try to access the same folder).
                if (serviceManifestType.CodePackage != null)
                {
                    var duplicateDetector = new DuplicateDetector("CodePackage", "Name", serviceManifestFile);
                    foreach (var pkg in serviceManifestType.CodePackage)
                    {
                        duplicateDetector.Add(pkg.Name);
                    }
                }

                if (serviceManifestType.DataPackage != null)
                {
                    var duplicateDetector = new DuplicateDetector("DataPackage", "Name", serviceManifestFile);
                    foreach (var pkg in serviceManifestType.DataPackage)
                    {
                        duplicateDetector.Add(pkg.Name);
                    }
                }

                if (serviceManifestType.ConfigPackage != null)
                {
                    var duplicateDetector = new DuplicateDetector("ConfigPackage", "Name", serviceManifestFile);
                    foreach (var pkg in serviceManifestType.ConfigPackage)
                    {
                        duplicateDetector.Add(pkg.Name);
                    }
                }
            }

            TimeoutHelper timeoutHelper = new TimeoutHelper(timeout);
            var serviceManifest = new ServiceManifest(serviceManifestType);
            string serviceManifestName = serviceManifestType.Name;

            List<Task<CodePackage>> codePackageTasks = new List<Task<CodePackage>>();
            TimeSpan remainingTime = timeoutHelper.GetRemainingTime();
            foreach (CodePackageType codePackageType in serviceManifestType.CodePackage)
            {
                var codePackageTask = this.ParseCodePackageAsync(applicationTypeName, serviceManifestName, codePackageType, remainingTime);
                codePackageTasks.Add(codePackageTask);
            }

            List<Task<ConfigPackage>> configPackageTasks = new List<Task<ConfigPackage>>();
            if (serviceManifestType.ConfigPackage != null)
            {
                foreach (ConfigPackageType configPackageType in serviceManifestType.ConfigPackage)
                {
                    var configPackageTask = this.ParseConfigPackageAsync(applicationTypeName, serviceManifestName, configPackageType, remainingTime);
                    configPackageTasks.Add(configPackageTask);
                }
            }

            List<Task<DataPackage>> dataPackageTasks = new List<Task<DataPackage>>();
            if (serviceManifestType.DataPackage != null)
            {
                foreach (DataPackageType dataPackageType in serviceManifestType.DataPackage)
                {
                    var dataPackageTask = this.ParseDataPackageAsync(applicationTypeName, serviceManifestName, dataPackageType, remainingTime);
                    dataPackageTasks.Add(dataPackageTask);
                }
            }

            List<Task> packageTasks = new List<Task>();
            packageTasks.AddRange(codePackageTasks);
            packageTasks.AddRange(configPackageTasks);
            packageTasks.AddRange(dataPackageTasks);

            await Task.WhenAll(packageTasks);

            codePackageTasks.ForEach(task => serviceManifest.CodePackages.Add(task.Result));
            configPackageTasks.ForEach(task => serviceManifest.ConfigPackages.Add(task.Result));
            dataPackageTasks.ForEach(task => serviceManifest.DataPackages.Add(task.Result));

            ImageBuilderSorterUtility.SortServiceManifestType(serviceManifest.ServiceManifestType);

			foreach (var serviceType in serviceManifest.ServiceManifestType.ServiceTypes)
            {
                if (serviceType is ServiceTypeType)
                {
                    this.ValidateServiceLoadMetric(serviceType as ServiceTypeType);
                }
                else if (serviceType is ServiceGroupTypeType)
                {
                    this.ValidateServiceGroupLoadMetric(serviceType as ServiceGroupTypeType);
                }        
            }

            return serviceManifest;
        }

        // validate invalid name and value of default load metric user specified in service
        private void ValidateServiceLoadMetric(ServiceTypeType serviceType)
        {
            if (serviceType == null || serviceType.LoadMetrics == null)
            {
                return;
            }

            if (serviceType is StatefulServiceTypeType)
            {
                foreach (var loadMetric in serviceType.LoadMetrics)
                {
                    if ((loadMetric.Name == "PrimaryCount" && (loadMetric.PrimaryDefaultLoad != 1 || loadMetric.SecondaryDefaultLoad != 0))
                        || (loadMetric.Name == "ReplicaCount" && (loadMetric.PrimaryDefaultLoad != 1 || loadMetric.SecondaryDefaultLoad != 1))
                        || (loadMetric.Name == "SecondaryCount" && (loadMetric.PrimaryDefaultLoad != 0 || loadMetric.SecondaryDefaultLoad != 1))
                        || (loadMetric.Name == "Count" && (loadMetric.PrimaryDefaultLoad != 1 || loadMetric.SecondaryDefaultLoad != 1))
						|| (loadMetric.Name == "InstanceCount"))
                    {
                        ImageBuilderUtility.TraceAndThrowValidationError(
                                TraceType,
                                StringResources.Error_InvalidDefaultMetricValue);
                    }
                }
            }
            else if (serviceType is StatelessServiceTypeType)
            {
                foreach (var loadMetric in serviceType.LoadMetrics)
                {
                    if ((loadMetric.Name == "PrimaryCount")
                        || (loadMetric.Name == "ReplicaCount")
                        || (loadMetric.Name == "SecondaryCount")
                        || (loadMetric.Name == "Count" && loadMetric.DefaultLoad != 1)
						|| (loadMetric.Name == "InstanceCount" && loadMetric.DefaultLoad != 1))
                    {
                        ImageBuilderUtility.TraceAndThrowValidationError(
                                TraceType,
                                StringResources.Error_InvalidDefaultMetricValue);
                    }
                }
            }           
        }

        // validate invalid name and value of default load metric user specified in service group
        private void ValidateServiceGroupLoadMetric(ServiceGroupTypeType serviceType)
        {
            if (serviceType == null || serviceType.LoadMetrics == null)
            {
                return;
            }

            if (serviceType is StatefulServiceGroupTypeType)
            {
                foreach (var loadMetric in serviceType.LoadMetrics)
                {
                    if ((loadMetric.Name == "PrimaryCount" && (loadMetric.PrimaryDefaultLoad != 1 || loadMetric.SecondaryDefaultLoad != 0))
                        || (loadMetric.Name == "ReplicaCount" && (loadMetric.PrimaryDefaultLoad != 1 || loadMetric.SecondaryDefaultLoad != 1))
                        || (loadMetric.Name == "SecondaryCount" && (loadMetric.PrimaryDefaultLoad != 0 || loadMetric.SecondaryDefaultLoad != 1))
                        || (loadMetric.Name == "Count" && (loadMetric.PrimaryDefaultLoad != 1 || loadMetric.SecondaryDefaultLoad != 1))
                        || (loadMetric.Name == "InstanceCount"))
                    {
                        ImageBuilderUtility.TraceAndThrowValidationError(
                                TraceType,
                                StringResources.Error_InvalidDefaultMetricValue);
                    }
                }
            }
            else if (serviceType is StatelessServiceGroupTypeType)
            {
                foreach (var loadMetric in serviceType.LoadMetrics)
                {
                    if ((loadMetric.Name == "PrimaryCount")
                        || (loadMetric.Name == "ReplicaCount")
                        || (loadMetric.Name == "SecondaryCount")
                        || (loadMetric.Name == "Count" && loadMetric.DefaultLoad != 1)
                        || (loadMetric.Name == "InstanceCount" && loadMetric.DefaultLoad != 1))
                    {
                        ImageBuilderUtility.TraceAndThrowValidationError(
                                TraceType,
                                StringResources.Error_InvalidDefaultMetricValue);
                    }
                }
            }
        }

        private async Task<CodePackage> ParseCodePackageAsync(string applicationTypeName, string serviceManifestName, CodePackageType codePackageType, TimeSpan timeout)
        {
            TimeoutHelper timeoutHelper = new TimeoutHelper(timeout);
            string codePackageName = codePackageType.Name;
            string codePackageVersion = codePackageType.Version;

            // Get build layout information
            string codePackageDirectory = this.localBuildLayoutSpecification.GetCodePackageFolder(serviceManifestName, codePackageName);

            string codePackageArchive = null;
            string localCodePackagePath = GetLocalPackagePath(codePackageDirectory, out codePackageArchive);

            // Get store layout information
            string storeCodePackageChecksumFile = this.storeLayoutSpecification.GetCodePackageChecksumFile(applicationTypeName, serviceManifestName, codePackageName, codePackageVersion);            
            var tryGetResult = await this.imageStoreWrapper.TryGetFromStoreAsync(storeCodePackageChecksumFile, timeoutHelper.GetRemainingTime());

            string storeCodePackageChecksum = tryGetResult.Item1;
            bool codePackageChecksumFoundInStore = tryGetResult.Item2;

            if (!string.IsNullOrEmpty(localCodePackagePath))
            {
                var codePackage = new CodePackage(codePackageType);

                string codePackageChecksumFile = this.localBuildLayoutSpecification.GetCodePackageChecksumFile(serviceManifestName, codePackageName);
                codePackage.Checksum = ImageBuilderUtility.EnsureHashFile(localCodePackagePath, codePackageChecksumFile, this.imageStoreServiceEnabled);

                if (codePackageChecksumFoundInStore)
                {
                    codePackage.ConflictingChecksum = storeCodePackageChecksum;
                    if (!ImageBuilderUtility.Equals(codePackage.Checksum, codePackage.ConflictingChecksum))
                    {
                        if (this.shouldSkipChecksumValidation)
                        {
                            ImageBuilder.TraceSource.WriteWarning(
                                TraceType,
                                "CodePackage with Name:{0} and Version:{1} in ServiceManifest:{2} is found in the store with a different checksum. Skipping validation since DisableChecksumValidation is set to true.",
                                codePackageName,
                                codePackageVersion,
                                serviceManifestName);

                            codePackage.ConflictingChecksum = codePackage.Checksum;
                        }
                        else
                        {
                            ImageBuilderUtility.TraceAndThrowValidationError(
                                TraceType,
                                StringResources.ImageBuilderError_PackageModfiedWithoutChangingVersion,
                                "CodePackage",
                                codePackageName,
                                codePackageVersion,
                                serviceManifestName);

                            return null;
                        }
                    }
                }

                return codePackage;
            }
            else if (codePackageChecksumFoundInStore)
            {
                ImageBuilder.TraceSource.WriteNoise(
                    TraceType,
                    "CodePackage with Name:{0} and Version:{1} in ServiceManifest:{2} is not provided, get from store",
                    codePackageName,
                    codePackageVersion,
                    serviceManifestName);

                var codePackage = new CodePackage(codePackageType);
                codePackage.Checksum = storeCodePackageChecksum;

                return codePackage;
            }
            else
            {
                var containerHost = codePackageType.EntryPoint.Item as ContainerHostEntryPointType;
                if (codePackageType.SetupEntryPoint == null &&
                    containerHost != null &&
                    String.IsNullOrEmpty(containerHost.FromSource))
                {
                    ImageBuilder.TraceSource.WriteNoise(
                        TraceType,
                        "CodePackage with Name:{0} and Version:{1} in ServiceManifest:{2} is ContainerHostEntryPoint and does not contain codepackage folder",
                        codePackageName,
                        codePackageVersion,
                        serviceManifestName);
                    return new CodePackage(codePackageType);
                }
                else
                {

                    ImageBuilderUtility.TraceAndThrowValidationError(
                        TraceType,
                        StringResources.ImageBuilderError_InvalidBuildLayout_MissingResourceForService,
                        localPath,
                        Path.GetFileName(codePackageDirectory),
                        serviceManifestName);
                    return null;
                }
            }
        }

        private async Task<ConfigPackage> ParseConfigPackageAsync(string applicationTypeName, string serviceManifestName, ConfigPackageType configPackageType, TimeSpan timeout)
        {
            TimeoutHelper timeoutHelper = new TimeoutHelper(timeout);
            string configPackageName = configPackageType.Name;
            string configPackageVersion = configPackageType.Version;

            // Get build layout information
            string configPackageDirectory = this.localBuildLayoutSpecification.GetConfigPackageFolder(serviceManifestName, configPackageName);

            string configPackageArchive = null;
            string localConfigPackagePath = GetLocalPackagePath(configPackageDirectory, out configPackageArchive);
            
            // Get store layout information
            string storeConfigPackageChecksumFile = this.storeLayoutSpecification.GetConfigPackageChecksumFile(applicationTypeName, serviceManifestName, configPackageName, configPackageVersion);            
            var tryGetResult = await this.imageStoreWrapper.TryGetFromStoreAsync(storeConfigPackageChecksumFile, timeoutHelper.GetRemainingTime());

            string storeConfigPackageChecksum = tryGetResult.Item1;
            bool configPackageChecksumFoundInStore = tryGetResult.Item2;

            if (!string.IsNullOrEmpty(localConfigPackagePath))
            {
                string settingsFile = this.localBuildLayoutSpecification.GetSettingsFile(configPackageDirectory);

                if (!string.IsNullOrEmpty(configPackageArchive))
                {
                    if (!FabricDirectory.Exists(configPackageDirectory))
                    {
                        FabricDirectory.CreateDirectory(configPackageDirectory);
                    }

                    ImageBuilderUtility.ExtractFromArchive(configPackageArchive, Path.GetFileName(settingsFile), settingsFile);
                }

                SettingsType settingsType = null;
                bool locallyGenerated = false;
                if (FabricFile.Exists(settingsFile))
                {
                    settingsType = ImageBuilderUtility.ReadXml<SettingsType>(settingsFile, this.validatingXmlReaderSettings);
                }
                else
                {
                    ImageBuilder.TraceSource.WriteInfo(
                        TraceType,
                        "Settings file is not found. Creating an empty Settings file. ApplicatioName:{0} ServiceManifestName:{1} ConfigPackageName:{2} ConfigPackageVersion{3}.",
                        applicationTypeName,
                        serviceManifestName,
                        configPackageName,
                        configPackageVersion);

                    // If no settings file is found, create an empty Settings file
                    settingsType = new SettingsType();
                    ImageBuilderUtility.WriteXml<SettingsType>(settingsFile, settingsType);
                    locallyGenerated = true;

                    if (!string.IsNullOrEmpty(configPackageArchive))
                    {
                        ImageBuilderUtility.AddToArchive(configPackageArchive, settingsFile, Path.GetFileName(settingsFile));
                    }
                }

                var configPackage = new ConfigPackage(configPackageType) { SettingsType = settingsType, SettingsFileLocallyGenerated = locallyGenerated };

                // Ensure checksum is present
                string configPackageChecksumFile = this.localBuildLayoutSpecification.GetConfigPackageChecksumFile(serviceManifestName, configPackageType.Name);
                configPackage.Checksum = ImageBuilderUtility.EnsureHashFile(localConfigPackagePath, configPackageChecksumFile, this.imageStoreServiceEnabled);

                if (configPackageChecksumFoundInStore)
                {
                    configPackage.ConflictingChecksum = storeConfigPackageChecksum;
                    if (!ImageBuilderUtility.Equals(configPackage.Checksum, configPackage.ConflictingChecksum))
                    {
                        if (this.shouldSkipChecksumValidation)
                        {
                            ImageBuilder.TraceSource.WriteWarning(
                                TraceType,
                                "ConfigPackage with Name:{0} and Version:{1} in ServiceManifest:{2} is found in the store with a different checksum. Skipping validation since DisableChecksumValidation is set to true.",
                                configPackageName,
                                configPackageVersion,
                                serviceManifestName);

                            configPackage.ConflictingChecksum = configPackage.Checksum;
                        }
                        else
                        {
                            ImageBuilderUtility.TraceAndThrowValidationError(
                                TraceType,
                                StringResources.ImageBuilderError_PackageModfiedWithoutChangingVersion,
                                "ConfigPackage",
                                configPackageName,
                                configPackageVersion,
                                serviceManifestName);

                            return null;
                        }
                    }
                }

                return configPackage;
            }
            else if (configPackageChecksumFoundInStore)
            {
                ImageBuilder.TraceSource.WriteNoise(
                    TraceType,
                    "ConfigPackage with Name:{0} and Version:{1} in ServiceManifest:{2} is not provided, get from store.",
                    configPackageName,
                    configPackageVersion,
                    serviceManifestName);

                var configPackage = new ConfigPackage(configPackageType);
                configPackage.Checksum = storeConfigPackageChecksum;

                return configPackage;
            }
            else
            {
                ImageBuilderUtility.TraceAndThrowValidationError(
                    TraceType,
                    StringResources.ImageBuilderError_InvalidBuildLayout_MissingResourceForService,
                    localPath,
                    Path.GetFileName(configPackageDirectory),
                    serviceManifestName);
                return null;
            }
        }

        private async Task<DataPackage> ParseDataPackageAsync(string applicationTypeName, string serviceManifestName, DataPackageType dataPackageType, TimeSpan timeout)
        {
            string dataPackageName = dataPackageType.Name;
            string dataPackageVersion = dataPackageType.Version;

            // Get build layout information
            string dataPackageDirectory = this.localBuildLayoutSpecification.GetDataPackageFolder(serviceManifestName, dataPackageName);

            string dataPackageArchive = null;
            string localDataPackagePath = GetLocalPackagePath(dataPackageDirectory, out dataPackageArchive);

            // Get store layout information
            string storeDataPackageChecksumFile = this.storeLayoutSpecification.GetDataPackageChecksumFile(applicationTypeName, serviceManifestName, dataPackageName, dataPackageVersion);
            var tryGetResult = await this.imageStoreWrapper.TryGetFromStoreAsync(storeDataPackageChecksumFile, timeout);

            string storeDataPackageChecksum = tryGetResult.Item1;
            bool dataPackageChecksumFoundInStore = tryGetResult.Item2;

            if (!string.IsNullOrEmpty(localDataPackagePath))
            {
                var dataPackage = new DataPackage(dataPackageType);

                // Ensure checksum is present
                string dataPackageChecksumFile = this.localBuildLayoutSpecification.GetDataPackageChecksumFile(serviceManifestName, dataPackageType.Name);
                dataPackage.Checksum = ImageBuilderUtility.EnsureHashFile(localDataPackagePath, dataPackageChecksumFile, this.imageStoreServiceEnabled);

                if (dataPackageChecksumFoundInStore)
                {
                    dataPackage.ConflictingChecksum = storeDataPackageChecksum;
                    if (!ImageBuilderUtility.Equals(dataPackage.Checksum, dataPackage.ConflictingChecksum))
                    {
                        if (this.shouldSkipChecksumValidation)
                        {
                            ImageBuilder.TraceSource.WriteWarning(
                                TraceType,
                                "DataPackage with Name:{0} and Version:{1} in ServiceManifest:{2} is found in the store with a different checksum. Skipping validation since DisableChecksumValidation is set to true.",
                                dataPackageName,
                                dataPackageVersion,
                                serviceManifestName);

                            dataPackage.ConflictingChecksum = dataPackage.Checksum;
                        }
                        else
                        {
                            ImageBuilderUtility.TraceAndThrowValidationError(
                                TraceType,
                                StringResources.ImageBuilderError_PackageModfiedWithoutChangingVersion,
                                "DataPackage",
                                dataPackageName,
                                dataPackageVersion,
                                serviceManifestName);
                            return null;
                        }
                    }
                }
                
                return dataPackage;
            }
            else if (dataPackageChecksumFoundInStore)
            {
                ImageBuilder.TraceSource.WriteNoise(
                    TraceType,
                    "DataPackage with Name:{0} and Version:{1} in ServiceManifest:{2} is found in the store at {3}.",
                    dataPackageName,
                    dataPackageVersion,
                    serviceManifestName,
                    storeDataPackageChecksumFile);

                var dataPackage = new DataPackage(dataPackageType);
                dataPackage.Checksum = storeDataPackageChecksum;

                return dataPackage;
            }
            else
            {
                ImageBuilderUtility.TraceAndThrowValidationError(
                    TraceType,
                    StringResources.ImageBuilderError_InvalidBuildLayout_MissingResourceForService,
                    localPath,
                    Path.GetFileName(dataPackageDirectory),
                    serviceManifestName);
                return null;
            }
        }
        
        private void ValidateApplicationType(ApplicationTypeContext applicationTypeContext, 
        bool isComposeDeployment = false,
        bool isSFVolumeDiskServiceEnabled = false)
        {
            foreach (var validator in this.validators)
            {
                validator.Validate(applicationTypeContext, isComposeDeployment, isSFVolumeDiskServiceEnabled);
            }
        }
        
        private void ValidateAgainstPreviousVersion(ServiceManifestType currentServiceManifestType, ServiceManifestType targetServiceManifestType)
        {
            if (!ImageBuilderUtility.Equals(currentServiceManifestType.Version, targetServiceManifestType.Version))
            {
                return;
            }

            bool hasServiceTypesChanged = HasServiceTypesChanged(currentServiceManifestType, targetServiceManifestType);
            bool hasCodePackagesChanged = ImageBuilderUtility.IsNotEqual<CodePackageType>(currentServiceManifestType.CodePackage, targetServiceManifestType.CodePackage);
            bool hasConfigPackagesChanged = ImageBuilderUtility.IsNotEqual<ConfigPackageType>(currentServiceManifestType.ConfigPackage, targetServiceManifestType.ConfigPackage);
            bool hasDataPackagesChanged = ImageBuilderUtility.IsNotEqual<DataPackageType>(currentServiceManifestType.DataPackage, targetServiceManifestType.DataPackage);
            bool hasResourcesChanged = ImageBuilderUtility.IsNotEqual<ResourcesType>(currentServiceManifestType.Resources, targetServiceManifestType.Resources);

            if (hasServiceTypesChanged ||
                hasCodePackagesChanged ||
                hasConfigPackagesChanged ||
                hasDataPackagesChanged ||
                hasResourcesChanged)
            {
                ImageBuilderUtility.TraceAndThrowValidationError(
                        TraceType,
                        StringResources.ImageBuilderError_ServiceManifestModfiedWithoutChangingVersion,
                        targetServiceManifestType.Name,
                        targetServiceManifestType.Version);
            }
        }

        private static bool HasServiceTypesChanged(ServiceManifestType currentServiceManifest, ServiceManifestType targetServiceManifest)
        {
            ServiceTypeType[] currentServiceTypes = null;
            ServiceGroupTypeType[] currentServiceGroupTypes = null;
            if (currentServiceManifest.ServiceTypes != null)
            {
                currentServiceTypes = currentServiceManifest.ServiceTypes.Where(type => type is ServiceTypeType).Cast<ServiceTypeType>().ToArray();
                currentServiceGroupTypes = currentServiceManifest.ServiceTypes.Where(type => type is ServiceGroupTypeType).Cast<ServiceGroupTypeType>().ToArray();
            }

            ServiceTypeType[] targetServiceTypes = null;
            ServiceGroupTypeType[] targetServiceGroupTypes = null;
            if (targetServiceManifest.ServiceTypes != null)
            {
                targetServiceTypes = targetServiceManifest.ServiceTypes.Where(type => type is ServiceTypeType).Cast<ServiceTypeType>().ToArray();
                targetServiceGroupTypes = targetServiceManifest.ServiceTypes.Where(type => type is ServiceGroupTypeType).Cast<ServiceGroupTypeType>().ToArray();
            }

            bool hasChanged = ImageBuilderUtility.IsNotEqual<ServiceTypeType>(currentServiceTypes, targetServiceTypes) ||
                              ImageBuilderUtility.IsNotEqual<ServiceGroupTypeType>(currentServiceGroupTypes, targetServiceGroupTypes);

            return hasChanged;
        }

        private async Task ConvertAndUploadToStoreAsync(
            bool shouldUpload, 
            ApplicationTypeContext applicationTypeContext, 
            ApplicationProvisionProgressHandler progressHandler,
            TimeoutHelper timeoutHelper)
        {
            ImageBuilder.TraceSource.WriteInfo(
                TraceType,
                "Start converting buildlayout to storelayout and uploading to store.");

            string applicationTypeName = applicationTypeContext.ApplicationManifest.ApplicationTypeName;

            List<Task> tasks = new List<Task>();

            // Copies the ServiceManifest files
            foreach (ServiceManifest serviceManifest in applicationTypeContext.ServiceManifests)
            {
                ServiceManifestType serviceManifestType = serviceManifest.ServiceManifestType;
                string serviceManifestName = serviceManifestType.Name;

                // Copies the code package for each service
                foreach (CodePackage codePackage in serviceManifest.CodePackages)
                {
                    string packageName = codePackage.CodePackageType.Name;
                    string packageVersion = codePackage.CodePackageType.Version;

                    string sourcePackagePath = null;
                    string destinationPackagePath = null;

                    if (!TryGetUploadArguments(
                        applicationTypeName,
                        serviceManifestName,
                        packageName,
                        packageVersion,
                        (s, p) => { return this.localBuildLayoutSpecification.GetCodePackageFolder(s, p); },
                        (s, n) => { return this.remoteBuildLayoutSpecification.GetCodePackageFolder(s, n); },
                        (a, s, n, v) => { return this.storeLayoutSpecification.GetCodePackageFolder(a, s, n, v); },
                        out sourcePackagePath,
                        out destinationPackagePath))
                    {
                        continue;
                    }

                    string sourceCodePackageChecksumFile = this.localBuildLayoutSpecification.GetCodePackageChecksumFile(
                        serviceManifestType.Name, 
                        codePackage.CodePackageType.Name);

                    string destinationCodePackageChecksumFile = this.storeLayoutSpecification.GetCodePackageChecksumFile(
                        applicationTypeContext.ApplicationManifest.ApplicationTypeName,
                        serviceManifestType.Name,
                        codePackage.CodePackageType.Name,
                        codePackage.CodePackageType.Version);

                    var codePackageTask = this.UploadToStoreIfModifiedAsync(
                        progressHandler,
                        sourcePackagePath,
                        sourceCodePackageChecksumFile,
                        codePackage.Checksum,
                        destinationPackagePath,
                        destinationCodePackageChecksumFile,
                        codePackage.ConflictingChecksum,
                        shouldUpload,
                        timeoutHelper);

                    tasks.Add(codePackageTask);
                }

                foreach (ConfigPackage configPackage in serviceManifest.ConfigPackages)
                {
                    string packageName = configPackage.ConfigPackageType.Name;
                    string packageVersion = configPackage.ConfigPackageType.Version;

                    string sourcePackagePath = null;
                    string destinationPackagePath = null;

                    bool useLocalSource = configPackage.SettingsFileLocallyGenerated;

                    if (!TryGetUploadArguments(
                        applicationTypeName,
                        serviceManifestName,
                        packageName,
                        packageVersion,
                        (s, p) => { return this.localBuildLayoutSpecification.GetConfigPackageFolder(s, p); },
                        useLocalSource,
                        (s, n) => 
                        { 
                            return useLocalSource 
                                ? this.localBuildLayoutSpecification.GetConfigPackageFolder(s, n)
                                : this.remoteBuildLayoutSpecification.GetConfigPackageFolder(s, n); 
                        },
                        (a, s, n, v) => { return this.storeLayoutSpecification.GetConfigPackageFolder(a, s, n, v); },
                        out sourcePackagePath,
                        out destinationPackagePath))
                    {
                        continue;
                    }

                    string sourceConfigPackageChecksumFile = this.localBuildLayoutSpecification.GetConfigPackageChecksumFile(
                        serviceManifestType.Name, 
                        configPackage.ConfigPackageType.Name);

                    string destinationConfigPackageChecksumFile = this.storeLayoutSpecification.GetConfigPackageChecksumFile(
                        applicationTypeContext.ApplicationManifest.ApplicationTypeName,
                        serviceManifestType.Name,
                        configPackage.ConfigPackageType.Name,
                        configPackage.ConfigPackageType.Version);

                    // If settings file is locally generated, Upload the ConfigPackage folder
                    var configPackageTask = this.UploadToStoreIfModifiedAsync(
                        progressHandler,
                        sourcePackagePath,
                        sourceConfigPackageChecksumFile,
                        configPackage.Checksum,
                        destinationPackagePath,
                        destinationConfigPackageChecksumFile,
                        configPackage.ConflictingChecksum,
                        (useLocalSource || shouldUpload),
                        timeoutHelper);

                    tasks.Add(configPackageTask);
                }

                // Copies the data packages for each service
                if (serviceManifestType.DataPackage != null)
                {
                    foreach (DataPackage dataPackage in serviceManifest.DataPackages)
                    {
                        string packageName = dataPackage.DataPackageType.Name;
                        string packageVersion = dataPackage.DataPackageType.Version;

                        string sourcePackagePath = null;
                        string destinationPackagePath = null;

                        if (!TryGetUploadArguments(
                            applicationTypeName,
                            serviceManifestName,
                            packageName,
                            packageVersion,
                            (s, p) => { return this.localBuildLayoutSpecification.GetDataPackageFolder(s, p); },
                            (s, n) => { return this.remoteBuildLayoutSpecification.GetDataPackageFolder(s, n); },
                            (a, s, n, v) => { return this.storeLayoutSpecification.GetDataPackageFolder(a, s, n, v); },
                            out sourcePackagePath,
                            out destinationPackagePath))
                        {
                            continue;
                        }

                        string sourceDataPackageChecksumFile = this.localBuildLayoutSpecification.GetDataPackageChecksumFile(serviceManifestType.Name, dataPackage.DataPackageType.Name);

                        string destinationDataPackageChecksumFile = this.storeLayoutSpecification.GetDataPackageChecksumFile(
                            applicationTypeContext.ApplicationManifest.ApplicationTypeName,
                            serviceManifestType.Name,
                            dataPackage.DataPackageType.Name,
                            dataPackage.DataPackageType.Version);

                        var dataPackageTask = this.UploadToStoreIfModifiedAsync(
                            progressHandler,
                            sourcePackagePath,
                            sourceDataPackageChecksumFile,
                            dataPackage.Checksum,
                            destinationPackagePath,
                            destinationDataPackageChecksumFile,
                            dataPackage.ConflictingChecksum,
                            shouldUpload,
                            timeoutHelper);

                        tasks.Add(dataPackageTask);
                    }
                }

                // ServiceManifestType object is sorted. Upload the sorted manifest.
                string sourceServiceManifestFile = this.localBuildLayoutSpecification.GetServiceManifestFile(serviceManifestType.Name);                
                ImageBuilderUtility.WriteXml<ServiceManifestType>(sourceServiceManifestFile, serviceManifestType);

                string sourceServiceManifestChecksumFile = this.localBuildLayoutSpecification.GetServiceManifestChecksumFile(serviceManifestType.Name);
                if (!FabricFile.Exists(sourceServiceManifestChecksumFile))
                {
                    ImageBuilder.TraceSource.WriteInfo(
                        TraceType,
                        "Start writing service manifest checksum file {0}",
                        this.remoteBuildLayoutSpecification.GetServiceManifestChecksumFile(serviceManifestType.Name),
                        sourceServiceManifestChecksumFile);
                    ImageBuilderUtility.WriteStringToFile(sourceServiceManifestChecksumFile, serviceManifest.Checksum);
                }

                string destinationServiceManifestFile = this.storeLayoutSpecification.GetServiceManifestFile(
                    applicationTypeContext.ApplicationManifest.ApplicationTypeName,
                    serviceManifestType.Name,
                    serviceManifestType.Version);
                string destinationServiceManifestChecksumFile = this.storeLayoutSpecification.GetServiceManifestChecksumFile(
                    applicationTypeContext.ApplicationManifest.ApplicationTypeName,
                    serviceManifestType.Name,
                    serviceManifestType.Version);

                var serviceManifestTask = this.UploadToStoreIfModifiedAsync(
                    progressHandler,
                    sourceServiceManifestFile,
                    sourceServiceManifestChecksumFile,
                    serviceManifest.Checksum,
                    destinationServiceManifestFile,
                    destinationServiceManifestChecksumFile,
                    serviceManifest.ConflictingChecksum,
                    true,
                    timeoutHelper);

                tasks.Add(serviceManifestTask);
            }

            // Each package has two uploads: the package itself and the checksum
            totalUploadCount = tasks.Count * 2;

            string sourceApplicationManifestPath = this.localBuildLayoutSpecification.GetApplicationManifestFile();
            ImageBuilderUtility.WriteXml<ApplicationManifestType>(sourceApplicationManifestPath, applicationTypeContext.ApplicationManifest);

            string destinationAppManifestFile = this.storeLayoutSpecification.GetApplicationManifestFile(
                applicationTypeContext.ApplicationManifest.ApplicationTypeName,
                applicationTypeContext.ApplicationManifest.ApplicationTypeVersion);

            var applicationManifestTask = this.imageStoreWrapper.UploadContentAsync(destinationAppManifestFile, sourceApplicationManifestPath, timeoutHelper.GetRemainingTime());

            tasks.Add(applicationManifestTask);

            await Task.WhenAll(tasks);

            ImageBuilder.TraceSource.WriteInfo(
                TraceType,
                "Completed converting buildlayout to storelayout and uploading to store.");
        }

        private async Task UploadToStoreIfModifiedAsync(
            ApplicationProvisionProgressHandler progressHandler, 
            string sourcePath,
            string sourceChecksumPath,
            string sourceChecksum,
            string destinationPath,
            string destinationChecksumPath,
            string destinationChecksum,
            bool shouldUpload,
            TimeoutHelper timeoutHelper)
        {
            if (!ImageBuilderUtility.Equals(destinationChecksum, sourceChecksum))
            {
                ImageBuilder.TraceSource.WriteInfo(
                    TraceType,
                     "{0} from {1} to {2}, checksum \"{3}\" (previous \"{4}\")",
                    shouldUpload ? "Upload" : "Copy",
                    sourcePath,
                    destinationPath,
                    sourceChecksum,
                    destinationChecksum);

                if (shouldUpload)
                {
                    await this.imageStoreWrapper.UploadContentAsync(destinationPath, sourcePath, timeoutHelper.GetRemainingTime());
                }
                else
                {
                    await this.imageStoreWrapper.CopyContentAsync(destinationPath, sourcePath, timeoutHelper.GetRemainingTime(), CopyFlag.AtomicCopy, false);
                }

                ++completedUploadCount;

                WriteUploadProgress(progressHandler, sourcePath, shouldUpload);

                ImageBuilder.TraceSource.WriteInfo(
                    TraceType, 
                    "Upload checksum file from {0} to {1}: completed {2}/{3} packages", 
                    sourceChecksumPath, 
                    destinationChecksumPath,
                    completedUploadCount,
                    totalUploadCount);

                await this.imageStoreWrapper.UploadContentAsync(destinationChecksumPath, sourceChecksumPath, timeoutHelper.GetRemainingTime()); 

                ++completedUploadCount;

                WriteUploadProgress(progressHandler, sourceChecksumPath, shouldUpload);

            }
            else
            {
                completedUploadCount += 2;

                ImageBuilder.TraceSource.WriteInfo(
                    TraceType,
                    "Skip upload for {0} due to checksum match {1}",
                    sourcePath,
                    sourceChecksum);
            }
        }        

        private string GetLocalPackagePath(
            string subPackageDirectory,
            out string subPackageArchive)
        {
            subPackageArchive = null;

            string localPackage = null;
            string archivePath = this.localBuildLayoutSpecification.GetSubPackageArchiveFile(subPackageDirectory);

            if (FabricFile.Exists(archivePath))
            {
                subPackageArchive = archivePath;
                localPackage = archivePath;
            }
            else if (FabricDirectory.Exists(subPackageDirectory))
            {
                localPackage = subPackageDirectory;
            }

            return localPackage;
        }

        private bool TryGetUploadArguments(
            string applicationTypeName,
            string serviceManifestName,
            string subPackageName,
            string subPackageVersion,
            Func<string, string, string> GetLocalSubPackageFolder,
            Func<string, string, string> GetSourceSubPackageFolder,
            Func<string, string, string, string, string> GetDestinationSubPackageFolder,
            out string sourceSubPackagePath,
            out string destinationSubPackagePath)
        {
            return TryGetUploadArguments(
                applicationTypeName,
                serviceManifestName,
                subPackageName,
                subPackageVersion,
                GetLocalSubPackageFolder,
                false, // useLocalSource
                GetSourceSubPackageFolder,
                GetDestinationSubPackageFolder,
                out sourceSubPackagePath,
                out destinationSubPackagePath);
        }

        private bool TryGetUploadArguments(
            string applicationTypeName,
            string serviceManifestName,
            string subPackageName,
            string subPackageVersion,
            Func<string, string, string> GetLocalSubPackageFolder,
            bool useLocalSource,
            Func<string, string, string> GetSourceSubPackageFolder,
            Func<string, string, string, string, string> GetDestinationSubPackageFolder,
            out string sourceSubPackagePath,
            out string destinationSubPackagePath)
        {
            sourceSubPackagePath = null;
            destinationSubPackagePath = null;

            string subPackageDirectory = GetLocalSubPackageFolder(serviceManifestName, subPackageName);

            string subPackageArchive = null;
            string localPackagePath = GetLocalPackagePath(subPackageDirectory, out subPackageArchive);

            if (string.IsNullOrEmpty(localPackagePath))
            {
                return false;
            }

            sourceSubPackagePath = GetSourcePackagePath(
                subPackageArchive,
                serviceManifestName,
                subPackageName,
                useLocalSource,
                GetSourceSubPackageFolder);

            destinationSubPackagePath = GetDestinationPackagePath(
                subPackageArchive,
                applicationTypeName,
                serviceManifestName,
                subPackageName,
                subPackageVersion,
                GetDestinationSubPackageFolder);

            return true;
        }

        private string GetSourcePackagePath(
            string subPackageArchive,
            string serviceManifestName,
            string packageName,
            bool useLocalSource,
            Func<string, string, string> GetPackageFolder)
        {
            var packageFolder = GetPackageFolder(serviceManifestName, packageName);
            if (string.IsNullOrEmpty(subPackageArchive))
            {
                return packageFolder;
            }
            else if (useLocalSource)
            {
                return this.localBuildLayoutSpecification.GetSubPackageArchiveFile(packageFolder);
            }
            else
            {
                return this.remoteBuildLayoutSpecification.GetSubPackageArchiveFile(packageFolder);
            }
        }

        private string GetDestinationPackagePath(
            string subPackageArchive,
            string applicationTypeName,
            string serviceManifestName,
            string packageName,
            string packageVersion,
            Func<string, string, string, string, string> GetPackageFolder)
        {
            var packageFolder = GetPackageFolder(
                applicationTypeName,
                serviceManifestName,
                packageName,
                packageVersion);

            if (string.IsNullOrEmpty(subPackageArchive))
            {
                return packageFolder;
            }
            else
            {
                return this.storeLayoutSpecification.GetSubPackageArchiveFile(packageFolder);
            }
        }

        private void WriteUploadProgress(ApplicationProvisionProgressHandler progressHandler, string sourcePath, bool shouldUpload)
        {
            if (progressHandler != null && this.totalUploadCount > 0)
            {
                progressHandler.WriteStatusUploading(sourcePath, this.completedUploadCount, this.totalUploadCount, shouldUpload);
            }
        }
    }
}