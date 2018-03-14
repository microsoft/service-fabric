// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;

using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Health;

ReconfigurationStuckHealthReportDescriptor::ReconfigurationStuckHealthReportDescriptor() :
    reconfigurationStuckReason_(ReconfigurationProgressStages::Invalid),
    replicas_(),
    isClearWarningDescriptor_(true),
    reconfigurationStartTime_(StopwatchTime::MaxValue),
    reconfigurationPhaseStartTime_(StopwatchTime::MaxValue)
{
}

ReconfigurationStuckHealthReportDescriptor::ReconfigurationStuckHealthReportDescriptor(
    ReconfigurationProgressStages::Enum reconfigurationStuckReason,
    vector<HealthReportReplica>&& replicas,
    Common::StopwatchTime const & reconfigurationStartTime,
    Common::StopwatchTime const & reconfigurationPhaseStartTime) :
        reconfigurationStuckReason_(reconfigurationStuckReason),
        replicas_(move(replicas)),
        isClearWarningDescriptor_(false),
        reconfigurationStartTime_(reconfigurationStartTime),
        reconfigurationPhaseStartTime_(reconfigurationPhaseStartTime)
{
}

bool ReconfigurationStuckHealthReportDescriptor::operator==(ReconfigurationStuckHealthReportDescriptor const & rhs)
{
    if (reconfigurationStartTime_ != rhs.ReconfigurationStartTime)
    {
        return false;
    }

    if (reconfigurationPhaseStartTime_ != rhs.ReconfigurationPhaseStartTime)
    {
        return false;
    }

    if (reconfigurationStuckReason_ != rhs.ReconfigurationStuckReason)
    {
        return false;
    }

    return CompareHealthReportReplicaVectors(replicas_, rhs.replicas_);
}

bool ReconfigurationStuckHealthReportDescriptor::operator!=(ReconfigurationStuckHealthReportDescriptor const & rhs)
{
    return !(*this==rhs);
}

std::wstring ReconfigurationStuckHealthReportDescriptor::GenerateReportDescriptionInternal(HealthContext const &) const
{
    return GenerateReportDescriptionInternal();
}

std::wstring ReconfigurationStuckHealthReportDescriptor::GenerateReportDescriptionInternal() const
{
    if (!isClearWarningDescriptor_)
    {
        wstring description;
        StringWriter writer(description);

        writer.WriteLine(GetReasonDescription(replicas_.size()));
        TestAndWriteReplicaVector(replicas_, writer, L"Pending Replicas");

        writer.WriteLine();
        writer.WriteLine("Reconfiguration start time: {0}. Reconfiguration phase start time: {1}.", reconfigurationStartTime_, reconfigurationPhaseStartTime_);
        
        return description;
    }
    else
    {
        return L"Reconfiguration OK";
    }
}

std::wstring ReconfigurationStuckHealthReportDescriptor::GetReasonDescription(size_t replicas) const
{
    switch (reconfigurationStuckReason_)
    {
    case ReconfigurationProgressStages::Phase1_WaitingForReadQuorum:
    case ReconfigurationProgressStages::Phase3_PCBelowReadQuorum:
    case ReconfigurationProgressStages::Phase3_WaitingForReplicas:
    case ReconfigurationProgressStages::Phase4_UpReadyReplicasPending:
    case ReconfigurationProgressStages::Phase4_UpReadyReplicasActivated:
    case ReconfigurationProgressStages::Phase4_ReplicaPendingRestart:
    case ReconfigurationProgressStages::Phase4_ReplicaStuckIB:
        return wformatString(L" Waiting for response from {0} replicas", replicas);
    case ReconfigurationProgressStages::Phase2_NoReplyFromRap:
    case ReconfigurationProgressStages::Phase0_NoReplyFromRAP:
    case ReconfigurationProgressStages::Phase4_LocalReplicaNotReplied:
    case ReconfigurationProgressStages::Phase4_ReplicatorConfigurationUpdatePending:
        return L" Waiting for response from the local replica";
    case ReconfigurationProgressStages::Phase1_DataLoss:
        return L" Waiting for response from Failover Manager";
    case ReconfigurationProgressStages::Invalid:
    case ReconfigurationProgressStages::CurrentPhaseFinished:
    default:
        Common::Assert::CodingError("Invalid health report state during health report generation");
    }
}

void ReconfigurationStuckHealthReportDescriptor::TestAndWriteReplicaVector(const vector<HealthReportReplica> & v, Common::StringWriter & writer, wstring vectorName) const
{
    if (v.size() > 0)
    {
        writer.WriteLine();
        writer.WriteLine("{0}: ", vectorName);
        
        for (auto const & replica : v)
        {
            writer.WriteLine(replica.GetStringRepresentation());
        }
    }
}

bool ReconfigurationStuckHealthReportDescriptor::CompareHealthReportReplicaVectors(vector<HealthReportReplica> const & lhs, vector<HealthReportReplica> const & rhs)
{
    if (lhs.size() != rhs.size())
    {
        return false;
    }

    for (int  i = 0; i < lhs.size(); i++)
    {
        if (lhs[i] != rhs[i])
        {
            return false;
        }
    }

    return true;
}
