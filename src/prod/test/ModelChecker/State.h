// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"
#include "Reliability/Failover/Failover.h"
#include "Reliability/Failover/FMStateMachineTask.h"
#include "Reliability/Failover/FMStateUpdateTask.h"
#include "Reliability/Failover/FMStatelessCheckTask.h"
#include "Reliability/Failover/FMPendingTask.h"
#include "Reliability/Failover/FMMovementTask.h"
#include "Reliability/LoadBalancing/Movement.h"
#include "Reliability/Failover/FMFailoverUnitMessageProcessor.h"
#include "Reliability/Failover/ReplicaReplyMessageBody.h"
#include "Reliability/Failover/NodeUpMessageBody.h"
#include "ServiceModel/reliability/failover/ServiceDescription.h"

#include "FMNodeRecord.h"
#include "Event.h"
#include "Root.h"
#include "FauxCache.h"

namespace ModelChecker
{
    class State 
    {
    public:
        friend class StateHash;
        friend class StateEqual;

        State(RootSPtr referenceToRoot,
            std::vector<Reliability::FailoverManagerComponent::StateMachineTaskUPtr> const& statelessTasks,
            Reliability::FailoverManagerComponent::FailoverUnitUPtr && fmft,
            FauxCache&& fauxCache,
            std::vector<Event>&& enabledEvents);

        State(const State& other);
        
        State(State&& other);

        State& operator =(const State& other);
        
        State& operator =(State&& other);

        __declspec(property(get=get_EventList,put=set_EventList)) std::vector<Event> EventList;
        std::vector<Event> get_EventList() const { return enabledEvents_; } 
        void set_EventList(std::vector<Event> const& enabledEvents) { enabledEvents_ = enabledEvents; }

        __declspec(property(get=get_FailoverUnitUniquePointer)) Reliability::FailoverManagerComponent::FailoverUnitUPtr const& FailoverUnitUniquePointer;
        Reliability::FailoverManagerComponent::FailoverUnitUPtr const& get_FailoverUnitUniquePointer() const { return fmft_; }
        
        void MakeTransition(Event const& te);

    private:
        int GetLocationOfOneNode();

        std::vector<int> GetNodeLocations(int);

        std::wstring State::ConvertEpoch(Reliability::Epoch const & epoch);

        std::wstring CreateMessageBody(int targetReplicaSize, 
            int minReplicaSize, 
            int nodeLocation, 
            Event::Type type, 
            Reliability::Epoch previousConfigurationEpoch, 
            Reliability::Epoch currentConfigurationEpoch, 
            Reliability::FailoverManagerComponent::FailoverUnit::Flags failoverUnitFlags);

        int GetNodeLocationFromMessageBody(std::wstring const& msgBodyStr);

        bool NodeIsUp(const Event&);

        std::vector<int> GetIndicesOfDownNodes();

        std::vector<int> GetIndicesOfUpNodes();

        Reliability::Epoch EpochFromString(std::wstring const& value);

        Reliability::ReplicaInfo State::ReplicaInfoFromString(std::wstring const & replicaInfoStr, Reliability::FailoverManagerComponent::Replica::Flags::Enum & flags); 

        Reliability::ReplicaRole::Enum State::ReplicaRoleFromString(std::wstring const& value);

        Reliability::ReplicaStates::Enum State::ReplicaStateFromString(std::wstring const& value);

        Reliability::FailoverManagerComponent::Replica::Flags::Enum State::ReplicaFlagsFromString(std::wstring const& value);

        Federation::NodeId CreateNodeId(int id);

        Federation::NodeInstance CreateNodeInstance(int id, uint64 instance);

        std::pair<std::wstring, int> State::ParseActionString(std::wstring const& actionString);

        void State::GiveFailoverUnitBackToState(Reliability::FailoverManagerComponent::LockedFailoverUnitPtr const& lockedFailoverUnitPtr);

        std::vector<std::wstring> ActionsToString(std::vector<Reliability::FailoverManagerComponent::FMStateMachineActionUPtr> const& actions);

        Reliability::FailoverManagerComponent::FailoverUnit::Flags FailoverUnitFlagsFromString(std::wstring const& value);

        Reliability::FailoverManagerComponent::FailoverUnitUPtr FailoverUnitFromString(std::wstring const& failoverUnitStr);
        
        void TimerEventHandler(Event const&);

        void LocationReplyEventHandler(Event const&);

        void AddInstanceReplyEventHandler(Event const&);

        void RemoveInstanceReplyEventHandler(Event const&);

        void NodeUpEventHandler(Event const&);

        void NodeDownEventHandler(Event const&);

        // Contains Transport, HealthClient, FederationSubsystem, and FM 
        RootSPtr referenceToRoot_; 

        // Like statelessTasks in FMStateMachineTask.Test 
        std::vector<Reliability::FailoverManagerComponent::StateMachineTaskUPtr> const& tasks_; 

        Reliability::FailoverManagerComponent::FailoverUnitUPtr fmft_;

        // Fake cache: has (NodeInfo, HasAnInstance) for every node 
        FauxCache fauxCache_;

        // Allowed transition-producing events
        std::vector<Event> enabledEvents_;
    };

    // Computes a state's hash
    class StateHash
    {
    public:
        std::size_t StateHash::operator() (State const& state) const
        {
            std::hash<uint64> uint64Hash;
            std::hash<bool> boolHash;
            std::hash<int> intHash;
            std::hash<std::wstring> wstringHash;

            std::size_t seed = 0;
            std::size_t componentHashVal;

            // Combines component hashes 
            auto CombineWithSeed = [&seed](std::size_t hashValToCombine)
            {
                // Source: http://www.boost.org/doc/libs/1_35_0/doc/html/boost/hash_combine_id241013.html 
                // Motivation: XOR preserves randomness: the combined hash should be strong, as long as 
                // at least one of the component hashes is strong.
                
                seed ^= hashValToCombine + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            };

            // Hashes fake cache
            for each (auto const& nodeRecord in state.fauxCache_.nodeList_ )
            {
                componentHashVal = uint64Hash(nodeRecord.NodeInformation.Id.IdValue.Low);
                CombineWithSeed(componentHashVal);
                
                componentHashVal = boolHash( nodeRecord.NodeInformation.IsUp );
                CombineWithSeed(componentHashVal);
                
                componentHashVal = boolHash( nodeRecord.HasAnInstance );
                CombineWithSeed(componentHashVal);
            }
            
            // Hashes FailoverUnit 
            componentHashVal = intHash( state.fmft_->FailoverUnitFlags.Value );
            CombineWithSeed(componentHashVal);
            for (auto replicaIter = state.fmft_->BeginIterator; replicaIter != state.fmft_->EndIterator; ++replicaIter)
            {
                Reliability::FailoverManagerComponent::Replica const& replica = *replicaIter; 
                
                int replicaFlags = static_cast<int>( replica.ReplicaFlags );
                componentHashVal = intHash( replicaFlags );
                CombineWithSeed(componentHashVal);

                int replicaState = static_cast<int>( replica.ReplicaState );
                componentHashVal = intHash( replicaState );
                CombineWithSeed(componentHashVal);
                
                componentHashVal = boolHash( replica.IsUp );
                CombineWithSeed(componentHashVal);
            }

            // Hashes enabled events 
            for each (auto const& enabledEvent in state.enabledEvents_)
            {
                int eventType = static_cast<int>( enabledEvent.EventType );
                componentHashVal = intHash(eventType);
                CombineWithSeed(componentHashVal);

                componentHashVal = wstringHash(enabledEvent.MessageBodyStr);
                CombineWithSeed(componentHashVal);
            }

            return seed;
        }
    };

    // Checks if two states are the same - based on the state-members upon which the hash is based.
    // If any member does not match, the states are deemed different. 
    class StateEqual
    {
    public:
        bool operator() (State const& state1, State const& state2) const
        {
            bool equalSoFar = true;
            equalSoFar &= (state1.fauxCache_.nodeList_.size() == state2.fauxCache_.nodeList_.size());
            if (equalSoFar)
            {
                for (auto nodeRecordIter1 = state1.fauxCache_.nodeList_.begin(), nodeRecordIter2 = state2.fauxCache_.nodeList_.begin();
                    nodeRecordIter1 != state1.fauxCache_.nodeList_.end() && equalSoFar;
                    ++nodeRecordIter1, ++nodeRecordIter2)
                {
                    equalSoFar &= ( nodeRecordIter1->NodeInformation.Id.IdValue.Low == nodeRecordIter2->NodeInformation.Id.IdValue.Low );
                    equalSoFar &= ( nodeRecordIter1->NodeInformation.IsUp == nodeRecordIter2->NodeInformation.IsUp );
                    equalSoFar &= ( nodeRecordIter1->HasAnInstance == nodeRecordIter2->HasAnInstance );
                }
                if (equalSoFar)
                {
                    equalSoFar &= ( state1.fmft_->FailoverUnitFlags.Value == state2.fmft_->FailoverUnitFlags.Value );
                    if ( equalSoFar )
                    {
                        equalSoFar &= ( state1.fmft_->ReplicaCount == state2.fmft_->ReplicaCount );
                        if ( equalSoFar )
                        {
                            for (auto replicaIter1 = state1.fmft_->BeginIterator, replicaIter2 = state2.fmft_->BeginIterator;
                                replicaIter1 != state1.fmft_->EndIterator && equalSoFar;
                                ++replicaIter1, ++replicaIter2)
                            {
                                equalSoFar &= ( replicaIter1->ReplicaFlags == replicaIter2->ReplicaFlags );
                                equalSoFar &= ( replicaIter1->ReplicaState == replicaIter2->ReplicaState );
                                equalSoFar &= ( replicaIter1->IsUp == replicaIter2->IsUp );
                            }

                            if (equalSoFar) 
                            {
                                for (auto eventIter1 = state1.enabledEvents_.begin(), eventIter2 = state2.enabledEvents_.begin();
                                eventIter1 != state1.enabledEvents_.end() && equalSoFar;
                                ++eventIter1, ++eventIter2)
                                {
                                    equalSoFar &= ( eventIter1->EventType == eventIter2->EventType );
                                    equalSoFar &= ( eventIter1->MessageBodyStr == eventIter2->MessageBodyStr  );
                                }
                            }
                        }
                    }
                }
            }
            return equalSoFar;
        }
    };
}
