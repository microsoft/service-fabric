// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace RepairPolicyEngine;
using namespace Common;

TimeSpan RepairWorkflow::_requiredRepairHistory = TimeSpan::FromSeconds(0);

void RepairWorkflow::GenerateNodeRepairs(std::vector<std::shared_ptr<NodeRepairInfo>>& nodesRepairInfo, 
                                         RepairPolicyEnginePolicy& repairPolicyEnginePolicy, 
                                         Common::ComPointer<IFabricHealthClient> fabricHealthClient)
{
    Trace.WriteInfo(ComponentName, "RepairWorkflow::GenerateNodeRepairs() Started ...");

    for (auto iterator = nodesRepairInfo.begin(); iterator != nodesRepairInfo.end(); iterator++)
    {
        std::shared_ptr<NodeRepairInfo> nodeRepairInfo = (*iterator);
        Trace.WriteNoise(ComponentName, "RepairWorkflow::GenerateNodeRepairs() Node {0}: NodePolicyState = {1}, RepairCreated = {2}", 
            nodeRepairInfo->NodeName, 
            EnumToString::NodePolicyStateToString(nodeRepairInfo->NodePolicyState),
            nodeRepairInfo->IsRepairCreated());

        // Initiate Repairs
        if (nodeRepairInfo->NodePolicyState == NodePolicyState::Failing && 
            !nodeRepairInfo->IsRepairCreated())
        {
            std::shared_ptr<RepairPromotionRule> repairPromotionRule;
            std::shared_ptr<RepairPromotionRule> fastTrackRepairPromotionRule;
            std::shared_ptr<RepairPromotionRule> standardRepairPromotionRule;

			// This list needs to be ordered from smaller to larger times.
            for (auto ruleSet = repairPolicyEnginePolicy.RepairPromotionRuleSet.begin(); ruleSet != repairPolicyEnginePolicy.RepairPromotionRuleSet.end(); ruleSet++)
            {
                // Keep going through the loop until the repair has the longest threshold repair found.
                if ((nodeRepairInfo->LastHealthyAt + (*ruleSet)->get_NodeRepairActionPolicy().get_PolicyActionTime()) < DateTime::Now())
                {
					if ( (*ruleSet)->get_NodeRepairActionPolicy().get_IsEnabled() )
					{
                        standardRepairPromotionRule =  *ruleSet;
					}
                }
            }
            if(standardRepairPromotionRule != nullptr)
            {
                Trace.WriteNoise(ComponentName, "RepairWorkflow::GenerateNodeRepairs() standard promotion rule: {0}",
                    standardRepairPromotionRule->get_NodeRepairActionPolicy().get_RepairName());
            }
            else
            {
                Trace.WriteNoise(ComponentName, "RepairWorkflow::GenerateNodeRepairs() No standard promotion rule is set");
            }

            // Fast track repair 
            if(nodeRepairInfo->IsFastTrackRepairProvided())
            {
                // sort the fast track info and get the last repair
                std::sort(nodeRepairInfo->FastTrackRepair.begin(), nodeRepairInfo->FastTrackRepair.end(), RepairPromotionRule::OrderByIncreasingTime);

                // Iterate until the fast track repair is found 
                // the repair may not be found if that repair action or a repair action below has been disabled.
                for (auto ruleSet = repairPolicyEnginePolicy.RepairPromotionRuleSet.begin(); ruleSet != repairPolicyEnginePolicy.RepairPromotionRuleSet.end(); ruleSet++)
                {
                    if ( (*ruleSet)->get_NodeRepairActionPolicy().get_IsEnabled() )
				    {
                        // Use the last rule
                        if(StringUtility::AreEqualCaseInsensitive((*nodeRepairInfo->FastTrackRepair.rbegin())->get_NodeRepairActionPolicy().get_RepairName(),
                                                                  (*ruleSet)->get_NodeRepairActionPolicy().get_RepairName()))  
                        {
                            fastTrackRepairPromotionRule = *ruleSet;
                        }
				    }
                }
            }
            if(fastTrackRepairPromotionRule != nullptr)
            {
                Trace.WriteNoise(ComponentName, "RepairWorkflow::GenerateNodeRepairs() fast track promotion rule: {0}",
                    fastTrackRepairPromotionRule->get_NodeRepairActionPolicy().get_RepairName());
            }
            else
            {
                Trace.WriteNoise(ComponentName, "RepairWorkflow::GenerateNodeRepairs() No fast track promotion rule is set");
            }

            if(fastTrackRepairPromotionRule != nullptr && standardRepairPromotionRule == nullptr)
            {
                repairPromotionRule = fastTrackRepairPromotionRule;
            }
            if(fastTrackRepairPromotionRule == nullptr && standardRepairPromotionRule != nullptr)
            {
                repairPromotionRule = standardRepairPromotionRule;
            }
            
            // The standard promotion rule or the fast track rule whichever is bigger becomes 
            // the repair to schedule
            if(fastTrackRepairPromotionRule != nullptr && standardRepairPromotionRule != nullptr)
            {
                // The bigger of the two becomes the repair to schedule
                if(fastTrackRepairPromotionRule->get_NodeRepairActionPolicy().get_PolicyActionTime() > standardRepairPromotionRule->get_NodeRepairActionPolicy().get_PolicyActionTime())
                {
                    repairPromotionRule = fastTrackRepairPromotionRule;
                }
                else
                {
                    repairPromotionRule = standardRepairPromotionRule;
                }
            }
            
            CODING_ERROR_ASSERT(repairPromotionRule != nullptr);
            Trace.WriteNoise(ComponentName, "RepairWorkflow::GenerateNodeRepairs() Assigned repair promotion rule: {0}",
                repairPromotionRule->get_NodeRepairActionPolicy().get_RepairName());

            nodeRepairInfo->TraceState();

            std::wstring repairTypeToSchedule = RepairWorkflow::GetRepairType(repairPromotionRule);

            nodeRepairInfo->NodeRepairToSchedule = repairPromotionRule;

            nodeRepairInfo->TraceState();
        }
    }

    Trace.WriteNoise(ComponentName, "RepairWorkflow::GenerateNodeRepairs() Finished ...");
}

std::wstring RepairWorkflow::GetRepairType(std::shared_ptr<RepairPromotionRule> repairPromotionRule)
{
    if(repairPromotionRule != nullptr)
    {
        return repairPromotionRule->get_NodeRepairActionPolicy().get_RepairName();
    }

    return RepairTypeNone;
}

bool RepairWorkflow::RecoverFromInvalidState(std::shared_ptr<NodeRepairInfo> nodeRepairInfo, 
                                             RepairPolicyEngineConfiguration& repairPolicyEngineConfiguration, 
                                             ComPointer<IFabricHealthClient> fabricHealthClient, 
                                             Common::ComPointer<IFabricKeyValueStoreReplica2> keyValueStorePtr, bool& recovered)
{
    bool wasRecovered = false;
    bool succeeded = true;

    // Remark: NodePolicyState::Failing && !nodeRepairInfo.HasPendingRepair() is not considered an invalid state as it can recover from it and is expected in case the node health state is Unknown or Ok but within MinimumHealthyDuration

    if (nodeRepairInfo->NodePolicyState != NodePolicyState::Failing &&
        nodeRepairInfo->NodePolicyState != NodePolicyState::Healthy &&
        nodeRepairInfo->NodePolicyState != NodePolicyState::Probation)
    {
        // Prevents node to stuck in unsupported healthstate. The unsupported can happen due to e.g. failed parsing from health store.
        Trace.WriteError(ComponentName, "Correcting node repair info by setting NodePolicyState back to Healthy. Error: unsupported value of NodePolicyState");
        nodeRepairInfo->TraceState();
        nodeRepairInfo->NodePolicyState = NodePolicyState::Healthy;
        wasRecovered =  true;
    }

    if (nodeRepairInfo->NodePolicyState == NodePolicyState::Probation && nodeRepairInfo->EligibleFromProbationToFailingAt == DateTime::MaxValue)
    {
        Trace.WriteError(ComponentName, "Correcting node repair info by setting NodePolicyState back to Healthy. Error: NodePolicyState is Probation but EligibleFromProbationToFailingAt is Infinite");
        nodeRepairInfo->TraceState();
        nodeRepairInfo->NodePolicyState = NodePolicyState::Healthy;
        nodeRepairInfo->EligibleFromProbationToHealthyAt = DateTime::MaxValue;
        wasRecovered =  true;
    }

    if (nodeRepairInfo->NodePolicyState == NodePolicyState::Probation && nodeRepairInfo->EligibleFromProbationToHealthyAt == DateTime::MaxValue)
    {
        Trace.WriteError(ComponentName, "Correcting node repair info by setting NodePolicyState back to Healthy. Error: NodePolicyState is Probation but EligibleFromProbationToHealthyAt is Infinite");
        nodeRepairInfo->TraceState();
        nodeRepairInfo->NodePolicyState = NodePolicyState::Healthy;
        nodeRepairInfo->EligibleFromProbationToFailingAt = DateTime::MaxValue;
        wasRecovered =  true;
    }

    if (wasRecovered)
    {
        nodeRepairInfo->TraceState();
        succeeded = SUCCEEDED(KeyValueStoreHelper::WriteToPersistentStore(*nodeRepairInfo, keyValueStorePtr, repairPolicyEngineConfiguration));
    }

    recovered = wasRecovered;
    return succeeded;
}

bool RepairWorkflow::UpdateNodeStates(std::vector<std::shared_ptr<NodeRepairInfo>>& nodesRepairInfo, 
                                      RepairPolicyEngineConfiguration& repairConfiguration, 
                                      Common::ComPointer<IFabricHealthClient> fabricHealthClient, 
                                      Common::ComPointer<IFabricKeyValueStoreReplica2> keyValueStorePtr)
{
    Trace.WriteInfo(ComponentName, "RepairWorkflow::UpdateNodeStates() Started ...");

    Common::DateTime currentTime = DateTime::Now();
    HRESULT result = S_OK; 
    for (auto iterator = nodesRepairInfo.begin(); iterator != nodesRepairInfo.end(); iterator++)
    {
        std::shared_ptr<NodeRepairInfo> nodeRepairInfo = (*iterator);
        // If we detect fasttrack repair and the node is currently healthy move to F immediately
        if (nodeRepairInfo->IsFastTrackRepairProvided()  && 
            nodeRepairInfo->HealthState == FABRIC_HEALTH_STATE::FABRIC_HEALTH_STATE_ERROR &&
            nodeRepairInfo->NodePolicyState == NodePolicyState::Healthy)
        {
            // H -> F for fasttrack
            nodeRepairInfo->TraceState();
            nodeRepairInfo->NodePolicyState = NodePolicyState::Failing;
            nodeRepairInfo->EligibleFromProbationToFailingAt = DateTime::MaxValue;
            nodeRepairInfo->EligibleFromProbationToHealthyAt = DateTime::MaxValue;
            result = KeyValueStoreHelper::WriteToPersistentStore(*nodeRepairInfo, keyValueStorePtr, repairConfiguration);
            nodeRepairInfo->TraceState();
        }
        else if (nodeRepairInfo->NodePolicyState == NodePolicyState::Healthy &&
            nodeRepairInfo->HealthState == FABRIC_HEALTH_STATE::FABRIC_HEALTH_STATE_ERROR)
        {
            // H -> P
            nodeRepairInfo->TraceState();
            nodeRepairInfo->NodePolicyState = NodePolicyState::Probation;

			nodeRepairInfo->EligibleFromProbationToFailingAt = currentTime + repairConfiguration.get_NodeRepairPolicyConfiguration().get_ProbationToFailingDelay();
			nodeRepairInfo->EligibleFromProbationToHealthyAt = currentTime + repairConfiguration.get_NodeRepairPolicyConfiguration().get_ProbationToHealthyDelay();
            result = KeyValueStoreHelper::WriteToPersistentStore(*nodeRepairInfo, keyValueStorePtr, repairConfiguration);
            nodeRepairInfo->TraceState();
        }
        else if (nodeRepairInfo->NodePolicyState == NodePolicyState::Probation && 
            nodeRepairInfo->HealthState != FABRIC_HEALTH_STATE::FABRIC_HEALTH_STATE_ERROR &&
            nodeRepairInfo->EligibleFromProbationToHealthyAt < currentTime &&
            nodeRepairInfo->LastErrorAt + repairConfiguration.get_NodeRepairPolicyConfiguration().get_MinimumHealthyDuration() < currentTime)
        {
            // P -> H
            nodeRepairInfo->TraceState();
            nodeRepairInfo->NodePolicyState = NodePolicyState::Healthy;

            nodeRepairInfo->EligibleFromProbationToFailingAt = DateTime::MaxValue;
            nodeRepairInfo->EligibleFromProbationToHealthyAt = DateTime::MaxValue;
            result = KeyValueStoreHelper::WriteToPersistentStore(*nodeRepairInfo, keyValueStorePtr, repairConfiguration);
            nodeRepairInfo->TraceState();
        }
        else if (nodeRepairInfo->NodePolicyState == NodePolicyState::Probation &&
            nodeRepairInfo->HealthState == FABRIC_HEALTH_STATE::FABRIC_HEALTH_STATE_ERROR &&
            nodeRepairInfo->EligibleFromProbationToFailingAt < currentTime) 
        {
            // P -> F 
            nodeRepairInfo->TraceState();
            nodeRepairInfo->NodePolicyState = NodePolicyState::Failing;

            nodeRepairInfo->EligibleFromProbationToFailingAt = DateTime::MaxValue;
            nodeRepairInfo->EligibleFromProbationToHealthyAt = DateTime::MaxValue;
            result = KeyValueStoreHelper::WriteToPersistentStore(*nodeRepairInfo, keyValueStorePtr, repairConfiguration);
            nodeRepairInfo->TraceState();
        }
        else if (nodeRepairInfo->NodePolicyState == NodePolicyState::Failing &&
            nodeRepairInfo->HealthState != FABRIC_HEALTH_STATE::FABRIC_HEALTH_STATE_ERROR &&
            nodeRepairInfo->LastErrorAt + repairConfiguration.get_NodeRepairPolicyConfiguration().get_MinimumHealthyDuration() < currentTime)
        {
            if (nodeRepairInfo->IsRepairNotStarted())
            {
                // F -> P - Due to error free and no pending repair
                nodeRepairInfo->TraceState();
                nodeRepairInfo->NodePolicyState = NodePolicyState::Probation;
                nodeRepairInfo->EligibleFromProbationToFailingAt = currentTime + repairConfiguration.get_NodeRepairPolicyConfiguration().get_ProbationToFailingDelay();
                nodeRepairInfo->EligibleFromProbationToHealthyAt = currentTime + repairConfiguration.get_NodeRepairPolicyConfiguration().get_ProbationToHealthyDelay();
                nodeRepairInfo->LastRepairTaskId.clear();
                nodeRepairInfo->FastTrackRepair.clear();
                result = KeyValueStoreHelper::WriteToPersistentStore(*nodeRepairInfo, keyValueStorePtr, repairConfiguration);
                nodeRepairInfo->TraceState();
            }
            else
            {
                // Keep the state in F and set the intent to cancel if there's a repair in progress.
                nodeRepairInfo->set_ShouldScheduleCancel(true);
            }
        }
    }
    Trace.WriteNoise(ComponentName, "RepairWorkflow::UpdateNodeStates() Finished ...");
    return SUCCEEDED(result);
}
