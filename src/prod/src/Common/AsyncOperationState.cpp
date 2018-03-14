// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Common
{
    // Effects: If (*this & mask == expected), assigns (desired & mask) to (*this & mask),
    //          leaving other bits in *this unaffected.  Otherwise assigns *this to desired.
    //          Note that in the latter case, all bits of *this are returned through desired,
    //          regardless of mask.
    // 
    // Returns: true if (*this & mask == expected), false otherwise.

    bool compare_mask_swap(atomic_long & atomicValue, LONG & desired, LONG expected, LONG mask)
    {
        LONG check = atomicValue.load();

        for (;;)
        {
            if ((check & mask) != expected)
            {
                desired = check;
                return false;
            }

            LONG update = desired | (check & ~mask);

            if (atomicValue.compare_exchange_weak(check, update))
            {
                return true;
            }
        }
    }

    AsyncOperationState::AsyncOperationState()
        : operationState_(Created)
    {
    }

    AsyncOperationState::~AsyncOperationState()
    {
        LONG currentState = operationState_.load() & LifecycleMask;

        ASSERT_IF(
            ((currentState != Created) && (currentState != Completed) && (currentState != Ended)),
            "An AsyncOperation can only be destructed in Created, Completed, or Ended state. The current state is {0}.",
            operationState_.load());
    }

    void AsyncOperationState::TransitionStarted()
    {
        LONG desired = Started;
        ASSERT_IF(
            !compare_mask_swap(operationState_, desired, Created, LifecycleMask),
            "AsyncOperationState::TransitionStarted");
    }

    bool AsyncOperationState::TryTransitionCompleting()
    {
        LONG desired = Completing;
        return compare_mask_swap(operationState_, desired, Started, LifecycleMask);
    }

    void AsyncOperationState::TransitionCompleted()
    {
        LONG desired = Completed;
        ASSERT_IF(
            !compare_mask_swap(operationState_, desired, Completing, LifecycleMask),
            "AsyncOperationState::TransitionCompleted");
    }

    bool AsyncOperationState::TryTransitionEnded()
    {
        LONG desired = Ended;
        return compare_mask_swap(operationState_, desired, Completed, LifecycleMask);
    }

    void AsyncOperationState::TransitionEnded()
    {
        ASSERT_IF(
            !TryTransitionEnded(),
            "AsyncOperationState::TransitionEnded");
    }

    LONG AsyncOperationState::ObserveState() const
    {
        LONG desired = Started | ObservedBeforeComplete;
        if (!compare_mask_swap(operationState_, desired, Started, LifecycleMask))
        {
            ASSERT_IF((desired & Created) != 0, "Cannot observe state before calling Start.");
            if ((desired & Completing) != 0)
            {
                desired |= ObservedBeforeComplete;
                compare_mask_swap(operationState_, desired, Completing, LifecycleMask);
            }
        }
        return desired;
    }

    bool AsyncOperationState::TryMarkCancelRequested()
    {
        LONG desired = CancelRequested;
        return compare_mask_swap(operationState_, desired, 0, CancelRequested);
    }
}
