// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace ReconfigurationProgressStages
        {
            enum Enum
            {
                Invalid = 0,
                CurrentPhaseFinished = 1,
                Phase1_DataLoss = 2,
                Phase1_WaitingForReadQuorum = 3,
                Phase2_NoReplyFromRap = 4,
                Phase3_PCBelowReadQuorum = 5,
                Phase3_WaitingForReplicas = 6,
                Phase4_UpReadyReplicasPending = 7,
                Phase4_LocalReplicaNotReplied = 8,
                Phase4_ReplicatorConfigurationUpdatePending = 9,
                Phase4_ReplicaPendingRestart = 10,
                Phase4_ReplicaStuckIB = 11,
                Phase4_UpReadyReplicasActivated = 12,
                Phase0_NoReplyFromRAP = 13,
                LastValidEnum = Phase0_NoReplyFromRAP
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

            DECLARE_ENUM_STRUCTURED_TRACE(ReconfigurationProgressStages);
        }
        
    }
}
