// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

namespace Store
{
    using namespace Store::ReplicatedStoreTransactionReplicatorThrottleState;

    StringLiteral const TraceComponent("ThrottleStateMachine");

    ReplicatedStore::TransactionReplicator::ThrottleStateMachineUPtr ReplicatedStore::TransactionReplicator::ThrottleStateMachine::Create(ReplicatedStore & replicatedStore)
    {
        return make_unique<ThrottleStateMachine>(replicatedStore);
    }

    ReplicatedStore::TransactionReplicator::ThrottleStateMachine::~ThrottleStateMachine()
    {
        if (IsStarted || IsInitialized)
        {
            TRACE_ERROR_AND_TESTASSERT(TraceComponent, "{0}: Throttle state is not stopped", this->TraceId);
        }
    }

    void ReplicatedStore::TransactionReplicator::ThrottleStateMachine::TransitionToInitialized()
    {
        if (state_ == Started || state_ == Initialized)
        {
            TRACE_ERROR_AND_TESTASSERT(
                TraceComponent,
                "{0}: ThrottleStateMachine-TransitionToInitialized() - Invalid State {1} ", 
                this->TraceId,
                state_);
        }

        state_ = Initialized;
    }

    void ReplicatedStore::TransactionReplicator::ThrottleStateMachine::TransitionToStarted()
    {
        if (state_ != Initialized)
        {
            TRACE_ERROR_AND_TESTASSERT(
                TraceComponent,
                "{0}: ThrottleStateMachine-TransitionToStarted() - Invalid State {1} ", 
                this->TraceId,
                state_);
        }

        state_ = Started;
    }

    void ReplicatedStore::TransactionReplicator::ThrottleStateMachine::TransitionToStopped()
    {
        state_ = Stopped;
    }
}
