// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

FabricComponentState::FabricComponentState()
    : state_(Created), abortCalled_(false), closeCalled_(false)
{
}

FabricComponentState::~FabricComponentState()
{
}

ErrorCode FabricComponentState::TransitionToOpening()
{
    AcquireWriteLock grab(stateLock_);
    LONG desired = FabricComponentState::Opening;
    if(state_ != FabricComponentState::Created)
    {
        if(abortCalled_ || closeCalled_)
        {
            return ErrorCode(ErrorCodeValue::FabricComponentAborted);
        }
        else
        {
            return ErrorCode(ErrorCodeValue::InvalidState);
        }
    }

    state_ = desired;
    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode FabricComponentState::TransitionToOpened()
{
    AcquireWriteLock grab(stateLock_);
    LONG desired = FabricComponentState::Opened;

    ASSERT_IFNOT(
        state_ == FabricComponentState::Opening, 
        "Invalid state transition. Current {0}, Desired {1}",
        ToString(state_),
        desired);

    if(abortCalled_ || closeCalled_)
    {
        return ErrorCode(ErrorCodeValue::FabricComponentAborted);
    }
    else
    {
        state_ = desired;
        return ErrorCode(ErrorCodeValue::Success);
    }

}

ErrorCode FabricComponentState::TransitionToClosing()
{
    AcquireWriteLock grab(stateLock_);
    LONG desired = FabricComponentState::Closing;
    ErrorCode error;
    if(state_ == FabricComponentState::Opened)
    {
        state_ = desired;
        error = ErrorCode(ErrorCodeValue::Success);
    }
    else
    {
        if(abortCalled_ || closeCalled_)
        {
            error = ErrorCode(ErrorCodeValue::FabricComponentAborted);
        }
        else
        {
            error = ErrorCode(ErrorCodeValue::InvalidState);
        }
    }

    closeCalled_ = true;
    return error;
}

ErrorCode FabricComponentState::TransitionToClosed()
{
    AcquireWriteLock grab(stateLock_);
    LONG desired = FabricComponentState::Closed;

    ASSERT_IFNOT(
        state_ == FabricComponentState::Closing, 
        "Invalid state transition. Current {0}, Desired {1}",
        ToString(state_),
        desired);

    ASSERT_IFNOT(
        closeCalled_, 
        "Close should have been called when at TransitionToClosed ");

    if(abortCalled_)
    {
        // Abort was called while the object was closing so return error
        return ErrorCode(ErrorCodeValue::FabricComponentAborted);
    }
    else
    {
        state_ = desired;
        return ErrorCode(ErrorCodeValue::Success);
    }
}

void FabricComponentState::TransitionToFailed()
{
    AcquireWriteLock grab(stateLock_);
    LONG desired = FabricComponentState::Failed;
    ASSERT_IF(
        (state_ & FabricComponentState::FailedEntryMask) == 0, 
        "Invalid state transition. Current {0}, Desired {1}",
        ToString(state_),
        desired);
    state_ = desired;
}

void FabricComponentState::TransitionToAborted(bool & shouldCallOnAbort)
{
    AcquireWriteLock grab(stateLock_);
    LONG desired = FabricComponentState::Aborted;
    abortCalled_ = true;

    switch (static_cast<int>(state_))
    {
    case Opened:
    case Failed:
        //Current state is Opened | Failed hence we change the current state to Aborted
        //and return true so that Abort is called
        state_ = desired;
        shouldCallOnAbort = true;
        break;
    case Created:
    case Closed:
    case Aborted:
        //Current state is Created | Closed | Aborted hence we change the current state to Aborted
        //and return false so that Abort is not called
        state_ = desired;
        shouldCallOnAbort = false;
        break;
    case Opening:
    case Closing:
        //Current state is Opening | Closing and so we do not change the state yet
        //Once Open/Close complete FabricComponent will check whether Abort has been called and 
        //then transition to Aborted. We return false so that Abort is not called
        shouldCallOnAbort = false;
        break;
    default:
        Assert::CodingError("Invalid state: {0}", state_);
    }
}

void FabricComponentState::SetStateToAborted()
{
    AcquireWriteLock grab(stateLock_);
    // Force the state to abort irrespective of current state
    ASSERT_IFNOT(
        abortCalled_, 
        "Abort should have already been called when SetStateToAborted is invoked");
    state_ = FabricComponentState::Aborted;
}

std::wstring FabricComponentState::ToString(LONG state)
{
    switch (static_cast<int>(state))
    {
    case Created:
        return L"Created";
    case Opening:
        return L"Opening";
    case Opened:
        return L"Opened";
    case Closing:
        return L"Closing";
    case Closed:
        return L"Closed";
    case Failed:
        return L"Failed";
    case Aborted:
        return L"Aborted";
    default:
        std::wstring invalidStateString;
        StringWriter writer(invalidStateString);
        writer.Write("Invalid state: {0}", state);
        return move(invalidStateString);
    }
}

void FabricComponentState::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
{
    AcquireReadLock grab(stateLock_);
    w << ToString(state_);
}
