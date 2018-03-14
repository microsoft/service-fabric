// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class EntreeService::ForwardToTokenValidationServiceAsyncOperation 
        : public EntreeService::RequestAsyncOperationBase
    {
        DENY_COPY(ForwardToTokenValidationServiceAsyncOperation)

    public:
        ForwardToTokenValidationServiceAsyncOperation(
            __in GatewayProperties & properties,
            Transport::MessageUPtr &&message,
            Common::TimeSpan timeout,
            Common::AsyncCallback const& callback,
            Common::AsyncOperationSPtr const& parent)
            : RequestAsyncOperationBase(properties, std::move(message), timeout, callback, parent)
        {
        }

    protected:

        void OnStartRequest(Common::AsyncOperationSPtr const& thisSPtr);

    private:
        void OnComplete(Common::AsyncOperationSPtr const& operation);
        void GetServicePartitionReplicaList(Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetServicePartitionReplicaListCompleted(Common::AsyncOperationSPtr const& operation);
        void OnForwardToService(Common::AsyncOperationSPtr const& operation, Common::Guid const & partitionId);
    };
}
