// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Common
{
    const StringLiteral TraceComponent_ComAsyncOperationContext("ComAsyncOperationContext");

    ComAsyncOperationContext::ComAsyncOperationContext(bool skipCompleteOnCancel)
        : ComUnknownBase(),
          state_(),
          callback_(),
          result_(ErrorCodeValue::Success),
          childLock_(),
          child_(),
          rootSPtr_(),
          skipCompleteOnCancel_(skipCompleteOnCancel)
    {
    }

    ComAsyncOperationContext::~ComAsyncOperationContext() { }

    HRESULT STDMETHODCALLTYPE ComAsyncOperationContext::Initialize(
        __in ComponentRootSPtr const & rootSPtr,
        __in_opt IFabricAsyncOperationCallback * callback)
    {
        rootSPtr_ = rootSPtr;

        if (callback != NULL)
        {
            callback_.SetAndAddRef(callback);
        }

        return S_OK;
    }        

    BOOLEAN STDMETHODCALLTYPE ComAsyncOperationContext::IsCompleted()
    {
        return state_.IsCompleted;
    }

    BOOLEAN STDMETHODCALLTYPE ComAsyncOperationContext::CompletedSynchronously()
    {
        return state_.CompletedSynchronously;
    }

    HRESULT STDMETHODCALLTYPE ComAsyncOperationContext::get_Callback(
        __out IFabricAsyncOperationCallback ** state)
    {
        if (state == NULL)
        {
            return ComUtility::OnPublicApiReturn(E_INVALIDARG);
        }

        ComPointer<IFabricAsyncOperationCallback> copy = callback_;
        *state = copy.DetachNoRelease();
        return ComUtility::OnPublicApiReturn(S_OK);
    }

    HRESULT STDMETHODCALLTYPE ComAsyncOperationContext::Cancel()
    {
        if (state_.TryMarkCancelRequested())
        {
            AsyncOperationSPtr child;

            {
                AcquireExclusiveLock acquire(childLock_);
                child = child_.lock();
            }

            if (child)
            {
                child->Cancel();
            }

            if (!skipCompleteOnCancel_)
            {
                if (TryStartComplete())
                {
                    ComPointer<ComAsyncOperationContext> thisCPtr;
                    thisCPtr.SetAndAddRef(this);
                    FinishComplete(thisCPtr, E_ABORT);
                }
            }
        }

        return ComUtility::OnPublicApiReturn(S_OK);
    }

    ComAsyncOperationContextCPtr const & ComAsyncOperationContext::GetCPtr(AsyncOperationSPtr const & proxySPtr)
    {
        return AsyncOperationRoot<ComAsyncOperationContextCPtr>::Get(proxySPtr);
    }

    HRESULT ComAsyncOperationContext::Start(
        ComAsyncOperationContextCPtr const & thisCPtr)
    {
        CheckThisCPtr(thisCPtr);
        state_.TransitionStarted();

        AsyncOperationSPtr proxySPtr = AsyncOperationRoot<ComAsyncOperationContextCPtr>::Create(thisCPtr);

        Trace.WriteNoise(
            TraceComponent_ComAsyncOperationContext,
            proxySPtr->AsyncOperationTraceId,
            "Calling OnStart");

        OnStart(proxySPtr);

        AsyncOperationSPtr cancel;

        // If there is a racing Cancel call, we need to ensure that either we see that
        // this is canceled, or Cancel sees the child attached.
        //
        // Checking canceled under the lock guarantees that a non-trivial Cancel() cannot
        // start and finish between the check and setting the child.
         {
            AcquireExclusiveLock acquire(childLock_);

            if (state_.InternalIsCompletingOrCompleted || state_.InternalIsCancelRequested)
            {
                cancel = std::move(proxySPtr);
            }
       
            // Moves child if we did not already move to cancel, otherwise clears child_.
            child_ = std::move(proxySPtr);
        }

        if (cancel)
        {
            cancel->Cancel(true/*forceComplete*/);
        }

        return S_OK;
    }

    bool ComAsyncOperationContext::TryStartComplete()
    {
        return state_.TryTransitionCompleting();
    }

    void ComAsyncOperationContext::FinishComplete(
        AsyncOperationSPtr const & proxySPtr,
        HRESULT result)
    {
        FinishComplete(GetCPtr(proxySPtr), result);
    }

    void ComAsyncOperationContext::FinishComplete(
        ComAsyncOperationContextCPtr const & thisCPtr,
        HRESULT result)
    {
        FinishComplete(thisCPtr, ErrorCode::FromHResult(result));
    }

    void ComAsyncOperationContext::FinishComplete(
        ComAsyncOperationContextCPtr const & thisCPtr,
        ErrorCode const & error)
    {
        CheckThisCPtr(thisCPtr);

        result_ = error;
        state_.TransitionCompleted();
        
        if (callback_)
        {
            callback_->Invoke(this);
        }

        Cleanup();
    }

    bool ComAsyncOperationContext::TryComplete(
        AsyncOperationSPtr const & proxySPtr,
        HRESULT result)
    {
        return TryComplete(proxySPtr, ErrorCode::FromHResult(result));
    }

    bool ComAsyncOperationContext::TryComplete(
        AsyncOperationSPtr const & proxySPtr,
        ErrorCode const & result)
    {
        if (TryStartComplete())
        {
            Trace.WriteNoise(
                TraceComponent_ComAsyncOperationContext,
                proxySPtr->AsyncOperationTraceId,
                "FinishComplete calling with {0}",
                result);

            FinishComplete(GetCPtr(proxySPtr), result);
            return true;
        }
        else
        {
            return false;
        }
    }

    HRESULT ComAsyncOperationContext::End()
    {
        if (state_.TryTransitionEnded())
        {
            result_.SetThreadErrorMessage();

            return get_Result();
        }
        else
        {
            return FABRIC_E_OPERATION_NOT_COMPLETE;
        }
    }

    void ComAsyncOperationContext::Cleanup()
    {
        callback_.SetNoAddRef(NULL);

        {
            AcquireExclusiveLock acquire(childLock_);
            child_.reset();
        }
    }
}
