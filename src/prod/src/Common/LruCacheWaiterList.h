// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    static const StringLiteral LruCacheWaiterTraceType("LruCacheWaiterAsyncOperation");
    // This class is used by the LruCache to maintain an EmbeddedList
    // of waiter async operations. An EmbeddedList is used instead
    // of a normal vector so that waiters can unlink themselves from
    // the list efficiently on timeout.
    //
    // The first waiter will complete with a flag indicating that it
    // is the first waiter, but its corresponding async operation will
    // remain in the list to block subsequent waiters. The first waiter
    // is responsible for completing all waiters and clearing the list
    // on completion (either successfully or on failure).
    //
    template <typename TWaitEntry>
    class LruCacheWaiterAsyncOperation : public TimedAsyncOperation
    {
        DENY_COPY(LruCacheWaiterAsyncOperation)

    public:
        LruCacheWaiterAsyncOperation(
            TimeSpan const timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
            : TimedAsyncOperation(timeout, callback, parent)
            , entry_()
            , isFirstWaiter_(false)
            , onTimeout_(nullptr)
            , lock_()
        {
        }

        LruCacheWaiterAsyncOperation(
            std::shared_ptr<TWaitEntry> const & entry,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
            : TimedAsyncOperation(TimeSpan::Zero, callback, parent)
            , entry_(entry)
            , isFirstWaiter_(false)
            , onTimeout_(nullptr)
            , lock_()
        {
        }

        virtual ~LruCacheWaiterAsyncOperation() { }

        static ErrorCode End(
            AsyncOperationSPtr const& operation,
            __out bool & isFirstWaiter,
            __out std::shared_ptr<TWaitEntry> & entry)
        {
            auto casted = AsyncOperation::End<LruCacheWaiterAsyncOperation>(operation);

            if (casted->Error.IsSuccess())
            {
                isFirstWaiter = casted->isFirstWaiter_;
                entry = move(casted->entry_);
            }
            else
            {
                isFirstWaiter = false;
            }

            return casted->Error;
        }

        static std::shared_ptr<LruCacheWaiterAsyncOperation<TWaitEntry>> Create(
            std::shared_ptr<TWaitEntry> const & entry,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
        {
            auto waiter = std::make_shared<LruCacheWaiterAsyncOperation<TWaitEntry>>(
                entry,
                callback,
                parent);
            waiter->Start(waiter);

            return waiter;
        }

        static std::shared_ptr<LruCacheWaiterAsyncOperation<TWaitEntry>> Create(
            ErrorCode const & error,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
        {
            auto waiter = std::make_shared<LruCacheWaiterAsyncOperation<TWaitEntry>>(
                TimeSpan::Zero,
                callback,
                parent);
            waiter->Start(waiter);

            // Failed waiter is used to pass an error asynchronously to end.
            // It won't be added to the waiters list and hence won't
            // be completed by anyone else.
            //
            waiter->ExternalComplete(waiter, error);

            return waiter;
        }

        void InitializeIsFirstWaiter(bool isFirstWaiter)
        {
            isFirstWaiter_ = isFirstWaiter;
        }

        void InitializeOnTimeout(AsyncCallback const & onTimeout)
        {
            AcquireExclusiveLock lock(lock_);

            if (!this->InternalIsCompleted)
            {
                onTimeout_ = onTimeout;
            }
        }

        void StartOutsideLock(AsyncOperationSPtr const& thisSPtr)
        {
            if (entry_ || isFirstWaiter_)
            {
                this->TryComplete(thisSPtr, ErrorCodeValue::Success);
            }
            else
            {
                AcquireExclusiveLock lock(lock_);

                if (!this->InternalIsCompleted)
                {
                    TimedAsyncOperation::StartTimerCallerHoldsLock(thisSPtr);
                }
            }
        }

        void ExternalComplete(
            AsyncOperationSPtr const & thisSPtr,
            std::shared_ptr<TWaitEntry> const & entry)
        {
            if (this->TryStartComplete())
            {
                entry_ = entry;

                this->FinishComplete(thisSPtr, ErrorCodeValue::Success);
            }
        }

        void ExternalComplete(
            AsyncOperationSPtr const& thisSPtr,
            ErrorCode const & error)
        {
            this->TryComplete(thisSPtr, error);
        }

    protected:

        virtual void OnStart(AsyncOperationSPtr const &)
        {
            // intentional no-op: StartOutsideLock() must be called
        }

        virtual void OnTimeout(AsyncOperationSPtr const & thisSPtr)
        {
            if (onTimeout_)
            {
                onTimeout_(thisSPtr);
            }
        }

        virtual void OnCompleted()
        {
            {
                AcquireExclusiveLock lock(lock_);

                TimedAsyncOperation::OnCompleted();
            }

            // TimedAsyncOperation calls OnTimeout()
            // before FinishComplete()
            //
            onTimeout_ = nullptr;
        }

    private:
        std::shared_ptr<TWaitEntry> entry_;
        bool isFirstWaiter_;
        AsyncCallback onTimeout_;
        ExclusiveLock lock_;
    };

    template <typename TWaitEntry>
    class LruCacheWaiterList
    {
        DENY_COPY(LruCacheWaiterList)

    private:
        typedef LruCacheWaiterAsyncOperation<TWaitEntry> WaiterListEntry;
        typedef EmbeddedList<WaiterListEntry> WaiterList;
        typedef std::shared_ptr<WaiterList> WaiterListSPtr;

    public:
        LruCacheWaiterList()
            : waiters_(std::make_shared<WaiterList>())
            , waitersLock_()
        { }

        size_t GetSize() const
        {
            AcquireReadLock lock(waitersLock_);

            return waiters_->GetSize();
        }

        std::shared_ptr<LruCacheWaiterAsyncOperation<TWaitEntry>> AddWaiter(
            TimeSpan const timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
        {
            auto waiter = std::make_shared<WaiterListEntry>(
                timeout,
                callback,
                parent);
            waiter->Start(waiter);

            auto embeddedWaiter = std::make_shared<EmbeddedListEntry<WaiterListEntry>>(std::move(waiter));

            WaiterListSPtr list;
            bool isFirstWaiter;
            {
                AcquireReadLock lock(waitersLock_);

                list = waiters_;

                isFirstWaiter = (list->UpdateHead(embeddedWaiter).get() == nullptr);
            }

            if (isFirstWaiter)
            {
                embeddedWaiter->GetListEntry()->InitializeIsFirstWaiter(true);
            }
            else
            {
                embeddedWaiter->GetListEntry()->InitializeOnTimeout([list, embeddedWaiter](AsyncOperationSPtr const &)
                {
                    list->RemoveFromList(embeddedWaiter);
                });
            }

            // Caller must call StartOutsideLock() to start the waiter

            return embeddedWaiter->GetListEntry();
        }

        void CompleteWaiters(std::shared_ptr<TWaitEntry> const & entry)
        {
            WaiterListSPtr list = this->TakeWaitersList();

            if (list)
            {
                Threadpool::Post([list, entry]() { SuccessCompletionCallback(list, entry); });
            }
        }

        void CompleteWaiters(ErrorCode const & error)
        {
            WaiterListSPtr list = this->TakeWaitersList();

            if (list)
            {
                Threadpool::Post([list, error]() { ErrorCompletionCallback(list, error); });
            }
        }

    private:

        WaiterListSPtr TakeWaitersList()
        {
            // Must ensure that no other waiters can be added to this
            // list once it's been swapped or they won't get completed.
            // Removal is okay.
            //
            auto list = std::make_shared<WaiterList>();
            {
                AcquireWriteLock lock(waitersLock_);

                list.swap(waiters_);

                // Optimization: Don't need to post completion if only the first waiter exists.
                //               Clearing the first waiter from the list is sufficient since
                //               it's already completed.
                //
                auto size = list->GetSize();
                if (size < 1 || (size == 1 && list->GetHead()->GetListEntry()->IsCompleted))
                {
                    list.reset();
                }
            }
            return list;
        }

        static void SuccessCompletionCallback(
            WaiterListSPtr const & list,
            std::shared_ptr<TWaitEntry> const & entry)
        {
            auto waiters = list->TakeEntriesAndClear();
            for (auto const & waiter : waiters)
            {
                auto const & casted = waiter->GetListEntry();

                casted->ExternalComplete(casted, entry);
            }
        }

        static void ErrorCompletionCallback(
            WaiterListSPtr const & list,
            ErrorCode const & error)
        {
            auto waiters = list->TakeEntriesAndClear();
            for (auto const & waiter : waiters)
            {
                auto const & casted = waiter->GetListEntry();

                casted->ExternalComplete(casted, error);
            }
        }

        WaiterListSPtr waiters_;
        mutable RwLock waitersLock_;
    };
}
