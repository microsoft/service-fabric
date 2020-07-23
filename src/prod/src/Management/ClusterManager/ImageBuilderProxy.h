// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class ImageBuilderProxy 
            : public IImageBuilder
            , public Federation::NodeTraceComponent<Common::TraceTaskCodes::ClusterManager>
            , public Common::ComponentRoot
        {
        public:
            static Common::GlobalWString UnicodeBOM;
            static Common::GlobalWString Newline;

            static Common::GlobalWString TempWorkingDirectoryPrefix;
            static Common::GlobalWString ArchiveDirectoryPrefix;
            static Common::GlobalWString ImageBuilderOutputDirectory;
            static Common::GlobalWString AppParamFilename;
            static Common::GlobalWString AppTypeInfoOutputDirectory;
            static Common::GlobalWString AppTypeOutputDirectory;
            static Common::GlobalWString AppOutputDirectory;
            static Common::GlobalWString ImageBuilderExeName;
            static Common::GlobalWString CoreImageBuilderExeName;
            static Common::GlobalWString ApplicationTypeInfoOutputFilename;
            static Common::GlobalWString FabricUpgradeOutputDirectory;
            static Common::GlobalWString ClusterManifestOutputDirectory;
            static Common::GlobalWString FabricOutputDirectory;
            static Common::GlobalWString FabricVersionOutputFilename;
            static Common::GlobalWString FabricUpgradeResultFilename;
            static Common::GlobalWString CleanupListFilename;            
            static Common::GlobalWString ErrorDetailsFilename;
            static Common::GlobalWString ProgressDetailsFilename;
            static Common::GlobalWString DockerComposeFilename;
            static Common::GlobalWString DockerComposeOverridesFilename;
            static Common::GlobalWString OutputDockerComposeFilename;
            static Common::GlobalWString GenerateDnsNames;
            static Common::GlobalWString CurrentTypeVersion;
            static Common::GlobalWString TargetTypeVersion;
            static Common::GlobalWString SFVolumeDiskServiceEnabled;

            // ImageBuilder.exe command line arguments
            static Common::GlobalWString SchemaPath;
            static Common::GlobalWString WorkingDir;
            static Common::GlobalWString CurrentClusterManifest;
            static Common::GlobalWString Operation;
            static Common::GlobalWString StoreRoot;
            static Common::GlobalWString AppName;
            static Common::GlobalWString AppTypeName;
            static Common::GlobalWString AppTypeVersion;
            static Common::GlobalWString AppId;
            static Common::GlobalWString NameUri;
            static Common::GlobalWString AppParam;
            static Common::GlobalWString BuildPath;
            static Common::GlobalWString DownloadPath;
            static Common::GlobalWString Output;
            static Common::GlobalWString Input;
            static Common::GlobalWString Progress;
            static Common::GlobalWString ErrorDetails;            
            static Common::GlobalWString CurrentAppInstanceVersion;
            static Common::GlobalWString FabricCodeFilepath;
            static Common::GlobalWString FabricConfigFilepath;
            static Common::GlobalWString FabricConfigVersion;
            static Common::GlobalWString CurrentFabricVersion;
            static Common::GlobalWString TargetFabricVersion;
            static Common::GlobalWString InfrastructureManifestFile;
            static Common::GlobalWString DisableChecksumValidation;
            static Common::GlobalWString DisableServerSideCopy;
            static Common::GlobalWString Timeout;
            static Common::GlobalWString RegistryUserName;
            static Common::GlobalWString RegistryPassword;
            static Common::GlobalWString IsRegistryPasswordEncrypted;
            static Common::GlobalWString ComposeFilePath;
            static Common::GlobalWString OverrideFilePath;
            static Common::GlobalWString OutputComposeFilePath;
            static Common::GlobalWString CleanupComposeFiles;
            static Common::GlobalWString SingleInstanceApplicationDescriptionString;
            static Common::GlobalWString UseOpenNetworkConfig;
            static Common::GlobalWString UseLocalNatNetworkConfig;
            static Common::GlobalWString DisableApplicationPackageCleanup;
            static Common::GlobalWString GenerationConfig;
            static Common::GlobalWString MountPointForSettings;

            // ImageBuilder.exe operation values
            static Common::GlobalWString OperationBuildApplicationTypeInfo;
            static Common::GlobalWString OperationDownloadAndBuildApplicationType;
            static Common::GlobalWString OperationBuildApplicationType;
            static Common::GlobalWString OperationBuildApplication;
            static Common::GlobalWString OperationUpgradeApplication;
            static Common::GlobalWString OperationDelete;
            static Common::GlobalWString OperationGetFabricVersion;
            static Common::GlobalWString OperationProvisionFabric;
            static Common::GlobalWString OperationGetClusterManifest;
            static Common::GlobalWString OperationUpgradeFabric;
            static Common::GlobalWString OperationGetManifests;
            static Common::GlobalWString OperationValidateComposeFile;
            static Common::GlobalWString OperationBuildComposeDeploymentType;
            static Common::GlobalWString OperationBuildComposeApplicationTypeForUpgrade;
            static Common::GlobalWString OperationCleanupApplicationPackage;
            static Common::GlobalWString OperationBuildSingleInstanceApplication;
            static Common::GlobalWString OperationBuildSingleInstanceApplicationForUpgrade;

        protected:

            ImageBuilderProxy(
                std::wstring const & imageBuilderExeDirectory,
                std::wstring const & schemaOutputPath,
                std::wstring const & nodeName,
                Federation::NodeInstance const &);

            void Initialize();

        public:

            static std::shared_ptr<ImageBuilderProxy> Create(
                std::wstring const & imageBuilderExeDirectory,
                std::wstring const & schemaOutputPath,
                std::wstring const & nodeName,
                Federation::NodeInstance const &);

            virtual ~ImageBuilderProxy();

            virtual void UpdateSecuritySettings(Transport::SecuritySettings const &);

            virtual Common::ErrorCode VerifyImageBuilderSetup();

            virtual void Enable();

            virtual void Disable();

            virtual void Abort(HANDLE hProcess);

            virtual Common::ErrorCode GetApplicationTypeInfo(
                std::wstring const & buildPath, 
                Common::TimeSpan const,
                __out ServiceModelTypeName &, 
                __out ServiceModelVersion &) override;

            virtual Common::ErrorCode BuildManifestContexts(
                std::vector<ApplicationTypeContext> const&,
                Common::TimeSpan const,
                __out std::vector<StoreDataApplicationManifest> &,
                __out std::vector<StoreDataServiceManifest> &);

            virtual Common::ErrorCode Test_BuildApplicationType(
                Common::ActivityId const & activityId,
                std::wstring const & buildPath,
                std::wstring const & downloadPath,
                ServiceModelTypeName const &,
                ServiceModelVersion const &,
                Common::TimeSpan const,
                __out std::vector<ServiceModelServiceManifestDescription> &,
                __out std::wstring & applicationManifestContent,
                __out ServiceModel::ApplicationHealthPolicy &,
                __out std::map<std::wstring, std::wstring> &) override;

            virtual Common::ErrorCode ValidateComposeFile(
                Common::ByteBuffer const &,
                Common::NamingUri const&,
                ServiceModelTypeName const &,
                ServiceModelVersion const &,
                Common::TimeSpan const &);

            virtual Common::AsyncOperationSPtr BeginBuildComposeDeploymentType(
                Common::ActivityId const &,
                Common::ByteBufferSPtr const &,
                Common::ByteBufferSPtr const &,
                std::wstring const &,
                std::wstring const &,
                bool,
                Common::NamingUri const&,
                ServiceModelTypeName const &,
                ServiceModelVersion const &,
                Common::TimeSpan const,
                Common::AsyncCallback const &callback,
                Common::AsyncOperationSPtr const &parent);

            virtual Common::ErrorCode EndBuildComposeDeploymentType(
                Common::AsyncOperationSPtr const &operation,
                __out std::vector<ServiceModelServiceManifestDescription> &,
                __out std::wstring &,
                __out std::wstring &,
                __out ServiceModel::ApplicationHealthPolicy &,
                __out std::map<std::wstring, std::wstring> &,
                __out std::wstring &);

            virtual Common::AsyncOperationSPtr BeginBuildComposeApplicationTypeForUpgrade(
                Common::ActivityId const &,
                Common::ByteBufferSPtr const &composeFile,
                Common::ByteBufferSPtr const &overridesFile,
                std::wstring const &registryUserName,
                std::wstring const &registryPassword,
                bool isPasswordEncrypted,
                Common::NamingUri const &,
                ServiceModelTypeName const &,
                ServiceModelVersion const &,
                ServiceModelVersion const &,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            virtual Common::ErrorCode EndBuildComposeApplicationTypeForUpgrade(
                Common::AsyncOperationSPtr const &,
                __out std::vector<ServiceModelServiceManifestDescription> &,
                __out std::wstring & applicationManifestId,
                __out std::wstring & applicationManifestContent,
                __out ServiceModel::ApplicationHealthPolicy &,
                __out std::map<std::wstring, std::wstring> &,
                __out std::wstring &mergedComposeFile);

            virtual Common::AsyncOperationSPtr BeginBuildApplicationType(
                Common::ActivityId const &,
                std::wstring const & buildPath,
                std::wstring const & downloadPath,
                ServiceModelTypeName const &,
                ServiceModelVersion const &,
                ProgressDetailsCallback const &,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &) override;

            virtual Common::ErrorCode EndBuildApplicationType(
                Common::AsyncOperationSPtr const &,
                __out std::vector<ServiceModelServiceManifestDescription> &,
                __out std::wstring & applicationManifestId,
                __out std::wstring & applicationManifestContent,
                __out ServiceModel::ApplicationHealthPolicy &,
                __out std::map<std::wstring, std::wstring> &) override;

            virtual Common::AsyncOperationSPtr BeginBuildSingleInstanceApplicationForUpgrade(
                Common::ActivityId const &,
                ServiceModelTypeName const &,
                ServiceModelVersion const & currentTypeVersion,
                ServiceModelVersion const & targetTypeVersion,
                ServiceModel::ModelV2::ApplicationDescription const &,
                Common::NamingUri const & appName,
                ServiceModelApplicationId const & appId,
                uint64 currentApplicationVersion,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            virtual Common::ErrorCode EndBuildSingleInstanceApplicationForUpgrade(
                Common::AsyncOperationSPtr const &,
                __out std::vector<ServiceModelServiceManifestDescription> &,
                __out std::wstring & applicationManifestContent,
                __out ServiceModel::ApplicationHealthPolicy &,
                __out std::map<std::wstring, std::wstring> &,
                __out DigestedApplicationDescription &,
                __out DigestedApplicationDescription &);
            
            virtual Common::AsyncOperationSPtr BeginBuildSingleInstanceApplication(
                Common::ActivityId const &,
                ServiceModelTypeName const &,
                ServiceModelVersion const &,
                ServiceModel::ModelV2::ApplicationDescription const &,
                Common::NamingUri const & appName,
                ServiceModelApplicationId const & appId,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &) override;

            virtual Common::ErrorCode EndBuildSingleInstanceApplication(
                Common::AsyncOperationSPtr const &,
                __out std::vector<ServiceModelServiceManifestDescription> &,
                __out std::wstring & applicationManifestContent,
                __out ServiceModel::ApplicationHealthPolicy &,
                __out std::map<std::wstring, std::wstring> &,
                __out DigestedApplicationDescription &) override;

            virtual Common::ErrorCode Test_BuildApplication(
                Common::NamingUri const &,
                ServiceModelApplicationId const &,
                ServiceModelTypeName const &,
                ServiceModelVersion const &,
                std::map<std::wstring, std::wstring> const &,
                Common::TimeSpan const,
                __out DigestedApplicationDescription &);
            
            virtual Common::AsyncOperationSPtr BeginBuildApplication(
                Common::ActivityId const &,
                Common::NamingUri const &,
                ServiceModelApplicationId const &,
                ServiceModelTypeName const &,
                ServiceModelVersion const &,
                std::map<std::wstring, std::wstring> const &,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            virtual Common::ErrorCode EndBuildApplication(
                Common::AsyncOperationSPtr const &,
                __out DigestedApplicationDescription &);

            virtual Common::AsyncOperationSPtr BeginUpgradeApplication(
                Common::ActivityId const &,
                Common::NamingUri const &,
                ServiceModelApplicationId const &,
                ServiceModelTypeName const &,
                ServiceModelVersion const &,
                uint64 currentApplicationVersion,
                std::map<std::wstring, std::wstring> const &,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            virtual Common::ErrorCode EndUpgradeApplication(
                Common::AsyncOperationSPtr const &,
                __out DigestedApplicationDescription & currentApplicationResult,
                __out DigestedApplicationDescription & targetApplicationResult);

            virtual Common::ErrorCode GetFabricVersionInfo(
                std::wstring const & codeFilepath,
                std::wstring const & clusterManifestFilepath,
                Common::TimeSpan const,
                __out Common::FabricVersion &);

            virtual Common::ErrorCode ProvisionFabric(
                std::wstring const & codeFilepath,
                std::wstring const & clusterManifestFilepath,
                Common::TimeSpan const timeout);

            virtual Common::ErrorCode GetClusterManifestContents(
                Common::FabricConfigVersion const &,
                Common::TimeSpan const timeout,
                __out std::wstring & clusterManifestContents);

            virtual Common::ErrorCode UpgradeFabric(
                Common::FabricVersion const & currentVersion,
                Common::FabricVersion const & targetVersion,
                Common::TimeSpan const timeout,
                __out bool & isConfigOnly);

            virtual Common::ErrorCode Test_CleanupApplication(
                ServiceModelTypeName const &,
                ServiceModelApplicationId const &,
                Common::TimeSpan const);

            virtual Common::AsyncOperationSPtr BeginCleanupApplication(
                Common::ActivityId const &,
                ServiceModelTypeName const &,
                ServiceModelApplicationId const &,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            virtual Common::ErrorCode EndCleanupApplication(
                Common::AsyncOperationSPtr const &);

            virtual Common::ErrorCode Test_CleanupApplicationInstance(
                ServiceModel::ApplicationInstanceDescription const &,
                bool deleteApplicationPackage,
                std::vector<ServiceModel::ServicePackageReference> const &,
                Common::TimeSpan const timeout);

            virtual Common::AsyncOperationSPtr BeginCleanupApplicationInstance(
                Common::ActivityId const &,
                ServiceModel::ApplicationInstanceDescription const &,
                bool deleteApplicationPackage,
                std::vector<ServiceModel::ServicePackageReference> const &,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            virtual Common::ErrorCode EndCleanupApplicationInstance(
                Common::AsyncOperationSPtr const & operation);

            virtual Common::ErrorCode CleanupApplicationType(
                ServiceModelTypeName const &,
                ServiceModelVersion const &,
                std::vector<ServiceModelServiceManifestDescription> const &,
                bool isLastApplicationType,
                Common::TimeSpan const);

            virtual Common::ErrorCode CleanupFabricVersion(
                Common::FabricVersion const &,
                Common::TimeSpan const);

            virtual Common::ErrorCode CleanupApplicationPackage(
                std::wstring const & buildPath,
                Common::TimeSpan const timeout) override;

            std::wstring Test_GetImageBuilderTempWorkingDirectory();
            void Test_DeleteOrArchiveDirectory(std::wstring const &, Common::ErrorCode const &);

            __declspec(property(get=get_PerfCounters)) ImageBuilderPerformanceCounters const & PerfCounters;
            ImageBuilderPerformanceCounters const & get_PerfCounters() const { return *perfCounters_; }

        private:
            class ImageBuilderAsyncOperationBase;
            class RunImageBuilderExeAsyncOperation;
            class DeleteFilesAsyncOperation;
            class BuildApplicationAsyncOperation;
            class BuildApplicationTypeAsyncOperation;
            class BuildComposeDeploymentAppTypeAsyncOperation;
            class BuildComposeApplicationTypeForUpgradeAsyncOperation;
            class BuildSingleInstanceApplicationForUpgradeAsyncOperation; // TODO: Make this common code between CGS
            class BuildSingleInstanceApplicationAsyncOperation;
            class UpgradeApplicationAsyncOperation;
            class CleanupApplicationAsyncOperation;
            class CleanupApplicationInstanceAsyncOperation;
            class ApplicationJobItem;
            class ApplicationJobQueueBase;
            class ApplicationJobQueue;
            class UpgradeJobQueue;
            
            void InitializeCommandLineArguments(
                __inout std::wstring & args);

            void AddImageBuilderArgument(
                __inout std::wstring & args, 
                std::wstring const & argSwitch, 
                std::wstring const & argValue);

            std::wstring CreatePair(
                std::wstring const & key, 
                std::wstring const & value);

            Common::ErrorCode AddImageBuilderApplicationParameters(
                __inout std::wstring & args,
                std::map<std::wstring, std::wstring> const &,
                std::wstring const & tempWorkingDir);

            Common::ErrorCode GetManifests(
                std::wstring const& argumentFile,
                Common::TimeSpan const);

            Common::ErrorCode RunImageBuilderExe(
                std::wstring const & operation,
                std::wstring const & outputDirectoryOrFile,
                __in std::wstring & args,
                Common::TimeSpan const timeout,
                std::map<std::wstring, std::wstring> const & applicationParameters = std::map<std::wstring, std::wstring>());

            Common::AsyncOperationSPtr BeginRunImageBuilderExe(
                    Common::ActivityId const &,
                std::wstring const & operation,
                std::wstring const & outputDirectoryOrFile,
                __in std::wstring & args,
                std::map<std::wstring, std::wstring> const & applicationParameters,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            Common::ErrorCode EndRunImageBuilderExe(
                Common::AsyncOperationSPtr const &);

            Common::ErrorCode TryStartImageBuilderProcess(
                std::wstring const & operation,
                std::wstring const & outputDirectoryOrFile,
                __in std::wstring & args,
                std::map<std::wstring, std::wstring> const & applicationParameters,
                __inout Common::TimeSpan & timeout,
                __out std::wstring & tempWorkingDirectory,
                __out std::wstring & errorDetailsFile,
                __out HANDLE & hProcess,
                __out HANDLE & hThread,
                __out pid_t & pid);

            Common::ErrorCode FinishImageBuilderProcess(
                Common::ErrorCode const & waitForProcessError,
                DWORD exitCode,
                std::wstring const & tempWorkingDirectory,
                std::wstring const & errorDetailsFile,
                HANDLE hProcess,
                HANDLE hThread);

            std::wstring GetImageBuilderTempWorkingDirectory();

            Common::ErrorCode GetImageBuilderError(
                DWORD exitCode,
                std::wstring const & errorDetailsFile);

            Common::ErrorCode ReadApplicationTypeInfo(
                std::wstring const & inputFile, 
                __out ServiceModelTypeName &, 
                __out ServiceModelVersion &);

            Common::ErrorCode ReadFabricVersion(
                std::wstring const & inputFile, 
                __out Common::FabricVersion &);

            Common::ErrorCode ReadClusterManifestContents(
                std::wstring const & inputDirectory, 
                __out std::wstring &);

            Common::ErrorCode ReadFabricUpgradeResult(
                std::wstring const & inputFile, 
                __out Common::FabricVersion & currentVersion,
                __out Common::FabricVersion & targetVersion,
                __out bool & isConfigOnly);

            Common::ErrorCode ReadImageBuilderOutput(
                std::wstring const & inputFile,                
                __out std::wstring & result,
                __in bool BOMPresent = true);

            Common::ErrorCode WriteToFile(
                __in std::wstring const &fileName,
                __in Common::ByteBuffer const &bytes);

            Common::ErrorCode ReadFromFile(
                __in Common::File & file,
                __out std::wstring & result,
                __in bool BOMPresent = true);

            Common::ErrorCode WriteFilenameList(
                std::wstring const & inputFile,
                std::vector<std::wstring> const &);

            Common::ErrorCode DeleteFiles(
                std::wstring const & inputFile,
                std::vector<std::wstring> const & filepaths,
                Common::TimeSpan const);

            Common::AsyncOperationSPtr BeginDeleteFiles(
                Common::ActivityId const &,
                std::wstring const & inputFile,
                std::vector<std::wstring> const & filepaths,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            Common::ErrorCode EndDeleteFiles(
                Common::AsyncOperationSPtr const &);

            Common::ErrorCode ReadApplicationManifestContent(
                std::wstring const & appManifestFile,
                __out std::wstring & appManifestContent);

            Common::ErrorCode ReadServiceManifestContent(
                std::wstring const & serviceManifestFile,
                __out std::wstring & serviceManifestContent);

            Common::ErrorCode ReadApplicationManifest(
                std::wstring const & applicationManifestFile,
                __out ServiceModel::ApplicationManifestDescription &);

            template<typename LayoutSpecification>
            Common::ErrorCode ReadServiceManifests(
                ServiceModel::ApplicationManifestDescription const &,
                LayoutSpecification const &,
                __out std::vector<ServiceModel::ServiceManifestDescription> &);

            Common::ErrorCode ReadApplication(
                std::wstring const & applicationFile,
                __out ServiceModel::ApplicationInstanceDescription &);

            Common::ErrorCode ReadPackages(
                ServiceModel::ApplicationInstanceDescription const &,
                Management::ImageModel::StoreLayoutSpecification const &,
                __out std::vector<ServiceModel::ServicePackageDescription> &);

            template<typename LayoutSpecification>
            Common::ErrorCode ParseApplicationManifest(
                ServiceModel::ApplicationManifestDescription const &,
                LayoutSpecification const &,
                __out std::vector<ServiceModelServiceManifestDescription> &);
            
            Common::ErrorCode ParseApplication(
                Common::NamingUri const &,
                ServiceModel::ApplicationInstanceDescription const &,
                Management::ImageModel::StoreLayoutSpecification const &,
                __out DigestedApplicationDescription &);

            Common::ErrorCode ParseServicePackages(
                Common::NamingUri const &,
                ServiceModel::ApplicationInstanceDescription const &,
                Management::ImageModel::StoreLayoutSpecification const &,
                __out std::vector<StoreDataServicePackage> &,
                __out DigestedApplicationDescription::CodePackageDescriptionMap &,
                __out std::map<ServiceModelTypeName, ServiceModel::ServiceTypeDescription> &,
                __out std::map<ServiceModel::ServicePackageIdentifier, ServiceModel::ServicePackageResourceGovernanceDescription> &,
                __out std::vector<std::wstring> &);

            Common::ErrorCode ParseServiceTemplates(
                Common::NamingUri const & appName,
                ServiceModel::ApplicationInstanceDescription const &,
                std::map<ServiceModelTypeName, ServiceModel::ServiceTypeDescription> const &,
                __out std::vector<StoreDataServiceTemplate> &);

            Common::ErrorCode ParseServiceGroupTemplate(
                ServiceModel::ApplicationServiceDescription const &,
                Reliability::ServiceDescription &);

            Common::ErrorCode ParseDefaultServices(
                Common::NamingUri const & appName,
                ServiceModel::ApplicationInstanceDescription const &,
                std::map<ServiceModelTypeName, ServiceModel::ServiceTypeDescription> const &,
                __out std::vector<Naming::PartitionedServiceDescriptor> &);

            Common::ErrorCode ParseDefaultServiceGroup(
                __in ServiceModel::ServiceTypeDescription const & typeDescription,
                __in ServiceModel::ApplicationServiceDescription const & serviceDescription,
                __in Common::NamingUri const & serviceName,
                __out std::vector<ServiceModel::ServiceLoadMetricDescription> & memberLoadMetrics,
                __out std::vector<byte> & initData);
                
            Common::ErrorCode ParseFabricVersion(
                std::wstring const & code, 
                std::wstring const & config,
                __out Common::FabricVersion & result);
    
            std::wstring GetOutputDirectoryPath(Common::ActivityId const & activityId);

            Common::ErrorCode CreateOutputDirectory(std::wstring const &);
            void DeleteDirectory(std::wstring const &);
            void DeleteOrArchiveDirectory(std::wstring const &, Common::ErrorCode const &);
            std::wstring GetEscapedString(std::wstring &);

            static int64 HandleToInt(HANDLE);

            bool TryRemoveProcessHandle(HANDLE);

            std::wstring GetServiceManifestFileName(
                Management::ImageModel::BuildLayoutSpecification const&,
                std::wstring const &appTypeName,
                std::wstring const &serviceManName,
                std::wstring const &serviceManVersion);

            std::wstring GetServiceManifestFileName(
                Management::ImageModel::StoreLayoutSpecification const&,
                std::wstring const &appTypeName,
                std::wstring const &serviceManName,
                std::wstring const &serviceManVersion);

        private:
            std::wstring workingDir_;
            std::wstring nodeName_;
            std::wstring imageBuilderExePath_;
            std::wstring imageBuilderOutputBaseDirectory_;

            // use a different output directory to isolate the output of each command
            std::wstring appTypeInfoOutputBaseDirectory_;
            std::wstring appTypeOutputBaseDirectory_;
            std::wstring appOutputBaseDirectory_;
            std::wstring fabricOutputBaseDirectory_;

            bool isEnabled_;
            std::vector<HANDLE> processHandles_;
            Common::ExclusiveLock processHandlesLock_;

            Transport::SecuritySettings securitySettings_;
            Common::RwLock securitySettingsLock_;

            std::unique_ptr<ApplicationJobQueue> applicationJobQueue_;
            std::unique_ptr<UpgradeJobQueue> upgradeJobQueue_;
            ImageBuilderPerformanceCountersSPtr perfCounters_;
        };
    }
}
