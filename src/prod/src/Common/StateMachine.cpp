// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

StringLiteral TraceType_StateMachine = "StateMachine";

class StateMachine::TransitionWaitAsyncOperation 
    : public TimedAsyncOperation,
    private TextTraceComponent<TraceTaskCodes::Common>
{
    DENY_COPY(TransitionWaitAsyncOperation)

public:
    TransitionWaitAsyncOperation(
        __in StateMachine const & owner,
        uint64 const targetStateMask,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : TimedAsyncOperation(timeout, callback, parent),
        owner_(owner),
        targetStateMask_(targetStateMask),
        waiterId_(Guid::NewGuid().ToString())
    {
    }

    virtual ~TransitionWaitAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<TransitionWaitAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        bool matching = false;
        bool terminal = false;

        {
            AcquireWriteLock lock(owner_.Lock);
            matching = IsMatching(owner_.current_);
            terminal = ((owner_.GetTerminalStatesMask() & owner_.current_) != 0);

            if (!matching && !terminal)
            {
                owner_.transitionWaiters_.insert(make_pair(waiterId_, thisSPtr));
            }
        }

        if (matching)
        {
            // current state is matching the target mask
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
            return;
        }
        else
        {
            if (terminal)
            {
                // current does not match the target mask and the object is aborted
                TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::ObjectClosed));
                return;
            }
            // else 
            // current does not match the target mask and the object is not aborted, the state machine
            // transition methods will complete the waiter if the new state matches the target mask
        }
    }

    void OnCompleted()
    {
        TimedAsyncOperation::OnCompleted();

        {
            AcquireWriteLock lock(owner_.lock_);

            auto iter = owner_.transitionWaiters_.find(waiterId_);
            if (iter != owner_.transitionWaiters_.end())
            {
                owner_.transitionWaiters_.erase(iter);
            }
        }
    }
    
private:
    bool IsMatching(uint64 currentState) const 
    {
        return ((currentState & targetStateMask_) != 0);
    }

private:
    StateMachine const & owner_;
    uint64 const targetStateMask_;
    wstring waiterId_;
    friend class StateMachine;
};

StateMachine::StateMachine(uint64 initialState)
    : current_(initialState), 
    abortCalled_(false), 
    lock_(),
    transitionWaiters_(),
    currentRefCount_(0)
{
    traceId_ = wformatString("{0}", static_cast<void*>(this));
    ASSERT_IF(current_ == Aborted, "Initial state cannot be Aborted.");
}

StateMachine::~StateMachine()
{
}

uint64 StateMachine::GetState() const
{
    {
        AcquireReadLock lock(lock_);
        return current_;
    }
}

void StateMachine::GetStateAndRefCount(__out uint64 & state, __out uint64 & refCount) const
{
    {
        AcquireReadLock lock(lock_);

        state = this->GetState_CallerHoldsLock();
        refCount = this->GetRefCount_CallerHoldsLock();
    }
}

uint64 StateMachine::GetState_CallerHoldsLock() const
{
    return current_;
}

ErrorCode StateMachine::TransitionTo(uint64 target, uint64 entryMask)
{
    uint64 before;
    return TransitionTo(target, entryMask, before);
}

ErrorCode StateMachine::TransitionTo(uint64 target, uint64 entryMask, __out uint64 & before)
{
    ASSERT_IF(target == Aborted, "Call Abort method instead.");

    ErrorCode error;

    uint64 after = 0;
    uint64 afterRefCount = 0;

    bool doAbort = false;
    vector<AsyncOperationSPtr> waitersToComplete;

    {
        AcquireWriteLock lock(lock_);
        before = current_;

        if ((current_ & entryMask) != 0)
        {
            // transition is allowed
            
            if (abortCalled_)
            {
                // Abort was called while in the previous state. 
                
                if (current_ != Aborted)
                {
                    // StateMachine is not in aborted state.
                    // 
                    // If current state is a simple state, StateMachine must transition to 'Aborted' state.
                    // 
                    // If current state is ref counted state and this is an outgoing transition i.e. target != current_
                    //   1) Decrease the ref count.
                    //   2) If ref count = zero, StateMachine must transition to 'Aborted' state.
                    //   3) If ref count > zero, fail the transition with ObjectClosed error.
                    // 
                    // If current state is ref counted state and this is a loop transition i.e. target = current_,  
                    // fail the transition with ObjectClosed error.
                    // 
                    if (this->IsRefCountedState(current_))
                    {
                        if (target != current_) 
                        {
                            if (--currentRefCount_ == 0)
                            {
                                doAbort = true;
                                current_ = Aborted;
                            }
                        }
                    }
                    else
                    {
                        doAbort = true;
                        current_ = Aborted;
                    }
                    
                }
                error = ErrorCode(ErrorCodeValue::ObjectClosed);
            }
            else
            {
                // 
                // If current state is a simple state:
                //   1) If target state is a ref counted state, increment ref count.
                //   2) Complete transition successfully.
                // 
                // If current state is ref counted state and this is an outgoing transition i.e. target != current_
                //   1) Decrease ref count
                //   2) If ref count = zero
                //        a) If target state is a ref counted state, increment ref count. 
                //        b) Complete transition successfully.
                //   3) If ref count > zero
                //        a) Current state remains same.
                //        b) Complete transition with error code OperationsPending.
                // 
                // If current state is ref counted state and this is a loop transition i.e. target = current_,  
                // increase the ref count and complete transition successfully.
                // 
                if (this->IsRefCountedState(current_))
                {
                    if (target != current_)
                    {
                        if (--currentRefCount_ == 0)
                        {
                            if (this->IsRefCountedState(target))
                            {
                                currentRefCount_++;
                            }

                            current_ = target;
                            error = ErrorCode(ErrorCodeValue::Success);
                        }
                        else
                        {
                            error = ErrorCode(ErrorCodeValue::OperationsPending);
                        }
                    }
                    else
                    {
                        currentRefCount_++;
                        error = ErrorCode(ErrorCodeValue::Success);
                    }
                }
                else
                {
                    if (this->IsRefCountedState(target))
                    {
                        currentRefCount_++;
                    }

                    current_ = target;
                    error = ErrorCode(ErrorCodeValue::Success);
                }                
            }
        }
        else
        {
            // transition is not allowed
            error = ErrorCode(ErrorCodeValue::InvalidState);
        }

        after = current_;
        afterRefCount = currentRefCount_;

        if ((after != before) && (transitionWaiters_.size() != 0))
        {
            for(auto iter = transitionWaiters_.begin(); iter != transitionWaiters_.end(); ++iter)
            {
                auto casted = AsyncOperation::Get<TransitionWaitAsyncOperation>(iter->second);
                if (casted->IsMatching(after))
                {
                    waitersToComplete.push_back(iter->second);
                }
            }
        }
    }

    WriteTrace(
        (error.IsSuccess() || error.IsError(ErrorCodeValue::OperationsPending)) ? LogLevel::Noise : LogLevel::Warning,
        TraceType_StateMachine,
        traceId_,
        "Transition: Target={0}, Before:{1}, After={2}, AfterRefCount={3}, Error={4}, CallAbort={5}.",
        StateToString(target),
        StateToString(before),
        StateToString(after),
        afterRefCount,
        error,
        doAbort);

    if (doAbort)
    {
        OnAbort();
    }

    for(auto iter = waitersToComplete.begin(); iter != waitersToComplete.end(); ++iter)
    {
        (*iter)->TryComplete(*iter, ErrorCode(ErrorCodeValue::Success));
    }

    if ((after & GetTerminalStatesMask()) != 0)
    {
        CancelTransitionWaiters();
    }

    return error;
}

void StateMachine::TransitionToAborted()
{
    ErrorCode error;
    uint64 before;
    uint64 after;
    bool doAbort = false;
    uint64 target = Aborted;
    vector<AsyncOperationSPtr> waitersToComplete;

    {
        AcquireWriteLock lock(lock_);
        before = current_;

        if (abortCalled_) 
        { 
            doAbort = false;
        }
        else
        {
            abortCalled_ = true;
            
            if ((current_ & GetAbortEntryMask()) != 0)
            {
                current_ = target;
                doAbort = true;
            }
            else
            {
                doAbort = false;
            }
        }

        after = current_;

        if ((after != before) && (transitionWaiters_.size() != 0))
        {
            for(auto iter = transitionWaiters_.begin(); iter != transitionWaiters_.end(); ++iter)
            {
                auto casted = AsyncOperation::Get<TransitionWaitAsyncOperation>(iter->second);
                if (casted->IsMatching(after))
                {
                    waitersToComplete.push_back(iter->second);
                }
            }
        }
    }

    WriteInfo(
        TraceType_StateMachine,
        traceId_,
        "Transition: Target={0}, Before:{1}, After={2}, CallAbort={3}",
        StateToString(target),
        StateToString(before),
        StateToString(after),
        doAbort);

    if (doAbort)
    {
        OnAbort();
    }

    for(auto iter = waitersToComplete.begin(); iter != waitersToComplete.end(); ++iter)
    {
        (*iter)->TryComplete(*iter, ErrorCode(ErrorCodeValue::Success));
    }

    if ((after & GetTerminalStatesMask()) != 0)
    {
        CancelTransitionWaiters();
    }
}

void StateMachine::CancelTransitionWaiters()
{
    vector<AsyncOperationSPtr> waitersToCancel;
    {
        AcquireWriteLock lock(lock_);
        for(auto iter = transitionWaiters_.begin(); iter != transitionWaiters_.end(); ++iter)
        {
            waitersToCancel.push_back(iter->second);
        }

        transitionWaiters_.clear();
    }

    for(auto iter = waitersToCancel.begin(); iter != waitersToCancel.end(); ++iter)
    {
        (*iter)->TryComplete(*iter, ErrorCode(ErrorCodeValue::ObjectClosed));
    }
}

wstring StateMachine::StateToString(uint64 const state) const
{
    if (state == Aborted)
    {
        return L"Aborted";
    }
    else
    {
        wstring retval;
        StringWriter writer(retval);
        writer.Write("Unknown:{0}", state);
        writer.Flush();
        return retval;
    }
}

void StateMachine::Abort()
{
    TransitionToAborted();
}

uint64 StateMachine::GetRefCountedStatesMask()
{
    // No ref counted state by default.
    // Deriving class should override this if it has any ref counted states.
    return 0;
}

bool StateMachine::IsRefCountedState(uint64 state)
{
    return ((state & this->GetRefCountedStatesMask()) != 0);
}

uint64 StateMachine::GetRefCount_CallerHoldsLock() const
{
    return currentRefCount_;
}

AsyncOperationSPtr StateMachine::BeginWaitForTransition(
    uint64 const targetStateMask,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent) const
{
    WriteNoise(
        TraceType_StateMachine,
        traceId_,
        "Waiting for state machine to transition to {0}",
        targetStateMask);

    return AsyncOperation::CreateAndStart<TransitionWaitAsyncOperation>(
        *this,
        targetStateMask,
        timeout,
        callback,
        parent);
}

AsyncOperationSPtr StateMachine::BeginWaitForTransition(
    uint64 const targetStateMask,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent) const
{
    return BeginWaitForTransition(
        targetStateMask,
        TimeSpan::MaxValue,
        callback,
        parent);
}

ErrorCode StateMachine::EndWaitForTransition(AsyncOperationSPtr const & operation) const
{
    auto casted = AsyncOperation::Get<TransitionWaitAsyncOperation>(operation);
    auto error = TransitionWaitAsyncOperation::End(operation);
    WriteTrace(
        error.ToLogLevel(),
        TraceType_StateMachine,
        traceId_,
        "Wait for state {0} completed with {1}.",
        casted->targetStateMask_,
        error);

    return error;
}

AsyncOperationSPtr StateMachine::BeginAbortAndWaitForTermination(
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent) 
{
    this->Abort();
    return BeginWaitForTransition(GetTerminalStatesMask(), callback, parent);
}

void StateMachine::EndAbortAndWaitForTermination(
    AsyncOperationSPtr const & operation)
{
    EndWaitForTransition(operation).ReadValue();
}

void StateMachine::AbortAndWaitForTermination()
{
    AsyncOperationWaiter waiter;

    this->AbortAndWaitForTerminationAsync(
        [&waiter](AsyncOperationSPtr const &)
        {
            waiter.Set();
        },
        AsyncOperationSPtr());

    waiter.WaitOne();
}

void StateMachine::AbortAndWaitForTerminationAsync(
    CompletionCallback const & completionCallback,
    AsyncOperationSPtr const & parent)
{
    auto operation = this->BeginAbortAndWaitForTermination(
        [this,completionCallback](AsyncOperationSPtr const & operation)
        {
            if (!operation->CompletedSynchronously)
            {
                this->EndAbortAndWaitForTermination(operation);
                completionCallback(operation);
            }
        },
        parent);
    if (operation->CompletedSynchronously)
    {
        this->EndAbortAndWaitForTermination(operation);
        completionCallback(operation);
    }
}
