// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
namespace Communication
{
    namespace TcpServiceCommunication
    {
        class ServiceCommunicationListener
            :public Api::IServiceCommunicationListener,
             public Common::ComponentRoot,
             public Common::StateMachine,
            protected Common::TextTraceComponent<Common::TraceTaskCodes::FabricTransport>
        {
            using Common::TextTraceComponent<Common::TraceTaskCodes::FabricTransport>::WriteNoise;
            using Common::TextTraceComponent<Common::TraceTaskCodes::FabricTransport>::WriteInfo;
            using Common::TextTraceComponent<Common::TraceTaskCodes::FabricTransport>::WriteWarning;
            using Common::TextTraceComponent<Common::TraceTaskCodes::FabricTransport>::WriteError;

            DENY_COPY(ServiceCommunicationListener);

            STATEMACHINE_STATES_03(Created, Opened,Closed);
            STATEMACHINE_TRANSITION(Opened,Created);
            STATEMACHINE_TRANSITION(Closed,Opened);
            STATEMACHINE_ABORTED_TRANSITION(Opened | Closed);
            STATEMACHINE_TERMINAL_STATES(Aborted|Closed);


        public:
            ServiceCommunicationListener(
                Api::IServiceCommunicationTransportSPtr const& transport)
                :transport_(transport),
                 StateMachine(Created)
            {                
            }

            void CreateAndSetDispatcher(Api::IServiceCommunicationMessageHandlerPtr const &  messageHandler,
                                        Api::IServiceConnectionHandlerPtr const  & connectionHandler,
                                        std::wstring  const & servicePath)
            {
                dispatcher_ = std::make_shared<ServiceMethodCallDispatcher>(
                    *this,
                    *this,
                    servicePath,
                    messageHandler,
                    connectionHandler,
                    transport_->GetSettings().MaxConcurrentCalls,
                    transport_->GetSettings().MaxQueueSize,
                    transport_->GetSettings().OperationTimeout);
            }

            virtual Common::AsyncOperationSPtr BeginOpen(
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            virtual Common::ErrorCode EndOpen(
                Common::AsyncOperationSPtr const & asyncOperation,
                __out std::wstring & serviceAddress);

            virtual Common::AsyncOperationSPtr BeginClose(
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            virtual Common::ErrorCode EndClose(
                Common::AsyncOperationSPtr const & asyncOperation);

            virtual void AbortListener();

            ServiceMethodCallDispatcherSPtr const & GetDispatcher()
            {
                return dispatcher_;
            }

            ~ServiceCommunicationListener();
        private:

            void OnAbort();
            void Cleanup();
            Api::IServiceCommunicationTransportSPtr transport_;
            std::wstring endpointAddress_;
            ServiceMethodCallDispatcherSPtr dispatcher_;
            class OpenAsyncOperation;
            class CloseAsyncOperation;
        };
    }
}
