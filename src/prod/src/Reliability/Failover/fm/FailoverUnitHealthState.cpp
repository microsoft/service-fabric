// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        namespace FailoverUnitHealthState
        {
            void WriteToTextWriter(Common::TextWriter & w, FailoverUnitHealthState::Enum const& val)
            {
                switch (val)
                {
                case FailoverUnitHealthState::Healthy:
                    w << "Healthy";
                    break;
                case FailoverUnitHealthState::Placement:
                    w << "Placement";
                    break;
                case FailoverUnitHealthState::PlacementStuck:
                    w << "PlacementStuck";
                    break;
                case FailoverUnitHealthState::PlacementStuckBelowMinReplicaCount:
                    w << "PlacementStuckBelowMinReplicaCount";
                    break;
                case FailoverUnitHealthState::Build:
                    w << "Build";
                    break;
                case FailoverUnitHealthState::BuildStuck:
                    w << "BuildStuck";
                    break;
                case FailoverUnitHealthState::BuildStuckBelowMinReplicaCount:
                    w << "BuildStuckBelowMinReplicaCount";
                    break;
                case FailoverUnitHealthState::Reconfiguration:
                    w << "Reconfiguration";
                    break;
                case FailoverUnitHealthState::ReconfigurationStuck:
                    w << "ReconfigurationStuck";
                    break;
                case FailoverUnitHealthState::QuorumLost:
                    w << "QuorumLost";
                    break;
                case FailoverUnitHealthState::DeletionInProgress:
                    w << "DeletionInProgress";
                    break;
                case FailoverUnitHealthState::PlacementStuckAtZeroReplicaCount:
                    w << "PlacementStuckAtZeroReplicaCount";
                    break;
                default:
                    w << "Invalid";
                    break;
                }
            }
        }
    }
}
