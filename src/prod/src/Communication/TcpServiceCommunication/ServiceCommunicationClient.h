// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace Communication
{
    namespace TcpServiceCommunication
    {
        class ServiceCommunicationClient :
            public Api::IServiceCommunicationClient,
            public Common::ComponentRoot,
            public Common::StateMachine,
            protected Common::TextTraceComponent<Common::TraceTaskCodes::FabricTransport>
        {

            using Common::TextTraceComponent<Common::TraceTaskCodes::FabricTransport>::WriteNoise;
            using Common::TextTraceComponent<Common::TraceTaskCodes::FabricTransport>::WriteInfo;
            using Common::TextTraceComponent<Common::TraceTaskCodes::FabricTransport>::WriteWarning;
            using Common::TextTraceComponent<Common::TraceTaskCodes::FabricTransport>::WriteError;
        
            DENY_COPY(ServiceCommunicationClient);


            STATEMACHINE_STATES_07(Created,Opening, Opened, Connected,ConnectFailed,Disconnected,Closed);
            STATEMACHINE_TRANSITION(Opening, Created);
            STATEMACHINE_TRANSITION(Opened, Opening);
            STATEMACHINE_TRANSITION(Connected, Opened);
            STATEMACHINE_TRANSITION(ConnectFailed, Opened);
            STATEMACHINE_TRANSITION(Disconnected, Connected);
            STATEMACHINE_TRANSITION(Closed, Connected | ConnectFailed|Disconnected| Opened);
            STATEMACHINE_ABORTED_TRANSITION(Opened  | Connected | ConnectFailed | Disconnected);
            STATEMACHINE_TERMINAL_STATES(ConnectFailed|Disconnected|Aborted | Closed);

        public:

            ServiceCommunicationClient(
                ServiceCommunicationTransportSettingsUPtr const & securitySettings,
                std::wstring const & connectionAddress,
                Api::IServiceCommunicationMessageHandlerPtr const & messageHandler,
                Api::IServiceConnectionEventHandlerPtr const & eventHandler,
                bool explicitConnect);

            virtual Common::AsyncOperationSPtr BeginOpen(
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            virtual Common::ErrorCode EndOpen(
                Common::AsyncOperationSPtr const &  operation
            ) ;

            virtual Common::AsyncOperationSPtr BeginClose(
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent) ;

            virtual Common::ErrorCode EndClose(
                Common::AsyncOperationSPtr const &  operation
            ) ;

            virtual Common::ErrorCode Open();

            /* IServiceCommunicationClient Methods */
          

            virtual void AbortClient();

            virtual void CloseClient();

            /* ICommunicationMessageSender Methods */

            virtual Common::AsyncOperationSPtr BeginRequest(
                Transport::MessageUPtr && message,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            virtual Common::ErrorCode EndRequest(
                Common::AsyncOperationSPtr const &  operation,
                Transport::MessageUPtr & reply);

            virtual Common::ErrorCode SendOneWay(
                Transport::MessageUPtr && message);

            ~ServiceCommunicationClient();

        private:
            Common::ErrorCode connectError_;
            std::wstring clientId_;
            std::shared_ptr<Transport::IDatagramTransport> transport_;
            Transport::DemuxerUPtr demuxer_;
            Transport::RequestReply requestReply_;
            Common::TimeSpan defaultTimeout_;
            Common::TimeSpan connectTimeout_;
            std::wstring serverAddress_;
            std::wstring serviceLocation_;
            Common::RwLock connectLock_;
            bool explicitConnect_;
            Common::AsyncOperationSPtr primaryConnectOperation_;
            Transport::ISendTarget::SPtr serverSendTarget_;
            Api::IServiceConnectionEventHandlerPtr eventHandler_;
            Api::IServiceCommunicationMessageHandlerPtr messageHandler_;
            Transport::SecuritySettings securitySettings_;
            class ProcessRequestAsyncOperation;
            class SendRequestAsyncOperation;
            class TryConnectAsyncOperation;
            class DisconnectAsyncOperation;
            void OnSending(Transport::MessageUPtr & message, Common::TimeSpan const & timeout);
            void OnDisconnect(Common::ErrorCode const & error);
            void ProcessRequest(
                Api::IServiceCommunicationMessageHandlerPtr const & handler,
                Transport::MessageUPtr & message,
                Transport::ReceiverContextUPtr & context);
            void OnProcessRequestComplete(
                Common::AsyncOperationSPtr const & operation,
                bool expectedCompletedSynchronously);
            Common::ErrorCode  SetServiceLocation();
            virtual  Common::ErrorCode  OnOpen();
            virtual  Common::ErrorCode  OnClose();
            virtual  void  OnAbort();
            Common::ErrorCode OnConnect(Common::AsyncOperationSPtr const & parent,
                                        Common::AsyncCallback const& callback);
            virtual Common::ErrorCode Release();
            KBuffer::SPtr CreateSharedHeaders();
            KBuffer::SPtr  commonMessageHeadersSptr_;
        };
    }
}
