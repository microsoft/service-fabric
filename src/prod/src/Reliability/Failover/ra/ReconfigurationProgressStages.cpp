// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace Common;
using namespace Reliability::ReconfigurationAgentComponent;

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace ReconfigurationProgressStages
        {
            void ReconfigurationProgressStages::WriteToTextWriter(TextWriter & w, Enum const & val)
            {
                switch (val)
                {
                case CurrentPhaseFinished:
                    w << "CurrentPhaseFinished"; return;
                case Phase1_DataLoss:
                    w << L"Phase1_DataLoss"; return;
                case Phase1_WaitingForReadQuorum:
                    w << L"Phase1_WaitingForReadQuorum"; return;
                case Phase2_NoReplyFromRap:
                    w << "Phase2_NoReplyFromRap"; return;
                case Phase3_PCBelowReadQuorum:
                    w << "Phase3_PCBelowReadQuorum"; return;
                case Phase3_WaitingForReplicas:
                    w << "Phase3_WaitingForReplicas"; return;
                case Phase4_UpReadyReplicasPending:
                    w << "Phase4_UpReadyReplicasPending"; return;
                case Phase4_LocalReplicaNotReplied:
                    w << "Phase4_LocalReplicaNotReplied"; return;
                case Phase4_ReplicaPendingRestart:
                    w << "Phase4_ReplicaPendingRestart"; return;
                case Phase4_ReplicaStuckIB:
                    w << "Phase4_ReplicaStuckIB"; return;
                case Phase4_ReplicatorConfigurationUpdatePending:
                    w << "Phase4_ReplicatorConfigurationUpdatePending"; return;
                case Phase4_UpReadyReplicasActivated:
                    w << "Phase4_UpReadyReplicasActivated"; return;
                case Phase0_NoReplyFromRAP:
                    w << "Phase0_NoReplyFromRAP"; return;
                case Invalid:
                    w << L"Invalid"; return;
                default:
                    Common::Assert::CodingError("Unknown Reconfiguration Progress Stage {0}", static_cast<int>(val));
                }
            }

            ENUM_STRUCTURED_TRACE(ReconfigurationProgressStages, Invalid, LastValidEnum);
        }
    }
}
