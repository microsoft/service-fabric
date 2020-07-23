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

        __declspec(property(get = get_ServicePackageInstanceId)) ServicePackageInstanceIdentifier const & ServicePackageInstanceId;
        inline ServicePackageInstanceIdentifier const & get_ServicePackageInstanceId() const 
        { 
            return codePackageInstance_->CodePackageInstanceId.ServicePackageInstanceId; 
        }

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

        std::wstring GetUniqueContainerName();

        virtual bool GetLinuxContainerIsolation() override;

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

        Common::ErrorCode GetContainerNetworkConfigDescription(
            __out ContainerNetworkConfigDescription & networkConfig,
            __out wstring & assignedIpAddresses);

        Common::ErrorCode GetAssignedNetworkResources(
            __out wstring & openNetworkAssignedIpAddress,
            __out std::map<std::wstring, std::wstring> & overlayNetworkResources,
            __out wstring & assignedIpAddresses);

        Common::ErrorCode GetContainerDnsServerConfiguration(
            ContainerNetworkConfigDescription const & networkConfig,
            __out vector<wstring> & dnsServers);

        std::wstring GetUniqueName();
        void NotifyTermination();
        std::wstring GetContainerGroupId();
        std::wstring GetServiceName();
        void EmitDiagnosticTrace();
        bool ShouldNotify();

    private:
        class GetProcessAndContainerDescriptionAsyncOperation;

        class OpenAsyncOperation;
        friend class OpenAsyncOperation;

        class CloseAsyncOperation;
        friend class CloseAsyncOperation;

        class ApplicationHostCodePackageOperation;
        class GetContainerInfoAsyncOperation;
        class SendDependentCodePackageEventAsyncOperation;

    private:
        CodePackageActivationId const codePackageActivationId_;
        CodePackageInstanceSPtr const codePackageInstance_;
        CodePackageRuntimeInformationSPtr codePackageRuntimeInformation_;
        ContainerNetworkConfigDescription networkConfig_;

        //
        // This flag is set to true after codePackageInstance_ has been
        // notified of termination. We can set the codePackageInstance_ to 
        // nullptr but this class has certain properties which may get
        // accessed after termination. As a conveinence we use this
        // flag for notification purposes and let the properties be accessed
        // without taking locks.
        //
        bool shouldNotify_;

        Common::atomic_bool activationFailed_;
        Common::ErrorCode activationFailedError_;

        Common::RwLock notificationLock_;
        Common::atomic_bool terminatedExternally_;
        Common::atomic_bool isCodePackageActive_;
        Common::atomic_uint64 exitCode_;
        Common::HandleUPtr procHandle_;
    };
}