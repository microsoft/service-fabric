// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace RepairPolicyEngine
{
    /* This is a static class that takes input data from the RepairManager about
       an application environment and computes the next steps of actions to be taken to
       schedule a repair or more a node through its state machine. */
    class RepairWorkflow
    {
    public:
        static void GenerateNodeRepairs(std::vector<std::shared_ptr<NodeRepairInfo>>& nodesRepairInfo,  
                                        RepairPolicyEnginePolicy& repairPolicyEnginePolicy, 
                                        Common::ComPointer<IFabricHealthClient> fabricHealthClient);

        static bool UpdateNodeStates(std::vector<std::shared_ptr<NodeRepairInfo>>& nodesRepairInfo, 
                                     RepairPolicyEngineConfiguration& repairConfiguration, 
                                     Common::ComPointer<IFabricHealthClient> fabricHealthClient, 
                                     Common::ComPointer<IFabricKeyValueStoreReplica2> keyValueStorePtr);

        static bool RecoverFromInvalidState(std::shared_ptr<NodeRepairInfo> nodeRepairInfo, 
                                            RepairPolicyEngineConfiguration& repairConfiguration, 
                                            Common::ComPointer<IFabricHealthClient> fabricHealthClient, 
                                            Common::ComPointer<IFabricKeyValueStoreReplica2>  keyValueStorePtr, bool& recovered);

    private:
        static Common::TimeSpan _requiredRepairHistory;
        static std::wstring GetRepairType(std::shared_ptr<RepairPromotionRule> repairPromotionRule);
    };
}
