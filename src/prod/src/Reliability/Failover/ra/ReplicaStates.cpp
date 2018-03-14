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
        namespace ReplicaStates
        {

            void WriteToTextWriter(TextWriter & w, Enum const & val)
            {
                switch (val)
                {
                    case InCreate: 
                        w << L"IC"; return;
                    case InBuild: 
                        w << L"IB"; return;
                    case Ready: 
                        w << L"RD"; return;
                    case InDrop: 
                        w << L"ID"; return;
                    case Dropped: 
                        w << L"DD"; return;
                    case StandBy: 
                        w << L"SB"; return;
                    default: 
                        Assert::CodingError("Unknown Replica State");
                }
            }

            ENUM_STRUCTURED_TRACE(ReplicaStates, InCreate, LastValidEnum);

            Reliability::ReplicaStates::Enum ReplicaStates::ToReplicaDescriptionState(ReplicaStates::Enum state)
            {
                switch (state)
                {
                    case ReplicaStates::InDrop:
                    case ReplicaStates::Dropped:        
                        return Reliability::ReplicaStates::Dropped;
                    case ReplicaStates::StandBy:
                        return Reliability::ReplicaStates::StandBy;
                    case ReplicaStates::InCreate:
                    case ReplicaStates::InBuild:
                        return Reliability::ReplicaStates::InBuild;
                    default:
                        return Reliability::ReplicaStates::Ready;
                }
            }

            ReplicaStates::Enum ReplicaStates::FromReplicaDescriptionState(Reliability::ReplicaStates::Enum state)
            {
                switch (state)
                {
                    case Reliability::ReplicaStates::Dropped:
                        return ReplicaStates::Dropped;
                    case Reliability::ReplicaStates::StandBy:
                        return ReplicaStates::StandBy;
                    case Reliability::ReplicaStates::InBuild:
                        return ReplicaStates::InCreate;
                    default:
                        return ReplicaStates::Ready;
                }
            }

        }
    }
}
