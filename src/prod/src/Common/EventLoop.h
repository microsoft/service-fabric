// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class EventLoop : public Common::TextTraceComponent<Common::TraceTaskCodes::Common>
    {
    public:
        typedef std::function<void(int fd, uint event)> Callback;

        EventLoop();
        ~EventLoop();

        class FdContext;
        FdContext* RegisterFd(int fd, uint events, bool dispatchEventAsync, Callback const & cb);
        void UnregisterFd(FdContext* fdc, bool waitForCallback);

        Common::ErrorCode Activate(FdContext* fdc);

        static bool IsFdClosedOrInError(uint events) { return events & (EPOLLERR|EPOLLHUP); }

        void SetSchedParam(int policy, int priority);

    private:
        typedef std::unordered_map<int, std::shared_ptr<FdContext>> FdMap;

        void Setup();
        void Cleanup();
        void Loop();
        static void* PthreadFunc(void*);

        Common::RwLock lock_;
        std::wstring id_;
        int epfd_;
        pthread_t tid_;
        FdMap fdMap_;
        volatile size_t fdMapSize_;
        std::vector<epoll_event> reportList_;
    };

    class EventLoopPool : public Common::TextTraceComponent<Common::TraceTaskCodes::Common>
    {
    public:
        EventLoopPool(
            std::wstring const & tag = L"",
            uint concurrency = 0);

        EventLoop& Assign();
        void AssignPair(EventLoop** inLoop, EventLoop** outLoop);

        void SetSchedParam(int policy, int priority);

        static EventLoopPool* GetDefault(); 

    private:
        EventLoop* Assign_CallerHoldingLock();

        Common::RwLock lock_;
        std::wstring id_;
        std::vector<std::unique_ptr<EventLoop>> pool_;
        uint assignmentIndex_;
    };
}
