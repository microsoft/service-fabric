// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace CentralSecretService
    {
        class CentralSecretServiceReplica;
        using CloseReplicaCallback = std::function<void(Common::Guid const &)>;

        using StartRequestProcessHandler =
            std::function<Common::AsyncOperationSPtr(
                __in CentralSecretServiceReplica &,
                __in Transport::MessageUPtr,
                __in Transport::IpcReceiverContextUPtr,
                Common::TimeSpan const &,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &)>;

        using CompleteRequestProcessHandler =
            std::function<Common::ErrorCode(
                Common::AsyncOperationSPtr const & asyncOperation,
                __out Transport::MessageUPtr & replyMsgUPtr,
                __out Transport::IpcReceiverContextUPtr & receiverContextUPtr)>;

        class CentralSecretServiceReplica
            : public Store::KeyValueStoreReplica
            , protected Common::TextTraceComponent<Common::TraceTaskCodes::CentralSecretService>
        {
            using Common::TextTraceComponent<Common::TraceTaskCodes::CentralSecretService>::WriteNoise;
            using Common::TextTraceComponent<Common::TraceTaskCodes::CentralSecretService>::WriteInfo;
            using Common::TextTraceComponent<Common::TraceTaskCodes::CentralSecretService>::WriteWarning;
            using Common::TextTraceComponent<Common::TraceTaskCodes::CentralSecretService>::WriteError;
            using Common::TextTraceComponent<Common::TraceTaskCodes::CentralSecretService>::WriteTrace;

            DENY_COPY(CentralSecretServiceReplica);

        public:
            CentralSecretServiceReplica(
                Common::Guid const & partitionId,
                FABRIC_REPLICA_ID const & replicaId,
                std::wstring const & serviceName,
                __in SystemServices::ServiceRoutingAgentProxy & routingAgentProxy,
                Api::IClientFactoryPtr const & clientFactory,
                Federation::NodeInstance const & nodeInstance);

            virtual ~CentralSecretServiceReplica();

            __declspec(property(get = get_NodeInstance)) Federation::NodeInstance const & NodeInstance;
            Federation::NodeInstance const & get_NodeInstance() const { return this->nodeInstance_; }

            __declspec(property(get = get_ServiceName)) std::wstring const & ServiceName;
            std::wstring const & get_ServiceName() const { return this->serviceName_; }

            __declspec(property(get = get_SecretManager)) SecretManager & SecretManagerAgent;
            SecretManager & get_SecretManager() { return *(this->secretManager_); }

            __declspec(property(get = get_ResourceManager)) Management::ResourceManager::IpcResourceManagerService & ResourceManager;
            Management::ResourceManager::IpcResourceManagerService & get_ResourceManager() { return *(this->resourceManagerSvc_); }

            CloseReplicaCallback OnCloseReplicaCallback;

            Common::TimeSpan GetRandomizedOperationRetryDelay(Common::ErrorCode const &) const;
        protected:
            // *******************
            // StatefulServiceBase
            // *******************
            virtual Common::ErrorCode OnOpen(Common::ComPointer<IFabricStatefulServicePartition> const & servicePartition);
            virtual Common::ErrorCode OnChangeRole(::FABRIC_REPLICA_ROLE newRole, __out std::wstring & serviceLocation);
            virtual Common::ErrorCode OnClose();
            virtual void OnAbort();

        private:
            struct RequestProcessor
            {
            public:
                StartRequestProcessHandler const StartHandler;
                CompleteRequestProcessHandler const CompleteHandler;

                RequestProcessor(StartRequestProcessHandler const & startHandler, CompleteRequestProcessHandler const & completeHandler);
            };

            Management::ResourceManager::IpcResourceManagerServiceSPtr resourceManagerSvc_;
            Api::IClientFactoryPtr clientFactory_;
            SecretManagerUPtr secretManager_;

            // ****************
            // Message Handling
            // ****************
            void InitializeRequestHandlers();

            void RegisterMessageHandler();
            void UnregisterMessageHandler();

            void ProcessMessage(Transport::MessageUPtr &&, Transport::IpcReceiverContextUPtr &&);

            void OnProcessMessageComplete(
                CentralSecretServiceReplica::RequestProcessor const & requestProcessor,
                Common::ActivityId const &,
                Transport::MessageId const &,
                Common::AsyncOperationSPtr const &,
                bool expectedCompletedSynchronously);

            void AddHandler(
                wstring const & action,
                StartRequestProcessHandler const & startHandler,
                CompleteRequestProcessHandler const & completeHandler);

            Common::ErrorCode QuerySecretResourceMetadata(
                __in std::wstring const & resourceName,
                __in Common::ActivityId const & activityId,
                __out ResourceManager::ResourceMetadata & metadata);

            std::wstring const serviceName_;
            Federation::NodeInstance const nodeInstance_;
            SystemServices::SystemServiceLocation serviceLocation_;

            // The reference of the RoutingAgentProxy is held by CentralSecretServiceFactory
            SystemServices::ServiceRoutingAgentProxy & routingAgentProxy_;

            unordered_map<wstring, RequestProcessor> requestHandlers_;
        };
    }
}
