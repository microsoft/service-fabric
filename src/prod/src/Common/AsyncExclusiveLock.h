// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class LockAsyncOperation;

    class AsyncExclusiveLock
    {
        DENY_COPY(AsyncExclusiveLock);
    public:
        AsyncExclusiveLock();

        Common::AsyncOperationSPtr BeginAcquire(
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & root); 

        Common::ErrorCode EndAcquire(Common::AsyncOperationSPtr const & lockOperation);

        void Release(Common::AsyncOperationSPtr const & operation);

        // Pending operations need to be managed by the user
        void IncrementPendingOperations();

        LONG DecrementPendingOperations();

        size_t Test_GetListSize() { return operationQueue_.size(); }

    private:
        std::list<std::shared_ptr<LockAsyncOperation>> operationQueue_;
        Common::RwLock operationQueueLock_;
        Common::atomic_long pendingOperationCount_;
    };

    typedef std::shared_ptr<AsyncExclusiveLock> AsyncExclusiveLockSPtr;
}
