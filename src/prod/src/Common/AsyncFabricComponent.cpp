// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

class AsyncFabricComponent::OpenAsyncOperation : public AsyncOperation
{
    DENY_COPY(OpenAsyncOperation)

public : 
    OpenAsyncOperation(
        AsyncFabricComponent & owner,
        TimeSpan timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent) 
        : AsyncOperation(callback, parent),
        owner_(owner), timeout_(timeout)
    {
    }

    ~OpenAsyncOperation(void)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & asyncOperation)
    {
        auto thisPtr = AsyncOperation::End<OpenAsyncOperation>(asyncOperation);
        return thisPtr->Error;
    }

protected:
    virtual void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        ErrorCode error = owner_.state_.TransitionToOpening();
        if(error.IsSuccess())
        {
            owner_.OnBeginOpen(
                timeout_,
                [](AsyncOperationSPtr const & onOpenOperation){ AsyncFabricComponent::OpenAsyncOperation::OnOnOpenCompleted(onOpenOperation); },
                thisSPtr);
        }
        else
        {
            TryComplete(thisSPtr, error);
        }
    }

    virtual void OnCompleted()
    {
        AsyncOperation::OnCompleted();
    }

private:
    static void OnOnOpenCompleted(AsyncOperationSPtr const & onOpenOperation)
    {
        auto thisPtr = AsyncOperation::Get<OpenAsyncOperation>(onOpenOperation->Parent);

        auto errorCode = thisPtr->owner_.OnEndOpen(onOpenOperation);
        if (errorCode.IsSuccess())
        {
            errorCode = thisPtr->owner_.state_.TransitionToOpened();
        }

        if (!errorCode.IsSuccess())
        {
            thisPtr->owner_.state_.TransitionToFailed();
            thisPtr->owner_.Abort();
        }

        thisPtr->TryComplete(onOpenOperation->Parent, errorCode);
    }

    AsyncFabricComponent & owner_;
    TimeSpan timeout_;
};

class AsyncFabricComponent::CloseAsyncOperation : public AsyncOperation
{
    DENY_COPY(CloseAsyncOperation)

public : 
    CloseAsyncOperation(
        AsyncFabricComponent & owner, 
        TimeSpan timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent) 
        : AsyncOperation(callback, parent),
        owner_(owner), timeout_(timeout)
    {
    }

    ~CloseAsyncOperation(void)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & asyncOperation)
    {
        auto thisPtr = AsyncOperation::End<CloseAsyncOperation>(asyncOperation);
        return thisPtr->Error;
    }

protected:
    virtual void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.state_.TransitionToClosing();
        if (error.IsSuccess())
        {
            owner_.OnBeginClose(
                timeout_,
                [](AsyncOperationSPtr const & onCloseOperation){ AsyncFabricComponent::CloseAsyncOperation::OnOnCloseCompleted(onCloseOperation); },
                thisSPtr);
        }
        else
        {
            TryComplete(thisSPtr, error);
        }
    }

    virtual void OnCompleted()
    {
        AsyncOperation::OnCompleted();
    }

private:
    static void OnOnCloseCompleted(AsyncOperationSPtr const & onCloseOperation)
    {
        auto thisPtr = AsyncOperation::Get<CloseAsyncOperation>(onCloseOperation->Parent);

        auto errorCode = thisPtr->owner_.OnEndClose(onCloseOperation);       
        if (errorCode.IsSuccess())
        {
            errorCode = thisPtr->owner_.state_.TransitionToClosed();
            if(!errorCode.IsSuccess())
            {
                // Since OnEndClose succeeded just need to set state to aborted
                // No need to call abort anymore since Close was successful
                thisPtr->owner_.state_.SetStateToAborted();
            }
        }
        else
        {
            thisPtr->owner_.state_.TransitionToFailed();
            thisPtr->owner_.Abort();
        }

        thisPtr->TryComplete(onCloseOperation->Parent, errorCode);
    }

    AsyncFabricComponent & owner_;
    TimeSpan timeout_;
};

AsyncFabricComponent::AsyncFabricComponent(void) : state_()
{
}

AsyncFabricComponent::~AsyncFabricComponent(void)
{
    ASSERT_IF(
        ((state_.Value & FabricComponentState::DestructEntryMask) == 0),
        "The AsyncFabricComponent must be in Created, Aborted or Closed state at the time of destruction. Current value is {0}.",
        State);
}

AsyncOperationSPtr AsyncFabricComponent::BeginOpen(TimeSpan timeout, AsyncCallback const & callback, AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<OpenAsyncOperation>(*this, timeout, callback, parent);
}

ErrorCode AsyncFabricComponent::EndOpen(AsyncOperationSPtr const & asyncOperation)
{
    return OpenAsyncOperation::End(asyncOperation);
}

AsyncOperationSPtr AsyncFabricComponent::BeginClose(TimeSpan timeout, AsyncCallback const & callback, AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CloseAsyncOperation>(*this, timeout, callback, parent);
}

ErrorCode AsyncFabricComponent::EndClose(AsyncOperationSPtr const & asyncOperation)
{
    return CloseAsyncOperation::End(asyncOperation);
}

void AsyncFabricComponent::Abort()
{
    bool shouldCallOnAbort;
    state_.TransitionToAborted(shouldCallOnAbort);

    //If result is false it means component has already been aborted
    if (shouldCallOnAbort)
    {
        OnAbort();
    }
}

void AsyncFabricComponent::ThrowIfCreatedOrOpening()
{
    ASSERT_IFNOT(
        (State.Value & (FabricComponentState::Created | FabricComponentState::Opening)) == 0,
        "AsyncFabricComponent has never been opened. Current state {0}",
        State);
}

RwLock & AsyncFabricComponent::get_StateLock()
{
    return state_.stateLock_;
}
