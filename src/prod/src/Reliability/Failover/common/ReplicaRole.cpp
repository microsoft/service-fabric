// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Common.Stdafx.h"

using namespace Common;

namespace Reliability
{
    namespace ReplicaRole
    {
        void WriteToTextWriter(TextWriter & w, Enum const & val)
        {
            switch (val)
            {
                case Unknown:
                    w << L"U"; return;
                case None: 
                    w << L"N"; return;
                case Idle: 
                    w << L"I"; return;
                case Secondary: 
                    w << L"S"; return;
                case Primary: 
                    w << L"P"; return;
                default: 
                    Common::Assert::CodingError("Unknown Replica Role");
            }
        }

        ENUM_STRUCTURED_TRACE(ReplicaRole, Unknown, LastValidEnum);

        ::FABRIC_REPLICA_ROLE Reliability::ReplicaRole::ConvertToPublicReplicaRole(ReplicaRole::Enum toConvert)
        {
            switch(toConvert)
            {
            case Unknown:
                return ::FABRIC_REPLICA_ROLE_UNKNOWN;
            case None:
                return ::FABRIC_REPLICA_ROLE_NONE;
            case Idle:
                return ::FABRIC_REPLICA_ROLE_IDLE_SECONDARY;
            case Secondary:
                return ::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY;
            case Primary:
                return ::FABRIC_REPLICA_ROLE_PRIMARY;
            default:
                Common::Assert::CodingError("Unknown Replica Role");
            }
        }

        Reliability::LoadBalancingComponent::ReplicaRole::Enum Reliability::ReplicaRole::ConvertToPLBReplicaRole(bool isStateful, ReplicaRole::Enum toConvert)
        {
            switch(toConvert)
            {
            case Primary:
                return Reliability::LoadBalancingComponent::ReplicaRole::Primary;

            case Secondary:
            case Idle:
                return Reliability::LoadBalancingComponent::ReplicaRole::Secondary;

            case Unknown:
            case None:
                if (isStateful)
                {
                    return Reliability::LoadBalancingComponent::ReplicaRole::None;
                }
                else
                {
                    return Reliability::LoadBalancingComponent::ReplicaRole::Secondary;
                }

            default:
                Common::Assert::CodingError("Unknown Replica Role");
            }
        }
    }
}
