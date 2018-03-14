// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    namespace ReplicaState
    {
        enum Enum
        {
            Building = 0,
            BuiltError = 1,
            Built = 2,
            CCOnly = 3,
            PCOnly = 4,
            PCandCC = 5,
            NA = 6
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & val);
    }
    
    class ComTestReplica
    {
    public:
        
        ComTestReplica(ReplicaState::Enum state = ReplicaState::Enum::NA) : state_(state) {};

        __declspec (property(get = getReplicaState)) ReplicaState::Enum State;
        ReplicaState::Enum getReplicaState() const { return state_; } 

        void BuildReplica();
        void BuildReplicaOnComplete(HRESULT hr);
        void RemoveReplica();
        void UpdateCatchup(bool isInCC, bool isInPC, FABRIC_REPLICA_ID replicaId, FABRIC_SEQUENCE_NUMBER replicaLSN = 1);
        void UpdateCurrent(bool isInCC, FABRIC_REPLICA_ID replicaId);

    private:
        ReplicaState::Enum state_;
    };
};
