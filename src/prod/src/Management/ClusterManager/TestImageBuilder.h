// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class TestImageBuilder : public IImageBuilder
        {
        public:
            struct SharedState;
            typedef std::shared_ptr<SharedState> SharedStateSPtr;
            class SharedStateWrapper;

            TestImageBuilder() : sharedState_(std::make_shared<SharedState>()) { }
            TestImageBuilder(SharedStateSPtr const & sharedState) : sharedState_(sharedState) { }

            virtual ~TestImageBuilder() { }

            virtual void UpdateSecuritySettings(Transport::SecuritySettings const &) { };

            virtual Common::ErrorCode VerifyImageBuilderSetup() { return Common::ErrorCodeValue::Success; }

            virtual void Enable() { }

            virtual void Disable() { }

            std::unique_ptr<TestImageBuilder> Clone();

            void MockUnprovisionApplicationType(
                ServiceModelTypeName const & typeName,
                ServiceModelVersion const & typeVersion);

            virtual Common::ErrorCode GetApplicationTypeInfo(
                std::wstring const & buildPath, 
                Common::TimeSpan const,
                __out ServiceModelTypeName &, 
                __out ServiceModelVersion &) override;

            virtual Common::ErrorCode BuildManifestContexts(
                std::vector<ApplicationTypeContext> const& applicationTypeContexts,
                Common::TimeSpan const timeout,
                __out std::vector<StoreDataApplicationManifest> & appManifestContexts,
                __out std::vector<StoreDataServiceManifest> & serviceManifestContexts);

            virtual Common::ErrorCode Test_BuildApplicationType(
                Common::ActivityId const & activityId,
                std::wstring const & buildPath,
                std::wstring const & downloadPath,
                ServiceModelTypeName const &,
                ServiceModelVersion const &,
                Common::TimeSpan const,
                __out std::vector<ServiceModelServiceManifestDescription> &,
                __out std::wstring &,
                __out ServiceModel::ApplicationHealthPolicy &,
                __out std::map<std::wstring, std::wstring> &);

            virtual Common::ErrorCode ValidateComposeFile(
                Common::ByteBuffer const &,
                Common::NamingUri const &,
                ServiceModelTypeName const &,
                ServiceModelVersion const &,
                Common::TimeSpan const &)
            {
                return Common::ErrorCodeValue::Success;
            }

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
                Common::AsyncOperationSPtr const &parent)
            {
                return Common::AsyncOperation::CreateAndStart<Common::CompletedAsyncOperation>(
                    Common::ErrorCodeValue::NotImplemented,
                    callback,
                    parent);
            }

            virtual Common::ErrorCode EndBuildComposeDeploymentType(
                Common::AsyncOperationSPtr const &operation,
                __out std::vector<ServiceModelServiceManifestDescription> &,
                __out std::wstring &,
                __out std::wstring &,
                __out ServiceModel::ApplicationHealthPolicy &,
                __out std::map<std::wstring, std::wstring> &,
                __out std::wstring &)
            {
                return Common::CompletedAsyncOperation::End(operation);
            }

            virtual Common::AsyncOperationSPtr BeginBuildComposeApplicationTypeForUpgrade(
                Common::ActivityId const &,
                Common::ByteBufferSPtr const &,
                Common::ByteBufferSPtr const &,
                std::wstring const &,
                std::wstring const &,
                bool,
                Common::NamingUri const&,
                ServiceModelTypeName const &,
                ServiceModelVersion const &,
                ServiceModelVersion const &,
                Common::TimeSpan const,
                Common::AsyncCallback const &callback,
                Common::AsyncOperationSPtr const &parent)
            {
                return Common::AsyncOperation::CreateAndStart<Common::CompletedAsyncOperation>(
                    Common::ErrorCodeValue::NotImplemented,
                    callback,
                    parent);
            }

            virtual Common::ErrorCode EndBuildComposeApplicationTypeForUpgrade(
                Common::AsyncOperationSPtr const &operation,
                __out std::vector<ServiceModelServiceManifestDescription> &,
                __out std::wstring &,
                __out std::wstring &,
                __out ServiceModel::ApplicationHealthPolicy &,
                __out std::map<std::wstring, std::wstring> &,
                __out std::wstring &)
            {
                return Common::CompletedAsyncOperation::End(operation);
            }

            virtual Common::AsyncOperationSPtr BeginBuildApplicationType(
                Common::ActivityId const &,
                std::wstring const &,
                std::wstring const &,
                ServiceModelTypeName const &,
                ServiceModelVersion const &,
                ProgressDetailsCallback const &,
                Common::TimeSpan const,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent)
            {
                return Common::AsyncOperation::CreateAndStart<Common::CompletedAsyncOperation>(
                    Common::ErrorCodeValue::NotImplemented,
                    callback,
                    parent);
            }

            virtual Common::ErrorCode EndBuildApplicationType(
                Common::AsyncOperationSPtr const & operation,
                __out std::vector<ServiceModelServiceManifestDescription> &,
                __out std::wstring &,
                __out std::wstring &,
                __out ServiceModel::ApplicationHealthPolicy &,
                __out std::map<std::wstring, std::wstring> &)
            {
                return Common::CompletedAsyncOperation::End(operation);
            }

            virtual Common::ErrorCode Test_BuildApplication(
                Common::NamingUri const &,
                ServiceModelApplicationId const &,
                ServiceModelTypeName const &,
                ServiceModelVersion const &,
                std::map<std::wstring, std::wstring> const &,
                Common::TimeSpan const,
                __out DigestedApplicationDescription &);

            Common::AsyncOperationSPtr BeginBuildSingleInstanceApplicationForUpgrade(
                Common::ActivityId const &,
                ServiceModelTypeName const &,
                ServiceModelVersion const & ,
                ServiceModelVersion const & ,
                ServiceModel::ModelV2::ApplicationDescription const &,
                Common::NamingUri const & ,
                ServiceModelApplicationId const & ,
                uint64 ,
                Common::TimeSpan const,
                Common::AsyncCallback const &callback,
                Common::AsyncOperationSPtr const &parent)
            {
                return Common::AsyncOperation::CreateAndStart<Common::CompletedAsyncOperation>(
                    Common::ErrorCodeValue::NotImplemented,
                    callback,
                    parent);
            }

            Common::ErrorCode EndBuildSingleInstanceApplicationForUpgrade(
                Common::AsyncOperationSPtr const &operation,
                __out std::vector<ServiceModelServiceManifestDescription> &,
                __out std::wstring & ,
                __out ServiceModel::ApplicationHealthPolicy &,
                __out std::map<std::wstring, std::wstring> &,
                __out DigestedApplicationDescription &,
                __out DigestedApplicationDescription &)
            {
                return Common::CompletedAsyncOperation::End(operation);
            }

            Common::AsyncOperationSPtr BeginBuildSingleInstanceApplication(
                Common::ActivityId const &,
                ServiceModelTypeName const &,
                ServiceModelVersion const &,
                ServiceModel::ModelV2::ApplicationDescription const &,
                Common::NamingUri const &,
                ServiceModelApplicationId const &,
                Common::TimeSpan const,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent) override
            {
                return Common::AsyncOperation::CreateAndStart<Common::CompletedAsyncOperation>(
                    Common::ErrorCodeValue::NotImplemented,
                    callback,
                    parent);
            }

            virtual Common::ErrorCode EndBuildSingleInstanceApplication(
                Common::AsyncOperationSPtr const & operation,
                __out std::vector<ServiceModelServiceManifestDescription> &,
                __out std::wstring &,
                __out ServiceModel::ApplicationHealthPolicy &,
                __out std::map<std::wstring, std::wstring> &,
                __out DigestedApplicationDescription &) override
            {
                return Common::CompletedAsyncOperation::End(operation);
            }

            virtual Common::AsyncOperationSPtr BeginBuildApplication(
                Common::ActivityId const &,
                Common::NamingUri const &,
                ServiceModelApplicationId const &,
                ServiceModelTypeName const &,
                ServiceModelVersion const &,
                std::map<std::wstring, std::wstring> const &,
                Common::TimeSpan const,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent)
            {
                return Common::AsyncOperation::CreateAndStart<Common::CompletedAsyncOperation>(
                    Common::ErrorCodeValue::NotImplemented,
                    callback,
                    parent);
            }

            virtual Common::ErrorCode EndBuildApplication(
                Common::AsyncOperationSPtr const & operation,
                __out DigestedApplicationDescription &)
            {
                return Common::CompletedAsyncOperation::End(operation);
            }

            virtual Common::AsyncOperationSPtr BeginUpgradeApplication(
                Common::ActivityId const &,
                Common::NamingUri const &,
                ServiceModelApplicationId const &,
                ServiceModelTypeName const &,
                ServiceModelVersion const &,
                uint64,
                std::map<std::wstring, std::wstring> const &,
                Common::TimeSpan const,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent)
            {
                return Common::AsyncOperation::CreateAndStart<Common::CompletedAsyncOperation>(
                    Common::ErrorCodeValue::NotImplemented,
                    callback,
                    parent);
            }

            virtual Common::ErrorCode EndUpgradeApplication(
                Common::AsyncOperationSPtr const & operation,
                __out DigestedApplicationDescription &,
                __out DigestedApplicationDescription &)
            {
                return Common::CompletedAsyncOperation::End(operation);
            }

            virtual Common::ErrorCode GetFabricVersionInfo(
                std::wstring const & codeFilepath,
                std::wstring const & clusterManifestFilepath,
                Common::TimeSpan const timeout,
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
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent)
            {
                return Common::AsyncOperation::CreateAndStart<Common::CompletedAsyncOperation>(
                    Common::ErrorCodeValue::NotImplemented,
                    callback,
                    parent);
            }

            virtual Common::ErrorCode EndCleanupApplication(
                Common::AsyncOperationSPtr const & operation)
            {
                return Common::CompletedAsyncOperation::End(operation);
            }

            virtual Common::ErrorCode Test_CleanupApplicationInstance(
                ServiceModel::ApplicationInstanceDescription const &,
                bool deleteApplicationPackage,
                std::vector<ServiceModel::ServicePackageReference> const &,
                Common::TimeSpan const timeout);

            virtual Common::AsyncOperationSPtr BeginCleanupApplicationInstance(
                Common::ActivityId const &,
                ServiceModel::ApplicationInstanceDescription const &,
                bool,
                std::vector<ServiceModel::ServicePackageReference> const &,
                Common::TimeSpan const,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent)
            {
                return Common::AsyncOperation::CreateAndStart<Common::CompletedAsyncOperation>(
                    Common::ErrorCodeValue::NotImplemented,
                    callback,
                    parent);
            }

            virtual Common::ErrorCode EndCleanupApplicationInstance(
                Common::AsyncOperationSPtr const & operation)
            {
                return Common::CompletedAsyncOperation::End(operation);
            }

            virtual Common::ErrorCode CleanupApplicationType(
                ServiceModelTypeName const &,
                ServiceModelVersion const &,
                std::vector<ServiceModelServiceManifestDescription> const &,
                bool isLastApplicationType,
                Common::TimeSpan const);

            virtual Common::ErrorCode CleanupFabricVersion(
                Common::FabricVersion const &,
                Common::TimeSpan const timeout);

            virtual Common::ErrorCode CleanupApplicationPackage(
                std::wstring const & buildPath,
                Common::TimeSpan const timeout) override;

            void MockProvision(
                std::wstring const & buildPath, 
                ServiceModelTypeName const & typeName, 
                ServiceModelVersion const & typeVersion);

            void AddPackage(
                Common::NamingUri const & appName,
                ServiceModelTypeName const &,
                ServiceModelPackageName const &);

            void AddTemplate(
                Common::NamingUri const & appName,
                Naming::PartitionedServiceDescriptor const &);

            void AddDefaultService(
                Common::NamingUri const & appName,
                Naming::PartitionedServiceDescriptor const &);

            void SetNextError(Common::ErrorCodeValue::Enum, int operationCount = 0);

            void SetServiceManifests(
                ServiceModelTypeName const &,
                ServiceModelVersion const &,
                std::vector<ServiceModelServiceManifestDescription> &&);

            SharedStateWrapper GetSharedState();

        public:
            //
            // TestImageBuilder is cloned for each FabricNode in the mock cluster
            //
            struct SharedState
            {
                SharedState() : NextError(Common::ErrorCodeValue::Success) { }

                std::map<ServiceModelTypeName, std::set<ServiceModelVersion>> ProvisionedAppTypes;
                std::map<ServiceModelTypeName, std::map<ServiceModelVersion, std::vector<ServiceModelServiceManifestDescription>>> AppTypeServiceManifests;
                std::map<Common::NamingUri, std::map<ServiceModelTypeName, ServiceModelPackageName>> AppPackageMap;
                std::map<Common::NamingUri, std::vector<Naming::PartitionedServiceDescriptor>> AppTemplatesMap;
                std::map<Common::NamingUri, std::vector<Naming::PartitionedServiceDescriptor>> AppDefaultServicesMap;

                Common::ErrorCodeValue::Enum NextError;
                int NextErrorOperationCount;
            };
            
            //
            // Give CIT access to shared state used by mock ImageBuilders across all FabricNodes
            //
            class SharedStateWrapper
            {
            public:
                SharedStateWrapper(SharedStateSPtr const & sharedState) : sharedState_(sharedState) { }

                bool TryGetServiceManifest(
                    ServiceModelTypeName const & appTypeName, 
                    ServiceModelVersion const & appTypeVersion,
                    std::wstring const & name,
                    std::wstring const & version,
                    __out ServiceModelServiceManifestDescription &);

            private:
                SharedStateSPtr sharedState_;
            };

        private:
            Common::ErrorCode GetNextError();

            static ServiceModelVersion GetServicePackageVersion();

            SharedStateSPtr sharedState_;
        };
    }
}
