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
    switch (phaseStage) {
        case ReconfigurationProgressStages::Phase1_WaitingForReadQuorum:
        case ReconfigurationProgressStages::Phase3_PCBelowReadQuorum:
        case ReconfigurationProgressStages::Phase3_WaitingForReplicas:
        case ReconfigurationProgressStages::Phase4_UpReadyReplicasPending:
        case ReconfigurationProgressStages::Phase4_ReplicaStuckIB:
        case ReconfigurationProgressStages::Phase4_ReplicaPendingRestart:
        case ReconfigurationProgressStages::Phase4_UpReadyReplicasActivated:
            PopulateHealthReportReplicaVector(replicas, replicaStore, phaseStage);
    }
    
   return make_shared<ReconfigurationStuckHealthReportDescriptor>(phaseStage, std::move(replicas), reconfigurationStartTime, reconfigurationPhaseStartTime);
}

ReconfigurationStuckDescriptorSPtr ReconfigurationStuckHealthReportDescriptorFactory::CreateReconfigurationStuckHealthReportDescriptor()
{
    return make_shared<ReconfigurationStuckHealthReportDescriptor>();
}

void ReconfigurationStuckHealthReportDescriptorFactory::PopulateHealthReportReplicaVector(vector<HealthReportReplica> & v, ReplicaStore::ConfigurationReplicaStore const & replicaStore, ReconfigurationProgressStages::Enum phaseStage)
{
    for (auto const & replica : replicaStore)
    {
        if (replica.IsInConfiguration && !replica.IsDropped)
        {
            if (replica.IsLSNSet() && phaseStage == ReconfigurationProgressStages::Phase1_WaitingForReadQuorum)
            {
                continue;
            }

            if (phaseStage != ReconfigurationProgressStages::Phase4_ReplicaPendingRestart && phaseStage != ReconfigurationProgressStages::Phase4_ReplicaStuckIB)
            {
                v.push_back(move(HealthReportReplica(replica)));
            }            
            else if (phaseStage == ReconfigurationProgressStages::Phase4_ReplicaStuckIB)
            {
                if (replica.IsInBuild)
                {
                    v.push_back(move(HealthReportReplica(replica)));
                }
            }
            else if (phaseStage == ReconfigurationProgressStages::Phase4_ReplicaPendingRestart)
            {
                if (replica.ToBeRestarted)
                {
                    v.push_back(move(HealthReportReplica(replica)));
                }
            }
        }
    }
}
