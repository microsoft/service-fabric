// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Common/AsyncExclusiveLockEventSource.h"

using namespace std;

namespace Common
{
    AsyncExclusiveLockEventSource const LockTrace;

    class LockAsyncOperation : public TimedAsyncOperation
    {
    public:
        LockAsyncOperation(
            __in list<shared_ptr<LockAsyncOperation> > & operationQueue,
            __in RwLock & operationQueueLock,
            TimeSpan timeout, 
            AsyncCallback const & callback,
            AsyncOperationSPtr const & root)
          : TimedAsyncOperation(timeout, callback, root)
          , operationQueue_(operationQueue)
          , operationQueueLock_(operationQueueLock)
        {
        }

        static ErrorCode End(AsyncOperationSPtr const & operation)
        {
            auto casted = AsyncOperation::End<LockAsyncOperation>(operation);
            return casted->Error;
        }

        void SynchronousTakeLock(AsyncOperationSPtr const & thisSPtr)
        {
            LockTrace.EndLockAcquireSuccess(reinterpret_cast<uint64>(thisSPtr->Parent.get()));
            this->TryComplete(thisSPtr, ErrorCodeValue::Success);
        }

        bool TryAsynchronousTakeLock(AsyncOperationSPtr const & thisSPtr)
        {
            if (TryStartComplete())
            {
                LockTrace.EndLockAcquireSuccess(reinterpret_cast<uint64>(thisSPtr->Parent.get()));
                Threadpool::Post([thisSPtr]() { thisSPtr->FinishComplete(thisSPtr, ErrorCodeValue::Success); });
                return true;
            }
            else
            {
                return false;
            }
        }

    protected:
        void OnStart(AsyncOperationSPtr const & thisSPtr)
        {
            shared_ptr<LockAsyncOperation> thisLockSPtr = static_pointer_cast<LockAsyncOperation, AsyncOperation>(thisSPtr);
            LockTrace.BeginLockAcquire(reinterpret_cast<uint64>(thisLockSPtr->Parent.get()));

            bool hasLock = false;
            {
                AcquireExclusiveLock lock(operationQueueLock_);
                if (operationQueue_.empty())
                {
                    hasLock = true;
                }

                operationQueue_.push_back(thisLockSPtr); 
            }
            
            if (hasLock)
            {
                SynchronousTakeLock(thisLockSPtr);
            }
            else
            {
                TimedAsyncOperation::OnStart(thisSPtr);
            }
        }

        void OnTimeout(AsyncOperationSPtr const & thisSPtr)
        {
            AcquireExclusiveLock lock(operationQueueLock_);

            LockTrace.EndLockAcquireTimeout(reinterpret_cast<uint64>(thisSPtr->Parent.get()));

            operationQueue_.remove_if([&thisSPtr](shared_ptr<LockAsyncOperation> const & item) { return (item.get() == thisSPtr.get()); });
        }

    private: 
        list<shared_ptr<LockAsyncOperation>> & operationQueue_;
        RwLock & operationQueueLock_;
    };

    AsyncExclusiveLock::AsyncExclusiveLock()
      : operationQueue_()
      , operationQueueLock_()
      , pendingOperationCount_(0)
    {
    }

    AsyncOperationSPtr AsyncExclusiveLock::BeginAcquire(
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
    {
        return AsyncOperation::CreateAndStart<LockAsyncOperation>(
            operationQueue_,
            operationQueueLock_,
            timeout,
            callback,
            root);
    }

    ErrorCode AsyncExclusiveLock::EndAcquire(AsyncOperationSPtr const & lockOperation)
    {
        return LockAsyncOperation::End(lockOperation);
    }

    void AsyncExclusiveLock::Release(AsyncOperationSPtr const & operation)
    {
        ASSERT_IF(operationQueue_.empty(), "Nothing is holding the lock.");
        ASSERT_IFNOT(
            operation == operationQueue_.front()->Parent,
            "AsyncOperation {0} does not own lock.",
            reinterpret_cast<uint64>(operation.get()));

        LockTrace.LockRelease(reinterpret_cast<uint64>(operation.get()));
        bool lockAcquired = false;
        bool queueEmpty = false;
        shared_ptr<LockAsyncOperation> lockOwner = nullptr;

        while(!lockAcquired && !queueEmpty)
        {
            {
                AcquireExclusiveLock lock(operationQueueLock_);
                operationQueue_.pop_front();
                if (operationQueue_.empty())
                {
                    queueEmpty = true;
                }
                else
                {                
                    lockOwner = operationQueue_.front();
                    queueEmpty = false;
                }
            }

            if (lockOwner)
            {
                lockAcquired = lockOwner->TryAsynchronousTakeLock(lockOwner);
            }
        }        
    }
    void AsyncExclusiveLock::IncrementPendingOperations()
    {
        ++pendingOperationCount_;
    }

    LONG AsyncExclusiveLock::DecrementPendingOperations()
    {
        LONG value = --pendingOperationCount_;
        ASSERT_IF(value < 0, "Cannot decrement below 0");
        return value;
    }
}
