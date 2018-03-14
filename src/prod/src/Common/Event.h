// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    /// <summary>
    /// The class for representing an event
    /// </summary>
    template <class TEventArgs=EventArgs>
    class EventT
    {
        DENY_COPY(EventT);
    public:

        typedef std::function<void(TEventArgs const&)> EventHandler;
        typedef int64 HHandler;
        static const int64 InvalidHHandler = -1;

        EventT() 
        : closed_(false),
        copyNeeded_(true),
        handlerId_(0),
        handlerCache_(std::make_shared<std::vector<EventHandler>>(std::vector<EventHandler>()))
        {}

        ~EventT() {}

        __declspec (property(get=getHandlerCount)) size_t HandlerCount;
        size_t getHandlerCount() const
        {
            Common::AcquireReadLock grab(lock_);
            return handlers_.size();
        }

        HHandler Add(EventHandler const & handler)
        {
            Common::AcquireWriteLock grab(lock_);
            if (closed_) { return InvalidHHandler; }

            Invariant(handlerId_ < std::numeric_limits<decltype(handlerId_)>::max());
            ++ handlerId_;

            handlers_.insert(make_pair(handlerId_, handler));
            copyNeeded_ = true;
            return handlerId_;
        }

        bool Remove(HHandler handleOfHandler)
        {
            Common::AcquireWriteLock grab(lock_);
            if (closed_) { return false; }

            try
            {
                handlers_.erase(handleOfHandler);
	            handlerCache_.reset();
                copyNeeded_ = true;
            }
            catch (...)
            {
                int i = 1;
                
                throw i;
            }
            return true;
        }

        void Close()
        {
            Common::AcquireWriteLock grab(lock_);
            closed_ = true;
            handlers_.clear();
            handlerCache_.reset();
        }

        void Fire(TEventArgs const & args, bool useAnotherThread = false) const
        {
            std::shared_ptr<std::vector<EventHandler>> handlersLocalCopy;
            {
                Common::AcquireWriteLock grab(lock_);
                if (closed_) { return; }

                if (copyNeeded_)
                {
                    handlersLocalCopy = std::make_shared<std::vector<EventHandler>>(handlers_.size());
                    for (auto iter = handlers_.begin(); iter != handlers_.end(); ++ iter)
                    {
                        handlersLocalCopy->push_back(iter->second);
                    }
                    handlerCache_ = handlersLocalCopy;
                    copyNeeded_ = false;
                }
                else
                {
                    handlersLocalCopy = handlerCache_;
                }
            }

            for (auto iter = handlersLocalCopy->begin(); iter != handlersLocalCopy->end(); ++ iter)
            {
                EventHandler & handler = *iter;
                if (handler == nullptr)
                {
                    continue;
                }

                if (useAnotherThread)
                {
                    Threadpool::Post([handler, args]()
                    {
                        handler(args);
                    });
                }
                else
                {
                    handler(args);
                }
            }
        }

    private:
        bool closed_;
        std::map<HHandler, EventHandler> handlers_;
        mutable std::shared_ptr<std::vector<EventHandler>> handlerCache_;
        mutable bool copyNeeded_;
        HHandler handlerId_;
        mutable Common::RwLock lock_;
    };

    typedef EventT<>::EventHandler EventHandler;
    typedef EventT<>::HHandler HHandler;
    typedef EventT<> Event;
}

