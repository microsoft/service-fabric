// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class MultiCodePackageApplicationHostProxy :
        public ApplicationHostProxy
    {
        DENY_COPY(MultiCodePackageApplicationHostProxy)

    public:
        MultiCodePackageApplicationHostProxy(
            HostingSubsystemHolder const & hostingHolder,
            std::wstring const & hostId,
            ApplicationHostIsolationContext const & isolationContext,
            std::wstring const & runAsId,
            ServicePackageInstanceIdentifier const & servicePackageInstanceId,
            bool removeServiceFabricRuntimeAccess);
        
        virtual ~MultiCodePackageApplicationHostProxy();

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
        void NotifyTermination(std::vector<std::pair<CodePackageActivationId, CodePackageInstanceSPtr>> const & hostedCodePackages);
    
    private:
        class OpenAsyncOperation;
        friend class OpenAsyncOperation;

        class CloseAsyncOperation;
        friend class CloseAsyncOperation;;

        class ActivateCodePackageAsyncOperation;
        class DeactivateCodePackageAsyncOperation;
        class CodePackageTable;

    private:
        Common::AsyncManualResetEvent hostRegisteredEvent_;
        CodePackageRuntimeInformationSPtr codePackageRuntimeInformation_;
        std::unique_ptr<CodePackageTable> hostedCodePackageTable_;
        Common::atomic_bool terminatedExternally_;
        Common::atomic_uint64 exitCode_;
        Common::ProcessWaitSPtr apphostMonitor_;
        Common::HandleUPtr procHandle_;
    };
}
