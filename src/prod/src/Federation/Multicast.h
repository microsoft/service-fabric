// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class MulticastReplyContext :
        public IMultipleReplyContext,
        public Common::TextTraceComponent<Common::TraceTaskCodes::Multicast>,
        public std::enable_shared_from_this<MulticastReplyContext>
    {
    public:
        static IMultipleReplyContextSPtr CreateAndStart(Transport::MessageUPtr && message, std::vector<NodeInstance> && destinations, SiteNode & siteNode);
        ~MulticastReplyContext();

        Common::AsyncOperationSPtr BeginReceive(
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndReceive(
            Common::AsyncOperationSPtr const & operation,
            __out Transport::MessageUPtr & message,
            __out NodeInstance & requestDestination);

        void Close();

    private:
        MulticastReplyContext(Transport::MessageUPtr && message, std::vector<NodeInstance> && destinations, SiteNode & siteNode);
        void Start();
        void OnReply(Common::AsyncOperationSPtr const & operation, NodeInstance const & requestDestination);

        Transport::MessageUPtr message_;
        std::vector<NodeInstance> destinations_;
        SiteNode & siteNode_;
        std::vector<Common::AsyncOperationSPtr> requests_;
        std::shared_ptr<Common::ReaderQueue<Transport::Message>> replies_;
    };
}
