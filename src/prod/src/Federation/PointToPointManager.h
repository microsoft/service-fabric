// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class PointToPointManager : public Common::TextTraceComponent<Common::TraceTaskCodes::P2P>
    {
        DENY_COPY(PointToPointManager);

    public:
        explicit PointToPointManager(__in SiteNode & siteNode);

        ~PointToPointManager();

        void Stop();

        void RegisterMessageHandler(
            Transport::Actor::Enum actor,
            OneWayMessageHandler const & oneWayMessageHandler,
            RequestMessageHandler const & requestMessageHandler,
            bool dispatchOnTransportThread,
            IMessageFilterSPtr && filter);

        bool UnRegisterMessageHandler(Transport::Actor::Enum actor, IMessageFilterSPtr const & filter);

        void ProcessIncomingTransportMessages(__in Transport::MessageUPtr & message, Transport::ISendTarget::SPtr const & sender);

        void CancelPToPRequest(Transport::MessageId const & messageId);

        // Will send a request message again without updating the context table.  RetryHeader will be added.
        void RetryPToPSendRequest(Transport::MessageUPtr && request, PartnerNodeSPtr const & to, bool exactInstance, PToPActor::Enum actor, LONG retryNumber);

        void PToPSend(Transport::MessageUPtr && message, PartnerNodeSPtr const & to, bool exactInstance, PToPActor::Enum actor) const;

        void PToPMulticastSend(
            Transport::MessageUPtr && message,
            std::vector<PartnerNodeSPtr> const & targets,
            Transport::MessageHeadersCollection && headersCollection,
            PToPActor::Enum actor);

        std::shared_ptr<Common::AsyncOperation> BeginPToPSendRequest(
            Transport::MessageUPtr && request,
            PartnerNodeSPtr const & to,
            bool exactInstance,
            PToPActor::Enum actor,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) const;

        Common::ErrorCode EndPToPSendRequest(Common::AsyncOperationSPtr const & operation, __out Transport::MessageUPtr & reply) const;

        void ActorDispatchOneWay(__inout Transport::MessageUPtr & message, __inout OneWayReceiverContextUPtr & receiverContext);

        void ActorDispatchRequest(__inout Transport::MessageUPtr & message, __inout RequestReceiverContextUPtr & requestReceiverContext);

        static void RemovePToPHeaders(__inout Transport::Message & message);

        static bool GetFromInstance(__inout Transport::Message & message, NodeInstance & fromInstance);

    private:
        SiteNode & siteNode_;
        mutable Transport::RequestTable requestTable_;

        struct ActorMapEntry
        {
            DEFAULT_COPY_CONSTRUCTOR(ActorMapEntry)
        
            ActorMapEntry() {}

            ActorMapEntry(ActorMapEntry && entry)
                : handlers_(std::move(entry.handlers_))
            {
            }
        
            std::vector<std::pair<IMessageFilterSPtr, MessageHandlerPair>> handlers_;
        };

        class JobItem
        {
            DENY_COPY(JobItem);
        public:
            JobItem();
            JobItem(Transport::MessageUPtr && message);
            JobItem(JobItem &&);
            JobItem& operator=(JobItem&& other);

            bool ProcessJob(SiteNode & siteNode);

        private:
            Transport::MessageUPtr message_;
        };

        typedef Common::JobQueue<JobItem, Federation::SiteNode> PointToPointManagerLoopbackDispatcherQueue;

        mutable PointToPointManagerLoopbackDispatcherQueue loopbackDispatcher_;

        MUTABLE_RWLOCK(Federation.PointToPointManager, applicationActorMapEntryLock_);
        std::map<Transport::Actor::Enum, ActorMapEntry> applicationActorMap_;
        bool applicationActorMapClosed_;

        MessageHandlerPair pointToPointActorMap_[PToPActor::UpperBound];

        void AddPToPHeaders(__inout Transport::Message & message, PartnerNodeSPtr const & to, bool exactInstance, PToPActor::Enum actor) const;

        bool ProcessPToPHeaders(__in Transport::Message & message, __out PToPHeader & header, __out PartnerNodeSPtr & source);

        void DispatchOneWayAndRequestMessages(__in Transport::MessageUPtr & message, PartnerNodeSPtr const & from, NodeInstance const & fromInstance, PToPActor::Enum actor);
    };
}
