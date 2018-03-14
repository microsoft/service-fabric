// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class RepairDescription
    {
    public:
        RepairDescription(
            ::FABRIC_REPLICA_ROLE replicaRole,
            RepairCondition::Enum repairCondition)
            : replicaRole_(replicaRole)
            , repairCondition_(repairCondition)
        {
        }

        __declspec(property(get=get_ReplicaRole)) ::FABRIC_REPLICA_ROLE const & ReplicaRole;
        __declspec(property(get=get_RepairCondition)) RepairCondition::Enum RepairCondition;

        ::FABRIC_REPLICA_ROLE get_ReplicaRole() const { return replicaRole_; }
        RepairCondition::Enum get_RepairCondition() const { return repairCondition_; }

    private:
        ::FABRIC_REPLICA_ROLE replicaRole_;
        RepairCondition::Enum repairCondition_;
    };
}
