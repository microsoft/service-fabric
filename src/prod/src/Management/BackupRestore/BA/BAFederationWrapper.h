// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace BackupRestoreAgentComponent
    {
        class BAFederationWrapper : public Common::RootedObject, Common::TextTraceComponent<Common::TraceTaskCodes::BA>
        {
            DENY_COPY(BAFederationWrapper);

        public:
            BAFederationWrapper(Federation::FederationSubsystem & federation, Reliability::ServiceResolver & resolver, Api::IQueryClientPtr queryClientPtr);

            __declspec (property(get = get_NodeId)) Federation::NodeId Id;
            Federation::NodeId get_NodeId() const { return federation_.Id; }

            __declspec (property(get = get_Instance)) Federation::NodeInstance const& Instance;
            Federation::NodeInstance const& get_Instance() const { return federation_.Instance; }

            __declspec (property(get = get_InstanceIdStr)) std::wstring const& InstanceIdStr;
            std::wstring const& get_InstanceIdStr() const { return instanceIdStr_; }

            __declspec(property(get = get_IsShutdown)) bool IsShutdown;
            bool get_IsShutdown() const { return federation_.IsShutdown; }

            Federation::FederationSubsystem & get_FederationSubsystem() { return federation_; }

            void SendToNode(Transport::MessageUPtr && message, Federation::NodeInstance const & target);
            
            Common::AsyncOperationSPtr BeginSendToServiceNode(
                Transport::MessageUPtr && message,
                Transport::IpcReceiverContextUPtr && receiverContext,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);
            Common::ErrorCode EndSendToServiceNode(Common::AsyncOperationSPtr const & operation, Transport::MessageUPtr & reply, Transport::IpcReceiverContextUPtr & receiverContext) const;
            
            virtual Common::AsyncOperationSPtr BeginRequestToNode(
                Transport::MessageUPtr && message,
                Federation::NodeInstance const & target,
                bool useExactRouting,
                Common::TimeSpan const retryInterval,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent) const;
            virtual Common::ErrorCode EndRequestToNode(Common::AsyncOperationSPtr const & operation, Transport::MessageUPtr & reply) const;

            void RegisterFederationSubsystemEvents(Federation::OneWayMessageHandler const & oneWayMessageHandler, Federation::RequestMessageHandler const & requestMessageHandler);

        private:
            void RouteCallback(
                Common::AsyncOperationSPtr const& routeOperation,
                wstring const & action,
                Federation::NodeInstance const & target);

            Federation::FederationSubsystem & federation_;
            Reliability::ServiceResolver & serviceResolver_;
            Api::IQueryClientPtr queryClientPtr_;
            const std::wstring traceId_;
            std::wstring instanceIdStr_;
        };
    }
}
