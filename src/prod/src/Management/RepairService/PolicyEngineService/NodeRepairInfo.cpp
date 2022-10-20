// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace RepairPolicyEngine;
using namespace Common;

namespace PE = RepairPolicyEngine;

NodeRepair::NodeRepair(std::wstring repairType, DateTime scheduledAt) :
    RepairType(repairType),
    ScheduledAt(scheduledAt)
{
}

NodeRepair::NodeRepair() :
    RepairType(RepairTypeNone),
    ScheduledAt(DateTime::Zero)
{
}

NodeRepairInfo::NodeRepairInfo(std::wstring nodeName) :
    NodeName(nodeName),
    RepairHistory(),
    NodePolicyState(NodePolicyState::Healthy),
    EligibleFromProbationToHealthyAt(DateTime::MaxValue),
    EligibleFromProbationToFailingAt(DateTime::MaxValue),
    NodeRepairToSchedule(NULL),
    LastErrorAt(DateTime::Zero),
    LastHealthyAt(DateTime::get_Now()),
    isRecentlyAdded_(true),
    repairProgressState_(RepairProgressState::NotStarted),
    shouldScheduleCancel_(false),
    isRequestInProgress_(false),
    isCancelInProgress_(false)
{
}

std::shared_ptr<NodeRepairInfo> NodeRepairInfo::CreateNodeRepairInfo(std::wstring nodeName)
{
    return std::make_shared<NodeRepairInfo>(nodeName);
}

int NodeRepairInfo::GetCompletedRepairsCount(std::wstring repairType, TimeSpan maximumRepairWindow)
{
    int count = 0;
    for(auto i = RepairHistory.begin(); i != RepairHistory.end(); i++)
    {
        if (StringUtility::AreEqualCaseInsensitive(i->RepairType,repairType) && 
            (maximumRepairWindow.TotalMilliseconds() < 0 || i->ScheduledAt + maximumRepairWindow > DateTime::Now()))
        {
            count++;
        }
    }

    return count;
}

bool NodeRepairInfo::HasPendingRepair()
{
    return StringUtility::AreEqualCaseInsensitive(RepairTypeToSchedule(),RepairTypeNone) == false;
}

DateTime NodeRepairInfo::GetLastRepairAt()
{
    if (RepairHistory.size() > 0)
    {
        return RepairHistory.back().ScheduledAt;
    }

    return DateTime::Zero;
}

void NodeRepairInfo::TruncateRepairHistory(int maxCount)
{
    while (RepairHistory.size() > maxCount)
    {
        // Erase the oldest repairs from beginning, as the latest are at the end
        RepairHistory.erase(RepairHistory.begin());
    }
}

HRESULT NodeRepairInfo::GetNodeHealthResult(LPCWSTR nodeName, RepairPolicyEngineConfiguration repairConfiguration, ComPointer<IFabricHealthClient> fabricHealthClientCPtr, __out IFabricNodeHealthResult** nodeHealthResult)
{
    Trace.WriteNoise(ComponentName, "NodeRepairInfo::GetNodeHealthResult() - Started ..."); 

    ComPointer<GetNodeHealthAsyncOperationCallback> GetNodeHealthAsyncOperationCallback = make_com<PE::GetNodeHealthAsyncOperationCallback>();

    GetNodeHealthAsyncOperationCallback->Initialize(fabricHealthClientCPtr);

    ComPointer<IFabricAsyncOperationContext> context;
    HRESULT result = fabricHealthClientCPtr->BeginGetNodeHealth(
        nodeName,
        nullptr,
        static_cast<DWORD>(repairConfiguration.WindowsFabricQueryTimeoutInterval.TotalPositiveMilliseconds()),
        GetNodeHealthAsyncOperationCallback.GetRawPointer(),
        context.InitializationAddress());

    if (FAILED(result))
    {
        Trace.WriteError(ComponentName, "NodeRepairInfo::GetNodeHealthResult() - Finished ...(with error {0})", result); 
        return result;
    }

    result = GetNodeHealthAsyncOperationCallback->Wait();

    if (FAILED(result))
    {
        Trace.WriteError(ComponentName, "NodeRepairInfo::GetNodeHealthResult() - Finished ...(with error {0})", result); 
        return result;
    }

    *nodeHealthResult = GetNodeHealthAsyncOperationCallback->Result.DetachNoRelease();

    Trace.WriteNoise(ComponentName, "NodeRepairInfo::GetNodeHealthResult() - Finished ..."); 

    return S_OK;
}

HRESULT NodeRepairInfo::Refresh(const FABRIC_NODE_QUERY_RESULT_ITEM * const nodeToRefreshFrom, ComPointer<IFabricHealthClient> fabricHealthClient, RepairPolicyEngineConfiguration& repairConfiguration,
                               const RepairPolicyEnginePolicy& repairPolicyEnginePolicy)
{
    Trace.WriteNoise(ComponentName, "Refreshing state of node '{0}' from health store and fabricQuery. Started ...", NodeName);

    HRESULT success = S_OK;

    bool shouldTraceState = HealthState != nodeToRefreshFrom->AggregatedHealthState;

    HealthState = nodeToRefreshFrom->AggregatedHealthState;

    if ((HealthState == FABRIC_HEALTH_STATE::FABRIC_HEALTH_STATE_OK || 
        HealthState == FABRIC_HEALTH_STATE::FABRIC_HEALTH_STATE_WARNING) && NodePolicyState == NodePolicyState::Healthy)
    {
        LastHealthyAt = DateTime::Now();
    }
    else if (HealthState == FABRIC_HEALTH_STATE::FABRIC_HEALTH_STATE_ERROR)
    {
        LastErrorAt = DateTime::Now();
    }
    
    // If the state is in Error see if there is any repairs requested in the errors.
    if (HealthState == FABRIC_HEALTH_STATE::FABRIC_HEALTH_STATE_ERROR)
    {
        ComPointer<IFabricNodeHealthResult> nodeHealthResult;

        HRESULT hr = GetNodeHealthResult(nodeToRefreshFrom->NodeName, repairConfiguration, fabricHealthClient, nodeHealthResult.InitializationAddress());

        bool continueWithHealthQuery = true;
        const FABRIC_NODE_HEALTH * fabricNodeHealth = nullptr;

        if (FAILED(hr))
        {
            Trace.WriteError(
                ComponentName,
                "GetNodeHealthResult() for node {0} failed with error code:{1}. Cannot refresh/restore the NodeRepairInfo from health properties. Default values will be used.",
                nodeToRefreshFrom->NodeName,
                hr);
            success = hr;
            continueWithHealthQuery = false;
        }
        else if (nodeHealthResult.GetRawPointer() == nullptr)
        {
            Trace.WriteError(
                ComponentName,
                "GetNodeHealthResult() for node {0} returned null pointer to nodeHealthResult. Cannot refresh/restore the NodeRepairInfo from health properties. Default values will be used.",
                nodeToRefreshFrom->NodeName);
            success = E_POINTER;
            continueWithHealthQuery = false;
        }
        else 
        {
            fabricNodeHealth = nodeHealthResult->get_NodeHealth();
            if (fabricNodeHealth == nullptr || fabricNodeHealth->HealthEvents == nullptr) 
            {
                Trace.WriteError(
                    ComponentName,
                    "IFabricNodeHealthResult.get_NodeHealth() for node {0} returned null pointer or its HealthEvents field is null. Cannot refresh/restore the NodeRepairInfo from health properties. Default values will be used.",
                    nodeToRefreshFrom->NodeName);
                success = E_POINTER;
                continueWithHealthQuery = false;
            }
        }

        // Remark: nodeHealthResult can be nullptr if the node entity doesn't exist in HealthStore due to any reason (e.g. node in invalid state)
        for (ULONG i = 0; continueWithHealthQuery && i < fabricNodeHealth->HealthEvents->Count; i++)
        {
            const FABRIC_HEALTH_EVENT nodeHealthEvent = fabricNodeHealth->HealthEvents->Items[i];

            if (nodeHealthEvent.HealthInformation == nullptr || nodeHealthEvent.HealthInformation->Property == nullptr || nodeHealthEvent.HealthInformation->Description == nullptr)
            {
                Trace.WriteWarning(
                    ComponentName, 
                    "FABRIC_HEALTH_EVENT.HealthInformation or HealthInformation->Property or HealthInformation->Description is null. \
                    Refreshing of the NodeRepairInfo from health store for the node '{0}' can be incomplete and default value can be used.",
                    nodeToRefreshFrom->NodeName);
                success = FAILED(success) ? success : S_FALSE; // preserve the more serious error
                continue;
            }

            // Figure out if any fasttrack repairs are in the health property. 
            // The property should be RequestRepair and the repair should be one of the accepted
            // repairs.
            const std::wstring requestRepair = L"RequestRepair";
            if(nodeHealthEvent.HealthInformation->State ==  FABRIC_HEALTH_STATE::FABRIC_HEALTH_STATE_ERROR)
            {
                if(StringUtility::AreEqualCaseInsensitive(nodeHealthEvent.HealthInformation->Property , requestRepair))
                {
                    for (auto ruleSet = repairPolicyEnginePolicy.RepairPromotionRuleSet.begin(); ruleSet != repairPolicyEnginePolicy.RepairPromotionRuleSet.end(); ruleSet++)
                    {
                        if(StringUtility::AreEqualCaseInsensitive(nodeHealthEvent.HealthInformation->Description , (*ruleSet)->get_NodeRepairActionPolicy().get_RepairName()))
                        {
                            ScheduleFastTrackRepair(*ruleSet);
                            Trace.WriteInfo(
                                ComponentName, 
                                "Valid repairs found for RequestRepair request. \
                                Node '{0}' Source '{1}' Description '{2}' .",
                                nodeToRefreshFrom->NodeName, nodeHealthEvent.HealthInformation->SourceId, nodeHealthEvent.HealthInformation->Description);
                        }
                    }
                    // If a request is requested it has to be one of the repairs specified in the 
                    // configurations. 
                    if (!IsFastTrackRepairProvided())
                    {
                        Trace.WriteWarning(
                            ComponentName, 
                            "No valid repairs found for RequestRepair request. \
                            Node '{0}' Source '{1}' Description '{2}' .",
                            nodeToRefreshFrom->NodeName, nodeHealthEvent.HealthInformation->SourceId, nodeHealthEvent.HealthInformation->Description);
                    }
                }
            }
        }
    }

    if (isRecentlyAdded_ || FAILED(success) || success == S_FALSE || shouldTraceState)
    {
        TraceState();
        isRecentlyAdded_ = false;
    }

    Common::LogLevel::Enum traceLevel = FAILED(success) ? Common::LogLevel::Enum::Warning : Common::LogLevel::Enum::Noise;
    Trace.WriteTrace(traceLevel, ComponentName, "Refreshing state of node '{0}' from health store and fabricQuery. Finished ... Success=={1}", NodeName, success);

    return success;
}

void NodeRepairInfo::TraceState()
{
    std::wstring repairHistoryDescription;
    GetRepairHistoryDescription(repairHistoryDescription);

    Trace.WriteInfo(
        ComponentName,
        "NodeRepairInfo:NodeName:{0};NodePolicyState:{1};EligibleFromProbationToHealthyAt:{2};EligibleFromProbationToFailingAt:{3};LastErrorAt:{4};LastHealthyAt:{5};RepairTypeToSchedule:{6};HealthState:{7};RepairHistory:{8}",
        NodeName,
        EnumToString::NodePolicyStateToString(NodePolicyState),
        EligibleFromProbationToHealthyAt.ToString(),
        EligibleFromProbationToFailingAt.ToString(),
        LastErrorAt.ToString(),
        LastHealthyAt.ToString(),
        RepairTypeToSchedule(),
        HealthState,
        repairHistoryDescription);

    Trace.WriteInfo(
        ComponentName,
        "NodeRepairInfo:NodeName:{0};RepairProgressState_:{1};FastTrackRepairProvided:{2};ShouldScheduleCancel:{3};IsRequestInProgress:{4};IsCancelInProgress:{5}",
        NodeName,
        EnumToString::RepairProgressStateToString(repairProgressState_),
        IsFastTrackRepairProvided(),
        get_shouldScheduleCancel(),
        get_IsRequestInProgress(),
        get_IsCancelInProgress()
        );
}

void NodeRepairInfo::GetRepairHistoryDescription(std::wstring& repairHistoryDescription)
{
    for (auto r = RepairHistory.begin(); r != RepairHistory.end(); r++)
    {
        repairHistoryDescription += 
            r->RepairType + 
            L"=" +
            r->ScheduledAt.ToString() +
            L";";
    }
}

bool NodeRepairInfo::PublishToHealthStore(ComPointer<IFabricHealthClient> fabricHealthClient, const RepairPolicyEngineConfiguration& repairConfiguration)
{
    bool success = true;

    TruncateRepairHistory(repairConfiguration.MaxNumberOfRepairsToRemember);
    std::wstring repairHistoryDescription;
    GetRepairHistoryDescription(repairHistoryDescription);

    success &= ReportHealth(L"RepairPolicyEngineService::RepairHistory", repairHistoryDescription, FABRIC_HEALTH_STATE::FABRIC_HEALTH_STATE_OK, fabricHealthClient);
    success &= ReportHealth(L"RepairPolicyEngineService::LastErrorAt", LastErrorAt.ToString(), FABRIC_HEALTH_STATE::FABRIC_HEALTH_STATE_OK, fabricHealthClient);
    success &= ReportHealth(L"RepairPolicyEngineService::LastHealthyAt", LastHealthyAt.ToString(), FABRIC_HEALTH_STATE::FABRIC_HEALTH_STATE_OK, fabricHealthClient);

    PEEvents::Trace->NodeLastHealthyAt(NodeName, LastHealthyAt);

    return success;
}

bool NodeRepairInfo::ReportHealth(std::wstring propertyName, std::wstring description, FABRIC_HEALTH_STATE healthState, ComPointer<IFabricHealthClient> fabricHealthClient)
{
    FABRIC_HEALTH_INFORMATION healthInformation = FABRIC_HEALTH_INFORMATION();
    healthInformation.Property = propertyName.c_str();
    healthInformation.Description = description.c_str();
    healthInformation.SourceId = L"RepairPolicyEngineService";
    healthInformation.State = healthState;
    healthInformation.TimeToLiveSeconds = FABRIC_HEALTH_REPORT_INFINITE_TTL;
    healthInformation.SequenceNumber = FABRIC_AUTO_SEQUENCE_NUMBER;

    FABRIC_NODE_HEALTH_REPORT nodeHealthReport = FABRIC_NODE_HEALTH_REPORT();
    nodeHealthReport.NodeName = NodeName.c_str();
    nodeHealthReport.HealthInformation = (const FABRIC_HEALTH_INFORMATION*) &healthInformation;

    FABRIC_HEALTH_REPORT healthReport = FABRIC_HEALTH_REPORT();
    healthReport.Kind = FABRIC_HEALTH_REPORT_KIND::FABRIC_HEALTH_REPORT_KIND_NODE;
    healthReport.Value = &nodeHealthReport;

    HRESULT hr = fabricHealthClient->ReportHealth(&healthReport);

    LogLevel::Enum logLevel = LogLevel::Enum::Noise;

    if (FAILED(hr))
    {
        logLevel = LogLevel::Enum::Error;
    }

    Trace.WriteTrace(logLevel, ComponentName, "Reporting health state of node '{0}': Property:'{1}' Description:'{2}' ReportHealth() error code:{3} ", NodeName, propertyName, description, hr);

    return SUCCEEDED(hr);
}

std::wstring NodeRepairInfo::RepairTypeToSchedule()
{
    if (NodeRepairToSchedule != NULL)
    {
        return NodeRepairToSchedule->get_NodeRepairActionPolicy().get_RepairName();
    }
    return RepairTypeNone;
}

HRESULT NodeRepairInfo::ReportState(RepairStateTransition currentState, 
                                 const RepairPolicyEngineConfiguration& repairConfiguration, 
                                 Common::ComPointer<IFabricKeyValueStoreReplica2> keyValueStorePtr)
{
    Trace.WriteInfo(ComponentName, "NodeRepairInfo::ReportState() Started with transition {0}", EnumToString::RepairStateTransitionToString(currentState));

    DateTime currentTime = DateTime::Now();
    TraceState();

    switch (currentState)
    {
        case RepairStateTransition::NotStartedToCreatingRepair:
            // NotStarted -> CreatingRepair
            CODING_ERROR_ASSERT(repairProgressState_ == RepairProgressState::NotStarted);
            CODING_ERROR_ASSERT(!LastRepairTaskId.empty());
            repairProgressState_ = RepairProgressState::CreatingRepair;
            break;
        case RepairStateTransition::CreatingRepairToCreatedRepair:
            // CreatingRepair -> CreatedRepair
            CODING_ERROR_ASSERT(repairProgressState_ == RepairProgressState::CreatingRepair);
            CODING_ERROR_ASSERT(!LastRepairTaskId.empty());
            if (NodeRepairToSchedule != NULL) NodeRepairToSchedule->RepairsOutstanding++;
            repairProgressState_ = RepairProgressState::CreatedRepair;
            break;
        case RepairStateTransition::RepairCompleted:
            // CreatedRepair -> NotStarted
            CODING_ERROR_ASSERT(repairProgressState_ == RepairProgressState::CreatedRepair);
            CODING_ERROR_ASSERT(!LastRepairTaskId.empty());
            // F -> P - A repair action has finished. Probation state prevents further repairs.
            NodePolicyState = NodePolicyState::Probation;
            if (NodeRepairToSchedule != NULL)
            {
                EligibleFromProbationToFailingAt = currentTime + NodeRepairToSchedule->get_NodeRepairActionPolicy().get_ProbationToFailingAfterRepairDelay();
                EligibleFromProbationToHealthyAt = currentTime + NodeRepairToSchedule->get_NodeRepairActionPolicy().get_ProbationToHealthyAfterRepairDelay();
                RepairHistory.push_back(NodeRepair(NodeRepairToSchedule->get_NodeRepairActionPolicy().get_RepairName(), currentTime));
                NodeRepairToSchedule->RepairsOutstanding--;
                NodeRepairToSchedule = NULL;
            }
            else
            {
                Trace.WriteWarning(ComponentName, "NodeRepairInfo::ReportState() - NodeRepairToSchedule is NULL when repair is completed. Using default values");
                // Use default delays and name if the repair type is not valid
                EligibleFromProbationToFailingAt = currentTime + repairConfiguration.get_NodeRepairPolicyConfiguration().get_ProbationToFailingDelay();
                EligibleFromProbationToHealthyAt = currentTime + repairConfiguration.get_NodeRepairPolicyConfiguration().get_ProbationToHealthyDelay();
                RepairHistory.push_back(NodeRepair(RepairTypeDefault, currentTime));
            }
            FastTrackRepair.clear();
            LastRepairTaskId.clear();
            repairProgressState_ = RepairProgressState::NotStarted;
            break;
        case RepairStateTransition::RepairCancelled:
            // CreatedRepair -> NotStarted
            CODING_ERROR_ASSERT(repairProgressState_ == RepairProgressState::CreatedRepair);
            CODING_ERROR_ASSERT(!LastRepairTaskId.empty());
            // Put in probation but there is no repair happened thusly use normal probation periods.
            NodePolicyState = NodePolicyState::Probation;
            EligibleFromProbationToFailingAt = currentTime + repairConfiguration.get_NodeRepairPolicyConfiguration().get_ProbationToFailingDelay();
            EligibleFromProbationToHealthyAt = currentTime + repairConfiguration.get_NodeRepairPolicyConfiguration().get_ProbationToHealthyDelay();
            if (NodeRepairToSchedule != NULL)
            {
                NodeRepairToSchedule->RepairsOutstanding--;
                NodeRepairToSchedule = NULL;
            }
            else
            {
                Trace.WriteWarning(ComponentName, "NodeRepairInfo::ReportState() - NodeRepairToSchedule is NULL when repair is cancelled.");
            }
            FastTrackRepair.clear();
            LastRepairTaskId.clear();
            repairProgressState_ = RepairProgressState::NotStarted;
            break;
        default:
            Common::Assert::CodingError("RepairProgressState=={0} not implemented in ReportState function", (LONG)currentState);
    }

    TraceState();
    return KeyValueStoreHelper::WriteToPersistentStore(*this, keyValueStorePtr, repairConfiguration);
}

void NodeRepairInfo::ScheduleFastTrackRepair(std::shared_ptr<RepairPromotionRule> rule)
{
    // If the rule is already there do not push it
    auto found = std::find_if(FastTrackRepair.begin(), FastTrackRepair.end(), 
        [rule](std::shared_ptr<RepairPromotionRule> p) 
                { return StringUtility::AreEqualCaseInsensitive
                (p->get_NodeRepairActionPolicy().get_RepairName(), rule->get_NodeRepairActionPolicy().get_RepairName()); });
    if(FastTrackRepair.end() == found)
    {
        FastTrackRepair.push_back(rule);
    }
}

void NodeRepairInfo::RecoverNodeRepairToSchedule(FABRIC_REPAIR_TASK& repairTask, const RepairPolicyEnginePolicy& repairPolicyEnginePolicy)
{
    if (NodeRepairToSchedule != NULL)
    {
        // return as the NodeRepairToSchedule is valid
        return;
    }

    for (auto ruleSet = repairPolicyEnginePolicy.RepairPromotionRuleSet.begin(); ruleSet != repairPolicyEnginePolicy.RepairPromotionRuleSet.end(); ruleSet++)
    {
        if(StringUtility::AreEqualCaseInsensitive((*ruleSet)->get_NodeRepairActionPolicy().get_RepairName(), repairTask.Action))
        {
            NodeRepairToSchedule = *ruleSet;
            NodeRepairToSchedule->RepairsOutstanding++;
            Trace.WriteInfo(ComponentName, "NodeRepairInfo::RecoverNodeRepairToSchedule() - Recover NodeRepairInfoToSchedule for node {0}, repair action = {1}", 
                NodeName, repairTask.Action);
            return;
        }
    }
    Trace.WriteWarning(ComponentName, "NodeRepairInfo::RecoverNodeRepairToSchedule() - Unable to recover NodeRepairToSchedule, default values will be used");
}
