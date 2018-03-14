// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
namespace Communication
{
    namespace TcpServiceCommunication
    {
        class ServiceMethodCallDispatcher
            : public Common::TextTraceComponent<Common::TraceTaskCodes::FabricTransport>,
            public Common::RootedObject
        {
            DENY_COPY(ServiceMethodCallDispatcher)
        public:
            ServiceMethodCallDispatcher(
                Common::ComponentRoot const & root,
                ServiceCommunicationListener    &  queueRoot,
                std::wstring const &location,
                Api::IServiceCommunicationMessageHandlerPtr const &rootedServicePtr,
                Api::IServiceConnectionHandlerPtr const & connectionHandler,
                int maxConcurrentCalls,
                int maxQueueSize,
                Common::TimeSpan const & timeout)
                : Common::RootedObject(root)
                , rootedServicePtr_(rootedServicePtr)
                , serviceInfo_(location)
                , connectionHandler_(connectionHandler)
                , timeout_(timeout)
            {
                auto name = Common::wformatString("ServiceMethodCallDispatcher queue for {0}", location);
                requestQueue_ = Common::make_unique<Common::CommonTimedJobQueue<ServiceCommunicationListener>>(
                    name,
                    queueRoot,
                    false,
                    maxConcurrentCalls,
                    nullptr,
                    maxQueueSize);
                rootedServicePtr->Initialize();
            }

            ~ServiceMethodCallDispatcher();
   
            __declspec(property(get = get_ConnectionHandler)) Api::IServiceConnectionHandlerPtr const & ConnectionHandler;
            Api::IServiceConnectionHandlerPtr const & get_ConnectionHandler() const
            {
                return connectionHandler_;
            };


            __declspec(property(get = get_ServiceInfo)) std::wstring const & ServiceInfo;
            std::wstring const & get_ServiceInfo() const
            {
                return serviceInfo_;
            };

            Common::TimeSpan const & get_Timeout() const
            {
                return timeout_;
            };


           void AddToQueue(Transport::MessageUPtr & message, Transport::ReceiverContextUPtr & context);

            void Close();

           
        private:
            
            std::unique_ptr<Common::CommonTimedJobQueue<ServiceCommunicationListener>> requestQueue_;
            Api::IServiceCommunicationMessageHandlerPtr rootedServicePtr_;
            Api::IServiceConnectionHandlerPtr connectionHandler_;


            std::wstring serviceInfo_;
            Common::TimeSpan timeout_;
            class DispatchMessageAsyncOperation;
            class ServiceMethodCallWorkItem;
        };
    }
}
