// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace RepairPolicyEngine;
using namespace Common;

namespace PE = RepairPolicyEngine;

RepairPromotionRule::RepairPromotionRule(NodeRepairActionPolicy nodeRepairActionPolicy) :
    nodeRepairActionPolicy_(nodeRepairActionPolicy),
    RepairsOutstanding(0)
{
}

bool RepairPromotionRule::OrderByIncreasingTime(const std::shared_ptr<RepairPromotionRule>& rule1, const std::shared_ptr<RepairPromotionRule>& rule2)
{
    return rule1->get_NodeRepairActionPolicy().get_PolicyActionTime() < rule2->get_NodeRepairActionPolicy().get_PolicyActionTime();
}

RepairPolicyEnginePolicy::RepairPolicyEnginePolicy(const RepairPolicyEngineConfiguration& repairPolicyEngineConfiguration)
{
    auto repairActionPolicies = repairPolicyEngineConfiguration.get_NodeRepairPolicyConfiguration().get_NodeRepairActionPolicies();

    for (auto nodeRepairActionPolicy = repairActionPolicies.begin(); nodeRepairActionPolicy != repairActionPolicies.end(); nodeRepairActionPolicy++)
    {
        RepairPromotionRuleSet.push_back(std::make_shared<RepairPromotionRule> (*nodeRepairActionPolicy));
    }
	std::sort(RepairPromotionRuleSet.begin(), RepairPromotionRuleSet.end(), RepairPromotionRule::OrderByIncreasingTime);
};
