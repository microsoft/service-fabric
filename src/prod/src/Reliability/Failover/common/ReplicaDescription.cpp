// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Common.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;

ReplicaDescription::ReplicaDescription()
    : replicaId_(Constants::InvalidReplicaId),
      instanceId_(Constants::InvalidReplicaId),
      previousConfigurationRole_(ReplicaRole::None),
      currentConfigurationRole_(ReplicaRole::None),
      state_(ReplicaStates::Ready),
      isUp_(true),
      lastAcknowledgedLSN_(FABRIC_INVALID_SEQUENCE_NUMBER),
      firstAcknowledgedLSN_(FABRIC_INVALID_SEQUENCE_NUMBER),
      serviceLocation_(),
      replicationEndpoint_(),
      packageVersionInstance_(ServiceModel::ServicePackageVersionInstance::Invalid)
{
}

ReplicaDescription::ReplicaDescription(
    Federation::NodeInstance federationNodeInstance,
    int64 replicaId,
    int64 instanceId,
    ReplicaRole::Enum previousConfigurationRole,
    ReplicaRole::Enum currentConfigurationRole,
    ReplicaStates::Enum state,
    bool isUp,
    FABRIC_SEQUENCE_NUMBER lastAcknowledgedLSN,
    FABRIC_SEQUENCE_NUMBER firstAcknowledgedLSN,
    wstring const & serviceLocation,
    wstring const & replicationEndpoint,
    ServiceModel::ServicePackageVersionInstance const & version)
    : federationNodeInstance_(federationNodeInstance),
      replicaId_(replicaId),
      instanceId_(instanceId),
      previousConfigurationRole_(previousConfigurationRole),
      currentConfigurationRole_(currentConfigurationRole),
      isUp_(isUp),
      state_(state),
      lastAcknowledgedLSN_(lastAcknowledgedLSN),
      firstAcknowledgedLSN_(firstAcknowledgedLSN),
      serviceLocation_(serviceLocation),
      replicationEndpoint_(replicationEndpoint),
      packageVersionInstance_(version)
{
    ASSERT_IF(replicaId <= Constants::InvalidReplicaId || instanceId <= Constants::InvalidReplicaId, "ReplicaDescription.ctor - Invalid replica id or instance id");
}
        
ReplicaDescription & ReplicaDescription::operator = (ReplicaDescription && other)
{
    if (this != &other)
    {
        federationNodeInstance_ = other.federationNodeInstance_;
        replicaId_ = other.replicaId_;
        instanceId_ = other.instanceId_;
        currentConfigurationRole_ = other.currentConfigurationRole_;
        previousConfigurationRole_ = other.previousConfigurationRole_;
        state_ = other.state_;
        isUp_ = other.isUp_;
        lastAcknowledgedLSN_ = other.lastAcknowledgedLSN_;
        firstAcknowledgedLSN_ = other.firstAcknowledgedLSN_;
        serviceLocation_ = move(other.serviceLocation_);
        replicationEndpoint_ = move(other.replicationEndpoint_);
        packageVersionInstance_ = other.packageVersionInstance_;
    }

    return *this;
}

ReplicaDescription::ReplicaDescription(ReplicaDescription && other)
    : federationNodeInstance_(other.federationNodeInstance_),
      replicaId_(other.replicaId_),
      instanceId_(other.instanceId_),
      currentConfigurationRole_(other.currentConfigurationRole_),
      previousConfigurationRole_(other.previousConfigurationRole_),
      state_(other.state_),
      isUp_(other.isUp_),
      lastAcknowledgedLSN_(other.lastAcknowledgedLSN_),
      firstAcknowledgedLSN_(other.firstAcknowledgedLSN_),
      serviceLocation_(move(other.serviceLocation_)),
      replicationEndpoint_(move(other.replicationEndpoint_)),
      packageVersionInstance_(other.packageVersionInstance_)
{
}

ReplicaDescription ReplicaDescription::CreateDroppedReplicaDescription(
    Federation::NodeInstance const & nodeInstance,
    int64 replicaId,
    int64 instanceId)
{
    ReplicaDescription replicaDesc(nodeInstance, replicaId, instanceId);
    replicaDesc.IsUp = false;
    replicaDesc.State = Reliability::ReplicaStates::Dropped;
    return replicaDesc;
}

void ReplicaDescription::AssertLSNAreNotUnknown() const
{
    TESTASSERT_IF(LastAcknowledgedLSN == Constants::UnknownLSN || FirstAcknowledgedLSN == Constants::UnknownLSN, "LSN cannot be unknown");
}

void ReplicaDescription::AssertLastAckLSNIsValid() const
{
    TESTASSERT_IF(!IsLastAcknowledgedLSNValid, "Last Ack LSN must exist");
}

void ReplicaDescription::AssertFirstAckLSNIsValid() const
{
    TESTASSERT_IF(!IsFirstAcknowledgedLSNValid, "First Ack LSN must exist");
}

void ReplicaDescription::AssertLastAckLSNIsInvalid() const
{
    TESTASSERT_IF(IsLastAcknowledgedLSNValid, "Last Ack LSN must be invalid");
}

void ReplicaDescription::AssertFirstAckLSNIsInvalid() const
{
    TESTASSERT_IF(IsFirstAcknowledgedLSNValid, "First Ack LSN must exist");
}

void ReplicaDescription::AssertLSNsAreValid() const
{
    AssertLastAckLSNIsValid();
    AssertFirstAckLSNIsValid();
}

void ReplicaDescription::AssertLSNsAreInvalid() const
{
    AssertLastAckLSNIsInvalid();
    AssertFirstAckLSNIsInvalid();
}

void ReplicaDescription::WriteTo(TextWriter& w, FormatOptions const& option) const
{
	if (option.formatString != "s")
	{
	    w.Write("{0}/{1} ", previousConfigurationRole_, currentConfigurationRole_);
	}

    w.Write("{0} {1}",
        state_,
        isUp_ ? "U" : "D");

    w.Write(
        " {0} {1}:{2} {3} {4} {5}\r\n",
        federationNodeInstance_,
        replicaId_,
        instanceId_,
        firstAcknowledgedLSN_,
        lastAcknowledgedLSN_,
        packageVersionInstance_);
}

void ReplicaDescription::WriteToEtw(uint16 contextSequenceId) const
{
    wstring states = wformatString("{0}/{1} {2} {3}",
        previousConfigurationRole_,
        currentConfigurationRole_,
        state_,
        isUp_ ? "U" : "D");
    
    ReliabilityEventSource::Events->ReplicaDescription(
        contextSequenceId,
        states, 
        federationNodeInstance_,
        replicaId_,
        instanceId_,
        firstAcknowledgedLSN_,
        lastAcknowledgedLSN_,
        packageVersionInstance_);
}

