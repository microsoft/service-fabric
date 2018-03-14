// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class EntreeService::SendToNodeOperation : public EntreeService::RequestAsyncOperationBase
    {
    public:
        SendToNodeOperation(
            __in GatewayProperties & properties,
            Transport::MessageUPtr && receivedMessage,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent,
            Federation::NodeId nodeId,
            uint64 nodeInstanceId = 0);

    protected:
        void OnRetry(Common::AsyncOperationSPtr const & thisSPtr);
        void OnStartRequest(Common::AsyncOperationSPtr const & thisSPtr) override;

        void StartSend(Common::AsyncOperationSPtr const & thisSPtr);

        virtual void OnRouteToNodeSuccessful(Common::AsyncOperationSPtr const & thisSPtr, Transport::MessageUPtr & reply);
        virtual bool IsRetryable(Common::ErrorCode const & error);
        virtual void OnRouteToNodeFailedNonRetryableError(Common::AsyncOperationSPtr const & thisSPtr, Common::ErrorCode const & error);

    private:
        Federation::NodeId nodeId_;
        uint64 nodeInstanceId_;
    };
}
