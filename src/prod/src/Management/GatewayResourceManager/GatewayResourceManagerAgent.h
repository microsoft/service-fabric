// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace GatewayResourceManager
    {
        class GatewayResourceManagerAgent 
            : public Api::IGatewayResourceManagerAgent
            , public Common::ComponentRoot
            , private Federation::NodeTraceComponent<Common::TraceTaskCodes::GatewayResourceManager>
        {
            using Federation::NodeTraceComponent<Common::TraceTaskCodes::GatewayResourceManager>::WriteNoise;
            using Federation::NodeTraceComponent<Common::TraceTaskCodes::GatewayResourceManager>::WriteInfo;
            using Federation::NodeTraceComponent<Common::TraceTaskCodes::GatewayResourceManager>::WriteWarning;
            using Federation::NodeTraceComponent<Common::TraceTaskCodes::GatewayResourceManager>::WriteError;

        public:
            virtual ~GatewayResourceManagerAgent();

            static Common::ErrorCode Create(__out std::shared_ptr<GatewayResourceManagerAgent> & result);

            __declspec(property(get=get_TraceId)) std::wstring const & TraceId;
            std::wstring const & get_TraceId() const { return NodeTraceComponent::TraceId; }

            virtual void Release();

            virtual void RegisterGatewayResourceManager(
                ::FABRIC_PARTITION_ID,
                ::FABRIC_REPLICA_ID,
                Api::IGatewayResourceManagerPtr const &,  
                __out std::wstring & serviceAddress);

            virtual void UnregisterGatewayResourceManager(
                ::FABRIC_PARTITION_ID,
                ::FABRIC_REPLICA_ID);

            Common::AsyncOperationSPtr BeginGetGatewayResourceList(
                ServiceModel::ModelV2::GatewayResourceQueryDescription const & gatewayQueryDescription,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const &);
            Common::ErrorCode EndGetGatewayResourceList(
                Common::AsyncOperationSPtr const &,
                Transport::MessageUPtr & reply);

        private:
            GatewayResourceManagerAgent(
                 Federation::NodeInstance const & nodeInstance,
                __in Transport::IpcClient & ipcClient,
                Common::ComPointer<IFabricRuntime> const & runtimeCPtr);

            class DispatchMessageAsyncOperationBase;
            class CreateGatewayResourceAsyncOperation;
            class DeleteGatewayResourceAsyncOperation;
            class GatewayResourceManagerAgentQueryMessageHandler;

            Common::AsyncOperationSPtr BeginProcessQuery(
                Query::QueryNames::Enum queryName,
                ServiceModel::QueryArgumentMap const & queryArgs,
                Common::ActivityId const & activityId,
                Common::TimeSpan timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            Common::ErrorCode EndProcessQuery(
                Common::AsyncOperationSPtr const & operation,
                Transport::MessageUPtr & reply);

            void ReplaceServiceLocation(SystemServices::SystemServiceLocation const & location);
            void DispatchMessage(Api::IGatewayResourceManagerPtr const &, __in Transport::MessageUPtr &, __in Transport::IpcReceiverContextUPtr &);
            void OnDispatchMessageComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
            Common::ErrorCode Initialize();
            void OnDispatchMessageComplete2(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);


            SystemServices::ServiceRoutingAgentProxyUPtr routingAgentProxyUPtr_;
            std::list<SystemServices::SystemServiceLocation> registeredServiceLocations_;
            Common::ExclusiveLock lock_;

            Query::QueryMessageHandlerUPtr queryMessageHandler_;
            Common::ComPointer<IFabricRuntime> runtimeCPtr_;
            Api::IGatewayResourceManagerPtr servicePtr_;
        };
    }
}
