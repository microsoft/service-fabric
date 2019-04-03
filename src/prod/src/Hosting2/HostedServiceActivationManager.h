// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class HostedServiceActivationManager
        : public Common::RootedObject
        , public Common::AsyncFabricComponent
        , public Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(HostedServiceActivationManager)

    public:
        HostedServiceActivationManager(Common::ComponentRoot const & root,
            FabricActivationManager const & fabricHost,
            bool hostedServiceActivateHidden = false,
            bool skipFabricSetup = false);
        virtual ~HostedServiceActivationManager();

        void OnConfigChange(std::wstring const & section, std::wstring const & key);

        __declspec(property(get=get_ActivateHidden)) bool ActivateHidden;
        bool get_ActivateHidden()  { return this->hostedServiceActivateHidden_; }

        __declspec(property(get=get_TransportAddress)) std::wstring const & TransportAddress;
        inline std::wstring const & get_TransportAddress() const { return fabricHost_.TransportAddress; };

        __declspec(property(get=get_EndpointProvider))  const HttpEndpointSecurityProviderUPtr & EndpointProvider;
        inline HttpEndpointSecurityProviderUPtr const & get_EndpointProvider() const { return endpointProvider_; };

        _declspec(property(get=get_ProcessActivator)) ProcessActivatorUPtr const & ProcessActivatorObj;
        inline ProcessActivatorUPtr const & get_ProcessActivator() const { return this->fabricHost_.ProcessActivatorObj; }

        void OnHostedServiceTerminated(
            Common::ErrorCode const & waitResult,
            DWORD exitCode,
            std::wstring const & serviceName,
            std::wstring const & serviceId);

        Common::ErrorCode CreateSecurityUser(
            wstring const & applicationId,
            wstring const & serviceNodeName,
            bool runasSpecified,
            ServiceModel::SecurityPrincipalAccountType::Enum runasAccountType,
            wstring const & runasAccountName,
            wstring const & runasPassword,
            bool runasPasswordEncrypted,
            __out SecurityUserSPtr & secUser);

        Common::ErrorCode CreateSecurityUser(std::wstring const & section, Common::StringMap const & entries, __out SecurityUserSPtr & secUser);
        Common::ErrorCode CreateSecurityUser(HostedServiceParameters const & params, __out SecurityUserSPtr & secUser);

        void GetHostedServiceCertificateThumbprint(Common::StringMap const & entries, __out std::wstring & thumbprint);

    protected:
        virtual Common::AsyncOperationSPtr OnBeginOpen(
            Common::TimeSpan,
            Common::AsyncCallback const &, 
            Common::AsyncOperationSPtr const &);
        virtual Common::ErrorCode OnEndOpen(Common::AsyncOperationSPtr const &);


        virtual Common::AsyncOperationSPtr OnBeginClose(
            Common::TimeSpan,
            Common::AsyncCallback const & , 
            Common::AsyncOperationSPtr const &);
        virtual Common::ErrorCode OnEndClose(Common::AsyncOperationSPtr const &);

        virtual void OnAbort();

    private:
        Common::ErrorCode HostedServiceActivationManager::ValidateRunasParams(
            std::wstring AccountName,
            ServiceModel::SecurityPrincipalAccountType::Enum AccountType,
            std::wstring Password);

        static void SendReply(
            Common::ErrorCode error,
            __in Transport::IpcReceiverContextUPtr & context);

        void EnableChangeMonitoring();

        HostedServiceSPtr CreateHostedService(std::wstring const & section, Common::StringMap const & entries);
        HostedServiceSPtr CreateHostedService(HostedServiceParameters const & request);
        Common::ErrorCode CreateContainerActivatorService(__out HostedServiceSPtr & containerActivatorSerivce);

        void NotifyHostedServiceTermination(std::wstring const & hostedServiceName);

        void RegisterIpcRequestHandler();
        void UnregisterIpcRequestHandler();

        void ProcessIpcMessage(
            __in Transport::Message & message, 
            __in Transport::IpcReceiverContextUPtr & context);

        void ProcessActivateHostedServiceRequest(
            __in Transport::Message & message, 
            __in Transport::IpcReceiverContextUPtr & context);

        void ProcessDeactivateHostedServiceRequest(
            __in Transport::Message & message, 
            __in Transport::IpcReceiverContextUPtr & context);

        void StartHostedService(
            __in HostedServiceSPtr const & hostedService,
            __in_opt Transport::IpcReceiverContextUPtr context = nullptr);
        void FinishStartHostedService(Common::AsyncOperationSPtr op, bool expectedCompletedSynchronously);

        void UpdateHostedService(
            __in HostedServiceSPtr const & hostedService,
            __in HostedServiceParameters const & params,
            __in Transport::IpcReceiverContextUPtr context);
        void UpdateHostedService(
            __in HostedServiceSPtr const & hostedService,
            __in std::wstring const & section);
        void FinishUpdateHostedService(Common::AsyncOperationSPtr op, bool expectedCompletedSynchronously);

        void StopHostedService(
            __in HostedServiceSPtr const & hostedService,
            __in bool stopGracefully,
            __in_opt Transport::IpcReceiverContextUPtr context = nullptr);
        void FinishStopHostedService(Common::AsyncOperationSPtr op, bool expectedCompletedSynchronously);
        bool IsFabricService(const std::wstring & hostedServiceName, const std::wstring & serviceNodeName);

    private:

        bool hostedServiceActivateHidden_;
        HostedServiceMapSPtr servicesMap_;
        FabricActivationManager const & fabricHost_;
        HttpEndpointSecurityProviderUPtr endpointProvider_;

        bool skipFabricSetup_;
        BOOL isSystem_;

        Common::RwLock lock_;

        class OpenAsyncOperation;
        friend class OpenAsyncOperation;

        class CloseAsyncOperation;
        friend class CloseAsyncOperation;

        class StartHostedServiceAsyncOperation;
        friend class StartHostedServiceAsyncOperation;

        class UpdateHostedServiceAsyncOperation;
        friend class UpdateHostedServiceAsyncOperation;

        class StopHostedServiceAsyncOperation;
        friend class StopHostedServiceAsyncOperation;
    };
}
