// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
namespace Communication
{
    namespace TcpServiceCommunication
    {
        class ServiceCommunicationClient::DisconnectAsyncOperation
            : public Common::TimedAsyncOperation
        {
            DENY_COPY(DisconnectAsyncOperation);

        public:
            DisconnectAsyncOperation(
                __in ServiceCommunicationClient  & client,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            virtual ~DisconnectAsyncOperation() {}

            static Common::ErrorCode End(Common::AsyncOperationSPtr const & operation);

        protected:

            void OnStart(Common::AsyncOperationSPtr const & thisSPtr);

        private:
            ServiceCommunicationClient  &  owner_;
         
        };

    }
}
