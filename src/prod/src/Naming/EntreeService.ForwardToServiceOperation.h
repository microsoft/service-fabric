// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class EntreeService::ForwardToServiceOperation : public EntreeService::RequestAsyncOperationBase
    {
    public:
        ForwardToServiceOperation(
            __in GatewayProperties & properties,
            Transport::MessageUPtr && receivedMessage,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);

    protected:
        void OnRetry(Common::AsyncOperationSPtr const & thisSPtr);
        void OnStartRequest(Common::AsyncOperationSPtr const & thisSPtr) override;

        virtual void ProcessReply(Transport::MessageUPtr & reply);

        virtual void OnRouteToNodeFailedRetryableError(
            Common::AsyncOperationSPtr const & thisSPtr,
            Transport::MessageUPtr & reply,
            Common::ErrorCode const & error);

    private:

        bool TryInitializeStaticCuidTarget(Transport::Actor::Enum const &);
        bool TryValidateServiceTarget(Transport::Actor::Enum const &, ServiceTargetHeader const &);

        void StartResolve(Common::AsyncOperationSPtr const & thisSPtr);
        void OnResolveCompleted(
            Common::AsyncOperationSPtr const & asyncOperation,
            bool expectedCompletedSynchronously);

        std::wstring targetServiceName_;
        Reliability::ConsistencyUnitId targetCuid_;        
        SystemServices::ServiceRoutingAgentHeader routingAgentHeader_;
        std::wstring action_;
        std::vector<std::wstring> secondaryLocations_;
    };
}
