// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class ReconfigurationStuckHealthReportDescriptorFactory
        {
        public:

            ReconfigurationStuckHealthReportDescriptorFactory();

            //Creates a IHealthDescriptorSPtr based on the current reconfiguration stage and replica set
            ReconfigurationStuckDescriptorSPtr CreateReconfigurationStuckHealthReportDescriptor(
                ReconfigurationProgressStages::Enum,
                ReplicaStore::ConfigurationReplicaStore const &,
                Common::StopwatchTime const & reconfigurationStartTime,
                Common::StopwatchTime const & reconfigurationPhaseStartTime
            );

            //Creates a Descriptor to clear any existing health reports
            ReconfigurationStuckDescriptorSPtr CreateReconfigurationStuckHealthReportDescriptor();

        private:

            void PopulateHealthReportReplicaVector(vector<HealthReportReplica> & v, ReplicaStore::ConfigurationReplicaStore const & replicaStore, ReconfigurationProgressStages::Enum phaseStage);
        };
    }
}
