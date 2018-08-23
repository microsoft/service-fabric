// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

FabricComponent::FabricComponent(void) : state_()
{
}

FabricComponent::~FabricComponent(void)
{
    ASSERT_IF(
        ((this->State.Value & FabricComponentState::DestructEntryMask) == 0),
        "The FabricComponent must be in Created, Aborted or Closed state at the time of destruction. Current value is {0}.",
        this->State);
}

ErrorCode FabricComponent::Open()
{
    auto error = state_.TransitionToOpening();
    if (error.IsSuccess())
    {
        error = OnOpen();
        if (error.IsSuccess())
        {
            error = state_.TransitionToOpened();
        }

        if (!error.IsSuccess())
        {
            state_.TransitionToFailed();
            Abort();
        }
    }

    return error;
}

ErrorCode FabricComponent::Close()
{
    auto error = state_.TransitionToClosing();
    if (error.IsSuccess())
    {
        error = OnClose();
        if (error.IsSuccess())
        {
            error = state_.TransitionToClosed();
            if(!error.IsSuccess())
            {
                // Since OnClose succeeded just need to set state to aborted
                // No need to call abort anymore since Close was successful
                state_.SetStateToAborted();
            }
        }
        else
        {
            state_.TransitionToFailed();
            Abort();
        }
    }

    return error;
}

void FabricComponent::Abort()
{
    this->TryAbort();
}

bool FabricComponent::TryAbort()
{
    bool shouldCallOnAbort;
    state_.TransitionToAborted(shouldCallOnAbort);

    //If result is false it means component has already been aborted
    if (shouldCallOnAbort)
    {
        OnAbort();
    }

    return shouldCallOnAbort;
}

void FabricComponent::ThrowIfCreatedOrOpening()
{
    ASSERT_IFNOT(
        (this->State.Value & (FabricComponentState::Created | FabricComponentState::Opening)) == 0,
        "FabricComponent has never been opened. Current state {0}",
        this->State);
}

bool FabricComponent::IsOpeningOrOpened()
{
    return ((this->State.Value & (FabricComponentState::Opening | FabricComponentState::Opened)) != 0);
}

bool FabricComponent::IsOpened()
{
    return ((this->State.Value & FabricComponentState::Opened) != 0);
}

RwLock & FabricComponent::get_StateLock()
{
    return state_.stateLock_;
}
