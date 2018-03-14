// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class RoutingManager : public Common::TextTraceComponent<Common::TraceTaskCodes::Routing>
    {
        DENY_COPY(RoutingManager);

    public:
        explicit RoutingManager(__in SiteNode & siteNode);

        ~RoutingManager();

        Common::AsyncOperationSPtr BeginRoute(
                Transport::MessageUPtr && message,
                NodeId nodeId,
                uint64 instance,
                std::wstring const & toRing,
                bool useExactRouting,
                Common::TimeSpan retryTimeout,
                Common::TimeSpan timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndRoute(Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginRouteRequest(
                Transport::MessageUPtr && request,
                NodeId nodeId,
                uint64 instance,
                std::wstring const & toRing,
                bool useExactRouting,
                Common::TimeSpan retryTimeout,
                Common::TimeSpan timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndRouteRequest(Common::AsyncOperationSPtr const & operation, __out Transport::MessageUPtr & reply);

        void Stop();

        void PreProcessRoutedMessages(__in Transport::Message & message, __inout RequestReceiverContextUPtr & requestReceiverContext);

        void ProcessRoutedMessageHoldingList();

        void ClearRoutedMessageHoldingList();

        void SendReply(
            Transport::MessageUPtr && reply,
            PartnerNodeSPtr const & from, 
            Transport::MessageId const & relatesToId,
            RequestReceiverContextUPtr const & requestContext,
            bool sendAckToPreviousHop,
            bool isIdempotent);

        bool TryRemoveMessageIdFromProcessingSet(Transport::MessageId const & id);

        static bool IsRetryable(Common::ErrorCode error, bool isIdempotent);

    private:
        struct RoutingContext;
        typedef std::unique_ptr<RoutingContext> RoutingContextUPtr;

        SiteNode & siteNode_;
        Common::ExclusiveLock routedMessageHoldingListLock_;
        std::list<RoutingContextUPtr> routedMessageHoldingList_;
        Common::ExclusiveLock routedMessageProcessingSetLock_;
        std::set<Transport::MessageId> routedMessageProcessingSet_; // TODO: refactor into a common threadsafe set

        RoutingHeader GetRoutingHeader(
            __in Transport::Message & message,
            NodeInstance const & target,
            std::wstring const & toRing,
            Common::TimeSpan expiration,
            Common::TimeSpan retryTimeout,
            bool useExactRouting,
            bool expectsReply) const;

        Common::AsyncOperationSPtr InternalBeginRoute(
            Transport::MessageUPtr && message,
            bool isRequest,
            NodeId nodeId,
            uint64 instance,
            std::wstring const & toRing,
            bool useExactRouting,
            Common::TimeSpan retryTimeout,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        void ProcessRoutedMessages(RoutingContextUPtr && context);

        void DispatchRoutedOneWayAndRequestMessages(
            __inout Transport::MessageUPtr & message,
            PartnerNodeSPtr const & from,
            RoutingHeader const & routingHeader,
            __inout RequestReceiverContextUPtr && requestReceiverContext,
            bool sendRoutingAck);

        bool TryAddMessageIdToProcessingSet(Transport::MessageId const & id);

        bool TryGetRoutingHop(__in RoutingContextUPtr & context, std::wstring const & toRing, PartnerNodeSPtr & closestNode, __out bool & thisOwnsRoutingToken);

        void OnRouteReply(RoutingContextUPtr && context, bool sendRoutingAck, PartnerNodeSPtr const & hopTo, Common::AsyncOperationSPtr const & contextSPtr);

        struct RoutingContext
        {
            RoutingContext(
                Transport::MessageUPtr && message,
                RequestReceiverContextUPtr && requestReceiverContext,
                RoutingHeader const & routingHeader,
                NodeInstance const & hopFrom,
                Transport::MessageId const & messageId)
                : message_(std::move(message)),
                requestReceiverContext_(move(requestReceiverContext)),
                routingHeader_(routingHeader),
                hopFrom_(hopFrom),
                messageId_(messageId),
                timeoutHelper_(routingHeader.Expiration),
                isFirstHop_(messageId == routingHeader.MessageId)
            {
            }

            void UpdateRoutingHeader();

            Transport::MessageUPtr message_;
            RequestReceiverContextUPtr requestReceiverContext_;
            RoutingHeader routingHeader_;
            NodeInstance hopFrom_;
            Transport::MessageId messageId_;
            Common::TimeoutHelper timeoutHelper_;
            bool isFirstHop_;
        };
    };
}
