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
        
        virtual Common::AsyncOperationSPtr BeginApplicationHostCodePackageOperation(
            ApplicationHostCodePackageOperationRequest const & requestBody,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;

        virtual Common::ErrorCode EndApplicationHostCodePackageOperation(
            Common::AsyncOperationSPtr const & operation) override;

        virtual void SendDependentCodePackageEvent(
            CodePackageEventDescription const & eventDescription) override;

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
        __declspec(property(get = get_TypesToHost)) std::vector<GuestServiceTypeInfo> const & TypesToHost;
        std::vector<GuestServiceTypeInfo> const & get_TypesToHost() const { return codePackageInstance_->CodePackageObj.GuestServiceTypes; }

        __declspec(property(get = get_PackageDescription)) ServiceModel::ServicePackageDescription const & PackageDescription;
        ServiceModel::ServicePackageDescription const & get_PackageDescription() const 
        { 
            return codePackageInstance_->CodePackageObj.VersionedServicePackageObj.PackageDescription;
        }

        __declspec(property(get = get_TypeHostManager)) GuestServiceTypeHostManagerUPtr const & TypeHostManager;
        inline GuestServiceTypeHostManagerUPtr const & get_TypeHostManager() { return Hosting.GuestServiceTypeHostManagerObj; }

        __declspec(property(get = get_CodePackageInstanceIdentifier)) CodePackageInstanceIdentifier const & CodePackageInstanceId;
        inline CodePackageInstanceIdentifier const & get_CodePackageInstanceIdentifier() const { return codePackageInstance_->CodePackageInstanceId; }

    private:
        void NotifyCodePackageInstance();
        bool ShouldNotify();

    private:
        class OpenAsyncOperation;
        friend class OpenAsyncOperation;

        class CloseAsyncOperation;
        friend class CloseAsyncOperation;

        class ApplicationHostCodePackageAsyncOperation;
        friend class ApplicationHostCodePackageAsyncOperation;
    
    private:
        CodePackageActivationId codePackageActivationId_;
        CodePackageInstanceSPtr const codePackageInstance_;
        Common::atomic_bool isCodePackageActive_;
        Common::atomic_uint64 exitCode_;
        Common::RwLock notificationLock_;

        //
        // This flag is set to true after codePackageInstance_ has been
        // notified of termination. We can set the codePackageInstance_ to 
        // nullptr but this class has certain properties which may get
        // accessed after termination. As a conveinence we use this
        // flag for notification purposes and let the properties be accessed
        // without taking locks.
        //
        bool shouldNotify_;

        Common::atomic_bool terminatedExternally_;
        CodePackageRuntimeInformationSPtr codePackageRuntimeInformation_;
    };
}
