// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"
#include <string>
#include "State.h"

using namespace std;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;
using namespace Reliability::LoadBalancingComponent;
using namespace Common;
using namespace Federation;
using namespace ModelChecker;

State::State(RootSPtr referenceToRoot,
             vector<StateMachineTaskUPtr> const& statelessTasks,
            Reliability::FailoverManagerComponent::FailoverUnitUPtr && fmft,
            FauxCache && fauxCache,
            vector<ModelChecker::Event> && enabledEvents)
    : referenceToRoot_(referenceToRoot), 
    tasks_( statelessTasks ), 
    fmft_(move(fmft)),  
    fauxCache_( fauxCache ),
    enabledEvents_( enabledEvents )
{
}

State::State(State const& other)
    : referenceToRoot_(other.referenceToRoot_),
    tasks_(other.tasks_),
    fmft_(make_unique<FailoverUnit>( *other.fmft_ )),
    fauxCache_(other.fauxCache_),
    enabledEvents_(other.enabledEvents_)
{
}

State::State(State&& other)
    : referenceToRoot_(move(other.referenceToRoot_)),
    tasks_(move(other.tasks_)),
    fmft_(move(other.fmft_)),
    fauxCache_(move(other.fauxCache_)),
    enabledEvents_(move(other.enabledEvents_))
{
}

State& State::operator =(const State& other)
{
    if(this != &other)
    {
        referenceToRoot_ = other.referenceToRoot_;
        // tasks_ is constant
        fmft_ = make_unique<FailoverUnit>( *other.fmft_ );
        fauxCache_ = other.fauxCache_;
        enabledEvents_ = other.enabledEvents_;
    }
    
    return *this;
}

State& State::operator =(State&& other)
{
    if (this != &other)
    {
        referenceToRoot_ = move(other.referenceToRoot_);
        // tasks_ is constant
        fmft_ = move(other.fmft_);
        fauxCache_ = move(other.fauxCache_);
        enabledEvents_ = move(other.enabledEvents_);
    }

    return *this;
}

int State::GetLocationOfOneNode()
{
    int nodeLocation;
    auto node = find_if(fauxCache_.nodeList_.begin(), fauxCache_.nodeList_.end(),
        [](NodeRecord const& nodeRecord) 
        {
            return nodeRecord.NodeInformation.IsUp && !nodeRecord.HasAnInstance; 
        });
    if (node == fauxCache_.nodeList_.end()) {
        nodeLocation = -1;
    }else {
        nodeLocation = static_cast<int>(node->NodeInformation.NodeInstance.Id.IdValue.Low);
        //mark the node as unavailable
        fauxCache_.nodeList_.at(nodeLocation).HasAnInstance = true;
    }
    return nodeLocation;
}

vector<int> State::GetNodeLocations(int numberOfNodes)
{
    vector<int> nodeLocations;
    int location = -1;
    for (int i = 0; i < numberOfNodes; ++i) {
        location = GetLocationOfOneNode();
        if (location >= 0) {
            nodeLocations.push_back(location);
            fauxCache_.nodeList_[location].HasAnInstance = true;
        }
    }
    return nodeLocations;
}

wstring State::ConvertEpoch(Reliability::Epoch const & epoch)
{
    wstring result;
    result.push_back(static_cast<wchar_t>(L'0' + epoch.DataLossVersion));
    result.push_back(static_cast<wchar_t>(L'0' + (epoch.ConfigurationVersion >> 32)));
    result.push_back(static_cast<wchar_t>(L'0' + (epoch.ConfigurationVersion & 0xffffffff)));

    return result;
}

wstring State::CreateMessageBody(int targetReplicaSize, 
                                 int minReplicaSize, 
                                 int nodeLocation, 
                                 ModelChecker::Event::Type eventType, 
                                 Epoch previousConfigurationEpoch, 
                                 Epoch currentConfigurationEpoch, 
                                 FailoverUnit::Flags failoverUnitFlags)  
{
    // format should look like this: 3 0 - 000/111                 [1 N/N RD - Up]
    std::wstring const extra2 = (eventType == Event::Type::AddInstanceReply) ? L" N/N RD - Up]" : L" N/N DD - Down]";
    const std::wstring pcEpoch(ConvertEpoch(previousConfigurationEpoch));
    const std::wstring ccEpoch(ConvertEpoch(currentConfigurationEpoch));
    //std::wstring fuFlags;
    std::wstring result;
    StringWriter s(result);
    s << failoverUnitFlags;
    const std::wstring extra1(L" " + result + L" " + pcEpoch + L"/" + ccEpoch);
    // MSVC11 to_string lacks proper overload for int
    std::wstring msgBodyStr = std::to_wstring(static_cast<INT64>(targetReplicaSize));
    msgBodyStr.append(L" ");
    msgBodyStr.append(std::to_wstring(static_cast<INT64>(minReplicaSize)));
    msgBodyStr.append(extra1);
    msgBodyStr.append(L" [");
    msgBodyStr.append(std::to_wstring(static_cast<INT64>(nodeLocation)));
    msgBodyStr.append(extra2);
    return msgBodyStr;
}

int State::GetNodeLocationFromMessageBody(wstring const& msgBodyStr)
{
    // format should look like this: 3 0 - 000/111                 [1 N/N RD - Up]
    vector<wstring> tmpTokens;
    StringUtility::Split<wstring>(msgBodyStr, tmpTokens, L"[");
    vector<wstring> tokens;
    StringUtility::Split<wstring>(tmpTokens[1], tokens, L" ");
    
    return Int32_Parse(tokens[0]);
}


bool State::NodeIsUp(const ModelChecker::Event& event)
{
    int indexOfNode = GetNodeLocationFromMessageBody(event.MessageBodyStr);
    NodeRecord& nodeRecord = fauxCache_.nodeList_.at(indexOfNode);
    NodeInfo nodeInfo = nodeRecord.NodeInformation;
    
    return nodeInfo.IsUp;
}

vector<int> State::GetIndicesOfDownNodes()
{
    vector<int> downIndices;
    for (int i = 0; i < fauxCache_.nodeList_.size(); ++i)
    {
        if (fauxCache_.nodeList_[i].NodeInformation.IsUp == false)
        {
            downIndices.push_back(i);
        }
    }
    
    return downIndices;
}

vector<int> State::GetIndicesOfUpNodes()
{
    vector<int> upIndices;
    for (int i = 0; i < fauxCache_.nodeList_.size(); ++i)
    {
        if (fauxCache_.nodeList_[i].NodeInformation.IsUp)
        {
            upIndices.push_back( i );
        }
    }

    return upIndices;
}

NodeId State::CreateNodeId(int id)
{
    return NodeId( LargeInteger(0, id) );
}

Federation::NodeInstance State::CreateNodeInstance(int id, uint64 instance)
{
    return Federation::NodeInstance(CreateNodeId(id), instance);
}

pair<wstring, int> State::ParseActionString(wstring const& actionString)
{
    vector<wstring> actionStringComponents;
    StringUtility::Split<wstring>(actionString, actionStringComponents, L" ");
    vector<wstring> actionNameAndTarget;
    StringUtility::Split<wstring>(actionStringComponents[0], actionNameAndTarget, L"-");
    int targetNodeLocation = Int32_Parse(actionNameAndTarget[1].substr(1));
    return make_pair(actionNameAndTarget[0], targetNodeLocation);
}

void State::GiveFailoverUnitBackToState(LockedFailoverUnitPtr const& lockedFailoverUnitPtr)
{
    if (lockedFailoverUnitPtr)
    {
        FailoverUnit tmpFmft(*lockedFailoverUnitPtr.Current);
        fmft_ = move(make_unique<FailoverUnit>(tmpFmft));
    }
}

vector<wstring> State::ActionsToString(vector<FMStateMachineActionUPtr> const& actions)
{
    vector<wstring> result;
    for each (FMStateMachineActionUPtr const& action in actions)
    {
        wstring actionStr;
        StringWriter writer(actionStr);
        writer.Write("{0:s}", *action);
        result.push_back(actionStr);
    }
    sort(result.begin(), result.end());

    return result;
}

Reliability::FailoverManagerComponent::FailoverUnit::Flags State::FailoverUnitFlagsFromString(wstring const& value)
{
    // S:IsStateful, P:HasPersistedState, E:IsNoData, D:IsToBeDeleted
    bool isStateful = false;
    bool hasPersistedState = false;
    bool isNoData = false;
    bool isToBeDeleted = false;
    bool isSwappingPrimary = false;

    if (value != L"-")
    {
        set<wchar_t> charSet;
        for (size_t i = 0; i < value.size(); i++)
        {
            charSet.insert(value[i]);
        }

        auto itEnd = charSet.end();
        isStateful = (charSet.find(L'S') != itEnd);
        hasPersistedState = (charSet.find(L'P') != itEnd);
        isNoData = (charSet.find(L'E') != itEnd);
        isToBeDeleted = (charSet.find(L'D') != itEnd);
        isSwappingPrimary = (charSet.find(L'W') != itEnd);
    }

    return FailoverUnit::Flags(isStateful, hasPersistedState, isNoData, isToBeDeleted, isSwappingPrimary);
}

Epoch State::EpochFromString(wstring const& value)
{
    size_t len = value.size();
    int64 v0 = (len >= 1 ? value[len - 1] - L'0' : 0);
    int64 v1 = (len >= 2 ? value[len - 2] - L'0' : 0);
    int64 v2 = (len >= 3 ? value[len - 3] - L'0' : 0);

    return Epoch(v2, (v1 << 32) + v0);
}

Reliability::ReplicaRole::Enum State::ReplicaRoleFromString(wstring const& value)
{
    if (value == L"N")
    {
        return Reliability::ReplicaRole::None;
    }
    else if (value == L"I")
    {
        return Reliability::ReplicaRole::Idle;
    }
    else if (value == L"S")
    {
        return Reliability::ReplicaRole::Secondary;
    }
    else if (value == L"P")
    {
        return Reliability::ReplicaRole::Primary;
    }
    else
    {
        Assert::CodingError("Invalid replica status: {0}", value);
    }
}

ReplicaStates::Enum State::ReplicaStateFromString(wstring const& value)
{
    if (value == L"RD")
    {
        return ReplicaStates::Ready;
    }
    else if (value == L"SB")
    {
        return ReplicaStates::StandBy;
    }
    else if (value == L"IB")
    {
        return ReplicaStates::InBuild;
    }
    else if (value == L"DD")
    {
        return ReplicaStates::Dropped;
    }
    else
    {
        //VERIFY_FAIL(L"Invalid replica status");
        return ReplicaStates::Ready;
    }
}

Replica::Flags::Enum State::ReplicaFlagsFromString(wstring const& value)
{
    // R:ToBeDropped, P:ToBePromoted
    Replica::Flags::Enum flags = Replica::Flags::None;
    if (value != L"-")
    {
        set<wchar_t> charSet;
        for (size_t i = 0; i < value.size(); i++)
        {
            charSet.insert(value[i]);
        }

        auto itEnd = charSet.end();

        if (charSet.find(L'D') != itEnd)
        {
            flags = static_cast<Replica::Flags::Enum>(flags | Replica::Flags::ToBeDroppedByFM);
        }

        if (charSet.find(L'R') != itEnd)
        {
            flags = static_cast<Replica::Flags::Enum>(flags | Replica::Flags::ToBeDroppedByPLB);
        }

        if (charSet.find(L'P') != itEnd)
        {
            flags = static_cast<Replica::Flags::Enum>(flags | Replica::Flags::ToBePromoted);
        }

        if (charSet.find(L'N') != itEnd)
        {
            flags = static_cast<Replica::Flags::Enum>(flags | Replica::Flags::PendingRemove);
        }
    }
    return flags;
}


ReplicaInfo State::ReplicaInfoFromString(wstring const & replicaInfoStr, Replica::Flags::Enum & flags)
{
    vector<wstring> tempStrings;
    StringUtility::Split<wstring>(replicaInfoStr, tempStrings, L" ");

    int replicaNodeId;
    uint64 replicaNodeInstance = 1;
    int64 replicaInstanceId = 1;
    if (tempStrings[0].find(L':') != wstring::npos)
    {
        vector<wstring> temp;
        StringUtility::Split<wstring>(tempStrings[0], temp, L":");

        replicaNodeId = Int32_Parse(temp[0]);
        replicaNodeInstance = static_cast<uint64>(Int32_Parse(temp[1]));
        if (temp.size() > 2)
        {
            replicaInstanceId = Int64_Parse(temp[2]);
        }
    }
    else
    {
        replicaNodeId = Int32_Parse(tempStrings[0]);
    }

    vector<wstring> roles;
    StringUtility::Split<wstring>(tempStrings[1], roles, L"/");

    Reliability::ReplicaRole::Enum pcRole, icRole, ccRole;
    if (roles.size() == 3)
    {
        pcRole = ReplicaRoleFromString(roles[0]);
        icRole = ReplicaRoleFromString(roles[1]);
        ccRole = ReplicaRoleFromString(roles[2]);
    }
    else if (roles.size() == 2)
    {
        pcRole = ReplicaRoleFromString(roles[0]);
        icRole = ReplicaRoleFromString(roles[1]);
        ccRole = icRole;
    }
    else
    {
        pcRole = Reliability::ReplicaRole::None;
        icRole = Reliability::ReplicaRole::None;
        ccRole = ReplicaRoleFromString(roles[0]);
    }

    auto state = ReplicaStateFromString(tempStrings[2]);
    flags = ReplicaFlagsFromString(tempStrings[3]);

    bool isReplicaUp = (tempStrings[4] == L"Up");

    Federation::NodeInstance nodeInstance = CreateNodeInstance(replicaNodeId, replicaNodeInstance);
    int64 replicaId = replicaNodeId;

    return ReplicaInfo(
        Reliability::ReplicaDescription(
            nodeInstance,
            replicaId,
            replicaInstanceId,
            pcRole,
            ccRole,
            state,
            isReplicaUp,
            tempStrings.size() > 5 ? Int64_Parse(tempStrings[5]) : FABRIC_INVALID_SEQUENCE_NUMBER,
            tempStrings.size() > 6 ? Int64_Parse(tempStrings[6]) : FABRIC_INVALID_SEQUENCE_NUMBER,
            L"",
            L"",
            ServiceModel::ServicePackageVersionInstance()),
        icRole);
}

Reliability::FailoverManagerComponent::FailoverUnitUPtr State::FailoverUnitFromString(wstring const& failoverUnitStr)
{
    vector<wstring> tokens;
    StringUtility::Split<wstring>(failoverUnitStr, tokens, L" ");

    wstring const serviceName = L"TestService";
    int targetReplicaSetSize = Int32_Parse(tokens[0]);
    int minReplicaSetSize = Int32_Parse(tokens[1]);
    FailoverUnit::Flags flags = FailoverUnitFlagsFromString(tokens[2]);

    vector<wstring> versions;
    StringUtility::Split<wstring>(tokens[3], versions, L"/");

    Reliability::Epoch pcEpoch = EpochFromString(versions[0]);
    Reliability::Epoch ccEpoch = EpochFromString(versions[1]);

    ServiceModel::ApplicationIdentifier appId;
    ServiceModel::ApplicationIdentifier::FromString(L"TestApp_App0", appId);
    ApplicationInfoSPtr application = make_shared<ApplicationInfo>(appId, 1);
    wstring serviceTypeName = L"TestServiceType";
    ServiceModel::ServiceTypeIdentifier typeId(ServiceModel::ServicePackageIdentifier(appId, L"TestPackage"), serviceTypeName);
    wstring placementConstraints = L"";
    int scaleoutCount = 0;
    auto serviceType = make_shared<ServiceType>(typeId, application);

    auto serviceInfo = make_shared<ServiceInfo>(
        Reliability::ServiceDescription(serviceName, 0, 0, 1, targetReplicaSetSize, minReplicaSetSize, flags.IsStateful(), flags.HasPersistedState(), TimeSpan::FromSeconds(60.0), TimeSpan::MaxValue, typeId, vector<ServiceCorrelationDescription>(), placementConstraints, scaleoutCount, vector<ServiceLoadMetricDescription>(), 0, vector<byte>()),
        serviceType,
		FABRIC_INVALID_SEQUENCE_NUMBER);

    FailoverUnitUPtr failoverUnit = FailoverUnitUPtr(new FailoverUnit(
        FailoverUnitId(),
        ConsistencyUnitDescription(),
        serviceInfo,
        flags,
        pcEpoch,
        ccEpoch,
        0,
        DateTime::Now(),
        0,
        0,
        PersistenceState::NoChange));

    if (tokens.size() > 4)
    {
        tokens.clear();
        StringUtility::Split<wstring>(failoverUnitStr, tokens, L"[");

        for (size_t i = 1; i < tokens.size(); i++)
        {
            StringUtility::TrimSpaces(tokens[i]);
            StringUtility::Trim<wstring>(tokens[i], L"]");

            Replica::Flags::Enum flags;
            ReplicaInfo replicaInfo = ReplicaInfoFromString(tokens[i], flags);
            Reliability::ReplicaDescription const & replica = replicaInfo.Description;
            NodeInstance const & nodeInstance = replica.FederationNodeInstance;

            NodeInfoSPtr nodeInfo = make_shared<NodeInfo>(nodeInstance, Reliability::NodeDescription(), nodeInstance.InstanceId >= 1, true, false, false, DateTime::Now());

                
            auto fmReplica = failoverUnit->AddReplica(
                move(nodeInfo),
                replica.ReplicaId,
                replica.InstanceId,
                replica.State,
                flags,
                replica.PreviousConfigurationRole,
                replica.CurrentConfigurationRole,
                replica.IsUp,
                ServiceModel::ServicePackageVersionInstance(),
                PersistenceState::NoChange,
                DateTime::Now());
                
            fmReplica->ReplicaDescription.LastAcknowledgedLSN = replica.LastAcknowledgedLSN;
            fmReplica->ReplicaDescription.FirstAcknowledgedLSN = replica.FirstAcknowledgedLSN;
        }
    }

    failoverUnit->PostUpdate(DateTime::Now());

    return move(failoverUnit);
}

void State::TimerEventHandler(ModelChecker::Event const&)
{
    FailoverUnitCacheEntrySPtr entry = make_shared<FailoverUnitCacheEntry>(*(referenceToRoot_->FM), move(fmft_));
    bool isDeleted;
    entry->Lock(DynamicStateMachineTaskUPtr(), Common::TimeSpan::Zero, true, isDeleted);
    LockedFailoverUnitPtr failoverUnit(entry);

    vector<FMStateMachineActionUPtr> actions;

    for each (StateMachineTaskUPtr const& task in tasks_)
    {
        task->CheckFailoverUnit(failoverUnit, actions);
    }
    int replicaDifference = failoverUnit->ReplicaDifference;
    if (replicaDifference != 0) 
    {
        if (replicaDifference > 0) // more replica is needed
        {
            wstring numberOfInstances(to_wstring(static_cast<INT64>(replicaDifference)));
            wstring updateVersion = to_wstring(failoverUnit->UpdateVersion);
            Event newEvent( Event::Type::LocationReply, numberOfInstances+L"|"+updateVersion );
            auto iter = find_if( enabledEvents_.begin(), enabledEvents_.end(), 
                        [&newEvent](Event const& te)
                        {
                            return newEvent == te;
                        });
            // event not found in the list of enabled events
            if (iter == enabledEvents_.end()) 
            {
                enabledEvents_.push_back(newEvent);
            }
        }
        else 
        {
            // Assume for now: no extra instance
        }
    }

    failoverUnit->set_PersistenceState(PersistenceState::NoChange);
    GiveFailoverUnitBackToState(failoverUnit);
}

void State::LocationReplyEventHandler(ModelChecker::Event const& event)
{
    vector<wstring> messageComponents;
    StringUtility::Split<wstring>(event.MessageBodyStr, messageComponents, L"|");
    vector<int> nodeLocations = GetNodeLocations(Int32_Parse(messageComponents[0]));

    if (nodeLocations.size() > 0) 
    {
        FailoverUnitCacheEntrySPtr entry = make_shared<FailoverUnitCacheEntry>(*(referenceToRoot_->FM), move(fmft_));
        bool isDeleted;
        entry->Lock(DynamicStateMachineTaskUPtr(), Common::TimeSpan::Zero, true, isDeleted);
        LockedFailoverUnitPtr failoverUnit(entry);

        vector<FailoverUnitMovement::PLBAction> plbActions;
        for (int i = 0; i < nodeLocations.size(); i++) {
            int nodeLocation = nodeLocations.at(i);
            plbActions.push_back(FailoverUnitMovement::PLBAction(CreateNodeId(nodeLocation), CreateNodeId(nodeLocation), FailoverUnitMovement::Type::AddSecondary));
        }

        FailoverUnitMovement failoverUnitMovement(failoverUnit->FUId.Guid, move( wstring(L"TestService")), false, Int64_Parse(messageComponents[1]), move(plbActions));
        MovementTask movementTask(*(referenceToRoot_->FM), fauxCache_, move(failoverUnitMovement));

        vector<FMStateMachineActionUPtr> actionsProducedByPLB;
        movementTask.CheckFailoverUnit(failoverUnit, actionsProducedByPLB);

        vector<FMStateMachineActionUPtr> actions;

        for each (StateMachineTaskUPtr const& task in tasks_) {
            task->CheckFailoverUnit(failoverUnit, actions);
        }

        vector<wstring> actionStrings = ActionsToString(actions);
        for each (wstring const& actionStr in actionStrings) {
            pair<wstring, int> actionNameAndTarget = ParseActionString(actionStr);
            if(actionNameAndTarget.first == L"AddInstance") {
                wstring msgBodyStr = CreateMessageBody(
                                        failoverUnit->TargetReplicaSetSize,
                                        failoverUnit->MinReplicaSetSize,
                                        actionNameAndTarget.second,
                                        Event::Type::AddInstanceReply,
                                        failoverUnit->PreviousConfigurationEpoch,
                                        failoverUnit->CurrentConfigurationEpoch,
                                        failoverUnit->FailoverUnitFlags);
                Event newEvent(Event::Type::AddInstanceReply, msgBodyStr);
                auto iter = find_if(enabledEvents_.begin(), enabledEvents_.end(), 
                        [&newEvent](Event const& te)
                        {
                            return newEvent == te;
                        });
                if (iter == enabledEvents_.end())
                {
                    enabledEvents_.push_back(newEvent);
                }
            }
        }

        auto matched = find(enabledEvents_.begin(), enabledEvents_.end(), event);
        enabledEvents_.erase(matched);

        // changes version of the failoverunit
        failoverUnit->PostUpdate(DateTime::Now());
        failoverUnit->set_PersistenceState(PersistenceState::NoChange);

        GiveFailoverUnitBackToState(failoverUnit);
    }
}

void State::AddInstanceReplyEventHandler(ModelChecker::Event const& event)
{
    if (NodeIsUp(event)) 
        { // see if the node is in a position to reply at all
            Reliability::FailoverManagerComponent::FailoverUnitUPtr msgFailoverUnit = FailoverUnitFromString(event.MessageBodyStr);
            vector<Reliability::ReplicaDescription> msgReplicaDescriptions;
            msgFailoverUnit->ForEachReplica([&] (Replica& replica)
            {
                msgReplicaDescriptions.push_back(replica.ReplicaDescription);
            });
            Reliability::FailoverManagerComponent::FailoverUnitMessageProcessor messageProcessor(*(referenceToRoot_->FM));
            
            FailoverUnitCacheEntrySPtr entry = make_shared<FailoverUnitCacheEntry>(*(referenceToRoot_->FM), move(fmft_));
            bool isDeleted;
            entry->Lock(DynamicStateMachineTaskUPtr(), Common::TimeSpan::Zero, true, isDeleted);
            LockedFailoverUnitPtr failoverUnit(entry);

            Reliability::ReplicaReplyMessageBody body(
                msgFailoverUnit->FailoverUnitDescription, 
                move(msgReplicaDescriptions.front()),
                ErrorCode(ErrorCodeValue::Success));
            // msgFailoverUnit has the additional part we want in the 
            // failoverUnit to be incorporated
            messageProcessor.AddInstanceReplyProcessor(failoverUnit, body);

            // inside addinstancereplyprocessor, when the service location of a replica is updated in the 
            // completebuildreplica, the persistece state is changed; we need to set it to none, for a following
            // timer or location reply event, whoever tries to get a lockedFailoverUnit
            failoverUnit->PostUpdate(DateTime::Now());
            failoverUnit->set_PersistenceState(PersistenceState::NoChange);
            
            auto matched = find(enabledEvents_.begin(), enabledEvents_.end(), event);
            enabledEvents_.erase(matched);

            GiveFailoverUnitBackToState(failoverUnit);
        }
}

void State::RemoveInstanceReplyEventHandler(ModelChecker::Event const& event)
{
    // has not been checked if this handler changes the persistence state of the failover unit
    if (NodeIsUp(event)) 
        {
            Reliability::FailoverManagerComponent::FailoverUnitUPtr msgFailoverUnit = FailoverUnitFromString(event.MessageBodyStr);
            vector<Reliability::ReplicaDescription> msgReplicaDescriptions;
            msgFailoverUnit->ForEachReplica([&] (Replica& replica)
            {
                msgReplicaDescriptions.push_back(replica.ReplicaDescription);
            });

            FailoverUnitCacheEntrySPtr entry = make_shared<FailoverUnitCacheEntry>(*(referenceToRoot_->FM), move(fmft_));
            bool isDeleted;
            entry->Lock(DynamicStateMachineTaskUPtr(), Common::TimeSpan::Zero, true, isDeleted);
            LockedFailoverUnitPtr failoverUnit(entry);

            unique_ptr<ReplicaReplyMessageBody> body = make_unique<ReplicaReplyMessageBody>(
                msgFailoverUnit->FailoverUnitDescription, 
                move(msgReplicaDescriptions.front()),
                ErrorCode(ErrorCodeValue::Success));

            NodeInstance from = body->ReplicaDescription.FederationNodeInstance;
            Reliability::FailoverManagerComponent::FailoverUnitMessageProcessor messageProcessor(*(referenceToRoot_->FM));
            messageProcessor.RemoveInstanceReplyProcessor(failoverUnit, RSMessage::GetRemoveInstanceReply().Action, move(body), from);

            failoverUnit->PostUpdate(DateTime::Now());
            failoverUnit->set_PersistenceState(PersistenceState::NoChange);

            auto matched = find( enabledEvents_.begin(), enabledEvents_.end(), event );
            enabledEvents_.erase( matched );

            GiveFailoverUnitBackToState(failoverUnit);
        }
}

void State::NodeUpEventHandler(ModelChecker::Event const&)
{
    // See if there are any down nodes; Randomly choose one, mark it as up, and call the stateless tasks. 
    vector<int> downNodeLocations = GetIndicesOfDownNodes();
    if (downNodeLocations.size() > 0)
    {
        int firstIndex = downNodeLocations[0];
        fauxCache_.nodeList_.erase(fauxCache_.nodeList_.begin() + firstIndex);
        NodeInfo nodeInfo(CreateNodeInstance(firstIndex, firstIndex), Reliability::NodeDescription(), true, false, false, false, DateTime::Now());
        NodeRecord nodeRecord(nodeInfo, false);
        fauxCache_.nodeList_.insert(fauxCache_.nodeList_.begin() + firstIndex, nodeRecord);
    }
}

void State::NodeDownEventHandler(ModelChecker::Event const&)
{
    // If there are upnodes, randomly choose one of them
    // if it has an instance, find the corresponding replica in the failover unit, update the replica info, and call the stateless tasks
    // else mark it as down in the cache 
    vector<int> upNodeLocations = GetIndicesOfUpNodes();
    if (upNodeLocations.size() > 0) 
    {
        // random choosing would have been better, but it makes
        // current undoTransition difficult
        int firstIndex = upNodeLocations[0];
        //int locationOfNodeToDown = upNodeLocations[firstIndex];
        if (fauxCache_.nodeList_[ firstIndex ].HasAnInstance) 
        {
            auto replica_found = find_if(fmft_->BeginIterator, fmft_->EndIterator, 
                [firstIndex]( Replica const& replica )
                {
                    return replica.NodeInfoObj->Id.IdValue.Low == firstIndex;
                });
            if (replica_found != fmft_->EndIterator)
            {
                Replica& replica = *replica_found;
                NodeInfo nodeInfo(CreateNodeInstance(firstIndex, 0), Reliability::NodeDescription(), false, false, false, false, DateTime::Now());
                replica.set_NodeInfo(make_shared<NodeInfo>(nodeInfo));

                FailoverUnitCacheEntrySPtr entry = make_shared<FailoverUnitCacheEntry>(*(referenceToRoot_->FM), move(fmft_));
                bool isDeleted;
                entry->Lock(DynamicStateMachineTaskUPtr(), Common::TimeSpan::Zero, true, isDeleted);
                LockedFailoverUnitPtr failoverUnit(entry);

                vector<FMStateMachineActionUPtr> actions;
                for each (StateMachineTaskUPtr const& task in tasks_)
                {
                    task->CheckFailoverUnit(failoverUnit, actions);
                }

                // changes version of the failoverunit
                failoverUnit->PostUpdate(DateTime::Now());
                failoverUnit->set_PersistenceState(PersistenceState::NoChange);
                fauxCache_.nodeList_[firstIndex].HasAnInstance = false;

                GiveFailoverUnitBackToState(failoverUnit);
            }
        }
        else 
        {
            fauxCache_.nodeList_.erase(fauxCache_.nodeList_.begin() + firstIndex);
            NodeInfo nodeInfo(CreateNodeInstance(firstIndex, firstIndex), Reliability::NodeDescription(), false, false, false, false, DateTime::Now());
            NodeRecord nodeRecord(nodeInfo, false);
            fauxCache_.nodeList_.insert(fauxCache_.nodeList_.begin() + firstIndex, nodeRecord);
        }
    }	
}

void State::MakeTransition(ModelChecker::Event const& event)
{
    switch(event.EventType) 
    {
        case Event::Type::Timer:
            TimerEventHandler(event);
            break;
        case Event::Type::LocationReply:
            LocationReplyEventHandler(event);
            break;
        case Event::Type::AddInstanceReply:
            AddInstanceReplyEventHandler(event);
            break;
        case Event::Type::RemoveInstanceReply:
            RemoveInstanceReplyEventHandler(event);
            break;
        case Event::Type::NodeUp:
            NodeUpEventHandler(event);
            break;
        case Event::Type::NodeDown:
            NodeDownEventHandler(event);
            break;
    }
}
