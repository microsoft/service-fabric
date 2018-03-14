// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace BackupRestoreAgentComponent
    {
        // Abstracts away the BA-BAP Transport
        class IBaToBapTransport
        {
            DENY_COPY(IBaToBapTransport);

        public:

            IBaToBapTransport()
            {
            }

            virtual ~IBaToBapTransport()
            {
            }

            virtual void SendOneWay(
                std::wstring const & target,
                Transport::MessageUPtr && message,
                Common::TimeSpan expiration) = 0;

            virtual void RegisterMessageHandler(
                Transport::Actor::Enum actor,
                Transport::IpcMessageHandler const & messageHandler,
                bool dispatchOnTransportThread) = 0;

            virtual Common::AsyncOperationSPtr BeginRequest(
                Transport::MessageUPtr && request,
                std::wstring const & client,
                Common::TimeSpan timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent) = 0;

            virtual Common::ErrorCode EndRequest(
                Common::AsyncOperationSPtr const & operation,
                Transport::MessageUPtr & reply) = 0;
        };

        class BaToBapTransport : public IBaToBapTransport
        {
            DENY_COPY(BaToBapTransport);

        public:
            BaToBapTransport(Transport::IpcServer & ipcServer)
                : ipcServer_(ipcServer)
            {
            }

            void SendOneWay(
                std::wstring const & target,
                Transport::MessageUPtr && message,
                Common::TimeSpan expiration) override
            {
                ipcServer_.SendOneWay(target, std::move(message), expiration);

            }

            void RegisterMessageHandler(
                Transport::Actor::Enum actor,
                Transport::IpcMessageHandler const & messageHandler,
                bool dispatchOnTransportThread) override
            {
                ipcServer_.RegisterMessageHandler(actor, messageHandler, dispatchOnTransportThread);
            }

            Common::AsyncOperationSPtr BeginRequest(
                Transport::MessageUPtr && request,
                std::wstring const & target,
                Common::TimeSpan timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent) override
            {
                return ipcServer_.BeginRequest(std::move(request), target, timeout, callback, parent);
            }

            Common::ErrorCode EndRequest(
                Common::AsyncOperationSPtr const & operation,
                Transport::MessageUPtr & reply) override
            {
                return ipcServer_.EndRequest(operation, reply);
            }

        private:
            Transport::IpcServer & ipcServer_;
        };
    }
}
