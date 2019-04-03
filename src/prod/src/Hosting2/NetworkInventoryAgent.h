// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace NetworkInventoryManager
    {

        class NetworkAllocationRequestMessage;
        class NetworkAllocationResponseMessage;
        class NetworkCreateRequestMessage;
        class NetworkCreateResponseMessage;
        class NetworkRemoveRequestMessage;
        class NetworkEnumerateRequestMessage;
        class NetworkEnumerateResponseMessage;

        class CreateNetworkMessageBody;
        class PublishNetworkTablesMessageRequest;
        class NetworkErrorCodeResponseMessage;
    }
}

namespace Reliability
{
    class IReliabilitySubsystem;
    class BasicFailoverReplyMessageBody;
}

namespace Hosting2
{
    // This class represents the network management subsystem that runs on every node.
    // Since it needs to interact with hosting and reliability subsystems, we get references to those objects.
    // This component receives and sends messages from and to the NetworkInformationService and then
    // invokes into the hosting subsystem to perform the real operations.
    class NetworkInventoryAgent :
        public Common::RootedObject,
        public Common::AsyncFabricComponent,
        Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(NetworkInventoryAgent)

    public:
        NetworkInventoryAgent(
            Common::ComponentRoot const & root,
            __in Federation::FederationSubsystem & federation,
            __in Reliability::IReliabilitySubsystem & reliability,
            __in IHostingSubsystemSPtr hosting);

        virtual ~NetworkInventoryAgent();

        // Send a NetworkAllocationRequestMessage to NIM service async.
        Common::AsyncOperationSPtr BeginSendAllocationRequestMessage(
            Management::NetworkInventoryManager::NetworkAllocationRequestMessage const & params,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback);

        // Complete message request and get the NetworkAllocationResponseMessage reply.
        Common::ErrorCode EndSendAllocationRequestMessage(
            Common::AsyncOperationSPtr const & operation,
            __out Management::NetworkInventoryManager::NetworkAllocationResponseMessage & reply);

        // Send a NetworkDeallocationRequestMessage to NIM service async.
        Common::AsyncOperationSPtr BeginSendDeallocationRequestMessage(
            Management::NetworkInventoryManager::NetworkRemoveRequestMessage const & params,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback);

        // Complete message request and get the NetworkDellocationResponseMessage reply.
        Common::ErrorCode EndSendDeallocationRequestMessage(
            Common::AsyncOperationSPtr const & operation,
            __out Management::NetworkInventoryManager::NetworkErrorCodeResponseMessage & reply);

        // Send a PublishNetworkTablesRequestMessage to NIM service async.
        Common::AsyncOperationSPtr BeginSendPublishNetworkTablesRequestMessage(
            Management::NetworkInventoryManager::PublishNetworkTablesMessageRequest const & params,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback);

        // Complete message request and get the PublishNetworkTablesResponseMessage reply.
        Common::ErrorCode EndSendPublishNetworkTablesRequestMessage(
            Common::AsyncOperationSPtr const & operation,
            __out Management::NetworkInventoryManager::NetworkErrorCodeResponseMessage & reply);

        // Send a NetworkCreateRequestMessage to NIM service async.
        Common::AsyncOperationSPtr BeginSendCreateRequestMessage(
            Management::NetworkInventoryManager::CreateNetworkMessageBody const & params,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback);

        // Complete message request and get the NetworkCreateRequestMessage reply.
        Common::ErrorCode EndSendCreateRequestMessage(
            Common::AsyncOperationSPtr const & operation,
            __out Reliability::BasicFailoverReplyMessageBody & reply);

        // Send a NetworkRemoveRequestMessage to NIM service async.
        Common::AsyncOperationSPtr BeginSendRemoveRequestMessage(
            Management::NetworkInventoryManager::NetworkRemoveRequestMessage const & params,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback);

        // Complete message request and get the NetworkRemoveRequestMessage reply.
        Common::ErrorCode EndSendRemoveRequestMessage(
            Common::AsyncOperationSPtr const & operation,
            __out Management::NetworkInventoryManager::NetworkErrorCodeResponseMessage & reply);

        // Send a NetworkEnumerateRequestMessage to NIM service async.
        Common::AsyncOperationSPtr BeginSendEnumerateRequestMessage(
            Management::NetworkInventoryManager::NetworkEnumerateRequestMessage const & params,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback);

        // Complete message request and get the NetworkErrorCodeResponseMessage reply.
        Common::ErrorCode EndSendEnumerateRequestMessage(
            Common::AsyncOperationSPtr const & operation,
            __out Management::NetworkInventoryManager::NetworkEnumerateResponseMessage & reply);

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
        void RegisterRequestHandler();
        void UnregisterRequestHandler();

        void ProcessMessage(
            __in Transport::MessageUPtr & message,
            __in Federation::RequestReceiverContextUPtr & context);

        // Message with route information for the network.
        void ProcessNetworkMappingTables(
            __in Transport::MessageUPtr & message,
            __in Federation::RequestReceiverContextUPtr & context);

        class OpenAsyncOperation;
        class CloseAsyncOperation;

        template <class TRequest, class TReply>
        class NISRequestAsyncOperationBase;
        template <class TRequest, class TReply>
        class NISRequestAsyncOperation;
        template <class TRequest, class TReply, class TMessageBody>
        class NISRequestAsyncOperationApi;

        Federation::FederationSubsystem & federation_;
        Reliability::IReliabilitySubsystem & reliability_;
        IHostingSubsystemSPtr hosting_;
    };
}