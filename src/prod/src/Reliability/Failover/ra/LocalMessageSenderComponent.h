// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        // LocalMessageComponent; used by RAP to retry failed messages
        class LocalMessageSenderComponent: 
            public Common::RootedObject
        {
            DENY_COPY(LocalMessageSenderComponent);

        public:
            LocalMessageSenderComponent(
                Common::ComponentRoot const & root,
                Transport::IpcClient & ipcClient);

            ~LocalMessageSenderComponent();

            void Open(ReconfigurationAgentProxyId const & id);
            void Close();
            void SetTimerCallerHoldsLock();
            void UpdateTimerCallerHoldsLock();

            void RetryCallback();
            
            void AddMessage(ProxyOutgoingMessageUPtr && message);
        private:

            void SendMessage(Transport::MessageUPtr const& message);

            std::list<ProxyOutgoingMessageUPtr> pendingMessages_;
            
            bool open_;
            Common::TimerSPtr retryTimerSPtr_;
            
            ReconfigurationAgentProxyId id_;

            Common::TimeSpan retryInterval_;
            Common::RwLock lock_;
            Transport::IpcClient & ipcClient_;
        };
    }
}
