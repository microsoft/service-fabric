// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class EntreeServiceProxy
      : public Common::FabricComponent
      , public Api::IClaimsTokenAuthenticator
      , public Client::HealthReportingTransport
      , public Federation::NodeTraceComponent<Common::TraceTaskCodes::EntreeServiceProxy>
    {
        DENY_COPY(EntreeServiceProxy)

    public:
        EntreeServiceProxy(
            Common::ComponentRoot const & root,
            std::wstring const & listenAddress,
            ProxyToEntreeServiceIpcChannel &proxyToEntreeServiceTransport,
            Transport::SecuritySettings const& securitySettings);

        ~EntreeServiceProxy();

        __declspec(property(get=get_Id)) Federation::NodeId Id;
        Federation::NodeId get_Id() const { return proxyToEntreeServiceTransport_.Id; }

        __declspec (property(get=get_Instance)) Federation::NodeInstance Instance;
        Federation::NodeInstance get_Instance() const { return proxyToEntreeServiceTransport_.Instance; }

        __declspec (property(get=get_NodeName)) std::wstring const & NodeName;
        std::wstring const & get_NodeName() const { return proxyToEntreeServiceTransport_.NodeName; }

        __declspec (property(get=get_InstanceString)) std::wstring & InstanceString;
        std::wstring & get_InstanceString() { return instanceString_; }

        __declspec(property(get=get_ReceivingChannel)) Transport::IDatagramTransportSPtr const & ReceivingChannel;
        Transport::IDatagramTransportSPtr const & get_ReceivingChannel() const { return receivingChannel_; }

        __declspec(property(get=get_DemuxChannel)) Transport::DemuxerUPtr & DemuxChannel;
        Transport::DemuxerUPtr & get_DemuxChannel() { return demuxer_; }

        Common::AsyncOperationSPtr BeginProcessRequest(
            Transport::MessageUPtr && message,
            Transport::ISendTarget::SPtr const &,
            Common::TimeSpan const&,
            Common::AsyncCallback const &, 
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndProcessRequest(
            Common::AsyncOperationSPtr const &,
            __out Transport::MessageUPtr &);

        void OnConnectionAccepted(Transport::ISendTarget const &);
        void OnConnectionFault(Transport::ISendTarget const &, Common::ErrorCode const &);
        Common::ErrorCode SetSecurity(Transport::SecuritySettings const & securitySettings);

        __declspec (property(get=get_ListenAddress)) std::wstring const& ListenAddress;
        std::wstring const& get_ListenAddress() const { return listenAddress_; }

        // IClaimsAuthenticator functions
        Common::AsyncOperationSPtr BeginAuthenticate(
            std::wstring const& inputToken,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const& callback,
            Common::AsyncOperationSPtr const& parent);

        Common::ErrorCode EndAuthenticate(
            Common::AsyncOperationSPtr const& operation,
            __out Common::TimeSpan &expirationTime,
            __out std::vector<ServiceModel::Claim> &claimsList);

        // HealthReportingTransport
        Common::AsyncOperationSPtr BeginReport(
            Transport::MessageUPtr && message,
            Common::TimeSpan,
            Common::AsyncCallback const &, 
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndReport(
            Common::AsyncOperationSPtr const &,
            __out Transport::MessageUPtr &);

    protected:
  	        Common::ErrorCode OnOpen(); 	
  	        Common::ErrorCode OnClose();
  	        void OnAbort();

        // Used by CIT
        virtual void ProcessClientMessage(
            Transport::MessageUPtr && message,
            Transport::ISendTarget::SPtr const & replyTarget);

    private:

        class RequestAsyncOperationBase;
        class ClientRequestJobItem;

        void OnProcessClientMessageComplete(
            Transport::MessageId,
            Transport::FabricActivityHeader const &,
            Transport::ISendTarget::SPtr const &,
            Common::AsyncOperationSPtr const &,
            std::wstring const &,
            bool);

        void ProcessEntreeServiceMessage(
            Transport::MessageUPtr &&message,
            Transport::IpcReceiverContextUPtr &context);
        
        void ReplyWithError(Transport::ISendTarget::SPtr const &, Transport::MessageId const &, Common::ErrorCode &&);
        
        void RegisterMessageHandlers();

        bool AccessCheck(Transport::MessageUPtr &message);
        bool DefaultActionHeaderBasedAccessCheck(Transport::MessageUPtr & message, Transport::RoleMask::Enum);
        bool ForwardToServiceAccessCheck(Transport::MessageUPtr & message);
        bool ForwardToFileStoreAccessCheck(Transport::MessageUPtr & message);
        bool PropertyBatchAccessCheck(Transport::MessageUPtr & message);
        bool QueryAccessCheck(Transport::MessageUPtr & message);
        bool RoutingAgentAccessCheck(Transport::MessageUPtr & message, SystemServices::ServiceRoutingAgentHeader const & routingAgentHeader);

        void AddRoleBasedAccessControlHandlers();

        void ClaimsHandler(
            __in Api::IClaimsTokenAuthenticator *tokenAuthenticator,
            std::wstring const& token, 
            Transport::SecurityContextSPtr const& context);

        Api::IClaimsTokenAuthenticator* CreateClaimsTokenAuthenticator();
        
        void OnAuthenticateComplete(
            __in Api::IClaimsTokenAuthenticator *tokenAuthenticator,
            Common::AsyncOperationSPtr const& operation, 
            Transport::SecurityContextSPtr const& context);

        void LoadIsAadServiceEnabled();

        void ValidateAadToken(
            std::wstring const & jwt,
            Transport::SecurityContextSPtr const& context);

        Common::ErrorCode InitializeExternalChannel();

   	    NamingConfig const &namingConfig_;
        std::wstring listenAddress_;
        std::wstring instanceString_;
        ProxyToEntreeServiceIpcChannel &proxyToEntreeServiceTransport_;
        Transport::IDatagramTransportSPtr receivingChannel_;
        Transport::DemuxerUPtr demuxer_;
        Transport::RequestReplySPtr requestReply_;
        Transport::SecuritySettings initialSecuritySettings_;
        FileTransferGatewayUPtr fileTransferGatewayUPtr_;
        std::unique_ptr<ServiceNotificationManagerProxy> notificationManagerProxyUPtr_;

        Common::RwLock roleBasedAccessCheckMapLock_;       
        typedef std::function<bool (Transport::MessageUPtr & message)> AccessCheckFunction;
        std::map<std::wstring, AccessCheckFunction> roleBasedAccessCheckMap_;
        Api::IClaimsTokenAuthenticatorSPtr testTokenAuthenticator_; 
        std::unique_ptr<Common::CommonJobQueue<EntreeServiceProxy>> clientRequestQueue_;

        GatewayPerformanceCountersSPtr perfCounters_;
        size_t connectedClientCount_;
        Common::RwLock connectedClientCountLock_;
        bool isAadServiceEnabled_;
        bool isClientRoleInEffect_ = Common::SecurityConfig::GetConfig().IsClientRoleInEffect();
    };
}
