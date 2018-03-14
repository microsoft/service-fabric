// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class ReplicatedStore::TransactionReplicator::ThrottleStateMachine
        : public PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ReplicatedStore>
    {
        DENY_COPY(ThrottleStateMachine)
    public:
        static ThrottleStateMachineUPtr Create(ReplicatedStore & replicatedStore);
        
        // Kept public for make_unique to work
        ThrottleStateMachine(ReplicatedStore & replicatedStore)
            : PartitionedReplicaTraceComponent(replicatedStore.PartitionedReplicaId)
            , state_(Store::ReplicatedStoreTransactionReplicatorThrottleState::Uninitialized)
        {
        }

        virtual ~ThrottleStateMachine();
                   
        void TransitionToInitialized();
        void TransitionToStarted();
        void TransitionToStopped();

        __declspec(property(get=get_IsStarted)) bool IsStarted;
        bool get_IsStarted() { return state_ == Store::ReplicatedStoreTransactionReplicatorThrottleState::Started;}

        __declspec(property(get=get_IsInitialized)) bool IsInitialized;
        bool get_IsInitialized() { return state_ == Store::ReplicatedStoreTransactionReplicatorThrottleState::Initialized;}

        __declspec(property(get=get_IsStopped)) bool IsStopped;
        bool get_IsStopped() { return state_ == Store::ReplicatedStoreTransactionReplicatorThrottleState::Stopped;}

    private:
        Store::ReplicatedStoreTransactionReplicatorThrottleState::Enum state_;
    };
}
