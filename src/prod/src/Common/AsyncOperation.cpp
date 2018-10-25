// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;

namespace Common
{
    StringLiteral TraceType_AsyncOperation = "AsyncOperation";

    const wstring AsyncOperationLifetimeSource = L"AsyncOperation.Lifetime";

    AsyncOperation::AsyncOperation()
        : enable_shared_from_this<AsyncOperation>(),
          state_(),
          callback_(nullptr),
          parent_(),
          error_(ErrorCode()),
          childLock_(),
          children_(),
          skipCompleteOnCancel_(false)
    {
        traceId_ = wformatString("{0}", static_cast<void*>(this));
    }

    AsyncOperation::AsyncOperation(AsyncCallback const & callback, AsyncOperationSPtr const & parent, bool skipCompleteOnCancel)
        : enable_shared_from_this<AsyncOperation>(),
          state_(),
          callback_(callback),
          parent_(parent),
          error_(ErrorCode()),
          childLock_(),
          children_(),
          skipCompleteOnCancel_(skipCompleteOnCancel)
    {
        traceId_ = wformatString("{0}", static_cast<void*>(this));
    }

    AsyncOperation::~AsyncOperation()
    {
    }

    void AsyncOperation::Start(AsyncOperationSPtr const & thisSPtr)
    {
        ASSERT_IF(thisSPtr.get() != this, "AsyncOperation::Start thisSPtr.get() != this");

        state_.TransitionStarted();
        bool attachToParentSucceeded = true;

        if (parent_)
        {
            attachToParentSucceeded = parent_->AttachChild(thisSPtr);
        }

        Trace.WriteNoise(
            TraceType_AsyncOperation,
            traceId_,
            "Calling OnStart");
        
        // ALWAYS Call OnStart() before attempting to cancel the operation as it is a contract many components depend on
        OnStart(thisSPtr);

        if (attachToParentSucceeded == false)
        {
            Trace.WriteNoise(
                TraceType_AsyncOperation,
                traceId_,
                "Attempting to cancel async op as attaching to parent failed");

            // One of the usage pattern is to CreateAndStart async operation under a lock,
            // with the expectation that no work is done in Start to take that lock.
            // When the operation is cancelled, it is completed, and OnComplete may take the same lock to change state.
            // To prevent any deadlocks, post Cancel so it executes outside the lock.
            // A better fix is tracked through SQL Server: 1209734: Change AsyncOperation Start to return ErrorCode 
            Threadpool::Post([thisSPtr]() { thisSPtr->Cancel(true/*forceCancel*/); });
            return;
        }
    }

    bool AsyncOperation::TryStartComplete()
    {
        return state_.TryTransitionCompleting();
    }

    void AsyncOperation::FinishComplete(AsyncOperationSPtr const & thisSPtr, ErrorCode const & error)
    {
        error_ = error;
        FinishCompleteInternal(thisSPtr);
    }

    void AsyncOperation::FinishComplete(AsyncOperationSPtr const & thisSPtr, ErrorCode && error)
    {
        error_ = move(error);
        FinishCompleteInternal(thisSPtr);
    }

    void AsyncOperation::FinishCompleteInternal(AsyncOperationSPtr const & thisSPtr)
    {
        ASSERT_IF(thisSPtr.get() != this, "AsyncOperation::FinishComplete thisSPtr.get() != this");

        Trace.WriteNoise(
            TraceType_AsyncOperation,
            traceId_,
            "FinishComplete called with {0}",
            error_);

        state_.TransitionCompleted();
        OnCompleted();

        if (callback_ != 0)
        {
            callback_(thisSPtr);
        }

        Cleanup(thisSPtr);
    }


    bool AsyncOperation::TryComplete(AsyncOperationSPtr const & thisSPtr, ErrorCode && error)
    {
        if (TryStartComplete())
        {
            FinishComplete(thisSPtr, move(error));
            return true;
        }
        else
        {
            error.ReadValue();
            return false;
        }
    }

    bool AsyncOperation::TryComplete(AsyncOperationSPtr const & thisSPtr, ErrorCode const & error)
    {
        if (TryStartComplete())
        {
            FinishComplete(thisSPtr, error);
            return true;
        }
        else
        {
            error.ReadValue();
            return false;
        }
    }

    void AsyncOperation::Cancel(bool forceComplete)
    {
        if(state_.TryMarkCancelRequested())
        {
            Trace.WriteNoise(
                TraceType_AsyncOperation,
                traceId_,
                "Cancel called with forceComplete={0} and skipCompleteOnCancel={1}",
                forceComplete,
                skipCompleteOnCancel_);

            // First attempt to Cancel the child, if any.
            vector<AsyncOperationSPtr> children;
            {
                AcquireExclusiveLock acquire(childLock_);
                for(auto & child : children_)
                {
                    auto objectToMove = child.lock();
                    if(objectToMove)
                    {
                        children.push_back(move(objectToMove));
                    }                        
                }

                children_.clear();
            }

            for(auto & child : children)
            {
                child->Cancel();
            }
            
            if (!forceComplete && skipCompleteOnCancel_)
            {
                // The case where Cancel is treated as only signal
                // TODO - create a separate method for this to indicate signal
                OnCancel();
            }
            else
            {
                if (TryStartComplete())
                {
                    OnCancel();
                    FinishComplete(shared_from_this(), ErrorCodeValue::OperationCanceled);
                }
            }
        }
    }

    void AsyncOperation::Cleanup(AsyncOperationSPtr const & thisSPtr)
    {
        {
            AcquireExclusiveLock acquire(childLock_);
            children_.clear();
        }

        if (parent_)
        {
            parent_->DetachChild(thisSPtr);
        }
        
        callback_ = nullptr;
    }

    void AsyncOperation::OnCancel()
    {
        // Override this if the notification is needed when cancelled
    }

    void AsyncOperation::OnCompleted()
    {
    }

    bool AsyncOperation::AttachChild(AsyncOperationSPtr && child)
    {
        Trace.WriteNoise(
            TraceType_AsyncOperation,
            traceId_,
            "Attempting to attach child AsyncOperation {0}.",
            wformatString("{0}", static_cast<void*>(child.get())));

        {
            AcquireExclusiveLock acquire(childLock_);
            // If there is a racing Cancel call, we need to ensure that either we see that
            // this is canceled, or Cancel sees the child attached.
            //
            // Checking canceled under the lock guarantees that a non-trivial Cancel() cannot
            // start and finish between the check and setting the child.
            if ((state_.InternalIsCompletingOrCompleted == false) && 
                (state_.InternalIsCancelRequested == false))
            {
                children_.push_back(move(child));
                return true;
            }
        }

        Trace.WriteNoise(
            TraceType_AsyncOperation,
            traceId_,
            "Not attaching child AsyncOperation {0} as parent in completed/completing/cancelling.",
            wformatString("{0}", static_cast<void*>(child.get())));

        return false;
    }

    bool AsyncOperation::AttachChild(AsyncOperationSPtr const & child)
    {
        AsyncOperationSPtr local = child;
        return AttachChild(move(local));
    }

    void AsyncOperation::DetachChild(AsyncOperationSPtr const & child)
    {
        Trace.WriteNoise(
            TraceType_AsyncOperation,
            traceId_,
            "Detaching child AsyncOperation {0}.",
            wformatString("{0}", static_cast<void*>(child.get())));

        {
            AcquireExclusiveLock acquire(childLock_);

            children_.remove_if([child](weak_ptr<AsyncOperation> const &value) 
            {
                auto toCheck = value.lock();
                return toCheck.get() == nullptr ? true : (child.get() == toCheck.get());
            });
        }

    }
}
