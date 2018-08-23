// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class IImageBuilder
        {
        public:
            typedef std::function<void(std::wstring const &)> ProgressDetailsCallback;

        public:
            IImageBuilder() { }

            virtual ~IImageBuilder() { }

            virtual void UpdateSecuritySettings(Transport::SecuritySettings const &) = 0;

            virtual Common::ErrorCode VerifyImageBuilderSetup() = 0;

            virtual void Enable() = 0;

            virtual void Disable() = 0;

            virtual Common::ErrorCode GetApplicationTypeInfo(
                std::wstring const & buildPath, 
                Common::TimeSpan const,
                __out ServiceModelTypeName &,
                __out ServiceModelVersion &) = 0;

            virtual Common::ErrorCode ValidateComposeFile(
                Common::ByteBuffer const &composeFile,
                Common::NamingUri const&,
                ServiceModelTypeName const &,
                ServiceModelVersion const &,
                Common::TimeSpan const&) = 0;

            virtual Common::ErrorCode BuildManifestContexts(
                std::vector<ApplicationTypeContext> const&,
                Common::TimeSpan const,
                __out std::vector<StoreDataApplicationManifest> &,
                __out std::vector<StoreDataServiceManifest> &) = 0;
                
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
                __out std::map<std::wstring, std::wstring> &) = 0;

            virtual Common::AsyncOperationSPtr BeginBuildApplicationType(
                Common::ActivityId const &,
                std::wstring const & buildPath,
                std::wstring const & downloadPath,
                ServiceModelTypeName const &,
                ServiceModelVersion const &,
                ProgressDetailsCallback const &,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &) = 0;

            virtual Common::ErrorCode EndBuildApplicationType(
                Common::AsyncOperationSPtr const &,
                __out std::vector<ServiceModelServiceManifestDescription> &,
                __out std::wstring & applicationManifestId,
                __out std::wstring & applicationManifestContent,
                __out ServiceModel::ApplicationHealthPolicy &,
                __out std::map<std::wstring, std::wstring> &) = 0;

            // Provision should be relatively quicker since there is no download involved.
            virtual Common::AsyncOperationSPtr BeginBuildComposeDeploymentType(
                Common::ActivityId const &,
                Common::ByteBufferSPtr const &composeFile,
                Common::ByteBufferSPtr const &overridesFile,
                std::wstring const &repositoryUserName,
                std::wstring const &repositoryPassword,
                bool isPasswordEncrypted,
                Common::NamingUri const &,
                ServiceModelTypeName const &,
                ServiceModelVersion const &,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &) = 0;

            virtual Common::ErrorCode EndBuildComposeDeploymentType(
                Common::AsyncOperationSPtr const &,
                __out std::vector<ServiceModelServiceManifestDescription> &,
                __out std::wstring & applicationManifestId,
                __out std::wstring & applicationManifestContent,
                __out ServiceModel::ApplicationHealthPolicy &,
                __out std::map<std::wstring, std::wstring> &,
                __out std::wstring &mergedComposeFile) = 0;

            virtual Common::AsyncOperationSPtr BeginBuildComposeApplicationTypeForUpgrade(
                Common::ActivityId const &,
                Common::ByteBufferSPtr const &composeFile,
                Common::ByteBufferSPtr const &overridesFile,
                std::wstring const &repositoryUserName,
                std::wstring const &repositoryPassword,
                bool isPasswordEncrypted,
                Common::NamingUri const &,
                ServiceModelTypeName const &,
                ServiceModelVersion const &,
                ServiceModelVersion const &,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &) = 0;

            virtual Common::ErrorCode EndBuildComposeApplicationTypeForUpgrade(
                Common::AsyncOperationSPtr const &,
                __out std::vector<ServiceModelServiceManifestDescription> &,
                __out std::wstring & applicationManifestId,
                __out std::wstring & applicationManifestContent,
                __out ServiceModel::ApplicationHealthPolicy &,
                __out std::map<std::wstring, std::wstring> &,
                __out std::wstring &mergedComposeFile) = 0;

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
                Common::AsyncOperationSPtr const &) = 0;

            virtual Common::ErrorCode EndBuildSingleInstanceApplicationForUpgrade(
                Common::AsyncOperationSPtr const &,
                __out std::vector<ServiceModelServiceManifestDescription> &,
                __out std::wstring & applicationManifestContent,
                __out ServiceModel::ApplicationHealthPolicy &,
                __out std::map<std::wstring, std::wstring> &,
                __out DigestedApplicationDescription &,
                __out DigestedApplicationDescription &) = 0;

            virtual Common::AsyncOperationSPtr BeginBuildSingleInstanceApplication(
                Common::ActivityId const &,
                ServiceModelTypeName const &,
                ServiceModelVersion const &,
                ServiceModel::ModelV2::ApplicationDescription const &,
                Common::NamingUri const & appName,
                ServiceModelApplicationId const & appId,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &) = 0;

            virtual Common::ErrorCode EndBuildSingleInstanceApplication(
                Common::AsyncOperationSPtr const &,
                __out std::vector<ServiceModelServiceManifestDescription> &,
                __out std::wstring & applicationManifestContent,
                __out ServiceModel::ApplicationHealthPolicy &,
                __out std::map<std::wstring, std::wstring> &,
                __out DigestedApplicationDescription &) = 0;

            virtual Common::ErrorCode Test_BuildApplication(
                Common::NamingUri const &,
                ServiceModelApplicationId const &,
                ServiceModelTypeName const &,
                ServiceModelVersion const &,
                std::map<std::wstring, std::wstring> const &,
                Common::TimeSpan const,
                __out DigestedApplicationDescription &) = 0;

            virtual Common::AsyncOperationSPtr BeginBuildApplication(
                Common::ActivityId const &,
                Common::NamingUri const &,
                ServiceModelApplicationId const &,
                ServiceModelTypeName const &,
                ServiceModelVersion const &,
                std::map<std::wstring, std::wstring> const &,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &) = 0;

            virtual Common::ErrorCode EndBuildApplication(
                Common::AsyncOperationSPtr const &,
                __out DigestedApplicationDescription &) = 0;

            virtual Common::AsyncOperationSPtr BeginUpgradeApplication(
                Common::ActivityId const &,
                Common::NamingUri const &,
                ServiceModelApplicationId const &,
                ServiceModelTypeName const &,
                ServiceModelVersion const &,
                uint64 currentApplicationVersion,
                std::map<std::wstring, std::wstring> const &,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent) = 0;

            virtual Common::ErrorCode EndUpgradeApplication(
                Common::AsyncOperationSPtr const & operation,
                __out DigestedApplicationDescription & currentApplicationResult,
                __out DigestedApplicationDescription & targetApplicationResult) = 0;

            virtual Common::ErrorCode GetFabricVersionInfo(
                std::wstring const & codeFilepath,
                std::wstring const & clusterManifestFilepath,
                Common::TimeSpan const timeout,
                __out Common::FabricVersion &) = 0;

            virtual Common::ErrorCode ProvisionFabric(
                std::wstring const & codeFilepath,
                std::wstring const & clusterManifestFilepath,
                Common::TimeSpan const timeout) = 0;

            virtual Common::ErrorCode GetClusterManifestContents(
                Common::FabricConfigVersion const &,
                Common::TimeSpan const timeout,
                __out std::wstring & clusterManifestContents) = 0;

            virtual Common::ErrorCode UpgradeFabric(
                Common::FabricVersion const & currentVersion,
                Common::FabricVersion const & targetVersion,
                Common::TimeSpan const timeout,
                __out bool & isConfigOnly) = 0;

            virtual Common::ErrorCode Test_CleanupApplication(
                ServiceModelTypeName const &,
                ServiceModelApplicationId const &,
                Common::TimeSpan const timeout) = 0;

            virtual Common::AsyncOperationSPtr BeginCleanupApplication(
                Common::ActivityId const &,
                ServiceModelTypeName const &,
                ServiceModelApplicationId const &,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &) = 0;

            virtual Common::ErrorCode EndCleanupApplication(
                Common::AsyncOperationSPtr const &) = 0;

            virtual Common::ErrorCode Test_CleanupApplicationInstance(
                ServiceModel::ApplicationInstanceDescription const &,
                bool deleteApplicationPackage,
                std::vector<ServiceModel::ServicePackageReference> const &,
                Common::TimeSpan const timeout) = 0;

            virtual Common::AsyncOperationSPtr BeginCleanupApplicationInstance(
                Common::ActivityId const &,
                ServiceModel::ApplicationInstanceDescription const &,
                bool deleteApplicationPackage,
                std::vector<ServiceModel::ServicePackageReference> const &,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent) = 0;

            virtual Common::ErrorCode EndCleanupApplicationInstance(
                Common::AsyncOperationSPtr const & operation) = 0;

            virtual Common::ErrorCode CleanupApplicationType(
                ServiceModelTypeName const &,
                ServiceModelVersion const &,
                std::vector<ServiceModelServiceManifestDescription> const &,
                bool isLastApplicationType,
                Common::TimeSpan const timeout) = 0;

            virtual Common::ErrorCode CleanupFabricVersion(
                Common::FabricVersion const &,
                Common::TimeSpan const timeout) = 0;

            virtual Common::ErrorCode CleanupApplicationPackage(
                std::wstring const & buildPath,
                Common::TimeSpan const timeout) = 0;
        };
    }
}
