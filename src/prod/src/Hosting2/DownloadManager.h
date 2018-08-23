// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    struct FileDownloadSpec
    {
    public:
        FileDownloadSpec()
            : RemoteSourceLocation(),
            LocalDestinationLocation(),
            ChecksumFileLocation(),
            Checksum(),
            DownloadToCacheOnly(false)
        {
        }

        std::wstring RemoteSourceLocation;
        std::wstring LocalDestinationLocation;
        std::wstring ChecksumFileLocation;
        std::wstring Checksum;
        bool DownloadToCacheOnly;
    };
    // downloads application and service package from imagestore
    class DownloadManager :
        public Common::RootedObject,
        public Common::AsyncFabricComponent,
        private Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(DownloadManager)

    public:
        DownloadManager(
            Common::ComponentRoot const & root,
            __in HostingSubsystem & hosting,
            Common::FabricNodeConfigSPtr const &);
        virtual ~DownloadManager();

        Common::AsyncOperationSPtr BeginDownloadApplication(
            ApplicationDownloadSpecification const & appDownloadSpec,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndDownloadApplication(
            Common::AsyncOperationSPtr const & operation,
            __out OperationStatusMapSPtr & appDownloadStatus);

        Common::AsyncOperationSPtr BeginDownloadApplicationPackage(
            ServiceModel::ApplicationIdentifier const & applicationId,
            ServiceModel::ApplicationVersion const & applicationVersion,
            ServiceModel::ServicePackageActivationContext const& activationContext,
            std::wstring const & applicationName,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::AsyncOperationSPtr BeginDownloadApplicationPackage(
            ServiceModel::ApplicationIdentifier const & applicationId,
            ServiceModel::ApplicationVersion const & applicationVersion,
            ServiceModel::ServicePackageActivationContext const& activationContext,
            std::wstring const & applicationName,
            ULONG maxFailureCount,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndDownloadApplicationPackage(
            Common::AsyncOperationSPtr const & operation,
            __out OperationStatus & downloadStatus,
            __out ServiceModel::ApplicationPackageDescription & applicationPackageDescription);

        Common::AsyncOperationSPtr BeginDownloadServicePackage(
            ServiceModel::ServicePackageIdentifier const & servicePackageId,
            ServiceModel::ServicePackageVersion const & servicePackageVersion,
            ServiceModel::ServicePackageActivationContext const & activationContext,
            std::wstring const & applicationName,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::AsyncOperationSPtr BeginDownloadServicePackage(
            ServiceModel::ServicePackageIdentifier const & servicePackageId,
            ServiceModel::ServicePackageVersion const & servicePackageVersion,
            ServiceModel::ServicePackageActivationContext const & activationContext,
            std::wstring const & applicationName,
            ULONG maxFailureCount,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndDownloadServicePackage(
            Common::AsyncOperationSPtr const & operation,
            __out OperationStatus & downloadStatus,
            __out ServiceModel::ServicePackageDescription & servicePackageDescription);

        Common::AsyncOperationSPtr BeginDownloadFabricUpgradePackage(
            Common::FabricVersion const & fabricVersion,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndDownloadFabricUpgradePackage(Common::AsyncOperationSPtr const & operation);

        //TODO: #12372718: Add a new API to force download the package during predeployment on node
        Common::AsyncOperationSPtr BeginDownloadServiceManifestPackages(
            std::wstring const & applicationTypeName,
            std::wstring const & applicationTypeVersion,
            std::wstring const & serviceManifestName,
            ServiceModel::PackageSharingPolicyList const & packageSharingPolicies,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndDownloadServiceManifestPackages(
            Common::AsyncOperationSPtr const & operation);

        // Note: The continuation token is expected to be the application name
        Common::ErrorCode GetApplicationsQueryResult(
            std::wstring const & filterApplicationName,
            __out std::vector<ServiceModel::DeployedApplicationQueryResult> & deployedApplications,
            wstring const & continuationToken);
        Common::ErrorCode GetServiceManifestQueryResult(
            std::wstring const & filterApplicationName,
            std::wstring const & filterServiceManifestName,
            std::vector<ServiceModel::DeployedServiceManifestQueryResult> & deployedServiceManifests);

        void InitializePassThroughClientFactory(Api::IClientFactoryPtr const &passThroughClientFactoryPtr);

        //
        // Test hooks
        //
        std::vector<std::wstring> Test_GetNtlmUserThumbprints() const;

        __declspec(property(get = test_Get_ImageStore)) Management::ImageStore::ImageStoreUPtr const & ImageStore;
        Management::ImageStore::ImageStoreUPtr const & test_Get_ImageStore() const { return imageStore_; }

    protected:
        virtual Common::AsyncOperationSPtr OnBeginOpen(
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        virtual Common::ErrorCode OnEndOpen(
            Common::AsyncOperationSPtr const & operation);

        virtual Common::AsyncOperationSPtr OnBeginClose(
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        virtual Common::ErrorCode OnEndClose(
            Common::AsyncOperationSPtr const & operation);

        virtual void OnAbort();

        Common::ErrorCode DeleteCorruptedFile(
            std::wstring const & localFilePath,
            std::wstring const & remoteObject,
            std::wstring const & type);

    private:
        __declspec(property(get=get_RunLayout)) Management::ImageModel::RunLayoutSpecification const & RunLayout;
        Management::ImageModel::RunLayoutSpecification const & get_RunLayout() const { return this->runLayout_; }

        __declspec(property(get=get_StoreLayout)) Management::ImageModel::StoreLayoutSpecification const & StoreLayout;
        Management::ImageModel::StoreLayoutSpecification const & get_StoreLayout() const { return this->storeLayout_; }

        __declspec(property(get=get_SharedLayout)) Management::ImageModel::StoreLayoutSpecification const & SharedLayout;
        Management::ImageModel::StoreLayoutSpecification const & get_SharedLayout() const { return this->sharedLayout_; }

        void ClosePendingDownloads();
        Common::ErrorCode DeployFabricSystemPackages(Common::TimeSpan timeout);

        using RefreshNTLMSecurityUsersCallback = std::function<void(Common::AsyncOperationSPtr const & parent, Common::ErrorCode const &)>;
        void StartRefreshNTLMSecurityUsers(
            bool updateExisting,
            Common::TimeSpan const timeout,
            RefreshNTLMSecurityUsersCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        void OnRefreshNTLMSecurityUsersCompleted(
            Common::AsyncOperationSPtr const & operation,
            RefreshNTLMSecurityUsersCallback const & callback,
            bool expectedCompletedSynchronously);

        void ScheduleRefreshNTLMSecurityUsersTimer();
        void CloseRefreshNTLMSecurityUsersTimer();
        void RefreshNTLMSecurityUsers();

        Common::ErrorCode EnsureApplicationFolders(std::wstring const & applicationId, bool createLogFolder = true);
        Common::ErrorCode EnsureApplicationLogFolder(std::wstring const & applicationId);

        static std::wstring GetOperationId(ServiceModel::ApplicationIdentifier const & applicationId, ServiceModel::ApplicationVersion const & applicationVersion);
        static std::wstring GetOperationId(ServiceModel::ServicePackageIdentifier const & servicePackageId, ServiceModel::ServicePackageVersion const & servicePackageVersion);
        static std::wstring GetOperationId(Common::FabricVersion const & fabricVersion);
        static std::wstring GetOperationId(std::wstring const & applicationTypeName, std::wstring const & applicationTypeVersion, std::wstring const & serviceManifestName);

    private:
        class OpenAsyncOperation;
        class CloseAsyncOperation;
        class DownloadAsyncOperation;
        class DownloadApplicationPackageAsyncOperation;
        class DownloadServicePackageAsyncOperation;
        class DownloadApplicationAsyncOperation;
        class DownloadFabricUpgradePackageAsyncOperation;
        class DownloadServiceManifestAsyncOperation;
        class DownloadContainerImagesAsyncOperation;
        class DownloadApplicationPackageContentsAsyncOperation;
        class DownloadServicePackageContentsAsyncOperation;
        class DownloadServiceManifestContentsAsyncOperation;
        class DownloadPackagesAsyncOperation;

    private:
        HostingSubsystem & hosting_;
        Management::ImageModel::RunLayoutSpecification const runLayout_;
        Management::ImageModel::StoreLayoutSpecification const storeLayout_;
        Management::ImageModel::WinFabRunLayoutSpecification const fabricUpgradeRunLayout_;
        Management::ImageModel::WinFabStoreLayoutSpecification const fabricUpgradeStoreLayout_;
        Management::ImageModel::StoreLayoutSpecification const sharedLayout_;
        PendingOperationMapUPtr pendingDownloads_;
        Management::ImageStore::ImageStoreUPtr imageStore_;
        ImageCacheManagerUPtr imageCacheManager_;
        Common::FabricNodeConfigSPtr nodeConfig_;
        Api::IClientFactoryPtr passThroughClientFactoryPtr_;
        Common::SynchronizedMap<std::wstring, Common::ErrorCode> nonRetryableFailedDownloads_;

        // Remember accounts already configured; on refresh, we need to create accounts based only on the new certificates found.
        std::vector<std::wstring> ntlmUserThumbprints_;
        // New accounts found, but not yet configured by ApplicationPrincipals
        std::unique_ptr<std::vector<std::wstring>> pendingNTLMUserThumbprints_;
        Common::TimerSPtr refreshNTLMUsersTimer_;
        MUTABLE_RWLOCK(DownloadManagerPrincipalLock, refreshNTLMUsersTimerLock_);
    };
}
