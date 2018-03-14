// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class SingleCodePackageApplicationHostProxy :
        public ApplicationHostProxy
    {
        DENY_COPY(SingleCodePackageApplicationHostProxy)

    public:
        SingleCodePackageApplicationHostProxy(
            HostingSubsystemHolder const & hostingHolder,
            std::wstring const & hostId,
            ApplicationHostIsolationContext const & isolationContext,
            CodePackageInstanceSPtr const & codePackageInstance);

        virtual ~SingleCodePackageApplicationHostProxy();

        __declspec(property(get = get_ApplicationName)) std::wstring const & ApplicationName;
        inline std::wstring const & get_ApplicationName() const { return codePackageInstance_->Context.ApplicationName; }

        __declspec(property(get = get_PortBindings)) map<wstring, wstring> const & PortBindings;
        inline map<wstring, wstring> const & get_PortBindings() const { return codePackageInstance_->PortBindings; }

        __declspec(property(get = get_ContainerPolicies)) ServiceModel::ContainerPoliciesDescription const & ContainerPolicies;
        inline ServiceModel::ContainerPoliciesDescription const & get_ContainerPolicies() const { return codePackageInstance_->ContainerPolicies; }

        __declspec(property(get = get_CodePackageInstanceIdentifier)) CodePackageInstanceIdentifier const & CodePackageInstanceId;
        inline CodePackageInstanceIdentifier const & get_CodePackageInstanceIdentifier() const { return codePackageInstance_->CodePackageInstanceId; }

        virtual Common::AsyncOperationSPtr BeginActivateCodePackage(
            CodePackageInstanceSPtr const & codePackage,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;

        virtual Common::ErrorCode EndActivateCodePackage(
            Common::AsyncOperationSPtr const & operation,
            __out CodePackageActivationId & activationId,
            __out CodePackageRuntimeInformationSPtr & codePackageRuntimeInformation) override;

        virtual Common::AsyncOperationSPtr BeginDeactivateCodePackage(
            CodePackageInstanceIdentifier const & codePackageInstanceId,
            CodePackageActivationId const & activationId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;

        virtual Common::ErrorCode EndDeactivateCodePackage(
            Common::AsyncOperationSPtr const & operation) override;

        virtual Common::AsyncOperationSPtr BeginGetContainerInfo(
            wstring const & containerInfoType,
            wstring const & containerInfoArgs,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;

        virtual Common::ErrorCode EndGetContainerInfo(
            Common::AsyncOperationSPtr const & operation,
            __out wstring & containerInfo) override;
        
        virtual bool HasHostedCodePackages() override;

        virtual void OnApplicationHostRegistered() override;
        virtual void OnApplicationHostTerminated(DWORD exitCode) override;

        virtual void OnContainerHealthCheckStatusChanged(ContainerHealthStatusInfo const & healthStatusInfo) override;

        virtual void OnCodePackageTerminated(
            CodePackageInstanceIdentifier const & codePackageInstanceId,
            CodePackageActivationId const & activationId) override;

        std::wstring GetUniqueContainerName();

    protected:
        virtual Common::AsyncOperationSPtr OnBeginOpen(
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) override;

        virtual Common::ErrorCode OnEndOpen(Common::AsyncOperationSPtr const &) override;

        virtual Common::AsyncOperationSPtr OnBeginClose(
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) override;

        virtual Common::ErrorCode OnEndClose(Common::AsyncOperationSPtr const &) override;

        virtual void OnAbort() override;

    private:
        Common::ErrorCode GetProcessDescription(__out ProcessDescriptionUPtr & processDescription);
        Common::ErrorCode GetAssignedIPAddress(__out wstring & assignedIP);
        void NotifyTermination();
        std::wstring GetContainerGroupId();
        std::wstring GetServiceName();
    
    private:
        class OpenAsyncOperation;
        friend class OpenAsyncOperation;

        class CloseAsyncOperation;
        friend class CloseAsyncOperation;
        
        class GetContainerInfoAsyncOperation;

    private:
        CodePackageActivationId const codePackageActivationId_;
        CodePackageInstanceSPtr const codePackageInstance_;
        CodePackageRuntimeInformationSPtr codePackageRuntimeInformation_;

        Common::RwLock notificationLock_;
        Common::atomic_bool terminatedExternally_;
        Common::atomic_bool isCodePackageActive_;
        Common::atomic_uint64 exitCode_;
        Common::ProcessWaitSPtr apphostMonitor_;
        Common::HandleUPtr procHandle_;
    };
}
