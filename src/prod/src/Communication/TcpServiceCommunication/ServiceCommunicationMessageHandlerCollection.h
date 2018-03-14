// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Communication
{
    namespace TcpServiceCommunication
    {
        class ServiceCommunicationMessageHandlerCollection :
            public Common::RootedObject,
            public Common::TextTraceComponent<Common::TraceTaskCodes::FabricTransport>
        {
            DENY_COPY(ServiceCommunicationMessageHandlerCollection);
        public:
            ServiceCommunicationMessageHandlerCollection(
                Common::ComponentRoot const &root,
                int maxConcurrentCallsPerRegistration,
                int maxQueueSize)
                : Common::RootedObject(root)
                , maxConcurrentCallsPerRegistration_(maxConcurrentCallsPerRegistration)
                , maxQueueSize_(maxQueueSize)
            {
            }

            Common::ErrorCode RegisterMessageHandler(
                ServiceMethodCallDispatcherSPtr const & dispatcher);
            
            void UnregisterMessageHandler(std::wstring const & location);

            ServiceMethodCallDispatcherSPtr GetMessageHandler(std::wstring const & location) const;

        private:
            std::map<std::wstring, ServiceMethodCallDispatcherSPtr> serviceRegistration_;
            mutable Common::RwLock serviceRegistrationLock_;
            int maxConcurrentCallsPerRegistration_;
            int maxQueueSize_;
        };

    }
}
