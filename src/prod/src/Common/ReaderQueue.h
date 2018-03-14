// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    template <class TUserItem>
    class ReaderQueue : public std::enable_shared_from_this<ReaderQueue<TUserItem>>
    {
        DENY_COPY( ReaderQueue );

        public:
            __declspec(property(get=get_PendingReaderCount)) size_t PendingReaderCount;
            __declspec(property(get=get_PendingDispatchCount)) size_t PendingDispatchCount;
            __declspec(property(get=get_ItemCount)) size_t ItemCount;

            static std::shared_ptr<ReaderQueue<TUserItem>> Create();
            size_t get_PendingReaderCount(void) const { return readers_.size(); }
            size_t get_PendingDispatchCount(void) const { return readersPendingDispatch_.size(); }
            size_t get_ItemCount(void) const { return items_.size(); }

            bool EnqueueAndDispatch(std::unique_ptr<TUserItem> &&);

            bool EnqueueWithoutDispatch(std::unique_ptr<TUserItem> &&);

            void Dispatch(void);

            Common::AsyncOperationSPtr BeginDequeue(
                Common::TimeSpan timeout, 
                AsyncCallback const & callback, 
                AsyncOperationSPtr const & parent);

            Common::ErrorCode EndDequeue(AsyncOperationSPtr const &, std::unique_ptr<TUserItem> &);

            // Waits until all items in the queue are dispatched;
            Common::AsyncOperationSPtr BeginWaitForQueueToDrain(
                AsyncCallback const & callback, 
                AsyncOperationSPtr const & parent);
            Common::ErrorCode EndWaitForQueueToDrain(AsyncOperationSPtr const &);

            void Close(void);

            void Abort(void);

            void ClearPendingItems(void);
            
            bool IsOpen() { return Opened == state_; }

        private:
            typedef Common::TimeSpan TimeSpan;
            typedef std::unique_ptr<TUserItem> UserItemSPtr;

            class ReaderContext;
            typedef std::shared_ptr<ReaderContext> ReaderContextSPtr;

            ReaderQueue(void);

            bool Enqueue(UserItemSPtr &&, bool);

            void CompleteReaders(std::list<AsyncOperationSPtr> const&);
            void FinishCompleteReaders(std::list<AsyncOperationSPtr> const&);

            enum State { Opened = 0, Closing, Closed };

            State state_;
            RWLOCK(ReaderQueue, thisLock);

            std::list<UserItemSPtr> items_;
            std::list<AsyncOperationSPtr> readers_;
            std::list<AsyncOperationSPtr> readersPendingDispatch_;
            AsyncOperationSPtr itemsDrainedAsyncOp_;

            class ReaderContext : public TimedAsyncOperation
            {
                public:
                    ReaderContext(AsyncCallback const & callback, TimeSpan timeout, AsyncOperationSPtr const & parent)
                        : TimedAsyncOperation(timeout, callback, parent),
                          itemSPtr(nullptr)
                    {
                    }

                    bool SetItem(UserItemSPtr && itemSPtr_arg)
                    {
                        if (TryStartComplete())
                        {
                            itemSPtr.swap(itemSPtr_arg);
                            return true;
                        }
                        else
                        {
                            return false;
                        }
                    }

                    UserItemSPtr GetItem()
                    {
                        return std::move(itemSPtr);
                    }

                    static ErrorCode End(AsyncOperationSPtr const & operationSPtr)
                    {
                        return (*AsyncOperation::End<ReaderContext>(operationSPtr)).Error;
                    }
                    
                private:
                    UserItemSPtr itemSPtr;
            }; // end inner class

            class WaitForQueueToDrainAsyncOperation : public AsyncOperation
            {
            public:
                WaitForQueueToDrainAsyncOperation(
                    AsyncCallback const & callback,
                    AsyncOperationSPtr const & state) 
                    :   AsyncOperation(callback, state)
                {
                }

                static ErrorCode End(AsyncOperationSPtr const & operationSPtr)
                {
                    return AsyncOperation::End<ReaderContext>(operationSPtr)->Error;
                }
            protected:
                virtual void OnStart(AsyncOperationSPtr const &)
                {
                }
            };            
    };

    template <class TUserItem>
    std::shared_ptr<ReaderQueue<TUserItem>> ReaderQueue<TUserItem>::Create()
    {
        return std::shared_ptr<ReaderQueue<TUserItem>>(new ReaderQueue<TUserItem>());
    }

    template <class TUserItem>
    ReaderQueue<TUserItem>::ReaderQueue() : 
        enable_shared_from_this<ReaderQueue<TUserItem>>(),
        state_(Opened), 
        items_(), 
        readers_(), 
        readersPendingDispatch_(),
        itemsDrainedAsyncOp_()
    {
    }

    template <class TUserItem>
    bool ReaderQueue<TUserItem>::EnqueueAndDispatch(UserItemSPtr && itemSPtr)
    {
        return Enqueue(std::move(itemSPtr), true);
    };

    template <class TUserItem>
    bool ReaderQueue<TUserItem>::EnqueueWithoutDispatch(UserItemSPtr && itemSPtr)
    {
        return Enqueue(std::move(itemSPtr), false);
    };

    template <class TUserItem>
    inline bool ReaderQueue<TUserItem>::Enqueue(UserItemSPtr && itemSPtr, bool doDispatch)
    {
        AsyncOperationSPtr aSPtr(nullptr);
        ReaderContext * readerPtr;
        bool doCompletion = false;

        {
            Common::AcquireExclusiveLock grab(thisLock);
            
            if (state_ != Opened)
            {
                return false;
            }

            if (readers_.empty())
            {
                items_.push_back(std::move(itemSPtr));
                return true;
            }

            bool success = false;
            do
            {
                aSPtr = readers_.front();
                readers_.pop_front();

                readerPtr = AsyncOperation::Get<ReaderContext>(aSPtr);

                // itemSPtr is still valid if SetItem() returns false.
                success = readerPtr->SetItem(std::move(itemSPtr));

                if (success)
                {
                    if (!doDispatch)
                    {
                        // Caller is expected to call Dispatch() later at some point
                        readersPendingDispatch_.push_back(std::move(aSPtr));
                    }
                    else
                    {
                        doCompletion = true;
                    }
                }
                else if (readers_.empty())
                {
                    // Don't do completion since the reader will complete
                    // with a timeout.
                    items_.push_back(std::move(itemSPtr));
                }
            } while (!success && !readers_.empty());
        } // end lock

        if (doCompletion)
        {
            // Caller is expected to schedule subsequent BeginDequeue()
            // in completion callback.
            readerPtr->FinishComplete(aSPtr, ErrorCode());
        }

        return true;
    };

    template <class TUserItem>
    void ReaderQueue<TUserItem>::Dispatch()
    {
        std::list<AsyncOperationSPtr> readersPendingDispatchSnapshot;
        
        {
            Common::AcquireExclusiveLock grab(thisLock);

            if (!readersPendingDispatch_.empty())
            {
                std::swap(readersPendingDispatch_, readersPendingDispatchSnapshot);
            }
        }
        
        FinishCompleteReaders(readersPendingDispatchSnapshot);
    };

    template <class TUserItem>
    AsyncOperationSPtr ReaderQueue<TUserItem>::BeginDequeue(
            TimeSpan timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & asyncState)
    {
        AsyncOperationSPtr asSPtr = AsyncOperation::CreateAndStart<ReaderContext>(
            callback,
            timeout,
            asyncState);

        auto readerPtr = AsyncOperation::Get<ReaderContext>(asSPtr);

        bool doCompletion = false;
        
        {
            Common::AcquireExclusiveLock grab(thisLock);

            if (Closed == state_ || (Closing == state_ && items_.empty()))
            {
                readerPtr->TryStartComplete();
                doCompletion = true;

                // Buffer has been flushed, so close down ReaderQueue
                if (Closing == state_ && items_.empty())
                {
                    state_ = Closed;
                    auto thisSPtr = this->shared_from_this();
                    Threadpool::Post([thisSPtr]() -> void { thisSPtr->CompleteReaders(thisSPtr->readers_); });
                }
            }
            else
            {
                if (items_.empty())
                {
                    readers_.push_back(asSPtr);
                }
                else 
                {
                    // itemSPtr is not swapped if SetItem() returns false.
                    if (readerPtr->SetItem(std::move(items_.front())))
                    {
                        items_.pop_front();
                                                
                        // This will complete on the same thread.
                        // The caller must schedule subsequent
                        // BeginDequeue calls in the completion callback
                        //
                        doCompletion = true;
                    }
                    else 
                    {
                        // Intentional no-op.
                        // Leave the item buffered for another reader
                        // because this reader will complete with
                        // a timeout.
                    }
                }
            }
        } // end lock

        if (doCompletion)
        {
            readerPtr->FinishComplete(asSPtr, ErrorCode());
        }

        return asSPtr;
    };

    template <class TUserItem>
    ErrorCode ReaderQueue<TUserItem>::EndDequeue(
            AsyncOperationSPtr const & contextSPtr,
            UserItemSPtr & result)
    {
        ErrorCode errorCode = ReaderContext::End(contextSPtr);

        // Result is only defined on success
        if (errorCode.IsSuccess())
        {
            AsyncOperation::Get<ReaderContext>(contextSPtr)->GetItem().swap(result);

             {
                Common::AcquireExclusiveLock grab(thisLock);
                if(itemsDrainedAsyncOp_ && items_.empty() && readersPendingDispatch_.empty())
                {
                    auto itemsDrainedOp = itemsDrainedAsyncOp_;
                    Threadpool::Post([itemsDrainedOp]()->void { itemsDrainedOp->TryComplete(itemsDrainedOp, ErrorCode(ErrorCodeValue::Success)); });
                }
            }
        }
        else if (errorCode.IsError(ErrorCodeValue::Timeout))
        {
            Common::AcquireExclusiveLock grab(thisLock);

            readers_.remove_if([&contextSPtr](AsyncOperationSPtr elemSPtr) -> bool
            {
                return (elemSPtr.get() == contextSPtr.get());
            });
        }

        return errorCode;
    };

    template <class TUserItem>
    void ReaderQueue<TUserItem>::Close()
    {
        std::list<AsyncOperationSPtr> readersSnapshot;

        {
            Common::AcquireExclusiveLock grab(thisLock);

            if (Closing == state_ || Closed == state_)
            {
                return;
            }

            state_ = Closing;

            if (items_.empty() && !readers_.empty())
            {
                state_ = Closed;
                std::swap(readers_, readersSnapshot);
            }
        }

        CompleteReaders(readersSnapshot);
    };


    template <class TUserItem>
    AsyncOperationSPtr ReaderQueue<TUserItem>::BeginWaitForQueueToDrain(
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
    {
        AsyncOperationSPtr asyncOp;
        bool shouldComplete = false;
        
        {
            Common::AcquireExclusiveLock grab(thisLock);
            // no more than one call can be made to wait for the reader queue to be drained.
            // this is because the contract for drain expects before starting to wait for drain, the
            // previous drain must have been completed and EndWaitForDrain must have been called.
            ASSERT_IF(itemsDrainedAsyncOp_ != nullptr, "a drain async operation is already defined.");
            itemsDrainedAsyncOp_ = AsyncOperation::CreateAndStart<WaitForQueueToDrainAsyncOperation>(callback, parent);

            if(items_.empty() && readersPendingDispatch_.size() == 0)
            {
                shouldComplete = true;
            }

            asyncOp = itemsDrainedAsyncOp_;
        }

        if(shouldComplete)
        {
            asyncOp->TryComplete(asyncOp, ErrorCode());
        }

        return asyncOp;
    }

    template <class TUserItem>
    Common::ErrorCode ReaderQueue<TUserItem>::EndWaitForQueueToDrain(
        AsyncOperationSPtr const & asyncOperation)
    {
        auto casted = AsyncOperation::End<WaitForQueueToDrainAsyncOperation>(asyncOperation);

        ErrorCode error = casted->Error;

        AsyncOperationSPtr asyncOp;
        {
            Common::AcquireExclusiveLock grab(thisLock);
            ASSERT_IF(!items_.empty() && !error.IsError(ErrorCodeValue::OperationCanceled), "EndWaitForQueueToDrain: {0} items left", items_.size());
            ASSERT_IF(itemsDrainedAsyncOp_ == nullptr, "EndWaitForQueueToDrain: itemsDrainedAsyncOp_ not set");
            itemsDrainedAsyncOp_.reset();
        }

        return error;
    }
    
    template <class TUserItem>
    void ReaderQueue<TUserItem>::Abort()
    {
        std::list<AsyncOperationSPtr> readersSnapshot;
        AsyncOperationSPtr itemsDrainedAsyncOp;

        {
            Common::AcquireExclusiveLock grab(thisLock);

            if (Closed == state_)
            {
                return;
            }

            state_ = Closed;

            if (!readers_.empty())
            {
                std::swap(readers_, readersSnapshot);
            }

            items_.clear();

            itemsDrainedAsyncOp = itemsDrainedAsyncOp_;
        }

        CompleteReaders(readersSnapshot);

        if (itemsDrainedAsyncOp)
        {
            itemsDrainedAsyncOp->TryComplete(itemsDrainedAsyncOp, ErrorCode());
        }
    };

    template <class TUserItem>
    void ReaderQueue<TUserItem>::ClearPendingItems()
    {
        AsyncOperationSPtr itemsDrainedAsyncOp;

        {
            Common::AcquireExclusiveLock grab(thisLock);

            items_.clear();

            if (readersPendingDispatch_.empty())
            {
                itemsDrainedAsyncOp = itemsDrainedAsyncOp_;
            }
        }

        if (itemsDrainedAsyncOp)
        {
            Threadpool::Post([itemsDrainedAsyncOp]()->void { itemsDrainedAsyncOp->TryComplete(itemsDrainedAsyncOp, ErrorCode(ErrorCodeValue::Success)); });
        }
    }

    template <class TUserItem>
    void ReaderQueue<TUserItem>::CompleteReaders(std::list<AsyncOperationSPtr> const& readers)
    {
        for(auto iter = readers.begin();
            iter != readers.end();
            iter++)
        {
            iter->get()->TryComplete(*iter, ErrorCode());
        }
    };

    template <class TUserItem>
    void ReaderQueue<TUserItem>::FinishCompleteReaders(std::list<AsyncOperationSPtr> const& readers)
    {
        for(auto iter = readers.begin();
            iter != readers.end();
            iter++)
        {
            iter->get()->FinishComplete(*iter, ErrorCode());
        }
    };
}

