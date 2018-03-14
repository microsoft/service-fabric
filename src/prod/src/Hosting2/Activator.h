// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    // activates application and service packages with retries. This is used by
    // ApplicationManager to satisfy on-demand activation requests from RA.
    class Activator :
        public Common::RootedObject,
        public Common::FabricComponent,
        private Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(Activator)

    public:
        Activator(
            Common::ComponentRoot const & root,
            __in ApplicationManager & applicationManager);
        virtual ~Activator();

        Common::AsyncOperationSPtr BeginActivateApplication(
            ServiceModel::ApplicationIdentifier const & applicationId,
            ServiceModel::ApplicationVersion const & applicationVersion,
            std::wstring const & applicationName,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndActivateApplication(
            Common::AsyncOperationSPtr const & operation,
            __out OperationStatus & activationStatus,
            __out Application2SPtr & application);

        void EnsureActivationAsync(
            ServiceModel::ApplicationUpgradeSpecification const & appUpgradeSpec);

        Common::AsyncOperationSPtr BeginActivateServicePackageInstance(
            Application2SPtr const & application,
            ServicePackageInstanceIdentifier const & servicePackageInstanceId,
            ServiceModel::ServicePackageVersionInstance const & servicePackageVersionInstance,
            std::shared_ptr<ServiceModel::ServicePackageDescription> const & packageDescriptionPtr,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndActivateServicePackageInstance(
            Common::AsyncOperationSPtr const & operation,
            __out OperationStatus & activationStatus);

        // Note: The continuation token is expected to be the application name
        Common::ErrorCode GetApplicationsQueryResult(
            std::wstring const & filterApplicationName,
            __out std::vector<ServiceModel::DeployedApplicationQueryResult> & deployedApplications,
            std::wstring const & continuationToken = L"");

        Common::ErrorCode GetServiceManifestQueryResult(
            std::wstring const & filterApplicationName,
            std::wstring const & filterServiceManifestName,
            std::vector<ServiceModel::DeployedServiceManifestQueryResult> & deployedServiceManifests);

    protected:
        virtual Common::ErrorCode OnOpen();
        virtual Common::ErrorCode OnClose();
        virtual void OnAbort();

    private:
        Common::AsyncOperationSPtr BeginActivateApplication(
            ServiceModel::ApplicationIdentifier const & applicationId,
            ServiceModel::ApplicationVersion const & applicationVersion,
            std::wstring const & applicationName,
            ULONG maxFailureCount,
            bool onlyIfUsed,
            bool ensureLatestVersion,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::AsyncOperationSPtr BeginActivateServicePackageInstance(
            Application2SPtr const & application,
            ServicePackageInstanceIdentifier const & servicePackageInstanceId,
            ServiceModel::ServicePackageVersionInstance const & servicePackageVersionInstance,
            std::shared_ptr<ServiceModel::ServicePackageDescription> const & packageDescription,
            ULONG maxFailureCount,
            bool onlyIfUsed,
            bool ensureLatestVersion,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

    private:
        void ClosePendingActivations();

        static std::wstring GetOperationId(ServiceModel::ApplicationIdentifier const & applicationId, ServiceModel::ApplicationVersion const & applicationVersion);
        static std::wstring GetOperationId(ServicePackageInstanceIdentifier const & servicePackageInstanceId, ServiceModel::ServicePackageVersionInstance const & servicePackageVersion);

    private:
        class ActivateAsyncOperation;
        class ActivateApplicationAsyncOperation;
        class ActivateServicePackageInstanceAsyncOperation;
        class EnsureActivationAsyncOperation;
        struct ActivateServicePackageAsyncOperationArgs;

    private:
        ApplicationManager & applicationManager_;
        PendingOperationMapUPtr pendingActivations_;
        Common::atomic_uint64 activationInstanceId_;
    };
}
