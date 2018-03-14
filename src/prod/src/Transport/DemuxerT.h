// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    template<typename TKey, typename TReceiverContext>
    class DemuxerT : public Common::FabricComponent, public Common::RootedObject, public Common::TextTraceComponent<Common::TraceTaskCodes::Transport>
    {
        DENY_COPY(DemuxerT);

    public:

        typedef std::unique_ptr<TReceiverContext> TReceiverContextUPtr;
        typedef std::function<void(MessageUPtr & message, TReceiverContextUPtr &)> MessageHandler;
        typedef std::pair<typename MessageHandler, bool/*dispatchOnTransportThread*/> MessageHandlerEntry;

        DemuxerT(Common::ComponentRoot const & root, IDatagramTransportSPtr const & datagramTransport);
        
        virtual ~DemuxerT();

        void SetReplyHandler(RequestReply & requestReply);

        void RegisterMessageHandler(TKey const & actor, MessageHandler const & messageHandler, bool dispatchOnTransportThread);

        void UnregisterMessageHandler(TKey const & actor);

    protected:

        virtual TReceiverContextUPtr CreateReceiverContext(Message & message, ISendTarget::SPtr const & replyTargetSPtr) = 0;

        virtual TKey GetActor(Message & message) = 0;

        Common::ErrorCode OnOpen();

        Common::ErrorCode OnClose();

        void OnAbort();

        IDatagramTransportSPtr datagramTransport_;

    private:
        typedef std::function<void(Message & message, ISendTarget::SPtr const &)> ReplyHandler;

        void Cleanup();

        void OnMessageReceived(MessageUPtr & message, ISendTarget::SPtr const & replyTargetSPtr);

        void ActorDispatch(TKey const & actorKey, MessageUPtr & message, TReceiverContextUPtr &);

        void CallMessageHandler(MessageHandlerEntry const & handlerEntry, MessageUPtr & message, TReceiverContextUPtr & context);

        RWLOCK(Demuxer, lock_);
        bool closed_;
        ReplyHandler replyHandler_;
        std::map<TKey, MessageHandlerEntry> actorMap_;
    };

    template<typename TKey, typename TReceiverContext>
    DemuxerT<TKey, TReceiverContext>::DemuxerT(
        Common::ComponentRoot const & root,
        IDatagramTransportSPtr const & datagramTransport)
        : Common::RootedObject(root),
        closed_(false),
        datagramTransport_(datagramTransport)
    {
    }

    template<typename TKey, typename TReceiverContext>
    DemuxerT<TKey, TReceiverContext>::~DemuxerT()
    {
    }

    template<typename TKey, typename TReceiverContext>
    Common::ErrorCode DemuxerT<TKey, TReceiverContext>::OnOpen()
    {
        Common::ComponentRootSPtr root = Root.CreateComponentRoot();
        Common::AcquireWriteLock grab(lock_);

        datagramTransport_->SetMessageHandler(
            [this, root] (MessageUPtr & message, ISendTarget::SPtr const & sender)
            {
                this->OnMessageReceived(message, sender);
            });

        return Common::ErrorCode::Success();
    }

    template<typename TKey, typename TReceiverContext>
    Common::ErrorCode DemuxerT<TKey, TReceiverContext>::OnClose()
    {
        WriteInfo(Constants::DemuxerTrace, "Demuxer:{0}: close called", TextTraceThis);

        Cleanup();

        return Common::ErrorCode::Success();
    }

    template<typename TKey, typename TReceiverContext>
    void DemuxerT<TKey, TReceiverContext>::OnAbort()
    {
        WriteInfo(Constants::DemuxerTrace, "Demuxer:{0}: abort called", TextTraceThis);

        Cleanup();
    }

    template<typename TKey, typename TReceiverContext>
    void DemuxerT<TKey, TReceiverContext>::Cleanup()
    {
        Common::AcquireWriteLock grab(lock_);

        replyHandler_ = nullptr;
        actorMap_.clear();

        closed_ = true;
    }

    template<typename TKey, typename TReceiverContext>
    void DemuxerT<TKey, TReceiverContext>::SetReplyHandler(RequestReply & requestReply)
    {
        Common::AcquireWriteLock grab(this->lock_);
        if (closed_)
        {
            WriteInfo(Constants::DemuxerTrace, "Demuxer:{0}:{1}: already closed", TextTraceThis, __FUNCTION__);
            return;
        }

        ASSERT_IF(this->replyHandler_, "reply handler already exists");
        this->replyHandler_ = [&requestReply] (Message & reply, ISendTarget::SPtr const & sendTarget) 
        { 
            requestReply.OnReplyMessage(reply, sendTarget); 
        };
    }

    template<typename TKey, typename TReceiverContext>
    void DemuxerT<TKey, TReceiverContext>::RegisterMessageHandler(TKey const & actor, MessageHandler const & messageHandler, bool dispatchOnTransportThread)
    {
        Common::AcquireWriteLock grab(this->lock_);
        if (closed_)
        {
            WriteInfo(Constants::DemuxerTrace, "Demuxer:{0}:{1}: already closed", TextTraceThis, __FUNCTION__);
            return;
        }

        ASSERT_IFNOT(this->actorMap_.find(actor) == this->actorMap_.end(), "actor {0} already exists", actor);

        auto handlerEntry = std::make_pair(messageHandler, dispatchOnTransportThread);
        auto mapEntry = std::make_pair(actor, std::move(handlerEntry));

        size_t beforeSize = this->actorMap_.size();

        this->actorMap_.insert(std::move(mapEntry));

        size_t afterSize = this->actorMap_.size();

        WriteInfo(
            Constants::DemuxerTrace,
            "Demuxer:{0}: registered message handler for actor = {1}, map size: {2}->{3}",
            TextTraceThis,
            actor,
            beforeSize,
            afterSize);
    }

    template<typename TKey, typename TReceiverContext>
    void DemuxerT<TKey, TReceiverContext>::UnregisterMessageHandler(TKey const & actor)
    {
        Common::AcquireWriteLock grab(this->lock_);
        if (closed_)
        {
            WriteInfo(Constants::DemuxerTrace, "Demuxer:{0}:{1}: already closed", TextTraceThis, __FUNCTION__);
            return;
        }

        this->actorMap_.erase(actor);

        WriteInfo(
            Constants::DemuxerTrace,
            "Demuxer:{0}: unregistered message handler for actor = {1}",
            TextTraceThis,
            actor);
    }

    template<typename TKey, typename TReceiverContext>
    void DemuxerT<TKey, TReceiverContext>::OnMessageReceived(MessageUPtr & message, ISendTarget::SPtr const & replyTargetSPtr)
    {
        if (message->RelatesTo.IsEmpty() && !message->IsUncorrelatedReply)
        {
            TReceiverContextUPtr receiverContext = this->CreateReceiverContext(*message, replyTargetSPtr);
            if (receiverContext)
            {
                this->ActorDispatch(this->GetActor(*message), message, receiverContext);
            }
        }
        else
        {
            // !!! it is assumed that replyHandler will not block
            ReplyHandler replyHandler;
            {
                Common::AcquireReadLock grab(this->lock_);
                replyHandler = this->replyHandler_;
            }

            if (replyHandler)
            {
                WriteNoise(
                    Constants::DemuxerTrace,
                    "Demuxer:{0}: dispatching reply message with RelatesTo = {1}",
                    TextTraceThis,
                    message->RelatesTo);
                replyHandler(*message, replyTargetSPtr);
            }
            else
            {
                WriteInfo(
                    Constants::DemuxerTrace,
                    "Demuxer:{0}: dropping message: missing handler, RelatesTo = {1}, Actor = {2}, Action = '{3}'",
                    TextTraceThis,
                    message->RelatesTo,
                    message->Actor,
                    message->Action);
            }
        }
    }

    template<typename TKey, typename TReceiverContext>
    void DemuxerT<TKey, TReceiverContext>::CallMessageHandler(
        MessageHandlerEntry const & handlerEntry,
        MessageUPtr & message,
        TReceiverContextUPtr & context)
    {
        if (handlerEntry.second/*dispatchOnTransportThread*/)
        {
            handlerEntry.first(message, context);
        }
        else
        {
            Common::MoveUPtr<Transport::Message> messageMover(message->Clone());
            Common::MoveUPtr<TReceiverContext> contextMover(std::move(context));
            MessageHandler const & handler = handlerEntry.first;
            Common::Threadpool::Post([handler, messageMover, contextMover] () mutable
            {
                TReceiverContextUPtr ctx = contextMover.TakeUPtr();
                MessageUPtr msg = messageMover.TakeUPtr();
                handler(msg, ctx);
            });
        }
    }

    template<typename TKey, typename TReceiverContext>
    void DemuxerT<TKey, TReceiverContext>::ActorDispatch(TKey const & actorKey, MessageUPtr & message, TReceiverContextUPtr & receiverContext)
    {
        MessageHandlerEntry handlerEntry;
        {
            Common::AcquireReadLock grab(lock_);

            auto iter = this->actorMap_.find(actorKey);
            if (iter == this->actorMap_.end())
            {
                WriteInfo(
                    Constants::DemuxerTrace,
                    "Demuxer:{0}: unknown actorKey {1}, dropping message {2}, Action = '{3}'",
                    TextTraceThis,
                    actorKey,
                    message->TraceId(),
                    message->Action);

                return;
            }

            handlerEntry = iter->second;
        } 

        CallMessageHandler(handlerEntry, message, receiverContext);
    }
}

