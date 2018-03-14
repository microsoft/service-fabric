// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace ServiceModel;
using namespace Infrastructure;

Replica Replica::InvalidReplica = Replica();

Replica::Replica() : 
    lastUpdated_(DateTime::Zero)
{
}

Replica::Replica(
    Federation::NodeInstance federationNodeInstance, 
    int64 replicaId,
    int64 instance) : 
    desc_(federationNodeInstance, replicaId, instance),
    lastUpdated_(DateTime::Now())
{
    Initialize();
}

// Construct replica from given replica
Replica::Replica(Replica && other) : 
    desc_(move(other.desc_)),
    intermediateConfigurationRole_(std::move(other.intermediateConfigurationRole_)),
    state_(std::move(other.state_)),
    messageStage_(std::move(other.messageStage_)),
    toBeDeactivated_(std::move(other.toBeDeactivated_)),
    toBeActivated_(std::move(other.toBeActivated_)),     
    toBeRestarted_(std::move(other.toBeRestarted_)),
    replicatorRemovePending_(std::move(other.replicatorRemovePending_)),
    lastUpdated_(std::move(other.lastUpdated_)),
    deactivationInfo_(std::move(other.deactivationInfo_))
{}

// Construct replica from given replica
Replica::Replica(Replica const & other) : 
    desc_(other.desc_.FederationNodeInstance,
          other.desc_.ReplicaId,
          other.desc_.InstanceId,
          other.desc_.PreviousConfigurationRole,
          other.desc_.CurrentConfigurationRole,
          other.desc_.State,
          other.desc_.IsUp,
          other.desc_.LastAcknowledgedLSN,
          other.desc_.FirstAcknowledgedLSN,
          other.desc_.ServiceLocation,
          other.desc_.ReplicationEndpoint,
          other.desc_.PackageVersionInstance),
    intermediateConfigurationRole_(other.intermediateConfigurationRole_),
    state_(other.state_),
    messageStage_(other.messageStage_),
    toBeDeactivated_(other.toBeDeactivated_),
    toBeActivated_(other.toBeActivated_),
    toBeRestarted_(other.toBeRestarted_),
    replicatorRemovePending_(other.replicatorRemovePending_),
    lastUpdated_(other.lastUpdated_),
    deactivationInfo_(other.deactivationInfo_)
{}

Replica & Replica::operator = (Replica && other)
{
    if (this != &other)
    {
        desc_ = move(other.desc_);
        intermediateConfigurationRole_ = std::move(other.intermediateConfigurationRole_);
        state_ = std::move(other.state_);
        messageStage_ = std::move(other.messageStage_);
        toBeDeactivated_ = std::move(other.toBeDeactivated_);
        toBeActivated_ = std::move(other.toBeActivated_);
        toBeRestarted_ = std::move(other.toBeRestarted_);
        replicatorRemovePending_ = std::move(other.replicatorRemovePending_);
        lastUpdated_ = std::move(other.lastUpdated_);
        deactivationInfo_ = std::move(other.deactivationInfo_);
    }

    return *this;
}

void Replica::set_State(Reliability::ReconfigurationAgentComponent::ReplicaStates::Enum value)
{
    state_ = value;
    
    desc_.State = ReplicaStates::ToReplicaDescriptionState(state_);
}

FABRIC_QUERY_SERVICE_REPLICA_STATUS Replica::get_QueryStatus() const
{
    if (IsDropped || IsInDrop)
    {
        return FABRIC_QUERY_SERVICE_REPLICA_STATUS_DROPPED;
    }
    else if (!IsUp)
    {
        return FABRIC_QUERY_SERVICE_REPLICA_STATUS_DOWN;
    }
    else if (IsInCreate || IsInBuild)
    {
        return FABRIC_QUERY_SERVICE_REPLICA_STATUS_INBUILD;
    }
    else if (IsReady)
    {
        return FABRIC_QUERY_SERVICE_REPLICA_STATUS_READY;
    }
    else if (IsStandBy)
    {
        return FABRIC_QUERY_SERVICE_REPLICA_STATUS_STANDBY;
    }
    else
    {
        Assert::TestAssert("Unknown query status {0}", *this);
        return FABRIC_QUERY_SERVICE_REPLICA_STATUS_INVALID;
    }
}

// Initialize replica, called during valid replica construction and reuse
void Replica::Initialize()
{
    desc_.CurrentConfigurationRole = ReplicaRole::None;
    desc_.PreviousConfigurationRole = ReplicaRole::None;
    intermediateConfigurationRole_ = ReplicaRole::None;

    State = ReplicaStates::InCreate;

    messageStage_ = ReplicaMessageStage::None;  

    replicatorRemovePending_ = false;
    
    Reset(true /*resetLSN*/);
}

void Replica::Reset(bool resetLSN)
{
    toBeDeactivated_ = false;
    toBeActivated_ = false;
    toBeRestarted_ = false;

    if (resetLSN)
    {
        ClearLSN();
    }

    desc_.ServiceLocation = wstring();
    desc_.ReplicationEndpoint = wstring();

    lastUpdated_ = DateTime::get_Now();
}

Reliability::ReplicaDescription Replica::GetDescription(ReplicaRole::Enum pcRole) const
{
    // IsUp and State need special handling because RA allows a replica to be ID U
    // The ToReplicaDescriptionState function converts ID to DD
    // In which case the isUp has to be false
    auto state = ReplicaStates::ToReplicaDescriptionState(State);
    bool isUp = IsInDrop ? false : IsUp;

    return Reliability::ReplicaDescription(
        FederationNodeInstance,
        ReplicaId,
        InstanceId,
        pcRole,
        desc_.CurrentConfigurationRole,
        state,
        isUp,
        desc_.LastAcknowledgedLSN,
        desc_.FirstAcknowledgedLSN,
        ServiceLocation,
        ReplicationEndpoint,
        PackageVersionInstance);
}


Reliability::ReplicaDescription Replica::GetCurrentConfigurationDescription() const
{
    return GetDescription(ReplicaRole::None);
}

Reliability::ReplicaDescription Replica::GetConfigurationDescriptionForDeactivateOrActivate(bool treatInBuildAsReady) const
{
    auto desc = GetDescription(PreviousConfigurationRole);

    if (treatInBuildAsReady && desc.IsInBuild)
    {
        desc.State = Reliability::ReplicaStates::Ready;
    }

    return desc;
}

Reliability::ReplicaDescription Replica::GetConfigurationDescriptionForReplicator() const
{
    auto description = GetDescription(PreviousConfigurationRole);

    /*
        Mark the replica as down if it is ToBeRestarted
        This will still consider the replica correctly in quroum calculations
        but will prevent RAP from giving it to the replicator
    */
    if (ToBeRestarted)
    {
        description.IsUp = false;
    }

    return description;
}

Reliability::ReplicaDescription Replica::GetConfigurationDescriptionForRebuildAndReplicaUp() const
{
    auto description = GetDescription(PreviousConfigurationRole);

    /*
        Clear the LSN 
        While composing the ft info for rebuild or replica up do not send lsn to fm
        fm may store the lsn in its view or it may have the lsn when it generates the ft
        after rebuild which can cause a subsequent DoReconfiguration to already have LSN
    */
    description.LastAcknowledgedLSN = FABRIC_INVALID_SEQUENCE_NUMBER;
    description.FirstAcknowledgedLSN = FABRIC_INVALID_SEQUENCE_NUMBER;

    return description;
}

void Replica::UpdateStateOnLocalReplicaDown(bool isLocalReplica)
{
    messageStage_ = ReplicaMessageStage::None;

    replicatorRemovePending_ = false;

    Reset(true /*resetLSN*/);

    if (isLocalReplica)
    {
        IsUp = false;

        // When a persisted replica goes down it becomes SB
        // Except if it was ID in which case keep it ID
        // So that on restart the drop continues as RA
        // does not know whether CR(None) has been invoked or not
        if (State != ReplicaStates::InDrop)
        {
            State = ReplicaStates::StandBy;
        }
    }
}

void Replica::UpdateStateOnReopen(bool isLocalReplica)
{
    TESTASSERT_IF(messageStage_ != ReplicaMessageStage::None, "For down replica message stage must have been set {0}", *this);
    TESTASSERT_IF(IsUp, "For down replica message isUp must have been false {0}", *this);
    TESTASSERT_IF(State != ReplicaStates::StandBy && State != ReplicaStates::InDrop, "For down replica message state must have been SB/ID {0}", *this);

    if (isLocalReplica)
    {
        // The replica is now Up
        IsUp = true;

        // Increment instance id
        desc_.InstanceId++;
    }
}

void Replica::UpdateInstance(Reliability::ReplicaDescription const & newReplicaDescription, bool resetLSN)
{
    desc_.FederationNodeInstance = newReplicaDescription.FederationNodeInstance;
    desc_.InstanceId = newReplicaDescription.InstanceId;
    desc_.IsUp = newReplicaDescription.IsUp;
    desc_.PackageVersionInstance = newReplicaDescription.PackageVersionInstance;

    desc_.ReplicaId = newReplicaDescription.ReplicaId;

    Reset(resetLSN);

    if (!resetLSN && IsLSNSet())
    {
        // Replicator loses the catchup capability on restarts
        desc_.FirstAcknowledgedLSN = 0;
    }

    desc_.ReplicationEndpoint = newReplicaDescription.ReplicationEndpoint;
}

bool Replica::IsStale(Reliability::ReplicaDescription const & incomingReplica) const
{
    if (incomingReplica.InstanceId < InstanceId)
    {
        return true;
    }
    else if (incomingReplica.InstanceId > InstanceId)
    {
        return false;
    }
    else
    {
        return incomingReplica.IsUp && !this->IsUp;
    }
}

void Replica::MarkAsDropped()
{
    desc_.IsUp = false;
    toBeRestarted_ = false;
    State = ReplicaStates::Dropped;
    messageStage_ = ReplicaMessageStage::None;
}

int64 Replica::GetLastAcknowledgedLSN() const
{
    return desc_.LastAcknowledgedLSN;
}

int64 Replica::GetFirstAcknowledgedLSN() const
{
    return desc_.FirstAcknowledgedLSN;
}

bool Replica::IsLSNUnknown() const
{
    return deactivationInfo_.IsDropped;
}

bool Replica::IsLSNSet() const
{
    return desc_.FirstAcknowledgedLSN != FABRIC_INVALID_SEQUENCE_NUMBER;
}

void Replica::SetFirstAndLastLSN(
    int64 firstLSN, 
    int64 lastLSN)
{
    TESTASSERT_IF(firstLSN == FABRIC_INVALID_SEQUENCE_NUMBER, "Cannot ask to set invalid LSN");
    TESTASSERT_IF(firstLSN == Constants::UnknownLSN || lastLSN == Constants::UnknownLSN, "Setting Unknown LSN must happen explicitly with the call to set unknown LSN");
    SetFirstAndLastLSNWithoutValidation(firstLSN, lastLSN);
}

void Replica::SetFirstAndLastLSNWithoutValidation(
    int64 firstLSN,
    int64 lastLSN)
{
    desc_.FirstAcknowledgedLSN = firstLSN;
    desc_.LastAcknowledgedLSN = lastLSN;
}

void Replica::SetFirstAndLastLSN(
    Reliability::ReplicaDescription const & newReplicaDesc)
{
    SetFirstAndLastLSN(newReplicaDesc.FirstAcknowledgedLSN, newReplicaDesc.LastAcknowledgedLSN);
}

bool Replica::TrySetFirstAndLastLSNIfPresentInParameter(
    Reliability::ReplicaDescription const & description)
{
    if (description.LastAcknowledgedLSN != FABRIC_INVALID_SEQUENCE_NUMBER)
    {
        TESTASSERT_IF(description.FirstAcknowledgedLSN == FABRIC_INVALID_SEQUENCE_NUMBER, "Inconsistent LSN received: {0}", description);
        
        SetFirstAndLastLSN(description);
        return true;
    }

    return false;
}

void Replica::SetProgress(
    int64 firstLSN,
    int64 lastLSN,
    Reliability::ReplicaDeactivationInfo const & deactivationInfo)
{
    SetFirstAndLastLSN(firstLSN, lastLSN);
    deactivationInfo_ = deactivationInfo;
}

void Replica::Test_SetProgress(
    int64 firstLSN,
    int64 lastLSN,
    Reliability::ReplicaDeactivationInfo const & deactivationInfo)
{
    SetFirstAndLastLSNWithoutValidation(firstLSN, lastLSN);
    deactivationInfo_ = deactivationInfo;
}

void Replica::SetProgress(
    Reliability::ReplicaDescription const & replicaDescription,
    Reliability::ReplicaDeactivationInfo const & deactivationInfo)    
{
    SetProgress(
        replicaDescription.FirstAcknowledgedLSN,
        replicaDescription.LastAcknowledgedLSN,
        deactivationInfo);
}

void Replica::SetLastLSN(int64 lastLSN)
{
    desc_.LastAcknowledgedLSN = lastLSN;
}

void Replica::SetLSNToUnknown(Reliability::ReplicaDescription const & incomingReplicaDescription)
{
    if (incomingReplicaDescription.IsLastAcknowledgedLSNValid)
    {
        incomingReplicaDescription.AssertLSNsAreValid();
        SetFirstAndLastLSN(incomingReplicaDescription);
    }
    else
    {
        SetFirstAndLastLSNWithoutValidation(Constants::UnknownLSN, Constants::UnknownLSN);
    }

    deactivationInfo_ = ReplicaDeactivationInfo::Dropped;
}

bool Replica::TryClearUnknownLSN()
{
    if (IsLSNUnknown())
    {
        ClearLSN();
        return true;
    }

    return false;
}

void Replica::ClearLSN()
{
    SetFirstAndLastLSNWithoutValidation(FABRIC_INVALID_SEQUENCE_NUMBER, FABRIC_INVALID_SEQUENCE_NUMBER);
    deactivationInfo_.Clear();
}

void Replica::AssertInvariants(FailoverUnit const & ) const
{
}

void Replica::WriteTo(TextWriter& writer, FormatOptions const &) const
{
    TraceWriter traceWriter(*this);

    writer.Write(traceWriter);
}

void Replica::WriteToEtw(uint16 contextSequenceId) const
{
    TraceWriter traceWriter(*this);

    RAEventSource::Events->RAReplica(contextSequenceId, traceWriter);
}

Replica::TraceWriter::TraceWriter(Replica const & replica)
: replica_(replica)
{
}

string Replica::TraceWriter::AddField(Common::TraceEvent & traceEvent, std::string const & name)
{   
    std::string format = "{0}/{1}/{2} {3} {4} {5} {6} {7} {8} {9} {10} {11}:{12} {13} {14} {15} {16}";
    size_t index = 0;

    traceEvent.AddEventField<ReplicaRole::Trace>(format, name + ".pcRole", index);
    traceEvent.AddEventField<ReplicaRole::Trace>(format, name + ".icRole", index);
    traceEvent.AddEventField<ReplicaRole::Trace>(format, name + ".ccRole", index);

    traceEvent.AddEventField<ReplicaStates::Trace>(format, name + ".state", index);
    traceEvent.AddEventField<string>(format, name + ".isup", index);
    traceEvent.AddEventField<ReplicaMessageStage::Trace>(format, name + ".messagestage", index);
    traceEvent.AddEventField<char>(format, name + ".tobedeactivated", index);
    traceEvent.AddEventField<char>(format, name + ".tobeactivated", index);    
    traceEvent.AddEventField<char>(format, name + ".replicatorRemovePending", index);
    traceEvent.AddEventField<char>(format, name + ".toBeRestarted", index);

    traceEvent.AddEventField<Federation::NodeInstance>(format, name + ".nodeInstance", index);
    traceEvent.AddEventField<int64>(format, name + ".replicaId", index);
    traceEvent.AddEventField<int64>(format, name + ".instanceId", index);
    traceEvent.AddEventField<ServicePackageVersionInstance>(format, name + ".spvi", index);
    
    traceEvent.AddEventField<int64>(format, name + ".firstAckLSN", index);
    traceEvent.AddEventField<int64>(format, name + ".lastAckLSN", index);

    traceEvent.AddEventField<Reliability::ReplicaDeactivationInfo>(format, name + ".deactivationInfo", index);

    return format;
}

void Replica::TraceWriter::FillEventData(Common::TraceEventContext & context) const
{
    context.WriteCopy<uint>(static_cast<uint>(replica_.PreviousConfigurationRole));
    context.WriteCopy<uint>(static_cast<uint>(replica_.IntermediateConfigurationRole));
    context.WriteCopy<uint>(static_cast<uint>(replica_.CurrentConfigurationRole));

    context.WriteCopy<uint>(static_cast<uint>(replica_.State));
    context.WriteCopy<string>(replica_.IsUp ? "U" : "D");
    context.WriteCopy<uint>(static_cast<uint>(replica_.MessageStage));
    context.WriteCopy(static_cast<char>(replica_.ToBeDeactivated));
    context.WriteCopy(static_cast<char>(replica_.ToBeActivated));    
    context.WriteCopy(static_cast<char>(replica_.replicatorRemovePending_));
    context.WriteCopy(static_cast<char>(replica_.toBeRestarted_));

    context.Write(replica_.FederationNodeInstance);
    context.WriteCopy(replica_.ReplicaId);
    context.WriteCopy(replica_.InstanceId);
    context.Write(replica_.PackageVersionInstance);
    context.WriteCopy(replica_.desc_.FirstAcknowledgedLSN);
    context.WriteCopy(replica_.desc_.LastAcknowledgedLSN);    
    context.Write(replica_.deactivationInfo_);
}

void Replica::TraceWriter::WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer.Write("{0}/{1}/{2} {3} {4} {5} {6} {7} {8} {9}",
        replica_.desc_.PreviousConfigurationRole,
        replica_.intermediateConfigurationRole_,
        replica_.desc_.CurrentConfigurationRole,
        replica_.State, 
        replica_.IsUp ? L"U" : L"D", 
        replica_.messageStage_, 
        replica_.toBeDeactivated_ ? L"1" : L"0",
        replica_.toBeActivated_ ? L"1" : L"0",
        replica_.replicatorRemovePending_ ? L"1" : L"0",
        replica_.toBeRestarted_ ? L"1" : L"0");

    writer.Write(" {0} {1}:{2} {3} {4} {5} {6}",
        replica_.desc_.FederationNodeInstance,
        replica_.desc_.ReplicaId,
        replica_.desc_.InstanceId,
        replica_.desc_.PackageVersionInstance,
        replica_.desc_.FirstAcknowledgedLSN,
        replica_.desc_.LastAcknowledgedLSN,
        replica_.deactivationInfo_);
}
