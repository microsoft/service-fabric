// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Reliability/Failover/Failover.h"
#include "Reliability/Failover/FMStateMachineTask.h"
#include "Reliability/Failover/FMStateUpdateTask.h"
#include "Reliability/Failover/FMStatelessCheckTask.h"
#include "Reliability/Failover/FMPendingTask.h"
#include "Reliability/Failover/FMMovementTask.h"


namespace ModelChecker {
    // A wrapper class for NodeInfo. In addition to NodeInfo
    // it keeps track if a node has an instance on it.
    class NodeRecord 
    {
        public:
            NodeRecord(
                Reliability::FailoverManagerComponent::NodeInfo const& nodeInfo,
                bool hasAnInstance);

            __declspec (property(get=get_NodeInformation)) Reliability::FailoverManagerComponent::NodeInfo const& NodeInformation;
            Reliability::FailoverManagerComponent::NodeInfo const& get_NodeInformation() const { return nodeInfo_; }

            __declspec (property(get=get_HasAnInstance, put=set_HasAnInstance)) bool HasAnInstance;
            bool get_HasAnInstance() const { return hasAnInstance_; }
            void set_HasAnInstance(bool hasAnInstance) { hasAnInstance_ = hasAnInstance; }

        private:
            Reliability::FailoverManagerComponent::NodeInfo nodeInfo_;
            bool hasAnInstance_;
    };
}
