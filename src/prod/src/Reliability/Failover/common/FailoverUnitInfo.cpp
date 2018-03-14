// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Common.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;

FailoverUnitInfo::FailoverUnitInfo() 
    : isReportFromPrimary_(false), 
      isLocalReplicaDeleted_(false)
{
}

FailoverUnitInfo::FailoverUnitInfo(
    Reliability::ServiceDescription const& serviceDescription,
    Reliability::FailoverUnitDescription const& failoverUnitDescription,
    Epoch const& icEpoch,
    vector<ReplicaInfo> && replicas)
    : serviceDescription_(serviceDescription),
      failoverUnitDescription_(failoverUnitDescription),
      icEpoch_(icEpoch),
      isReportFromPrimary_(false),
      replicas_(move(replicas)),
      isLocalReplicaDeleted_(false)
{
}

FailoverUnitInfo::FailoverUnitInfo(
    Reliability::ServiceDescription const& serviceDescription,
    Reliability::FailoverUnitDescription const& failoverUnitDescription,
    Epoch const& icEpoch,
    bool isReportFromPrimary,
    bool isLocalReplicaDeleted,
    vector<ReplicaInfo> && replicas)
    : serviceDescription_(serviceDescription),
      failoverUnitDescription_(failoverUnitDescription),
      icEpoch_(icEpoch),
      isReportFromPrimary_(isReportFromPrimary),
      replicas_(move(replicas)),
      isLocalReplicaDeleted_(isLocalReplicaDeleted)
{
}

FailoverUnitInfo::FailoverUnitInfo(FailoverUnitInfo && other)
    : serviceDescription_(move(other.serviceDescription_)),
      failoverUnitDescription_(move(other.failoverUnitDescription_)),
      icEpoch_(other.icEpoch_),
      isReportFromPrimary_(other.isReportFromPrimary_),
      replicas_(move(other.replicas_)),
      isLocalReplicaDeleted_(move(other.isLocalReplicaDeleted_))
{
}

FailoverUnitInfo & FailoverUnitInfo::operator = (FailoverUnitInfo && other)
{
    if (this != &other)
    {
        serviceDescription_ = move(other.serviceDescription_);
        failoverUnitDescription_ = move(other.failoverUnitDescription_);
        icEpoch_ = move(other.icEpoch_);
        isReportFromPrimary_ = move(other.isReportFromPrimary_);
        replicas_ = move(other.replicas_);
        isLocalReplicaDeleted_ = move(other.isLocalReplicaDeleted_);
    }

    return *this;
}

void FailoverUnitInfo::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.WriteLine(serviceDescription_);
    w.WriteLine("{0} {1} {2} {3:x}/{4:x}/{5:x} {6} {7}",
        failoverUnitDescription_.FailoverUnitId,
        serviceDescription_.TargetReplicaSetSize,
        serviceDescription_.MinReplicaSetSize,
        failoverUnitDescription_.PreviousConfigurationEpoch,
        icEpoch_,
        failoverUnitDescription_.CurrentConfigurationEpoch,
        isReportFromPrimary_ ? L"True" : L"False",
        isLocalReplicaDeleted_);

    for (ReplicaInfo const& replica : replicas_)
    {
        w.Write(replica);
    }
}

void FailoverUnitInfo::WriteToEtw(uint16 contextSequenceId) const
{
    ReliabilityEventSource::Events->FailoverUnitInfo(
        contextSequenceId,
        failoverUnitDescription_.FailoverUnitId.Guid,
        serviceDescription_.TargetReplicaSetSize,
        serviceDescription_.MinReplicaSetSize,
        failoverUnitDescription_.PreviousConfigurationEpoch,
        icEpoch_,
        failoverUnitDescription_.CurrentConfigurationEpoch,
        isReportFromPrimary_,
        isLocalReplicaDeleted_,
        replicas_);
}

class ConfigurationValidator
{
public:
    ConfigurationValidator(bool isEnabled, bool isPC) :
        isEnabled_(isEnabled),
        isPC_(isPC),
        primaryCount_(0),
        secondaryCount_(0)
    {
    }

    void ProcessReplica(ReplicaRole::Enum role)
    {
        if (role == ReplicaRole::Primary)
        {
            primaryCount_++;
        }
        else if (role == ReplicaRole::Secondary)
        {
            secondaryCount_++;
        }
    }

    void Validate(FailoverUnitInfo const & owner) const
    {
        if (!isEnabled_)
        {
            return;
        }

        /*
            PC is treated as special for the add primary case where primary+secondary count is allowed to be zero if the PC set is empty
        */
        if (!isPC_)
        {
            ASSERT_IF(primaryCount_ != 1, "Report should contain one primary {0}", owner);
            ASSERT_IF(primaryCount_ == 0 && secondaryCount_ == 0, "Report contains 0 primary and 0 secondary {0}", owner);
        }
        else
        {
            ASSERT_IF(primaryCount_ > 1, "Report contains more than one primary {0}", owner);
        }
    }

private:
    bool isEnabled_;
    bool isPC_;
    int primaryCount_;
    int secondaryCount_;
};

void FailoverUnitInfo::AssertInvariants() const
{
    ASSERT_IF(replicas_.empty(), "Empty report from node {0}", *this);

    // Exclude DD or Idle replicas
    if (IsDroppedOrIdleReplica())
    {
        return;
    }

    // Validate the PC
    ConfigurationValidator pcValidator(failoverUnitDescription_.PreviousConfigurationEpoch.IsValid(), true);
    ConfigurationValidator ccValidator(isReportFromPrimary_, false);
    ConfigurationValidator icValidator(ICEpoch.IsValid() && !isReportFromPrimary_, false);

    for (auto const & replica : replicas_)
    {
        pcValidator.ProcessReplica(replica.Description.PreviousConfigurationRole);
        ccValidator.ProcessReplica(replica.Description.CurrentConfigurationRole);
        icValidator.ProcessReplica(replica.ICRole);

        // FT Info is sent either during rebuild or replica up
        // In neither case should lsn be sent 
        // RA should send lsn to fm only in change configuration
        replica.Description.AssertLSNsAreInvalid();
    }

    pcValidator.Validate(*this);
    ccValidator.Validate(*this);
    icValidator.Validate(*this);
}

bool FailoverUnitInfo::IsDroppedOrIdleReplica() const
{
    if (replicas_.size() != 1)
    {
        return false;
    }

    auto const & replica = replicas_[0];
    return replica.Description.State == Reliability::ReplicaStates::Dropped ||
        replica.Description.CurrentConfigurationRole == ReplicaRole::Idle;
}
