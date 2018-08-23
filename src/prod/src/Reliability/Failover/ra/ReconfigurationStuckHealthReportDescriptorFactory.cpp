// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;

using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Health;

ReconfigurationStuckHealthReportDescriptorFactory::ReconfigurationStuckHealthReportDescriptorFactory()
{
}

ReconfigurationStuckDescriptorSPtr ReconfigurationStuckHealthReportDescriptorFactory::CreateReconfigurationStuckHealthReportDescriptor(
    ReconfigurationProgressStages::Enum phaseStage,
    ReplicaStore::ConfigurationReplicaStore const & replicaStore,
    Common::StopwatchTime const & reconfigurationStartTime,
    Common::StopwatchTime const & reconfigurationPhaseStartTime)
{    
    vector<HealthReportReplica> replicas;

    //Populates the replica vector if appropriate for this reason for being stuck
    PopulateHealthReportReplicaVector(replicas, replicaStore, phaseStage);

    return make_shared<ReconfigurationStuckHealthReportDescriptor>(phaseStage, std::move(replicas), reconfigurationStartTime, reconfigurationPhaseStartTime);
}

ReconfigurationStuckDescriptorSPtr ReconfigurationStuckHealthReportDescriptorFactory::CreateReconfigurationStuckHealthReportDescriptor()
{
    return make_shared<ReconfigurationStuckHealthReportDescriptor>();
}

void ReconfigurationStuckHealthReportDescriptorFactory::PopulateHealthReportReplicaVector(vector<HealthReportReplica> & v, ReplicaStore::ConfigurationReplicaStore const & replicaStore, ReconfigurationProgressStages::Enum phaseStage)
{
    std::function<bool(const Replica &replica)> replicaFilter;
    switch (phaseStage)
    {
        case ReconfigurationProgressStages::Phase1_WaitingForReadQuorum:
        {
            replicaFilter = [&v](const Replica &replica)
            {
                if (replica.IsInConfiguration && !replica.IsDropped && !replica.IsLSNSet())
                {
                    return true;
                }
                return false;
            };
        } break;
        case ReconfigurationProgressStages::Phase3_PCBelowReadQuorum:
        {
            replicaFilter = [&v](const Replica &replica)
            {
                if (replica.IsInPreviousConfiguration && replica.MessageStage != ReplicaMessageStage::None && !replica.IsDropped)
                {
                    return true;
                }
                return false;
            };
        } break;
        case ReconfigurationProgressStages::Phase3_WaitingForReplicas:
        {
            replicaFilter = [&v](const Replica &replica)
            {
                if (replica.IsInPreviousConfiguration && replica.IsUp && !replica.IsBuildInProgress && !replica.ToBeRestarted)
                {
                    return true;
                }
                return false;
            };
        } break;
        case ReconfigurationProgressStages::Phase4_UpReadyReplicasPending:
        {
            replicaFilter = [&v](const Replica &replica)
            {
                if (!replica.ReplicatorRemovePending &&
                    !replica.ToBeRestarted &&
                    replica.IsInCurrentConfiguration &&
                    replica.IsUp &&
                    replica.IsReady &&
                    replica.MessageStage!=ReplicaMessageStage::None)
                {
                    return true;
                }
                return false;
            };
        } break;
        case ReconfigurationProgressStages::Phase4_ReplicaStuckIB:
        {
            replicaFilter = [&v](const Replica &replica)
            {
                if (!replica.ReplicatorRemovePending &&
                    !replica.ToBeRestarted &&
                    replica.IsInCurrentConfiguration &&
                    replica.IsUp &&
                    replica.IsInBuild &&
                    replica.MessageStage != ReplicaMessageStage::None)
                {
                    return true;
                }
                return false;
            };
        } break;
        case ReconfigurationProgressStages::Phase4_ReplicaPendingRestart:
        {
            replicaFilter = [&v](const Replica &replica)
            {
                if (replica.ToBeRestarted)
                {
                    return true;
                }
                return false;
            };
        } break;
        case ReconfigurationProgressStages::Phase4_UpReadyReplicasActivated:
        {
            replicaFilter = [&v](const Replica &replica)
            {
                if (replica.ReplicatorRemovePending ||
                    replica.IsInCurrentConfiguration &&
                    replica.IsUp &&
                    !replica.IsReady &&
                    replica.MessageStage != ReplicaMessageStage::None)
                {
                    return true;
                }
                return false;
            };
        } break;
        default:
            return;
    }

    for_each(replicaStore.begin(), replicaStore.end(), [&v,&replicaFilter](const Replica &replica)
    {
        if (replicaFilter(replica))
        {
            v.emplace_back(HealthReportReplica(replica));
        }
    });
}
