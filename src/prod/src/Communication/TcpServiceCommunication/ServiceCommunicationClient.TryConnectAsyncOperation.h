// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
namespace Communication
{
    namespace TcpServiceCommunication
    {
        class ServiceCommunicationClient::TryConnectAsyncOperation
            : public Common::AsyncOperation
        {
            DENY_COPY(TryConnectAsyncOperation);

        public:
            TryConnectAsyncOperation(
                __in ServiceCommunicationClient  & client,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            virtual ~TryConnectAsyncOperation() {}

            static Common::ErrorCode End(Common::AsyncOperationSPtr const & operation);

        protected:

            void OnStart(Common::AsyncOperationSPtr const & thisSPtr);

        private:
            void OnConnected(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

            ServiceCommunicationClient  &  owner_;
            Common::TimeoutHelper timeoutHelper_;
            class OnConnectAsyncOperation;

        };

    }
}
