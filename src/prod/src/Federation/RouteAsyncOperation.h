// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class RouteAsyncOperation : public Common::AsyncOperation
    {
    public:
        RouteAsyncOperation(
            SiteNodeSPtr siteNode,
            Transport::MessageUPtr && message,
            NodeId nodeId,
            Common::TimeSpan retryTimeout,
            RoutingHeader const & header,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        ~RouteAsyncOperation();

        static Common::ErrorCode End(Common::AsyncOperationSPtr const & asyncOperation, Transport::MessageUPtr & reply);

    protected:

        void OnStart(Common::AsyncOperationSPtr const & thisSPtr);

        void MakeRequest(Common::AsyncOperationSPtr const & thisSPtr, bool newRequest);

    private:

        bool CanRetry() const
        {
            return !(this->retryTimeout_ == Common::TimeSpan::MaxValue);
        }

        void StopTimer();

        void CompleteWithTimeout(Common::AsyncOperationSPtr const & operation);

        void OnNoRoutingPartner(Common::AsyncOperationSPtr const & operation);

        void OnRequestCallback(Common::AsyncOperationSPtr const & operation, PartnerNodeSPtr const & hopTo);

        static void OnRetryCallback(Common::AsyncOperationSPtr const & operation);

        SiteNodeSPtr siteNode_;
        Transport::MessageUPtr message_;
        RoutingHeader header_;
        NodeId nodeId_;
        Common::TimeSpan retryTimeout_;
        Common::TimeoutHelper timeoutHelper_;
        Transport::MessageUPtr reply_;
        Common::TimerSPtr timer_;
        LONG retryCount_;
        Common::ExclusiveLock lock_;
    };
}
