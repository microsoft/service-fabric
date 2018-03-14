// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
namespace Communication
{
    namespace TcpServiceCommunication
    {
        class ServiceCommunicationClient::SendRequestAsyncOperation
            : public Common::AsyncOperation
        {
            DENY_COPY(SendRequestAsyncOperation);

        public:
            SendRequestAsyncOperation(
                __in ServiceCommunicationClient  & client,
                Transport::MessageUPtr && message,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            virtual ~SendRequestAsyncOperation() {}

            static Common::ErrorCode End(Common::AsyncOperationSPtr const & operation, __out Transport::MessageUPtr & reply);

        protected:

            void OnStart(Common::AsyncOperationSPtr const & thisSPtr);

        private:
            void SendRequest(Common::AsyncOperationSPtr const & thisSPtr);
            void OnConnected(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);
            void OnSendRequestComplete(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);

            ServiceCommunicationClient  &  owner_;
            Transport::MessageUPtr request_;
            std::wstring clientId_;
            Transport::ReceiverContextUPtr receiverContext_;
            Transport::MessageUPtr reply_;
            Common::TimeoutHelper timeoutHelper_;

        };

    }
}
