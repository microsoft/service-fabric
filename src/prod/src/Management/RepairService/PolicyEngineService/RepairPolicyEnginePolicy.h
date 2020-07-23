// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace RepairPolicyEngine
{
    // This class is used to contain the NodeRepairActionPolicy
    // This class is mutable. It keeps the current repairs outstanding 
    // for a particular repair action. The NodeRepairActionPolicy class 
    // itself is not mutable. This helps when reloading configurations for a policy the 
    // configuration changes but the outstanding repairs do not. 
	class RepairPromotionRule
    {
    public:
        RepairPromotionRule(NodeRepairActionPolicy nodeRepairActionPolicy);
        
        UINT RepairsOutstanding;

        static bool OrderByIncreasingTime(const std::shared_ptr<RepairPromotionRule>& rule1, const std::shared_ptr<RepairPromotionRule>& rule2);
        
        const NodeRepairActionPolicy& get_NodeRepairActionPolicy()
        { 
            return nodeRepairActionPolicy_;
        }

    private:
        NodeRepairActionPolicy nodeRepairActionPolicy_;
    };

    class RepairPolicyEnginePolicy
    {
    public:
        RepairPolicyEnginePolicy(const RepairPolicyEngineConfiguration& repairPolicyEngineConfiguration);

        std::vector<std::shared_ptr<RepairPromotionRule>> RepairPromotionRuleSet;
    };
}
