// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    template<bool ManualReset>
    class AsyncWaitHandle : public WaitHandle<ManualReset>
    {
        DENY_COPY(AsyncWaitHandle)

    public:
        AsyncWaitHandle(bool initialState = false, std::wstring const & eventName = L"");

        Common::AsyncOperationSPtr BeginWaitOne(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndWaitOne(
            Common::AsyncOperationSPtr const & operation);

#ifdef PLATFORM_UNIX

        ~AsyncWaitHandle();

    private:
        void OnClose() override;
        void OnSetEvent() override;

        ErrorCode TryAddWaitOperation(AsyncOperationSPtr const & wop);
        std::deque<AsyncOperationSPtr> TryStartComplete_MutexHeld();
        static void FinishComplete(std::deque<AsyncOperationSPtr> const & ops);
        void EventFdReadCallback(int fd, uint evts);

        int eventFd_ = -1;
        EventLoop* evtLoop_ = nullptr;
        EventLoop::FdContext* fdCtx_ = nullptr;

        class WaiterQueue
        {
        public:
            void Enqueue(AsyncOperationSPtr const & wop);
            void Erase(AsyncOperationSPtr const & wop);
            AsyncOperationSPtr PopFront();
            std::deque<AsyncOperationSPtr> PopAll();

        private:
            RwLock lock_;
            std::deque<AsyncOperationSPtr> queue_;
        };

        class WaitAsyncOperation : public TimedAsyncOperation
        {
        public:
            WaitAsyncOperation(
                AsyncWaitHandle<ManualReset> & owner,
                TimeSpan const timeout,
                AsyncCallback const & callback,
                AsyncOperationSPtr const & parent);

        protected:
            void OnStart(AsyncOperationSPtr const & thisSPtr) override; 
            void OnTimeout(AsyncOperationSPtr const & thisSPtr) override;
            void OnCancel() override;

        private:
            void LeaveWaiterQueue(AsyncOperationSPtr const & thisSPtr);

            AsyncWaitHandle& owner_;
            std::weak_ptr<WaiterQueue> waiterQueueWPtr_;
        };

        std::shared_ptr<WaiterQueue> waiters_;
#endif
    };

    using AsyncAutoResetEvent = AsyncWaitHandle<false>;
    using AsyncManualResetEvent = AsyncWaitHandle<true>;
}
