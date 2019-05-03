// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class EnvironmentManager :
        public Common::RootedObject,
        public Common::AsyncFabricComponent,
        protected Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(EnvironmentManager)

    public:
        EnvironmentManager(
            Common::ComponentRoot const & root,
            __in HostingSubsystem & hosting);
        virtual ~EnvironmentManager();

        __declspec(property(get=get_Hosting)) HostingSubsystem const & Hosting;
        HostingSubsystem const & get_Hosting() { return this->hosting_; }


        __declspec(property(get=get_CurrentUserSid)) std::wstring const & CurrentUserSid;
        std::wstring const & get_CurrentUserSid() { return this->currentUserSid_; }

        __declspec(property(get=get_IsSystem)) bool const & IsSystem;
        bool const & get_IsSystem() { return this->isSystem_; }

        // ****************************************************
        // Application environment specific methods
        // ****************************************************
        Common::AsyncOperationSPtr BeginSetupApplicationEnvironment(
            ServiceModel::ApplicationIdentifier const & applicationId,
            ServiceModel::ApplicationPackageDescription const & appPackageDescription,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndSetupApplicationEnvironment(
            Common::AsyncOperationSPtr const & asyncOperation,
            __out ApplicationEnvironmentContextSPtr & context);

        Common::AsyncOperationSPtr BeginCleanupApplicationEnvironment(
            ApplicationEnvironmentContextSPtr const & context,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndCleanupApplicationEnvironment(
            Common::AsyncOperationSPtr const & asyncOperation);

        void AbortApplicationEnvironment(ApplicationEnvironmentContextSPtr const & appEnvironmentContext);

        // ****************************************************
        // ServicePackageInstance environment specific methods
        // ****************************************************

        Common::AsyncOperationSPtr BeginSetupServicePackageInstanceEnvironment(
            ApplicationEnvironmentContextSPtr const & appEnvironmentContext,
            wstring const & applicationName,
            ServicePackageInstanceIdentifier const & servicePackageInstanceId,
            int64 instanceId,
            ServiceModel::ServicePackageDescription const & servicePackageDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndSetupServicePackageInstanceEnvironment(
            Common::AsyncOperationSPtr const & asyncOperation,
            _Out_ ServicePackageInstanceEnvironmentContextSPtr & packageEnvironmentContext);

        Common::AsyncOperationSPtr BeginCleanupServicePackageInstanceEnvironment(
            ServicePackageInstanceEnvironmentContextSPtr const & packageEnvironmentContext,
            ServiceModel::ServicePackageDescription const & servicePackageDescription,
            int64 instanceId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndCleanupServicePackageInstanceEnvironment(
            Common::AsyncOperationSPtr const & asyncOperation);

        void AbortServicePackageInstanceEnvironment(
            ServicePackageInstanceEnvironmentContextSPtr const & packageEnvironmentContext,
            ServiceModel::ServicePackageDescription const & packageDescription,
            int64 instanceId);

    protected:
        // ****************************************************
        // AsyncFabricComponent specific methods
        // ****************************************************
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

        Common::ErrorCode GetIsolatedNicIpAddress(std::wstring & ipAddress);

    private:
        Common::ErrorCode CleanupServicePackageInstanceEnvironment(
            ServicePackageInstanceEnvironmentContextSPtr const & packageEnvironmentContext,
            ServiceModel::ServicePackageDescription const & packageDescription,
            int64 servicePackageInstanceId);

        Common::ErrorCode GetEndpointBindingPolicies(
            std::vector<EndpointResourceSPtr> const & endpointResources,
            std::map<std::wstring, ServiceModel::EndpointCertificateDescription> digestedCertificates,
            std::vector<EndpointCertificateBinding> & bindings);

        static std::wstring GetServicePackageIdentifier(
            std::wstring const & servicePackageId,
            int64 instanceId);

    private:
        class OpenAsyncOperation;
        class CloseAsyncOperation;
        class SetupApplicationAsyncOperation;
        class CleanupApplicationAsyncOperation;
        class SetupServicePackageInstanceAsyncOperation;
        class CleanupServicePackageInstanceAsyncOperation;

    private:
        HostingSubsystem & hosting_;

        std::wstring currentUserSid_;
        bool isSystem_;

        // Provider that deals with settings up log collection;
        // the actual type depends on the specified infrastructure (Azure, etc)
        ILogCollectionProviderUPtr logCollectionProvider_;

        // Provider that configures endpoints
        EndpointProviderUPtr endpointProvider_;

        // Provider that configures ETW trace session
        EtwSessionProviderUPtr etwSessionProvider_;

        // Provider that configures crash dump collection
        CrashDumpProviderUPtr crashDumpProvider_;

        bool isAdminUser_;
    };
}
