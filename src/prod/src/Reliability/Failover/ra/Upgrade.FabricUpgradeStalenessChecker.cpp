// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Infrastructure;
using namespace Upgrade;

FabricUpgradeStalenessCheckResult::Enum FabricUpgradeStalenessChecker::CheckFabricUpgradeAtNodeUp(
    Common::FabricVersionInstance const & nodeVersion,
    Common::FabricVersionInstance const & incomingVersion)
{
    if (incomingVersion == FabricVersionInstance::Default || incomingVersion == FabricVersionInstance())
    {
        // this is from a v1 FM so no upgrade considerations
        return FabricUpgradeStalenessCheckResult::UpgradeNotRequired;
    }

    bool incomingHasInstanceId = incomingVersion.InstanceId != 0;
    bool nodeHasInstanceId = nodeVersion.InstanceId != 0;

    if (!nodeHasInstanceId && !incomingHasInstanceId)
    {
        // Ensure that the node version is an exact match before allowing the node to join the ring
        if (nodeVersion.Version == incomingVersion.Version)
        {
            return FabricUpgradeStalenessCheckResult::UpgradeNotRequired;
        }
        else
        {
            return FabricUpgradeStalenessCheckResult::UpgradeRequired;
        }
    }
    else if (!nodeHasInstanceId && incomingHasInstanceId)
    {
        // Perform an upgrade to the desired version
        return FabricUpgradeStalenessCheckResult::UpgradeRequired;
    }
    else if (nodeHasInstanceId && !incomingHasInstanceId)
    {
        // It is not possible for the node to have an instance id and the FM to not know about the instance
        // The FM must have persisted the instance before it started to get propagated to the nodes
        // Do not allow this node to join the ring
        return FabricUpgradeStalenessCheckResult::Assert;
    }
    else if (nodeHasInstanceId && incomingHasInstanceId)
    {
        if (incomingVersion.InstanceId > nodeVersion.InstanceId)
        {
            // Upgrade to the desired version
            return FabricUpgradeStalenessCheckResult::UpgradeRequired;
        }
        else if (incomingVersion.InstanceId == nodeVersion.InstanceId)
        {
            return FabricUpgradeStalenessCheckResult::UpgradeNotRequired;
        }
        else
        {
            // It is not possible for the FM to send a lower instance id than the node
            return FabricUpgradeStalenessCheckResult::Assert;
        }
    }

    Assert::CodingError("Cannot come here");
}

FabricUpgradeStalenessCheckResult::Enum FabricUpgradeStalenessChecker::CheckFabricUpgradeAtUpgradeMessage(
    Common::FabricVersionInstance const & nodeVersion,
    Common::FabricVersionInstance const & incomingVersion)
{
    if (incomingVersion == FabricVersionInstance::Default || incomingVersion == FabricVersionInstance())
    {
        // this is from a v1 FM so which cannot send upgrade message
        return FabricUpgradeStalenessCheckResult::Assert;
    }

    bool incomingHasInstanceId = incomingVersion.InstanceId != 0;
    bool nodeHasInstanceId = nodeVersion.InstanceId != 0;

    if (!nodeHasInstanceId && !incomingHasInstanceId)
    {
        // Ensure that the node version is an exact match before allowing the node to join the ring
        if (nodeVersion.Version == incomingVersion.Version)
        {
            return FabricUpgradeStalenessCheckResult::UpgradeNotRequired;
        }
        else
        {
            return FabricUpgradeStalenessCheckResult::UpgradeRequired;
        }
    }
    else if (!nodeHasInstanceId && incomingHasInstanceId)
    {
        // Perform an upgrade to the desired version
        return FabricUpgradeStalenessCheckResult::UpgradeRequired;
    }
    else if (nodeHasInstanceId && !incomingHasInstanceId)
    {
        // FM cannot send a message without an instance if the node has instance
        // The FM must have performed an upgrade to set the instance on the node
        return FabricUpgradeStalenessCheckResult::Assert;
    }
    else if (nodeHasInstanceId && incomingHasInstanceId)
    {
        if (incomingVersion.InstanceId > nodeVersion.InstanceId)
        {
            // Upgrade to the desired version
            return FabricUpgradeStalenessCheckResult::UpgradeRequired;
        }
        else 
        {
            return FabricUpgradeStalenessCheckResult::UpgradeNotRequired;
        }
    }

    Assert::CodingError("Cannot come here");
}
