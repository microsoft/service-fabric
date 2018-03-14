// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class InProcessApplicationHostProxy : public ApplicationHostProxy
    {
        DENY_COPY(InProcessApplicationHostProxy)

    public:
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

        virtual Common::AsyncOperationSPtr BeginUpdateCodePackageContext(
            CodePackageContext const & codePackageContext,
            CodePackageActivationId const & codePackageActivationId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;

        virtual Common::ErrorCode EndUpdateCodePackageContext(
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

        virtual void TerminateExternally() override;

        InProcessApplicationHostProxy(
            HostingSubsystemHolder const & hostingHolder,
            std::wstring const & hostId,
            ApplicationHostIsolationContext const & isolationContext, // TODO: This should not be needed.
            CodePackageInstanceSPtr const & codePackageInstance);

        virtual ~InProcessApplicationHostProxy();

    protected:

        virtual Common::AsyncOperationSPtr OnBeginOpen(
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;

        virtual Common::ErrorCode OnEndOpen(Common::AsyncOperationSPtr const &) override;

        virtual Common::AsyncOperationSPtr OnBeginClose(
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;

        virtual Common::ErrorCode OnEndClose(Common::AsyncOperationSPtr const &) override;

        virtual void OnAbort() override;
    
    private:
        __declspec(property(get = get_GuestServiceTypeNames)) std::vector<std::wstring> const & TypesToHost;
        std::vector<std::wstring> const & get_GuestServiceTypeNames() const { return codePackageInstance_->CodePackageObj.GuestServiceTypes; }

        __declspec(property(get = get_TypeHostManager)) GuestServiceTypeHostManagerUPtr const & TypeHostManager;
        inline GuestServiceTypeHostManagerUPtr const & get_TypeHostManager() { return Hosting.GuestServiceTypeHostManagerObj; }

    private:
        void NotifyCodePackageInstance();

    private:
        class OpenAsyncOperation;
        friend class OpenAsyncOperation;

        class CloseAsyncOperation;
        friend class CloseAsyncOperation;
    
    private:
        CodePackageActivationId codePackageActivationId_;
        CodePackageInstanceSPtr const codePackageInstance_;
        Common::atomic_bool isCodePackageActive_;
        Common::atomic_uint64 exitCode_;
        Common::RwLock notificationLock_;
        Common::atomic_bool terminatedExternally_;
        CodePackageRuntimeInformationSPtr codePackageRuntimeInformation_;
    };
}
