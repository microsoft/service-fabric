// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class FaultAnalysisServiceAgentQueryMessageHandler : public ClientRequestAsyncOperation
        {
        public:
            FaultAnalysisServiceAgentQueryMessageHandler(
                __in FaultAnalysisServiceAgent &,
                __in Query::QueryMessageHandlerUPtr & queryMessageHandler,
                Transport::MessageUPtr &&,
                Transport::IpcReceiverContextUPtr &&,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

        protected:
            virtual Common::AsyncOperationSPtr BeginAcceptRequest(
                std::shared_ptr<ClientRequestAsyncOperation> &&,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);
            virtual Common::ErrorCode EndAcceptRequest(Common::AsyncOperationSPtr const &);

        private:
            Query::QueryMessageHandlerUPtr & queryMessageHandler_;

        };
    }
}

