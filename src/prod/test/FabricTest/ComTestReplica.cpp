// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace TestCommon;
using namespace FabricTest;
using namespace TestHooks;

namespace FabricTest
{
    namespace ReplicaState
    {
        void WriteToTextWriter(TextWriter & w, Enum const & val)
        {
            switch (val)
            {
            case Building:
                w << L"Building";
                return;
            case BuiltError:
                w << L"BuiltError";
                return;
            case Built:
                w << L"Built";
                return;
            case CCOnly:
                w << L"CCOnly";
                return;
            case PCOnly:
                w << L"PCOnly";
                return;
            case PCandCC:
                w << L"PCandCC";
                return;
            case NA:
                w << L"NA";
                return;
            default:
                Common::Assert::CodingError("Unknown Replica State");
            }
        }
    }
}

void ComTestReplica::BuildReplica()
{
    ASSERT_IF(state_ != ReplicaState::NA && state_!= ReplicaState::CCOnly && state_ != ReplicaState::PCandCC, "Replica already in {0} state", state_);
    state_ = ReplicaState::Building;
}

void ComTestReplica::BuildReplicaOnComplete(HRESULT hr)
{
    ASSERT_IF(state_ != ReplicaState::Building, "Replica is in {0} state in BuildReplicaOnComplete", state_);
    state_ = FAILED(hr) ? ReplicaState::BuiltError : ReplicaState::Built;
}

void ComTestReplica::RemoveReplica()
{
    ASSERT_IF((state_ != ReplicaState::Built && state_ != ReplicaState::BuiltError && state_ != ReplicaState::NA), "RemoveReplica can't be called for replica in {0} state", state_);
    state_ = ReplicaState::NA;
}

void ComTestReplica::UpdateCatchup(bool isInCC, bool isInPC, FABRIC_REPLICA_ID replicaId, FABRIC_SEQUENCE_NUMBER replicaLSN)
{
    ReplicaState::Enum newReplicaState = ReplicaState::NA;
    if (isInCC && isInPC)
    {
        newReplicaState = ReplicaState::PCandCC;
    }
    else if (isInCC)
    {
        newReplicaState = ReplicaState::CCOnly;
    }
    else if (isInPC)
    {
        newReplicaState = ReplicaState::PCOnly;
    }
    
    // Transition from Built to PCOnly is possible in following scenario:
    // 1.	Replica was originally secondary and IC U state from primary replica�s view;
    // 2.	FM starts a reconfiguration and replica is transition from S->I
    // 3.	Then replica becomes S/I IC U, then S/I IB U and finally S/I RD U state;
    // 4.	The reconfiguration was in Phase2_Catchup and api IReplicator.UpdateCatchupConfiguraiton was called on primary replica;
    // 5.	When UpdateCatchup was called, that replica is transition from Built to PCOnly state.
    if (state_ == ReplicaState::Built)
    {
        ASSERT_IF((newReplicaState != ReplicaState::CCOnly && newReplicaState != ReplicaState::PCandCC && newReplicaState != ReplicaState::PCOnly), "Replica {0} can't transit from Built state to {1} state through UpdateCatchup", replicaId, newReplicaState);
        state_ = newReplicaState;
    }
    else if (state_ == ReplicaState::PCandCC)
    {
        ASSERT_IF((newReplicaState != ReplicaState::PCandCC && newReplicaState != ReplicaState::NA), "Replica {0} can't transit from PCandCC state to {1} state through UpdateCatchup", replicaId, newReplicaState);
        state_ = newReplicaState;
    }
    else if (state_ == ReplicaState::CCOnly)
    {
        ASSERT_IF(newReplicaState != ReplicaState::NA && newReplicaState != ReplicaState::PCandCC && newReplicaState != ReplicaState::PCOnly && newReplicaState != ReplicaState::CCOnly, "Replica {0} can't transit from CCOnly state to {1} state through UpdateCatchup", replicaId, newReplicaState);
        state_ = newReplicaState;
    }
    else if (state_ == ReplicaState::PCOnly)
    {
        ASSERT_IF(newReplicaState != ReplicaState::NA && newReplicaState != ReplicaState::PCOnly, "Replica {0} can't transit from PCOnly state to {1} state through UpdateCatchup", replicaId, newReplicaState);
        state_ = newReplicaState;
    }
    else if (state_ == ReplicaState::NA)
    {
        ASSERT_IF(newReplicaState != ReplicaState::NA && replicaLSN == -1, "Replica {0} LSN is -1 in transition from NA to {1}", replicaId, newReplicaState)
        state_ = newReplicaState;
    }
    else
    {
        ASSERT_IF(state_ == ReplicaState::Building || state_ == ReplicaState::BuiltError, "Replica {0} can't be in {1} state before UpdateCatchup call", replicaId, state_);
    }
}

void ComTestReplica::UpdateCurrent(bool isInCC, FABRIC_REPLICA_ID replicaId)
{
    ASSERT_IF((state_ != ReplicaState::CCOnly && state_ != ReplicaState::PCandCC && state_ != ReplicaState::PCOnly && state_ != ReplicaState::Built), "Replica {0} can't be in {1} state before UpdateCurrent call", replicaId, state_);
    
    auto newReplicaState = isInCC ? ReplicaState::CCOnly : ReplicaState::NA;

    if (state_ == ReplicaState::CCOnly)
    {
        ASSERT_IF((newReplicaState != ReplicaState::CCOnly && newReplicaState != ReplicaState::NA), "Replica {0} can't transit from CCOnly state to {1} state through UpdateCurrent", replicaId, newReplicaState);
        state_ = newReplicaState;
    }
    else if (state_ == ReplicaState::PCandCC)
    {
        ASSERT_IF((newReplicaState != ReplicaState::CCOnly && newReplicaState != ReplicaState::NA), "Replica {0} can't transit from PCandCC state to {1} state through UpdateCurrent", replicaId, newReplicaState);
        state_ = newReplicaState;
    }
    else if (state_ == ReplicaState::PCOnly)
    {
        ASSERT_IF(newReplicaState != ReplicaState::NA, "Replica {0} can't transit from PCOnly state to {1} state through UpdateCurrent", replicaId, newReplicaState);
        state_ = newReplicaState;
    }
    else if (state_ == ReplicaState::Built)
    {
        ASSERT_IF(newReplicaState != ReplicaState::CCOnly, "Replica {0} can't transit from Built state to {1} state through UpdateCurrent", replicaId, newReplicaState);
        state_ = newReplicaState;
    }
}
