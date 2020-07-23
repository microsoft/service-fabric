// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Fabric.Common;
    using System.Fabric.Common.ImageModel;
    using System.Fabric.Common.Tracing;
    using System.Fabric.ImageStore;
    using System.Fabric.Management.ImageBuilder.DockerCompose;
    using System.Fabric.Management.ImageBuilder.SingleInstance;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Net;
    using System.Text;
    using System.Xml;

    internal class ImageBuilder
    {
        private static FabricEvents.ExtensionsEvents traceSource;
        private static readonly string TraceType = "ImageBuilder";
        
        private readonly string workingDirectory;
        private ImageStoreWrapper sourceImageStoreWrapper;        
        private ImageStoreWrapper destinationImageStoreWrapper;        
        private XmlReaderSettings validatingXmlReaderSettings;
        private Collection<IApplicationValidator> applicationValidators;

        static ImageBuilder()
        {
            // Try reading config
            TraceConfig.InitializeFromConfigStore();

            // Create the trace source
            ImageBuilder.traceSource = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.ImageBuilder);
        }
        
        public ImageBuilder(IImageStore imageStore)
            : this(imageStore, imageStore, StringConstants.DefaultSchemaPath, Directory.GetCurrentDirectory())
        {
        }

        public ImageBuilder(IImageStore sourceImageStore, IImageStore destinationImageStore)
            : this(sourceImageStore, destinationImageStore, StringConstants.DefaultSchemaPath, Directory.GetCurrentDirectory())
        {
        }

        public ImageBuilder(IImageStore imageStore, string schemaPath, string workingDirectory)
            : this(imageStore, imageStore, schemaPath, workingDirectory)
        {
        }

        public ImageBuilder(IImageStore sourceImageStore, IImageStore destinationImageStore, string schemaPath, string workingDirectory)
        {
            if (sourceImageStore == null)
            {
                throw new ArgumentNullException("sourceImageStore");
            } 
            
            if (destinationImageStore == null)
            {
                throw new ArgumentNullException("destinationImageStore");
            }

            if (string.IsNullOrEmpty(schemaPath))
            {
                throw new ArgumentException(StringResources.ImageBuilderError_ArgumentNullOrEmpty, "schemaPath");
            }

            if (string.IsNullOrEmpty(workingDirectory))
            {
                throw new ArgumentException(StringResources.ImageBuilderError_ArgumentNullOrEmpty, "workingDirectory");
            }

            TraceSource.WriteInfo(
                ImageBuilder.TraceType,
                "schema path = '{0}'",
                schemaPath);

            this.InitializeValidatingXmlReaderSettings(schemaPath);
            this.InitializeApplicationValidators();

            this.workingDirectory = FabricPath.GetFullPath(workingDirectory);

            this.sourceImageStoreWrapper = new ImageStoreWrapper(sourceImageStore, this.workingDirectory);
            this.destinationImageStoreWrapper = new ImageStoreWrapper(destinationImageStore, this.workingDirectory);

            TraceSource.WriteInfo(
                ImageBuilder.TraceType,
                "working directory = '{0}'",
                this.workingDirectory);
        }

        internal static FabricEvents.ExtensionsEvents TraceSource
        {
            get
            {
                return traceSource;
            }
        }

        internal bool IsSFVolumeDiskServiceEnabled
        {
            get;
            set;
        }

        /* ImageBuilder operation for Application*/
        public ApplicationTypeInformation GetApplicationTypeInfo(
            string buildPackagePath,
            TimeSpan timeout,
            string outputFile = null)
        {
            if (string.IsNullOrEmpty(buildPackagePath))
            {
                throw new ArgumentException(StringResources.ImageBuilderError_ArgumentNullOrEmpty, "buildPackagePath");
            }

            var timeoutHelper = new TimeoutHelper(timeout);

            ImageBuilderUtility.ValidateRelativePath(buildPackagePath);

            if (!this.sourceImageStoreWrapper.DoesContentExists(buildPackagePath, timeout))
            {
                throw new DirectoryNotFoundException(string.Format(
                    CultureInfo.InvariantCulture,
                    StringResources.ImageBuilderError_ApplicationPackageFolderMissing,
                    buildPackagePath));
            }

            BuildLayoutSpecification buildLayoutSpecification = BuildLayoutSpecification.Create();
            buildLayoutSpecification.SetRoot(buildPackagePath);

            string applicationManifestFile = buildLayoutSpecification.GetApplicationManifestFile();
            string applicationManifestTempFileName = ImageBuilderUtility.GetTempFileName(this.workingDirectory);

            try
            {
                TraceSource.WriteInfo(
                    ImageBuilder.TraceType,
                    "Starting download of ApplicationManifest file {0} from {1}.",
                    applicationManifestFile,
                    buildPackagePath);

                if (!this.sourceImageStoreWrapper.DownloadIfContentExists(applicationManifestFile, applicationManifestTempFileName, timeoutHelper.GetRemainingTime()))
                {
                    throw new FileNotFoundException(
                        string.Format(
                        CultureInfo.CurrentCulture,
                        StringResources.ImageBuilderError_ApplicationManifestFileMissing,
                        applicationManifestFile));
                }

                return this.GetApplicationTypeInfoInner(applicationManifestTempFileName, outputFile);
            }
            finally
            {
                TraceSource.WriteInfo(
                    ImageBuilder.TraceType,
                    "Deleting the temporary ApplicationManifest file created locally at {0}",
                    applicationManifestTempFileName);

                ImageBuilderUtility.DeleteTempLocation(applicationManifestTempFileName);
            }
        }

        private ApplicationTypeInformation GetApplicationTypeInfoInner(
            string applicationManifestFile,
            string outputFile)
        {
            TraceSource.WriteInfo(
                ImageBuilder.TraceType,
                "Validating ApplicationManifest schema at {0}.",
                applicationManifestFile);

            ApplicationManifestType applicationManifestType = ImageBuilderUtility.ReadXml<ApplicationManifestType>(applicationManifestFile, this.validatingXmlReaderSettings);

            if (outputFile != null)
            {
                TraceSource.WriteInfo(
                    ImageBuilder.TraceType,
                    "Writing ApplicationManifestTypeInfo with Name:{0} Version:{1} to file {2}.",
                    applicationManifestType.ApplicationTypeName,
                    applicationManifestType.ApplicationTypeVersion,
                    outputFile);

                string appTypeInfo = string.Format(
                    CultureInfo.InvariantCulture,
                    "{0}:{1}", applicationManifestType.ApplicationTypeName,
                    applicationManifestType.ApplicationTypeVersion);

                ImageBuilderUtility.WriteStringToFile(outputFile, appTypeInfo, false, Encoding.Unicode);
            }

            return new ApplicationTypeInformation(applicationManifestType.ApplicationTypeName, applicationManifestType.ApplicationTypeVersion);
        }

        public void ValidateComposeDeployment(
            string composeFileName,
            string overridesFileName,
            string applicationName,
            string applicationTypeName,
            string applicationTypeVersion,
            TimeSpan timeout,
            out HashSet<string> ignoredKeys,
            bool cleanupFiles = false)
        {
            this.InnerBuildComposeDeploymentPackage(
                composeFileName,
                overridesFileName,
                timeout,
                applicationName,
                applicationTypeName,
                applicationTypeVersion,
                null,
                null,
                false,
                true,
                out ignoredKeys,
                null,
                null,
                true,  // validate
                cleanupFiles); // cleanup
        }

        public void BuildComposeDeploymentPackage(
            string composeFileName,
            string overridesFileName,
            TimeSpan timeout,
            string applicationName,
            string applicationTypeName,
            string applicationTypeVersion,
            string repositoryUseName,
            string repositoryPassword,
            bool isRepositoryPasswordEncrypted,
            bool generateDnsName,
            string outputComposeFileName,
            string localApplicationPackageFolder,
            bool cleanupComposeFiles,
            bool shouldSkipChecksumValidation = false)
        {
            HashSet<string> ignoredKeys;

            this.InnerBuildComposeDeploymentPackage(
                composeFileName,
                overridesFileName,
                timeout,
                applicationName,
                applicationTypeName,
                applicationTypeVersion,
                repositoryUseName,
                repositoryPassword,
                isRepositoryPasswordEncrypted,
                generateDnsName,
                out ignoredKeys,
                outputComposeFileName,
                localApplicationPackageFolder,
                false,
                cleanupComposeFiles,
                shouldSkipChecksumValidation);
        }

        public void BuildSingleInstanceApplication(
            SingleInstance.Application application,
            string applicationTypeName,
            string applicationTypeVersion,
            string applicationId,
            Uri nameUri,
            bool generateDnsNames,
            TimeSpan timeout,
            string localApplicationPackageFolder,
            string applicationOutputFolder,
            bool useOpenNetworkConfig,
            bool useLocalNatNetworkConfig,
            string mountPointForSettings,
            GenerationConfig generationConfig)
        {
            this.InnerBuildSingleInstanceApplication(
                application,
                applicationTypeName,
                applicationTypeVersion,
                applicationId,
                nameUri,
                generateDnsNames,
                timeout,
                localApplicationPackageFolder,
                applicationOutputFolder,
                useOpenNetworkConfig,
                useLocalNatNetworkConfig,
                mountPointForSettings,
                generationConfig,
                false);
        }

        private void InnerBuildSingleInstanceApplication(
            SingleInstance.Application application,
            string applicationTypeName,
            string applicationTypeVersion,
            string applicationId,
            Uri nameUri,
            bool generateDnsNames,
            TimeSpan timeout,
            string localApplicationPackageFolder,
            string applicationOutputFolder,
            bool useOpenNetworkConfig,
            bool useLocalNatNetworkConfig,
            string mountPointForSettings,
            GenerationConfig generationConfig,
            bool validateOnly = false)
        {
            TimeoutHelper timeoutHelper = new TimeoutHelper(timeout);

            if (application == null)
            {
                throw new ArgumentException(StringResources.ImageBuilderError_ArgumentNullOrEmpty, "application");
            }

            if (string.IsNullOrEmpty(applicationTypeName))
            {
                throw new ArgumentException(StringResources.ImageBuilderError_ArgumentNullOrEmpty, "applicationTypeName");
            }

            if (string.IsNullOrEmpty(applicationTypeVersion))
            {
                throw new ArgumentException(StringResources.ImageBuilderError_ArgumentNullOrEmpty, "applicationTypeVersion");
            }

            if (string.IsNullOrEmpty(applicationId))
            {
                throw new ArgumentException(StringResources.ImageBuilderError_ArgumentNullOrEmpty, "applicationId");
            }

            if (!validateOnly && string.IsNullOrEmpty(localApplicationPackageFolder))
            {
                throw new ArgumentException(StringResources.ImageBuilderError_ArgumentNullOrEmpty, "LocalApplicationPackageFolder");
            }

            if (application.services.Exists(s => s.codePackages.Exists(c => c.settings.Count > 0)) && string.IsNullOrEmpty(mountPointForSettings))
            {
                throw new ArgumentException(StringResources.ImageBuilderError_ArgumentNullOrEmpty, "mountPointForSettings");
            }

            generationConfig = PopulateTargetReplicaCount(generationConfig);

            ApplicationManifestType applicationManifest;
            IList<ServiceManifestType> serviceManifests;
            var generator = new SingleInstance.ApplicationPackageGenerator(
                applicationTypeName,
                applicationTypeVersion,
                application,
                useOpenNetworkConfig,
                useLocalNatNetworkConfig,
                mountPointForSettings,
                generateDnsNames,
                generationConfig);

            // Key is serviceManifest name
            Dictionary<string, Dictionary<string, SettingsType>> settingsType;
            generator.Generate(out applicationManifest, out serviceManifests, out settingsType);
            DockerComposeUtils.GenerateApplicationPackage(applicationManifest, serviceManifests, settingsType, localApplicationPackageFolder);
            ApplicationProvisionOperation appProvisionOperation = new ApplicationProvisionOperation(
                localApplicationPackageFolder, // There is no remote build path, the build spec for local and remote are same.
                localApplicationPackageFolder,
                this.validatingXmlReaderSettings,
                this.destinationImageStoreWrapper,
                this.applicationValidators,
                true); // we are generating the files. 

            appProvisionOperation.ProvisionApplicationAsync(
                false,
                true, // Server side copy is always disabled since we are generating the app package.
                null,
                timeoutHelper,
                true,
                IsSFVolumeDiskServiceEnabled).GetAwaiter().GetResult();

            ApplicationCreateInstanceOperation createInstanceOperation = new ApplicationCreateInstanceOperation(
                applicationTypeName,
                applicationTypeVersion,
                applicationId,
                nameUri,
                null, // user parameters
                this.destinationImageStoreWrapper,
                this.applicationValidators);
            createInstanceOperation.CreateInstanceAsync(applicationOutputFolder, timeoutHelper.GetRemainingTime(), null).Wait();
        }

        /// <summary>
        /// Builds the ServiceFabric application package for the container application described by the docker compose files.
        /// </summary>
        /// <param name="composeFileName">Docker compose file path</param>
        /// <param name="overridesFileName">Overrides file path</param>
        /// <param name="timeout">Timeout for the operation</param>
        /// <param name="repositoryUseName">When the containers in the compose file are in a private repository, this specifies the User name to connect to the repository.</param>
        /// <param name="repositoryPassword">When the containers in the compose file are in a private repository, this specifies the password to connect to the repository.</param>
        /// <param name="isRepositoryPasswordEncrypted">Specifies if the password given above is encrypted or not</param>
        /// <param name="applicationTypeName">Application type name to use for the generated package.</param>
        /// <param name="applicationTypeVersion">Application type version to use for the generated package.</param>
        /// <param name="outputComposeFileName">Specifies the target file where the merged contents of the compose file + overrides are written to.</param>
        /// <param name="localApplicationPackageFolder">Folder where the generated package will be written to.</param>
        /// <param name="validateOnly">Validates that the docker compose file can be converted to a service fabric application package</param>
        /// <param name="cleanupComposeFiles">Cleans up the docker compose files after performing the validation/generation operation.</param>
        /// <param name="shouldSkipChecksumValidation"></param>
        private void InnerBuildComposeDeploymentPackage(
            string composeFileName,
            string overridesFileName,
            TimeSpan timeout,
            string applicationName,
            string applicationTypeName,
            string applicationTypeVersion,
            string repositoryUseName,
            string repositoryPassword,
            bool isRepositoryPasswordEncrypted,
            bool generateDnsName,
            out HashSet<string> ignoredKeys,
            string outputComposeFileName = null,
            string localApplicationPackageFolder = null,
            bool validateOnly = false,
            bool cleanupComposeFiles = false,
            bool shouldSkipChecksumValidation = false)
        {
            TimeoutHelper timeoutHelper = new TimeoutHelper(timeout);

            if (!validateOnly && string.IsNullOrEmpty(localApplicationPackageFolder))
            {
                throw new ArgumentException(StringResources.ImageBuilderError_ArgumentNullOrEmpty, "LocalApplicationPackageFolder");
            }

            try
            {
                var composeAppTypeDescription = this.GetComposeApplicationTypeDescription(
                    applicationTypeName,
                    applicationTypeVersion,
                    composeFileName,
                    overridesFileName,
                    out ignoredKeys);

                ApplicationManifestType applicationManifest;
                IList<ServiceManifestType> serviceManifests;
                var containerApplicationPkgGenerator = new ContainerPackageGenerator(
                    composeAppTypeDescription,
                    new ContainerRepositoryCredentials()
                    {
                        RepositoryUserName = repositoryUseName,
                        RepositoryPassword = repositoryPassword,
                        IsPasswordEncrypted = isRepositoryPasswordEncrypted
                    },
                    applicationName,
                    generateDnsName);

                containerApplicationPkgGenerator.Generate(out applicationManifest, out serviceManifests);

                if (validateOnly)
                {
                    //
                    // Using a dummy path so that build layout construction succeeds. 
                    // Nothing is written to disk during validation.
                    //
                    var appTypeContext = new ApplicationTypeContext(
                        applicationManifest, 
                        Path.Combine(ImageBuilderUtility.GetTempFileName(this.workingDirectory)));

                    foreach (var manifestType in serviceManifests)
                    {
                        appTypeContext.ServiceManifests.Add(new ServiceManifest(manifestType));
                    }

                    foreach (var validator in this.applicationValidators)
                    {
                        validator.Validate(appTypeContext, true, IsSFVolumeDiskServiceEnabled);
                    }
                    
                    // return if validation is successful.
                    if (!string.IsNullOrEmpty(outputComposeFileName))
                    {
                        // TODO: This should return the merged file once we support it.
                        TraceSource.WriteInfo(
                            ImageBuilder.TraceType,
                            "Writing the compose file result to {0}",
                            outputComposeFileName);
                        ImageBuilderUtility.CopyFile(composeFileName, outputComposeFileName);
                    }
                    return;
                }
                else
                {
                    if (!string.IsNullOrEmpty(outputComposeFileName))
                    {
                        // TODO: This should return the merged file once we support it.
                        TraceSource.WriteInfo(
                            ImageBuilder.TraceType,
                            "Writing the compose file result to {0}",
                            outputComposeFileName);
                        ImageBuilderUtility.CopyFile(composeFileName, outputComposeFileName);
                    }
                }

                DockerComposeUtils.GenerateApplicationPackage(
                    applicationManifest, 
                    serviceManifests, 
                    localApplicationPackageFolder);

                ApplicationProvisionOperation appProvisionOperation = new ApplicationProvisionOperation(
                    localApplicationPackageFolder, // There is no remote build path, the build spec for local and remote are same.
                    localApplicationPackageFolder,
                    this.validatingXmlReaderSettings,
                    this.destinationImageStoreWrapper,
                    this.applicationValidators,
                    shouldSkipChecksumValidation);
                appProvisionOperation.ProvisionApplicationAsync(
                    false,
                    true, // Server side copy is always disabled since we are generating the app package.
                    null,
                    timeoutHelper,
                    true,
                    IsSFVolumeDiskServiceEnabled).GetAwaiter().GetResult();
            }
            finally
            {
                if (cleanupComposeFiles)
                {
                    TraceSource.WriteInfo(
                        ImageBuilder.TraceType,
                        "Cleaning up the compose file {0}, overrides file {1}",
                        composeFileName,
                        overridesFileName);

                    ImageBuilderUtility.DeleteTempLocation(composeFileName);
                    ImageBuilderUtility.DeleteTempLocation(overridesFileName);
                }
            }
        }

        public void BuildComposeDeploymentPackageForUpgrade(
            string composeFileName,
            string overridesFileName,
            TimeSpan timeout,
            string applicationName,
            string applicationTypeName,
            string currentTypeVersion,
            string targetTypeVersion,
            string repositoryUseName,
            string repositoryPassword,
            bool isRepositoryPasswordEncrypted,
            bool generateDnsName,
            string outputComposeFileName = null,
            string localApplicationPackageFolder = null,
            bool cleanupComposeFiles = false,
            bool shouldSkipChecksumValidation = false)
        {
            TimeoutHelper timeoutHelper = new TimeoutHelper(timeout);
            HashSet<string> ignoredKeys;
            try
            {
                var composeAppTypeDescription = this.GetComposeApplicationTypeDescription(
                    applicationTypeName,
                    targetTypeVersion,
                    composeFileName,
                    overridesFileName,
                    out ignoredKeys);

                ApplicationManifestType applicationManifest;
                IList<ServiceManifestType> serviceManifests;
                var containerApplicationPkgGenerator = new ContainerPackageGenerator(
                    composeAppTypeDescription,
                    new ContainerRepositoryCredentials()
                    {
                        RepositoryUserName = repositoryUseName,
                        RepositoryPassword = repositoryPassword,
                        IsPasswordEncrypted = isRepositoryPasswordEncrypted
                    },
                    applicationName,
                    generateDnsName);

                containerApplicationPkgGenerator.Generate(out applicationManifest, out serviceManifests);

                if (!string.IsNullOrEmpty(outputComposeFileName))
                {
                    // TODO: This should return the merged file once we support it.
                    TraceSource.WriteInfo(
                        ImageBuilder.TraceType,
                        "Writing the compose file result to {0}",
                        outputComposeFileName);
                    ImageBuilderUtility.CopyFile(composeFileName, outputComposeFileName);
                }

                //
                // If the compose upgrade is removing services, then the corresponding service type from the previous needs to be kept around
                // so that the app is healthy through the upgrade. In the next upgrade, the service type will automatically be discarded.
                //

                AdjustSingleInstanceManifestsForUpgrade(
                    applicationTypeName,
                    currentTypeVersion,
                    timeoutHelper,
                    null, //settingsType
                    ref applicationManifest,
                    ref serviceManifests,
                    true);

                DockerComposeUtils.GenerateApplicationPackage(
                    applicationManifest, 
                    serviceManifests, 
                    localApplicationPackageFolder);

                ApplicationProvisionOperation appProvisionOperation = new ApplicationProvisionOperation(
                    localApplicationPackageFolder, // There is no remote build path, the build spec for local and remote are same.
                    localApplicationPackageFolder,
                    this.validatingXmlReaderSettings,
                    this.destinationImageStoreWrapper,
                    this.applicationValidators,
                    shouldSkipChecksumValidation);

                appProvisionOperation.ProvisionApplicationAsync(
                    false,
                    true, // Server side copy is always disabled since we are generating the app package.
                    null,
                    timeoutHelper,
                    true,
                    IsSFVolumeDiskServiceEnabled).GetAwaiter().GetResult();

            }
            finally
            {
                if (cleanupComposeFiles)
                {
                    TraceSource.WriteInfo(
                        ImageBuilder.TraceType,
                        "Cleaning up the compose file {0}, overrides file {1}",
                        composeFileName,
                        overridesFileName);

                    ImageBuilderUtility.DeleteTempLocation(composeFileName);
                    ImageBuilderUtility.DeleteTempLocation(overridesFileName);
                }
            }
         }

         public void DownloadAndBuildApplicationType(
            string downloadPath,
            string applicationTypeName,
            string applicationTypeVersion,
            string localIncomingApplicationFolder,
            TimeSpan timeout,
            string progressFile = null,
            bool shouldSkipChecksumValidation = false)
        {
            var timeoutHelper = new TimeoutHelper(timeout);

            if (string.IsNullOrEmpty(downloadPath))
            {
                throw new ArgumentException(StringResources.ImageBuilderError_ArgumentNullOrEmpty, "downloadPath");
            }

            if (string.IsNullOrEmpty(applicationTypeName))
            {
                throw new ArgumentException(StringResources.ImageBuilderError_ArgumentNullOrEmpty, "applicationTypeName");
            }

            if (string.IsNullOrEmpty(applicationTypeVersion))
            {
                throw new ArgumentException(StringResources.ImageBuilderError_ArgumentNullOrEmpty, "applicationTypeVersion");
            }

            if (string.IsNullOrEmpty(localIncomingApplicationFolder))
            {
                throw new ArgumentException(StringResources.ImageBuilderError_ArgumentNullOrEmpty, "localIncomingApplicationFolder");
            }

            string fileName = ImageBuilderUtility.ValidateSfpkgDownloadPathAndGetFileName(downloadPath);

            ReleaseAssert.AssertIf(
                ImageBuilderUtility.NotEquals(this.sourceImageStoreWrapper.ImageStore.RootUri, this.destinationImageStoreWrapper.ImageStore.RootUri),
                "The ImageStoreConnectionString RootUri should be same when not ValidateOnly");
            
            if (!string.IsNullOrEmpty(progressFile))
            {
                progressFile = Path.Combine(localIncomingApplicationFolder, progressFile);
            }

            var progressHandler = string.IsNullOrEmpty(progressFile) ? null : new ApplicationProvisionProgressHandler(downloadPath, progressFile);

            if (progressHandler != null)
            {
                progressHandler.WriteStatusDownloading(downloadPath);
            }

            // Download the sfpkg to a temporary folder, which will be deleted at the end of the test.
            var tempFolder = ImageBuilderUtility.GetTempFileName(this.workingDirectory);
            var localSfpkgPath = Path.Combine(tempFolder, fileName);
            
            TraceSource.WriteInfo(
                ImageBuilder.TraceType,
                "Downloading {0} from {1} to {2}.",
                fileName,
                downloadPath,
                localSfpkgPath);

            try
            {
                // The HttpClient expects the folder to exist, so create it.
                ImageBuilderUtility.CreateFolderIfNotExists(tempFolder);

                using (var client = new WebClient())
                {
                    // DownloadProgressChangedEventHandler updates the status
                    // when underlying download requests are completing.
                    // The downloaded size differs based on many factors, seems to be between 828 bytes and 65536 bytes.
                    client.DownloadProgressChanged += (s, e) => 
                        {
                            if (progressHandler != null)
                            {
                                progressHandler.WriteStatusDownloadingExternalPackageProgress(
                                    downloadPath,
                                    e.BytesReceived,
                                    e.TotalBytesToReceive,
                                    e.ProgressPercentage);
                            }
                        };

                    var t = client.DownloadFileTaskAsync(downloadPath, localSfpkgPath);

                    var remainingTime = timeoutHelper.GetRemainingTime();
                    if (remainingTime == TimeSpan.MaxValue)
                    {
                        t.Wait();
                    }
                    else
                    {
                        // Calling t.Wait with TimeSpan.MaxValue throws ArgumentOutOfRangeException
                        var success = t.Wait(remainingTime);
                        if (!success)
                        {
                            // Cancel the client operation, no need to continue the download since timeout is exhausted.
                            client.CancelAsync();
                            throw new TimeoutException(
                                string.Format(
                                    CultureInfo.CurrentCulture,
                                    StringResources.ImageBuilder_DownloadSfpkg_TimedOut,
                                    downloadPath));
                        }
                    }
                }
                               
                // In rare cases, the sfpkg downloaded doesn't exist, even if the client download didn't throw an error.
                if (!File.Exists(localSfpkgPath))
                {
                    throw new FileNotFoundException(
                        string.Format(
                            CultureInfo.CurrentCulture,
                            StringResources.Error_SfpkgFileNotFound,
                            downloadPath,
                            localSfpkgPath));
                }

                // Download completed successfully.
                ImageBuilderUtility.RemoveReadOnlyFlag(localSfpkgPath);

                if (progressHandler != null)
                {
                    progressHandler.WriteStatusExtractingAndValidatingAppType(downloadPath);
                }

                // To check the application manifest version information against the specified app type information,
                // extract it to a temporary file. The temporary file is deleted when the temp directory is deleted.
                string applicationManifestTempFileName = ImageBuilderUtility.GetTempFileName(tempFolder);

                TraceSource.WriteInfo(
                    ImageBuilder.TraceType,
                    "Downloaded {0} from {1} to {2}. Extract application manifest information for validation to {3}",
                    fileName,
                    downloadPath,
                    localSfpkgPath,
                    applicationManifestTempFileName);
                
                var applicationManifestFile = "ApplicationManifest.xml";
                ImageBuilderUtility.ExtractFromArchive(localSfpkgPath, applicationManifestFile, applicationManifestTempFileName);
                timeoutHelper.ThrowIfExpired();

                if (!File.Exists(applicationManifestTempFileName))
                {
                    // The application manifest was not found in the archive
                    throw new FileNotFoundException(
                        string.Format(
                        CultureInfo.CurrentCulture,
                        StringResources.ImageBuilderError_ApplicationManifestFileMissing,
                        applicationManifestFile));
                }

                var typeInfo = this.GetApplicationTypeInfoInner(applicationManifestTempFileName, null);
                if (typeInfo.Name != applicationTypeName)
                {
                    ImageBuilderUtility.TraceAndThrowValidationError(
                        TraceType,
                        StringResources.ImageBuilderError_SfpkgApplicationTypeNameMismatch,
                        downloadPath,
                        typeInfo.Name,
                        applicationTypeName);
                }

                if (typeInfo.Version != applicationTypeVersion)
                {
                    ImageBuilderUtility.TraceAndThrowValidationError(
                       TraceType,
                       StringResources.ImageBuilderError_SfpkgApplicationTypeVersionMismatch,
                       downloadPath,
                       typeInfo.Version,
                       applicationTypeVersion);
                }

                // Passed validation.
                // Extract the whole package to the output folder and provision the application type.
                if (progressHandler != null)
                {
                    progressHandler.WriteStatusExtractingApplicationPackage(fileName);
                }

                TraceSource.WriteInfo(
                    ImageBuilder.TraceType,
                    "Extract .sfpkg file {0} to {1}.",
                    localSfpkgPath,
                    localIncomingApplicationFolder);

                // TODO: pass time to extract method.
                ImageBuilderUtility.ExtractArchive(localSfpkgPath, localIncomingApplicationFolder);
                timeoutHelper.ThrowIfExpired();

                TraceSource.WriteInfo(
                   ImageBuilder.TraceType,
                   "Finished extracting .sfpkg, start provision from {0}.",
                   localIncomingApplicationFolder);

                var appProvisionOperation = new ApplicationProvisionOperation(
                    localIncomingApplicationFolder, // no remote store, use the local folder
                    localIncomingApplicationFolder,
                    this.validatingXmlReaderSettings,
                    this.destinationImageStoreWrapper,
                    this.applicationValidators,
                    shouldSkipChecksumValidation);

                // Since the source is not in the image store, always disable server side copy.
                var context = appProvisionOperation.ProvisionApplicationAsync(
                    false /*validateOnly*/,
                    true /*disableServerSideCopy*/,
                    progressHandler,
                    timeoutHelper,
                    isSFVolumeDiskServiceEnabled: IsSFVolumeDiskServiceEnabled).Result;

                TraceSource.WriteInfo(
                   ImageBuilder.TraceType,
                   "Provision from '{0}' completed. App type info: {1}+{2}",
                   localIncomingApplicationFolder,
                   context.ApplicationManifest.ApplicationTypeName,
                   context.ApplicationManifest.ApplicationTypeVersion);
            }
            finally
            {
                // If the download from the external store times out, the client is cancelled.
                // Sometimes, the abort takes some time and the folder may still be in use at the time of the delete.
                // Add retries to make it more reliable.
                ImageBuilderUtility.DeleteTempLocationWithRetry(tempFolder);
            }
        }

        public void DeleteApplicationPackage(
            string applicationPackagePathInRemoteStore,
            TimeSpan timeout)
        {
            if (string.IsNullOrEmpty(applicationPackagePathInRemoteStore))
            {
                throw new ArgumentException(StringResources.ImageBuilderError_ArgumentNullOrEmpty, "applicationPackagePathInRemoteStore");
            }

            this.destinationImageStoreWrapper.DeleteContent(applicationPackagePathInRemoteStore, timeout);
        }

        public void BuildApplicationType(
            string buildPackagePath,
            TimeSpan timeout,
            string localIncomingApplicationFolder = null,
            string progressFile = null,
            bool shouldSkipChecksumValidation = false,
            bool disableServerSideCopy = false)
        {
            this.InnerBuildApplicationType(buildPackagePath, timeout, localIncomingApplicationFolder, progressFile, false, shouldSkipChecksumValidation, disableServerSideCopy);
        }

        private ApplicationTypeContext InnerBuildApplicationType(
            string buildPackagePath,
            TimeSpan timeout,
            string localIncomingApplicationFolder,
            string progressFile,
            bool validateOnly,
            bool shouldSkipChecksumValidation,
            bool disableServerSideCopy)
        {
            TimeoutHelper timeoutHelper = new TimeoutHelper(timeout);

            if (string.IsNullOrEmpty(buildPackagePath))
            {
                throw new ArgumentException(StringResources.ImageBuilderError_ArgumentNullOrEmpty, "buildPackagePath");
            }

            ImageBuilderUtility.ValidateRelativePath(buildPackagePath);

            bool deleteApplicationFolder = false;
            if (string.IsNullOrEmpty(localIncomingApplicationFolder))
            {
                localIncomingApplicationFolder = Path.Combine(
                    ImageBuilderUtility.GetTempFileName(this.workingDirectory), 
                    Path.GetFileName(buildPackagePath));
                deleteApplicationFolder = true;
            }

            TraceSource.WriteInfo(
                ImageBuilder.TraceType,
                "Starting download of Application {0} from store.",
                buildPackagePath);

            try
            {
                ReleaseAssert.AssertIf(
                    ImageBuilderUtility.NotEquals(this.sourceImageStoreWrapper.ImageStore.RootUri, this.destinationImageStoreWrapper.ImageStore.RootUri) && !validateOnly,
                    "The ImageStoreConnectionString RootUri should be same when not ValidateOnly");

                if (!string.IsNullOrEmpty(progressFile))
                {
                    progressFile = Path.Combine(localIncomingApplicationFolder, progressFile);
                }

                var progressHandler = string.IsNullOrEmpty(progressFile) ? null : new ApplicationProvisionProgressHandler(buildPackagePath, progressFile);

                if (progressHandler != null)
                {
                    progressHandler.WriteStatusDownloading(buildPackagePath);
                }

                if (!this.sourceImageStoreWrapper.DownloadIfContentExists(
                    buildPackagePath,
                    localIncomingApplicationFolder,
                    progressHandler,
                    timeoutHelper.GetRemainingTime()))
                {
                    throw new DirectoryNotFoundException(
                        string.Format(
                            CultureInfo.CurrentCulture,
                            StringResources.ImageBuilderError_ApplicationPackageFolderMissing,
                            buildPackagePath));
                }
                
                // It is possible that content exists when checked initially but unable to download or package is deleted in the meantime, so make sure downloaded content exists.
                if (!(Directory.Exists(localIncomingApplicationFolder) && Directory.GetFiles(localIncomingApplicationFolder).Length > 0))
                {
                    if (this.sourceImageStoreWrapper.IsXStoreBasedImageStore())
                    {
                        throw new DirectoryNotFoundException(
                            string.Format(
                                CultureInfo.CurrentCulture,
                                StringResources.ImageBuilderError_UnableToDownloadApplicationPackageFromXStore,
                                buildPackagePath));
                    }
                    else
                    {
                        throw new DirectoryNotFoundException(
                            string.Format(
                                CultureInfo.CurrentCulture,
                                StringResources.ImageBuilderError_UnableToDownloadApplicationPackage,
                                buildPackagePath));
                    }
                }

                TraceSource.WriteInfo(
                    ImageBuilder.TraceType,
                    "Completed download of Application {0} from store.",
                    buildPackagePath);

                timeoutHelper.ThrowIfExpired();
                var appProvisionOperation = new ApplicationProvisionOperation(
                    buildPackagePath,
                    localIncomingApplicationFolder,
                    this.validatingXmlReaderSettings,
                    this.destinationImageStoreWrapper,
                    this.applicationValidators,
                    shouldSkipChecksumValidation);

                return appProvisionOperation.ProvisionApplicationAsync(
                    validateOnly,
                    disableServerSideCopy,
                    progressHandler,
                    timeoutHelper,
                    isSFVolumeDiskServiceEnabled: IsSFVolumeDiskServiceEnabled).Result;
            }
            finally
            {
                if (deleteApplicationFolder)
                {
                    TraceSource.WriteInfo(
                        ImageBuilder.TraceType,
                        "Deleting the temporary Directory created locally at {0}",
                        localIncomingApplicationFolder);

                    ImageBuilderUtility.DeleteTempLocation(localIncomingApplicationFolder);
                }
            }
        }

        public void BuildSingleInstanceApplicationForUpgrade(
            Application application,
            string applicationTypeName,
            string currentTypeVersion,
            string targetTypeVersion,
            string applicationId,
            int currentApplicationInstanceVersion,
            Uri nameUri,
            bool generateDnsNames,
            TimeSpan timeout,
            string localApplicationPackageFolder,
            string applicationOutputFolder,
            bool useOpenNetworkConfig,
            bool useLocalNatNetworkConfig,
            string mountPointForSettings,
            GenerationConfig generationConfig)
        {
            TimeoutHelper timeoutHelper = new TimeoutHelper(timeout);
            try
            {
                ApplicationManifestType applicationManifest;
                IList<ServiceManifestType> serviceManifests;
                var generator = new SingleInstance.ApplicationPackageGenerator(
                    applicationTypeName,
                    targetTypeVersion,
                    application,
                    useOpenNetworkConfig,
                    useLocalNatNetworkConfig,
                    mountPointForSettings,
                    generateDnsNames,
                    generationConfig);

                // <serviceManifestName, <ConfigPackageName, settingsType>>
                Dictionary<string, Dictionary<string, SettingsType>> settingsType;
                generator.Generate(out applicationManifest, out serviceManifests, out settingsType);

                AdjustSingleInstanceManifestsForUpgrade(applicationTypeName, currentTypeVersion, timeoutHelper, settingsType, ref applicationManifest, ref serviceManifests);

                DockerComposeUtils.GenerateApplicationPackage(
                    applicationManifest,
                    serviceManifests,
                    settingsType,
                    localApplicationPackageFolder);

                ApplicationProvisionOperation appProvisionOperation = new ApplicationProvisionOperation(
                    localApplicationPackageFolder, // There is no remote build path, the build spec for local and remote are same.
                    localApplicationPackageFolder,
                    this.validatingXmlReaderSettings,
                    this.destinationImageStoreWrapper,
                    this.applicationValidators,
                    true); // we are generating the files. 

                appProvisionOperation.ProvisionApplicationAsync(
                    false,
                    true, // Server side copy is always disabled since we are generating the app package.
                    null,
                    timeoutHelper,
                    true,
                    IsSFVolumeDiskServiceEnabled).GetAwaiter().GetResult();

                ApplicationUpgradeInstanceOperation upgradeInstanceOperation = new ApplicationUpgradeInstanceOperation(
                    currentApplicationInstanceVersion,
                    applicationTypeName,
                    targetTypeVersion,
                    applicationId,
                    null, // user parameters
                    this.destinationImageStoreWrapper,
                    this.applicationValidators);
                upgradeInstanceOperation.UpgradeInstaceAsync(applicationOutputFolder, timeout).Wait();
            }
            finally
            {
            }
        }

        public void BuildApplication(
            string applicationTypeName,
            string applicationTypeVersion,
            string applicationId,
            Uri nameUri,
            IDictionary<string, string> userParameters,
            TimeSpan timeout,
            string outputFolder = null)
        {
            this.InnerBuildApplication(
                applicationTypeName, 
                applicationTypeVersion, 
                applicationId, 
                nameUri, 
                userParameters, 
                timeout, 
                outputFolder,
                null);
        }

        private void InnerBuildApplication(
            string applicationTypeName,
            string applicationTypeVersion,
            string applicationId,
            Uri nameUri,
            IDictionary<string, string> userParameters,
            TimeSpan timeout,
            string outputFolder = null,
            ApplicationTypeContext validatedApplicationTypeContext = null)
        {
            if (string.IsNullOrEmpty(applicationTypeName))
            {
                throw new ArgumentException(StringResources.ImageBuilderError_ArgumentNullOrEmpty, "applicationTypeName");
            }

            if (string.IsNullOrEmpty(applicationTypeVersion))
            {
                throw new ArgumentException(StringResources.ImageBuilderError_ArgumentNullOrEmpty, "applicationTypeVersion");
            }

            if (string.IsNullOrEmpty(applicationId))
            {
                throw new ArgumentException(StringResources.ImageBuilderError_ArgumentNullOrEmpty, "applicationId");
            }

            if (nameUri == null)
            {
                throw new ArgumentNullException("nameUri");
            }

            if (!nameUri.IsAbsoluteUri)
            {
                throw new ArgumentException(StringResources.ImageBuilderError_InvalidNameUri, "nameUri");
            }

            ApplicationCreateInstanceOperation createInstanceOperation = new ApplicationCreateInstanceOperation(
                applicationTypeName,
                applicationTypeVersion,
                applicationId,
                nameUri,
                userParameters,
                this.destinationImageStoreWrapper,
                this.applicationValidators);
            createInstanceOperation.CreateInstanceAsync(outputFolder, timeout, validatedApplicationTypeContext).Wait();            
        }

        public void UpgradeApplication(
            string applicationId,
            string applicationTypeName,
            int currentApplicationInstanceVersion,
            string targetAppTypeVersion,
            IDictionary<string, string> userParameters,
            TimeSpan timeout,
            string outputFolder = null)
        {
            if (string.IsNullOrEmpty(applicationId))
            {
                throw new ArgumentException(StringResources.ImageBuilderError_ArgumentNullOrEmpty, "applicationId");
            }

            if (string.IsNullOrEmpty(applicationTypeName))
            {
                throw new ArgumentException(StringResources.ImageBuilderError_ArgumentNullOrEmpty, "applicationTypeName");
            }

            if (string.IsNullOrEmpty(targetAppTypeVersion))
            {
                throw new ArgumentException(StringResources.ImageBuilderError_ArgumentNullOrEmpty, "targetAppTypeVersion");
            }

            ApplicationUpgradeInstanceOperation upgradeInstanceOperation = new ApplicationUpgradeInstanceOperation(
                currentApplicationInstanceVersion,
                applicationTypeName,
                targetAppTypeVersion,
                applicationId,
                userParameters,
                this.destinationImageStoreWrapper,
                this.applicationValidators);
            upgradeInstanceOperation.UpgradeInstaceAsync(outputFolder, timeout).Wait();
        }

        public void ValidateApplicationPackage(
            string buildPackagePath,
            IDictionary<string, string> userParameters)
        {
            ApplicationTypeContext validatedApplicationTypeContext = this.InnerBuildApplicationType(
                buildPackagePath,
                ImageBuilderUtility.ImageStoreDefaultTimeout,
                null, // output
                null, // progress
                true,   // validateOnly
                false,  // shouldSkipChecksumValidation
                false); // disableServerSideCopy

            if (userParameters != null)
            {
                this.InnerBuildApplication(
                    validatedApplicationTypeContext.ApplicationManifest.ApplicationTypeName,
                    validatedApplicationTypeContext.ApplicationManifest.ApplicationTypeVersion,
                    "mock_app_for_validation_App0",
                    new Uri("fabric:/mock_app_for_validation"),
                    userParameters,
                    ImageBuilderUtility.ImageStoreDefaultTimeout,
                    string.Empty,
                    validatedApplicationTypeContext);
            }
        }

        public void Delete(
            string inputFileName,
            TimeSpan timeout)
        {
            if (string.IsNullOrEmpty(inputFileName))
            {
                throw new ArgumentException(StringResources.ImageBuilderError_ArgumentNullOrEmpty, "inputFileName");
            }
            
            using (StreamReader reader = new StreamReader(File.Open(inputFileName, FileMode.Open)))
            {
                string resourceToDelete;
                while ((resourceToDelete = reader.ReadLine()) != null)
                {
                    ImageBuilder.TraceSource.WriteNoise(
                        TraceType,
                        string.Format(
                            CultureInfo.InvariantCulture,
                            "Attempting to delete: {0}.",
                            resourceToDelete));
                    this.destinationImageStoreWrapper.DeleteContent(resourceToDelete, timeout);
                }                
            }
        }

        /* ImageBuilder operation for Fabric*/
        public FabricVersion GetFabricVersionInfo(
            string codePath,
            string configPath,     
            TimeSpan timeout,
            string outputFile = null)
        {
            TimeoutHelper timeoutHelper = new TimeoutHelper(timeout);
            string codeVersion = string.Empty, configVersion = string.Empty;

            if (!string.IsNullOrEmpty(codePath))
            {
                ImageBuilderUtility.ValidateRelativePath(codePath);
            }

            if (!string.IsNullOrEmpty(configPath))
            {
                ImageBuilderUtility.ValidateRelativePath(configPath);
            }

            if (!string.IsNullOrEmpty(codePath))
            {
                // Preserving the file name since the version information is embedded in it
                string tempLocalCodePath = Path.Combine(ImageBuilderUtility.GetTempFileName(this.workingDirectory), Path.GetFileName(codePath));

                try
                {
                    if (!this.sourceImageStoreWrapper.DownloadIfContentExists(codePath, tempLocalCodePath, timeoutHelper.GetRemainingTime()))
                    {
                        throw new FileNotFoundException(StringResources.Error_FileNotFound, codePath);
                    }

                    codeVersion = FabricProvisionOperation.GetCodeVersion(tempLocalCodePath);
                }
                finally
                {
                    ImageBuilderUtility.DeleteTempLocation(tempLocalCodePath);
                }
            }

            if (!string.IsNullOrEmpty(configPath))
            {
                ClusterManifestType clusterManifest = this.destinationImageStoreWrapper.GetFromStore<ClusterManifestType>(configPath, timeoutHelper.GetRemainingTime());

                configVersion = clusterManifest.Version;
            }

            FabricVersion fabricVersion = new FabricVersion(codeVersion, configVersion);

            if (outputFile != null)
            {
                TraceSource.WriteInfo(
                    ImageBuilder.TraceType,
                    "Writing FabricInfo with CodeVersion:{0} and ConfigVersion:{1} to file {2}.",
                    codeVersion,
                    configVersion,
                    outputFile);

                ImageBuilderUtility.WriteStringToFile(outputFile, fabricVersion.ToString(), false, Encoding.Unicode);
            }

            return fabricVersion;
        }

        public void ProvisionFabric(
            string codePath,
            string configPath,
            string configurationCsvFilePath,
            string infrastructureManifestFilePath,
            TimeSpan timeout,
            bool validateOnly = false)
        {
            TimeoutHelper timeoutHelper = new TimeoutHelper(timeout);

            if (string.IsNullOrEmpty(codePath) &&
                string.IsNullOrEmpty(configPath))
            {
                throw new ArgumentException(StringResources.ImageBuilderError_CodeAndConfigPathNotSpecified);
            }

#if !DotNetCoreClrLinux && !DotNetCoreClrIOT
            if (string.IsNullOrEmpty(configurationCsvFilePath) || !FabricFile.Exists(configurationCsvFilePath))
            {
                throw new ArgumentException(
                    string.Format(CultureInfo.CurrentCulture, StringResources.ImageBuilderError_InvalidPath, configurationCsvFilePath),
                    "configurationCsvFilePath");
            }
#endif

            if (!string.IsNullOrEmpty(infrastructureManifestFilePath) && !FabricFile.Exists(infrastructureManifestFilePath))
            {
                // TODO: Once FabricTest has been modified to generate InfrastructureManifest.xml file, we should throw if the file
                // does not exist
            }

            if (!string.IsNullOrEmpty(codePath))
            {
                ImageBuilderUtility.ValidateRelativePath(codePath);
            }

            if (!string.IsNullOrEmpty(configPath))
            {
                ImageBuilderUtility.ValidateRelativePath(configPath);
            }

            string tempLocalCodePath = string.Empty;
            string tempLocalConfigPath = string.Empty;

            try
            {
                if (!string.IsNullOrEmpty(codePath))
                {
                    // Preserving the file name since the version information is embedded in it
                    tempLocalCodePath = Path.Combine(ImageBuilderUtility.GetTempFileName(this.workingDirectory), Path.GetFileName(codePath));

                    if (!this.sourceImageStoreWrapper.DownloadIfContentExists(codePath, tempLocalCodePath, timeoutHelper.GetRemainingTime()))
                    {
                        throw new FileNotFoundException(StringResources.Error_FileNotFound, codePath);
                    }

#if !DotNetCoreClrIOT
                    if (!FileSignatureVerifier.IsSignatureValid(tempLocalCodePath))
                    {
                        ImageBuilderUtility.TraceAndThrowValidationError(
                            TraceType,
                            StringResources.Error_InvalidCodePackage);
                    }
#endif
                }

                if (!string.IsNullOrEmpty(configPath))
                {
                    tempLocalConfigPath = ImageBuilderUtility.GetTempFileName(this.workingDirectory);

                    if (!this.sourceImageStoreWrapper.DownloadIfContentExists(configPath, tempLocalConfigPath, timeoutHelper.GetRemainingTime()))
                    {
                        throw new FileNotFoundException(StringResources.Error_FileNotFound, configPath);
                    }
                }

                FabricProvisionOperation provisionOperation = new FabricProvisionOperation(
                    this.validatingXmlReaderSettings,
                    this.destinationImageStoreWrapper);
#if DotNetCoreClrLinux || DotNetCoreClrIOT
                provisionOperation.ProvisionFabric(tempLocalCodePath, tempLocalConfigPath, infrastructureManifestFilePath, validateOnly, timeout);
#else
                provisionOperation.ProvisionFabric(tempLocalCodePath, tempLocalConfigPath, configurationCsvFilePath, infrastructureManifestFilePath, validateOnly, timeout);
#endif 
            }
            finally
            {
                ImageBuilderUtility.DeleteTempLocation(tempLocalCodePath);
                ImageBuilderUtility.DeleteTempLocation(tempLocalConfigPath);                
            }
        }

        public void GetClusterManifestContents(
            string version,
            string outputDirectory,
            TimeSpan timeout)
        {
            var winFabLayout = WinFabStoreLayoutSpecification.Create();
            var srcConfigPath = winFabLayout.GetClusterManifestFile(version);
            var destConfigPath = Path.Combine(outputDirectory, "ClusterManifest.xml");

            if (!this.sourceImageStoreWrapper.DownloadIfContentExists(srcConfigPath, destConfigPath, timeout))
            {
                throw new FileNotFoundException(StringResources.Error_FileNotFound, srcConfigPath);
            }
        }

        public void UpgradeFabric(
            string currentFabricVersion,
            string targetFabricVersion,
            string configurationCsvFilePath,
            TimeSpan timeout,
            string outputFile = null)
        {
#if !DotNetCoreClrLinux && !DotNetCoreClrIOT
            if (string.IsNullOrEmpty(configurationCsvFilePath) || !FabricFile.Exists(configurationCsvFilePath))
            {
                throw new ArgumentException(
                    string.Format(CultureInfo.CurrentCulture, StringResources.ImageBuilderError_InvalidPath, configurationCsvFilePath),
                    "configurationCsvFilePath");
            }
#endif
            FabricUpgradeOperation fabricUpgradeOperation = new FabricUpgradeOperation(this.destinationImageStoreWrapper);

            bool isConfigOnly = false;
            fabricUpgradeOperation.UpgradeFabric(
                currentFabricVersion, 
                targetFabricVersion,
#if !DotNetCoreClrLinux && !DotNetCoreClrIOT
                configurationCsvFilePath,
#endif
                timeout,
                out isConfigOnly);

            if (outputFile != null)
            {
                TraceSource.WriteInfo(
                    ImageBuilder.TraceType,
                    "Writing UpgradeFabric result with ConfigOnly:{0} to file {1}.",
                    isConfigOnly,
                    outputFile);

                // Include versions for sanity check by CM
                //
                string configOnlyResult = string.Format(
                    CultureInfo.InvariantCulture,
                    "{0}:{1}:{2}", 
                    currentFabricVersion,
                    targetFabricVersion,
                    isConfigOnly);

                ImageBuilderUtility.WriteStringToFile(outputFile, configOnlyResult, false, Encoding.Unicode);
            }
        }

        public void GetManifests(
            string fileName,
            TimeSpan timeout)
        {
            TimeoutHelper timeoutHelper = new TimeoutHelper(timeout);

            if (string.IsNullOrEmpty(fileName))
            {
                throw new ArgumentException(StringResources.ImageBuilderError_ArgumentNullOrEmpty, "inputFileName");
            }

            using (StreamReader reader = new StreamReader(File.Open(fileName, FileMode.Open)))
            {
                string packagePath;
                while ((packagePath = reader.ReadLine()) != null)
                {
                    string outputPath = reader.ReadLine();
                    if (outputPath == null)
                    {
                        throw new ArgumentException(
                            string.Format(CultureInfo.CurrentCulture, StringResources.ImageBuilderError_InvalidPath, packagePath),
                            "OutputDirectory");
                    }

                    if (!this.sourceImageStoreWrapper.DownloadIfContentExists(packagePath, outputPath, timeoutHelper.GetRemainingTime()))
                    {
                        throw new DirectoryNotFoundException(string.Format(
                            CultureInfo.CurrentCulture,
                            StringResources.ImageBuilderError_ApplicationPackageFolderMissing,
                            packagePath));
                    }

                    TraceSource.WriteInfo(
                        ImageBuilder.TraceType,
                        "Completed download of Application {0} from store.",
                        packagePath);
                }
            }
        }

        private ComposeApplicationTypeDescription GetComposeApplicationTypeDescription(
            string applicationTypeName,
            string applicationTypeVersion,
            string composeFilename,
            string overridesFileName,
            out HashSet<string> ignoredKeysInComposeFile)
        {
            var composeAppTypeDescription = new ComposeApplicationTypeDescription();
            ignoredKeysInComposeFile = new HashSet<string>();

            if (!string.IsNullOrEmpty(applicationTypeName))
            {
                composeAppTypeDescription.ApplicationTypeName = applicationTypeName;
            }
            if (!string.IsNullOrEmpty(applicationTypeVersion))
            {
                composeAppTypeDescription.ApplicationTypeVersion = applicationTypeVersion;
            }

            //
            // TODO: Read and merge the compose file and overrides file.
            //
            composeAppTypeDescription.Parse("", DockerComposeUtils.GetRootNode(composeFilename), ignoredKeysInComposeFile);
            composeAppTypeDescription.Validate();

            return composeAppTypeDescription;
        }

        private void InitializeValidatingXmlReaderSettings(string schemaPath)
        {
            this.validatingXmlReaderSettings = new XmlReaderSettings();
#if !DotNetCoreClr
            this.validatingXmlReaderSettings.ValidationType = ValidationType.Schema;
            this.validatingXmlReaderSettings.Schemas.Add(StringConstants.FabricNamespace, schemaPath);
            this.validatingXmlReaderSettings.XmlResolver = null;
#endif
        }

        private void InitializeApplicationValidators()
        {
            this.applicationValidators = new Collection<IApplicationValidator>();
            this.applicationValidators.Add(new ApplicationManifestValidator());
            this.applicationValidators.Add(new ServiceManifestValidator());
            this.applicationValidators.Add(new ApplicationDiagnosticsValidator());
        }

        //
        // * If the container codepackage section is same, then its version need not be incremented.
        // * If the deployment is removing services, then the corresponding service type from the previous needs to be kept around
        // so that the app is healthy through the upgrade. In the next upgrade, the service type will automatically be discarded.
        //
        private void AdjustSingleInstanceManifestsForUpgrade(
            string applicationTypeName,
            string currentTypeVersion,
            TimeoutHelper timeoutHelper,
            Dictionary<string, Dictionary<string, SettingsType>> settingsType,
            ref ApplicationManifestType targetApplicationManifest,
            ref IList<ServiceManifestType> targetServiceManifests,
            bool retainDnsName = false)
        {
            var storeLayoutSpecification = StoreLayoutSpecification.Create();
            var currentAppManifestFile = storeLayoutSpecification.GetApplicationManifestFile(applicationTypeName, currentTypeVersion);

            var currentApplicationManifest = this.destinationImageStoreWrapper.GetFromStore<ApplicationManifestType>(currentAppManifestFile, timeoutHelper.GetRemainingTime());
            var currentServiceManifests = this.GetAllServiceManifests(applicationTypeName, currentApplicationManifest, timeoutHelper);

            ImageBuilderUtility.CompareAndFixTargetManifestVersionsForUpgrade(
                storeLayoutSpecification,
                this.destinationImageStoreWrapper,
                timeoutHelper,
                settingsType,
                applicationTypeName,
                currentApplicationManifest,
                targetApplicationManifest,
                currentServiceManifests,
                ref targetServiceManifests);

            List<ApplicationManifestTypeServiceManifestImport> serviceManifestImportsToBeAdded = new List<ApplicationManifestTypeServiceManifestImport>();

            // service manifest imports that exist in current but not target
            var targetServiceManifestImports = targetApplicationManifest.ServiceManifestImport;
            var serviceManifestImportsToAdjust = currentApplicationManifest.ServiceManifestImport.Where(
                ci => !targetServiceManifestImports.Any(ti => ti.ServiceManifestRef.ServiceManifestName == ci.ServiceManifestRef.ServiceManifestName));

            foreach(ApplicationManifestTypeServiceManifestImport serviceManifestImportToAdjust in serviceManifestImportsToAdjust)
            {
                var serviceManifest = GetServiceManifestFromStore(
                    applicationTypeName,
                    serviceManifestImportToAdjust.ServiceManifestRef.ServiceManifestName,
                    serviceManifestImportToAdjust.ServiceManifestRef.ServiceManifestVersion,
                    timeoutHelper);

                foreach(DefaultServicesTypeService defaultServiceType in currentApplicationManifest.DefaultServices.Items)
                {
                    string serviceTypeName = defaultServiceType.Item.ServiceTypeName;
                    if (serviceManifest.ServiceTypes.Any(st =>
                        ((st is StatelessServiceTypeType) ? (st as StatelessServiceTypeType).ServiceTypeName : (st as StatefulServiceTypeType).ServiceTypeName) == serviceTypeName))
                    {
                        TraceSource.WriteInfo(
                            ImageBuilder.TraceType,
                            "To be removed service manifest = '{0}'",
                            serviceManifest.Name);

                        targetServiceManifests.Add(serviceManifest);
                        serviceManifestImportsToBeAdded.Add(serviceManifestImportToAdjust);
                    }
                }
            }

            if (serviceManifestImportsToBeAdded.Count > 0)
            {
                targetApplicationManifest.ServiceManifestImport = 
                    targetApplicationManifest.ServiceManifestImport.Concat(
                        serviceManifestImportsToBeAdded.ToArray()).ToArray();
            }

            if (retainDnsName)
            {
                //
                // In the initial implementation of compose->manifest translation, we were setting the dns name as the 'servicename'. This is changed to
                // 'servicename.appname' in later versions. We need to make sure we dont change the dns name of the service across upgrades when the compose app was
                // created in the old version of the runtime and the upgrade is taking place in the newer version.
                //
                foreach (DefaultServicesTypeService targetDefaultServiceType in targetApplicationManifest.DefaultServices.Items)
                {
                    foreach (DefaultServicesTypeService currentDefaultServiceType in currentApplicationManifest.DefaultServices.Items)
                    {
                        if (targetDefaultServiceType.Name == currentDefaultServiceType.Name &&
                            targetDefaultServiceType.Item.ServiceTypeName == currentDefaultServiceType.Item.ServiceTypeName &&
                            targetDefaultServiceType.ServiceDnsName != currentDefaultServiceType.ServiceDnsName)
                        {
                            targetDefaultServiceType.ServiceDnsName = currentDefaultServiceType.ServiceDnsName;
                        }
                    }
                }
            }
        }

        private IList<ServiceManifestType> GetAllServiceManifests(
            string applicationTypeName,
            ApplicationManifestType applicationManifestType,
            TimeoutHelper timeoutHelper)
        {
            List<ServiceManifestType> serviceManifests = new List<ServiceManifestType>();
            foreach (var import in applicationManifestType.ServiceManifestImport)
            {
                serviceManifests.Add(GetServiceManifestFromStore(
                    applicationTypeName,
                    import.ServiceManifestRef.ServiceManifestName,
                    import.ServiceManifestRef.ServiceManifestVersion,
                    timeoutHelper));
            }

            return serviceManifests;
        }

        private ServiceManifestType GetServiceManifestFromStore(
            string applicationTypeName,
            string serviceManifestName,
            string serviceManifestVersion,
            TimeoutHelper timeoutHelper)
        {
            var storeLayoutSpecification = StoreLayoutSpecification.Create();
            var serviceManifestFile = storeLayoutSpecification.GetServiceManifestFile(
                applicationTypeName,
                serviceManifestName,
                serviceManifestVersion);

            return this.destinationImageStoreWrapper.GetFromStore<ServiceManifestType>(serviceManifestFile, timeoutHelper.GetRemainingTime());
        }

        private GenerationConfig PopulateTargetReplicaCount (GenerationConfig generationConfig)
        {
            var configStore = NativeConfigStore.FabricGetConfigStore();
            var targetReplicaSetSize = configStore.ReadUnencryptedString(StringConstants.FailoverManagerSectionName, StringConstants.TargetReplicaSetSizeKeyName);

            if (!String.IsNullOrEmpty(targetReplicaSetSize))
            {
                var replicaCount = int.Parse(targetReplicaSetSize);
                if (generationConfig == null)
                {
                    generationConfig = new GenerationConfig() { TargetReplicaCount = replicaCount };
                }
                else
                {
                    generationConfig.TargetReplicaCount = replicaCount;
                }
            }
            return generationConfig;
        }

    }
}
