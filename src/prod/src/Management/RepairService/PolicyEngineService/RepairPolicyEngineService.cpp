// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <algorithm>

using namespace RepairPolicyEngine;
using namespace Common;

namespace PE = RepairPolicyEngine;

WCHAR RepairPolicyEngineService::RepairPolicyEngineName[] = L"RepairPolicyEngine/";

RepairPolicyEngineService::RepairPolicyEngineService() :
    _nodesRepairInfo(),
    _lastExecutedRepairAt(DateTime::Zero),
    _runCompleted(true), 
    _runShouldExit(false),
    _runStarted(false),
    _healthStateToReport(FABRIC_HEALTH_STATE::FABRIC_HEALTH_STATE_OK),
    _healthMessageWriter(_healthMessageOnProblem),
    _lock(),
    IsPrimaryReplica(false)
{
    Trace.WriteInfo(ComponentName, "RepairPolicyEngineService::RepairPolicyEngineService()"); 
}

HRESULT RepairPolicyEngineService::Initialize(Common::ComPointer<IFabricKeyValueStoreReplica2> keyValueStorePtr, IFabricCodePackageActivationContext *activationContext, Common::ManualResetEvent* exitServiceHostEventPtr, FABRIC_PARTITION_ID partitionId, FABRIC_REPLICA_ID replicaId)
{
    Trace.WriteInfo(ComponentName, "RepairPolicyEngineService::Initialize()"); 

    _healthStateToReport = FABRIC_HEALTH_STATE_OK;
    _healthMessageOnProblem.assign(L"Initialized.");

    HRESULT resultQueryClient = ::FabricCreateLocalClient(IID_IFabricQueryClient, _fabricQueryClientCPtr.VoidInitializationAddress());
    HRESULT resultHealthClient = ::FabricCreateLocalClient(IID_IFabricHealthClient, _fabricHealthClientCPtr.VoidInitializationAddress());
    HRESULT resultRepairClient = ::FabricCreateLocalClient(IID_IFabricRepairManagementClient, _fabricRepairManagementClientCPtr.VoidInitializationAddress());

    if (FAILED(resultQueryClient))
    {
        Trace.WriteError(ComponentName, "RepairPolicyEngineService::Initialize() Query - ::FabricCreateLocalClient failed with error '{0}'", resultQueryClient); 
        return resultQueryClient;
    }

    if (FAILED(resultHealthClient))
    {
        Trace.WriteError(ComponentName, "RepairPolicyEngineService::Initialize() Health - ::FabricCreateLocalClient failed with error '{0}'", resultHealthClient); 
        return resultHealthClient;
    }

	if (FAILED(resultRepairClient))
    {
        Trace.WriteError(ComponentName, "RepairPolicyEngineService::Initialize() Repair - ::FabricCreateLocalClient failed with error '{0}'", resultRepairClient); 
        return resultHealthClient;
    }

    if( _fabricQueryClientCPtr.GetRawPointer() == nullptr)
    {
        Trace.WriteError(ComponentName, "RepairPolicyEngineService::Initialize() Pointer to IFabricQueryClient is null."); 
        return E_POINTER;
    }

    if (_fabricHealthClientCPtr.GetRawPointer() == nullptr)
    {
        Trace.WriteError(ComponentName, "RepairPolicyEngineService::Initialize() Pointer to IFabricHealthClient is null."); 
        return E_POINTER;
    }

    if (_fabricRepairManagementClientCPtr.GetRawPointer() == nullptr)
    {
        Trace.WriteError(ComponentName, "RepairPolicyEngineService::Initialize() Pointer to IFabricRepairManagementClient is null."); 
        return E_POINTER;
    }

    _activationContextCPtr.SetAndAddRef(activationContext);
    _exitServiceHostEventPtr = exitServiceHostEventPtr;
    _replicaId = replicaId;
    _partitionId = partitionId;
    keyValueStorePtr_ = keyValueStorePtr;

    return S_OK;
}

void RepairPolicyEngineService::Stop()
{
    Trace.WriteInfo(ComponentName, "RepairPolicyEngineService::Stop() - Started ..."); 
    AcquireExclusiveLock lock(_lock);

    if (!_runCompleted.IsSet())
    {
        Trace.WriteInfo(ComponentName, "RepairPolicyEngineService::Stop() - Waiting for Run() to exit ..."); 
        _runShouldExit.Set();
        _runCompleted.WaitOne(INFINITE);
    }
    else
    {
        Trace.WriteInfo(ComponentName, "RepairPolicyEngineService::Stop() - Run() was not in execution ..."); 
    }

    Trace.WriteInfo(ComponentName, "RepairPolicyEngineService::Stop() - Finished ..."); 
}

void RepairPolicyEngineService::RunAsync()
{
    Trace.WriteInfo(ComponentName, "RepairPolicyEngineService::RunAsync() - Started ..."); 

    AcquireExclusiveLock lock(_lock);

    if (!_runCompleted.IsSet())
    {
        Trace.WriteWarning(ComponentName, "RepairPolicyEngineService::RunAsync() - Finishing ... Run() is already in execution ..."); 
        return;
    }

    Trace.WriteInfo(ComponentName, "RepairPolicyEngineService::RunAsync() - Starting Run() in background thread ..."); 

    _runCompleted.Reset();
    _runShouldExit.Reset();

    _runStarted.Reset();

    Threadpool::Post([this]()
    {
        this->Run();
    });

    _runStarted.WaitOne(INFINITE);

    Trace.WriteInfo(ComponentName, "RepairPolicyEngineService::RunAsync() - Finished ..."); 
}

void RepairPolicyEngineService::Run()
{   
    _runStarted.Set();
    RepairPolicyEngineConfiguration initialRepairPolicyEngineConfiguration = RepairPolicyEngineConfiguration();

    try
    {
        Trace.WriteInfo(ComponentName, "RepairPolicyEngineService::Run() Started ... IsPrimaryReplica=={0}", IsPrimaryReplica);

        _nodesRepairInfo.clear();
        _healthStateToReport = FABRIC_HEALTH_STATE_OK;
        _healthMessageOnProblem.assign(IsPrimaryReplica ? L"Is running as primary replica. " : L"Is running as secondary replica.");

        if (!initialRepairPolicyEngineConfiguration.Load(_activationContextCPtr).IsSuccess())
        {
            SetHealthState(FABRIC_HEALTH_STATE::FABRIC_HEALTH_STATE_ERROR);
            _healthMessageWriter.Write("Error in configuration.");
        }

        if (!IsDebuggerPresent())
        {
            int sleepToSyncInMilliseconds = static_cast<int>(initialRepairPolicyEngineConfiguration.WaitForHealthStatesAfterBecomingPrimaryInterval.TotalPositiveMilliseconds());
            Trace.WriteInfo(ComponentName, "RepairPolicyEngineService::Run() - Initial sleeping for '{0}' milliseconds to wait for latest health events from previous primary replica to appear in HealthStore", sleepToSyncInMilliseconds);
            _runShouldExit.WaitOne(sleepToSyncInMilliseconds);
        }

        bool firstRun = IsPrimaryReplica;

        while (!_runShouldExit.IsSet())
        {
            Trace.WriteInfo(ComponentName, "RepairPolicyEngineService::Run() Loop Started ... (FirstRunAsPrimaryReplica=={0})", firstRun);

            RepairPolicyEngineConfiguration repairConfiguration = RepairPolicyEngineConfiguration();

            // TODO: Should the load happen all the time? 
            if (!repairConfiguration.Load(_activationContextCPtr).IsSuccess())
            {
                SetHealthState(FABRIC_HEALTH_STATE::FABRIC_HEALTH_STATE_ERROR);
                _healthMessageWriter.Write("Error in configuration.");
            }

            RepairPolicyEnginePolicy repairPolicyEnginePolicy(repairConfiguration);

            if (IsPrimaryReplica && SUCCEEDED(RefreshNodesRepairInfo(repairConfiguration, firstRun, repairPolicyEnginePolicy)))
            {
                if (firstRun)
                {
                    FindLastExecutedRepairTime();
                }

                if (RepairWorkflow::UpdateNodeStates(_nodesRepairInfo, repairConfiguration, _fabricHealthClientCPtr, keyValueStorePtr_))
                {
                    RepairWorkflow::GenerateNodeRepairs(_nodesRepairInfo, repairPolicyEnginePolicy, _fabricHealthClientCPtr);

                    if(ScheduleRepairs(repairConfiguration, _fabricHealthClientCPtr))
                    {
                        PublishNodesRepairInfo(_fabricHealthClientCPtr, repairConfiguration);
                    }
                }

                firstRun = false;
            }

            ReportHealth(repairConfiguration);

            Trace.WriteNoise(ComponentName, "RepairPolicyEngineService::Run() Loop Finished ... (FirstRunAsPrimaryReplica=={0})", firstRun);

            // Don't allow to sleep less than 1s for performance reasons
            int sleepInMilliseconds = std::max(static_cast<int>(repairConfiguration.HealthCheckInterval.TotalPositiveMilliseconds()), 1000);

            if (IsDebuggerPresent())
            {
                // when debugger sleep shorter time;
                sleepInMilliseconds = std::min(sleepInMilliseconds, 2000); 
            }

            Trace.WriteInfo(ComponentName, "RepairPolicyEngineService::Run() Sleeping for '{0}' milliseconds before next health check of the nodes.", sleepInMilliseconds);
            _runShouldExit.WaitOne(sleepInMilliseconds);

            _healthStateToReport = FABRIC_HEALTH_STATE_OK;
            _healthMessageOnProblem.assign(IsPrimaryReplica ? L"Is running as primary replica. " : L"Is running as secondary replica.");
        }

        _runCompleted.Set();

        Trace.WriteInfo(ComponentName, "RepairPolicyEngineService::Run() Finished ...");
    }
    catch (exception const& e)
    {
        try
        {
            Trace.WriteError(ComponentName, "Unhandled Exception in RepairPolicyEngineService::Run(). Process will exit. Exception:{0}", e.what());
            SetHealthState(FABRIC_HEALTH_STATE::FABRIC_HEALTH_STATE_ERROR);
            _healthMessageWriter.Write("Fatal exception.");
            ReportHealth(initialRepairPolicyEngineConfiguration);
            PublishNodesRepairInfo(_fabricHealthClientCPtr, initialRepairPolicyEngineConfiguration);
        }
        catch (exception const& e)
        {
            UNREFERENCED_PARAMETER(e);
        }

        _runCompleted.Set();
        _exitServiceHostEventPtr->Set();
    }
}

void RepairPolicyEngineService::SetHealthState(FABRIC_HEALTH_STATE healthState)
{
    CODING_ERROR_ASSERT(healthState != FABRIC_HEALTH_STATE::FABRIC_HEALTH_STATE_UNKNOWN);
    CODING_ERROR_ASSERT(healthState != FABRIC_HEALTH_STATE::FABRIC_HEALTH_STATE_INVALID);

    _healthStateToReport = std::max(_healthStateToReport, healthState);
}

HRESULT RepairPolicyEngineService::GetNodeList(RepairPolicyEngineConfiguration & repairConfiguration, __out IFabricGetNodeListResult** nodeListResult)
{
    Trace.WriteInfo(ComponentName, "RepairPolicyEngineService::GetNodeList() - Started ..."); 

    ComPointer<GetNodeListAsyncOperationCallback> GetNodeListAsyncOperationCallback = make_com<PE::GetNodeListAsyncOperationCallback>();
    GetNodeListAsyncOperationCallback->Initialize(_fabricQueryClientCPtr);

    FABRIC_NODE_QUERY_DESCRIPTION queryDescription = { 0 };
    queryDescription.NodeNameFilter = nullptr;

    ComPointer<IFabricAsyncOperationContext> context;
    HRESULT result = _fabricQueryClientCPtr->BeginGetNodeList(
        &queryDescription,
        static_cast<DWORD>(repairConfiguration.WindowsFabricQueryTimeoutInterval.TotalPositiveMilliseconds()),
        GetNodeListAsyncOperationCallback.GetRawPointer(),
        context.InitializationAddress());

    if (FAILED(result))
    {
        Trace.WriteError(ComponentName, "RepairPolicyEngineService::GetNodeList() - Finished ...(with error {0})", result); 
        return result;
    }

    result = GetNodeListAsyncOperationCallback->Wait();

    if (FAILED(result))
    {
        Trace.WriteError(ComponentName, "RepairPolicyEngineService::GetNodeList() - Finished ...(with error {0})", result); 
        return result;
    }

    *nodeListResult = GetNodeListAsyncOperationCallback->Result.DetachNoRelease();

    Trace.WriteNoise(ComponentName, "RepairPolicyEngineService::GetNodeList() - Finished ..."); 

    return S_OK;
}

void RepairPolicyEngineService::ReportHealth(RepairPolicyEngineConfiguration& repairConfiguration)
{
    Trace.WriteNoise(ComponentName, "RepairPolicyEngineService::ReportHealth() Started ...");

    std::wstring propertyName = L"RepairPolicyEngineService";
    std::wstring description = _healthMessageOnProblem;
    std::wstring healthStateString;
    StringWriter healthStateStringWriter(healthStateString);
    healthStateStringWriter << _healthStateToReport;

    FABRIC_HEALTH_INFORMATION healthInformation = { 0 };
    healthInformation.Property = propertyName.c_str();
    healthInformation.Description = description.c_str();
    healthInformation.SourceId = L"RepairPolicyEngineService";
    healthInformation.State = _healthStateToReport;
    healthInformation.TimeToLiveSeconds = static_cast<DWORD>(repairConfiguration.ExpectedHealthRefreshInterval.TotalSeconds());
    healthInformation.SequenceNumber = DateTime::Now().Ticks;

    FABRIC_STATEFUL_SERVICE_REPLICA_HEALTH_REPORT replicaHealthReport = { 0 }; 
    replicaHealthReport.PartitionId = _partitionId;
    replicaHealthReport.ReplicaId = _replicaId;
    replicaHealthReport.HealthInformation = (const FABRIC_HEALTH_INFORMATION*) &healthInformation;

    FABRIC_HEALTH_REPORT healthReport;
    healthReport.Kind = FABRIC_HEALTH_REPORT_KIND::FABRIC_HEALTH_REPORT_KIND_STATEFUL_SERVICE_REPLICA;
    healthReport.Value = &replicaHealthReport;

    Trace.WriteInfo(ComponentName, "RepairPolicyEngineService::ReportHealth() PropertyName:{0}; HealthState:{1}; Description:{2}", propertyName, healthStateString, description);
    HRESULT healthReportResult = _fabricHealthClientCPtr->ReportHealth(&healthReport);

    if (FAILED(healthReportResult))
    {
        Trace.WriteError(ComponentName, "RepairPolicyEngineService::ReportHealth() Failed with error {0}", healthReportResult);
    }

    Trace.WriteNoise(ComponentName, "RepairPolicyEngineService::ReportHealth() Finished ...");
}

HRESULT RepairPolicyEngineService::RefreshNodesRepairInfo(RepairPolicyEngineConfiguration& repairConfiguration, bool firstRun, const RepairPolicyEnginePolicy& repairPolicyEnginePolicy)
{
    Trace.WriteInfo(ComponentName, "RepairPolicyEngineService::RefreshNodesRepairInfo() - Started ... (firstRun=={0})", firstRun); 

    HRESULT success = S_OK;

    ComPointer<IFabricGetNodeListResult> nodeListToRefreshFrom;
    HRESULT hr = GetNodeList(repairConfiguration, nodeListToRefreshFrom.InitializationAddress());

    bool continueWithHealthQuery = true;
    const FABRIC_NODE_QUERY_RESULT_LIST * fabricQueryNodeList = nullptr;

    if (FAILED(hr))
    {
        Trace.WriteError(
            ComponentName,
            "GetNodeList() failed with error code:{0}. Cannot continue with nodes health check.",
            hr);
        success = hr;
        continueWithHealthQuery = false;
    }
    else if (nodeListToRefreshFrom.GetRawPointer() == nullptr)
    {
        Trace.WriteError(
            ComponentName,
            "GetNodeList() returned null pointer. Cannot continue with nodes health check.");
        success = E_POINTER;
        continueWithHealthQuery = false;
    }
    else 
    {
        fabricQueryNodeList = nodeListToRefreshFrom->get_NodeList();
        if (fabricQueryNodeList == nullptr) 
        {
            Trace.WriteError(
                ComponentName,
                "IFabricGetNodeListResult.get_NodeList() returned null pointer. Cannot continue with nodes health check.");
            success = E_POINTER;
            continueWithHealthQuery = false;
        }
    }

    if (continueWithHealthQuery)
    {
        bool refreshFromHealthStore = fabricQueryNodeList->Count > 3 ? false : true; // 3 is to have more nodes to generally say something is wrong with health reporting

        // If this is not the first time to load from persistent store, this will always be true
        bool refreshFromPersistentStoreSuccessful = true;

        for (ULONG i = 0; i < fabricQueryNodeList->Count; i++)
        {
            CODING_ERROR_ASSERT(fabricQueryNodeList->Items != nullptr);
            const FABRIC_NODE_QUERY_RESULT_ITEM fabricNode = fabricQueryNodeList->Items[i];

            std::shared_ptr<NodeRepairInfo> nodeRepairInfo = nullptr;
            FindOrAddNodeRepairInfo(fabricNode.NodeName, nodeRepairInfo);
            CODING_ERROR_ASSERT(nodeRepairInfo != nullptr);

            // If we should, load from KVS first before updating LastHealthyAt and LastHealthyAt timestamps in Refresh()
            if (firstRun)
            {
                refreshFromPersistentStoreSuccessful &= KeyValueStoreHelper::LoadFromPersistentStore(*nodeRepairInfo, keyValueStorePtr_);
            }

            // some can fail as node doesn't need to exit for the noderepairinfo anymore.
            refreshFromHealthStore |= SUCCEEDED((nodeRepairInfo->Refresh(&fabricNode, _fabricHealthClientCPtr, repairConfiguration, repairPolicyEnginePolicy))); 
        }
        
        if (!refreshFromHealthStore)
        {
            Trace.WriteError(ComponentName, "RepairPolicyEngineService::RefreshNodesRepairInfo() - repair info failed to be refreshed for every node.");
            SetHealthState(FABRIC_HEALTH_STATE::FABRIC_HEALTH_STATE_ERROR);
            _healthMessageWriter.Write("Refreshing node repair information failed for all nodes.");
            success = E_FAIL;
        }

        if (!refreshFromPersistentStoreSuccessful)
        {
            // This is a warning because retrying the next time may be successful
            Trace.WriteWarning(ComponentName, "RepairPolicyEngineService::RefreshNodesRepairInfo() - Unable to load from persistent store.");
            success = E_FAIL;
        }
        else
        {
            // Only find outstanding repairs if we can refresh from persistent store successfully
            if (!FindStatusOfOutstandingRepairsForAllNodes(repairConfiguration, repairPolicyEnginePolicy, firstRun))
            {
                Trace.WriteError(ComponentName, "RepairPolicyEngineService::RefreshNodesRepairInfo() - Unable to find status of outstanding repairs for all nodes");
                success = E_FAIL;
            }
        }
    }
    else
    {
        SetHealthState(FABRIC_HEALTH_STATE_ERROR);
        _healthMessageWriter.Write("GetNodeList() failed with error '{0}'. Cannot continue with health check. ", success);
    }

    Trace.WriteInfo(ComponentName, "RepairPolicyEngineService::RefreshNodesRepairInfo() - Finished ..."); 

    return success;
}

void RepairPolicyEngineService::FindLastExecutedRepairTime()
{
    Trace.WriteInfo(ComponentName, "RepairPolicyEngineService::FindLastExecutedRepairTime() Started ...");

    for (auto n = _nodesRepairInfo.begin(); n!=_nodesRepairInfo.end(); n++)
    {
        for (auto r = (*n)->RepairHistory.begin(); r != (*n)->RepairHistory.end(); r++)
        {
            if (_lastExecutedRepairAt < r->ScheduledAt)
            {
                _lastExecutedRepairAt = r->ScheduledAt;
            }
        }
    }

    Trace.WriteInfo(ComponentName, "RepairPolicyEngineService::FindLastExecutedRepairTime() Finished ... Last executed repair at:{0}", _lastExecutedRepairAt);
}

bool RepairPolicyEngineService::PublishNodesRepairInfo(ComPointer<IFabricHealthClient> fabricHealthClient, 
                                                       const RepairPolicyEngineConfiguration& repairConfiguration)
{
    if (!IsPrimaryReplica)
    {
        Trace.WriteInfo(ComponentName, "RepairPolicyEngineService::PublishNodesRepairInfo() Skipping ... IsPrimaryReplica=={0}", IsPrimaryReplica);
        return false;
    }

    Trace.WriteInfo(ComponentName, "RepairPolicyEngineService::PublishNodesRepairInfo() Started ...");

    bool atLeastOneSuccess = _nodesRepairInfo.size() > 3 ? false : true; // 3 is to have more nodes to generally say something is wrong with health reporting

    for (auto nodeRepairInfo = _nodesRepairInfo.begin(); nodeRepairInfo!=_nodesRepairInfo.end(); nodeRepairInfo++)
    {
        bool result = (*nodeRepairInfo)->PublishToHealthStore(fabricHealthClient, repairConfiguration);
        atLeastOneSuccess |= result;

        // Persisting here at the end of main loop to persist any node that has data changed but doesn't reach any other persist point
        // because the node is healthy.
        // For example, in  each iteration of the main loop, every node has LastHealthyAt and LastErrorAt updated
        if (FAILED(KeyValueStoreHelper::WriteToPersistentStore(**nodeRepairInfo, keyValueStorePtr_, repairConfiguration)))
        {
            return false;
        }
    }

    if (!atLeastOneSuccess)
    {
        SetHealthState(FABRIC_HEALTH_STATE::FABRIC_HEALTH_STATE_ERROR);
        _healthMessageWriter.Write("Publish of all node repairs information failed.");
        Trace.WriteError(ComponentName, "RepairPolicyEngineService::PublishNodesRepairInfo() failed for all nodes. ...");
    }

    Trace.WriteNoise(ComponentName, "RepairPolicyEngineService::PublishNodesRepairInfo() Finished ...");
    return true;
}

void RepairPolicyEngineService::FindOrAddNodeRepairInfo(std::wstring nodeNameToFind, __out std::shared_ptr<NodeRepairInfo>& foundNode)
{
    for (auto n = _nodesRepairInfo.begin(); n!=_nodesRepairInfo.end(); n++)
    {
        if (StringUtility::AreEqualCaseInsensitive(nodeNameToFind, (*n)->NodeName))
        {
            foundNode = *n;
            return;
        }
    }

    Trace.WriteInfo(ComponentName, "Creating NodeRepairInfo for node '{0}'", nodeNameToFind);
    std::shared_ptr<NodeRepairInfo> nodeRepairInfo = NodeRepairInfo::CreateNodeRepairInfo(nodeNameToFind);
    _nodesRepairInfo.push_back(nodeRepairInfo);
    foundNode = _nodesRepairInfo.back();
}

bool RepairPolicyEngineService::FindStatusOfOutstandingRepairsForAllNodes(RepairPolicyEngineConfiguration& repairConfiguration, const RepairPolicyEnginePolicy& repairPolicyEnginePolicy, bool firstRun)
{
    FABRIC_REPAIR_SCOPE_IDENTIFIER scope;
    scope.Kind = FABRIC_REPAIR_SCOPE_IDENTIFIER_KIND_CLUSTER;
    scope.Value = NULL;

    FABRIC_REPAIR_TASK_QUERY_DESCRIPTION queryDescription = { 0 };
    queryDescription.Scope = &scope;
    queryDescription.TaskIdFilter = RepairPolicyEngineName;
    queryDescription.StateFilter = FABRIC_REPAIR_TASK_STATE_FILTER_ALL;

    DWORD timeout = static_cast<int>(repairConfiguration.WindowsFabricQueryTimeoutInterval.TotalPositiveMilliseconds()); 
    ComPointer<IFabricAsyncOperationContext> context;
    ComPointer<RepairAsyncOperationCallback> repairAsyncOperationCallback = make_com<PE::RepairAsyncOperationCallback>();
    repairAsyncOperationCallback->Initialize(_fabricRepairManagementClientCPtr);

    HRESULT result = _fabricRepairManagementClientCPtr->BeginGetRepairTaskList(
                    &queryDescription,
                    timeout,
                    repairAsyncOperationCallback.GetRawPointer(),
                    context.InitializationAddress());

    if (FAILED(result))
    {
        Trace.WriteError(ComponentName, "RepairPolicyEngineService::FindStatusOfOutstandingRepairsForAllNodes() - Finished ...(with error {0})", result); 
        return false;
    }

    // We cant execute until the information for the nodes is gotten so wait here
    result = repairAsyncOperationCallback->Wait();

    if (FAILED(result))
    {
        Trace.WriteError(ComponentName, "RepairPolicyEngineService::FindStatusOfOutstandingRepairsForAllNodes() - Finished ...(with error {0})", result); 
        return false;
    }
    ComPointer<IFabricGetRepairTaskListResult> getRepairListResult;

    result = _fabricRepairManagementClientCPtr->EndGetRepairTaskList(context.GetRawPointer(), getRepairListResult.InitializationAddress());
    if (FAILED(result))
    {
        Trace.WriteError(ComponentName, "RepairPolicyEngineService::FindStatusOfOutstandingRepairsForAllNodes() - Finished ...(with error {0})", result); 
        return false;
    }

    auto listPtr = getRepairListResult->get_Tasks();
    // for all the nodes look at the current state
    
    for (auto nodeRepairInfo = _nodesRepairInfo.begin(); nodeRepairInfo!=_nodesRepairInfo.end(); nodeRepairInfo++)
    {
        if (!(*nodeRepairInfo)->IsRepairNotStarted())
        {
	        bool foundNodeRepair = false;
            for (ULONG ix = 0; ix < listPtr->Count; ++ix)
            {
                FABRIC_REPAIR_TASK& repairTask = listPtr->Items[ix];
                if(StringUtility::AreEqualCaseInsensitive(repairTask.TaskId, (*nodeRepairInfo)->LastRepairTaskId))
                {
                    foundNodeRepair = true;
                    Trace.WriteInfo(ComponentName, "RepairPolicyEngineService::FindStatusOfOutstandingRepairsForAllNodes() - Found RepairInfo for node '{0}', Repair State: {1}, ResultStatus: {2}, flags: {3}", 
                        (*nodeRepairInfo)->NodeName, 
                        (LONG)repairTask.State, 
                        (LONG)repairTask.Result->ResultStatus, 
                        repairTask.Flags);

                    if (firstRun && (*nodeRepairInfo)->NodeRepairToSchedule == NULL)
                    {
                        // Only attempt to recover in the first run
                        (*nodeRepairInfo)->RecoverNodeRepairToSchedule(repairTask, repairPolicyEnginePolicy);
                    }

                    if (!(*nodeRepairInfo)->IsRepairCreated())
                    {
                        if(FAILED((*nodeRepairInfo)->ReportState(RepairStateTransition::CreatingRepairToCreatedRepair, repairConfiguration, keyValueStorePtr_)))
                        {
                            Trace.WriteError(ComponentName, "RepairPolicyEngineService::FindStatusOfOutstandingRepairsForAllNodes() Failed to report for transition CreatingRepairToCreatedRepair");
                            break;
                        }
                    }

                    // Check to see if the repair is finished
                    if (repairTask.State == FABRIC_REPAIR_TASK_STATE::FABRIC_REPAIR_TASK_STATE_COMPLETED)
                    {
                        (*nodeRepairInfo)->set_IsCancelInProgress(false);
                        if (repairTask.Result->ResultStatus != FABRIC_REPAIR_TASK_RESULT::FABRIC_REPAIR_TASK_RESULT_CANCELLED)
                        {
                            if (FAILED((*nodeRepairInfo)->ReportState(RepairStateTransition::RepairCompleted, repairConfiguration, keyValueStorePtr_)))
                            {
                                Trace.WriteError(ComponentName, "RepairPolicyEngineService::FindStatusOfOutstandingRepairsForAllNodes() Failed to report for transition RepairCompleted");
                                break;
                            }
                        }
                        else
                        {
                            if (FAILED((*nodeRepairInfo)->ReportState(RepairStateTransition::RepairCancelled, repairConfiguration, keyValueStorePtr_)))
                            {
                                Trace.WriteError(ComponentName, "RepairPolicyEngineService::FindStatusOfOutstandingRepairsForAllNodes() Failed to report for transition RepairCancelled");
                                break;
                            }
                        }
                        
                    }
                    // Check to see if the repair cancel flag is set
                    else if ((repairTask.Flags & FABRIC_REPAIR_TASK_FLAGS_CANCEL_REQUESTED) != 0)
                    {
                        (*nodeRepairInfo)->set_IsCancelInProgress(true);
                    }

                    break;
                }
            }

            if (!foundNodeRepair)
            {
                if ((*nodeRepairInfo)->IsRepairCreated())
                {
                    // If the repair is not found and we are already in CreatedRepair state, consider the repair as Completed
                    if (FAILED((*nodeRepairInfo)->ReportState(RepairStateTransition::RepairCompleted, repairConfiguration, keyValueStorePtr_)))
                    {
                        Trace.WriteError(ComponentName, "RepairPolicyEngineService::FindStatusOfOutstandingRepairsForAllNodes() Failed to report for transition RepairCancelled");
                    }
                }
            }
        }
    }
    return true;
}

bool NodeWithOlderRepairFirst(std::shared_ptr<NodeRepairInfo> node1, std::shared_ptr<NodeRepairInfo> node2)
{
    return node1->GetLastRepairAt() < node2->GetLastRepairAt();
}

HRESULT RepairPolicyEngineService::BeginCreateRepairRequest(RepairPolicyEngineConfiguration& repairConfiguration, 
                                                            ComPointer<IFabricHealthClient> fabricHealthClient,
                                                            std::shared_ptr<RepairPromotionRule> action, 
                                                            std::shared_ptr<NodeRepairInfo> nodeRepairInfo)
{
    CODING_ERROR_ASSERT(!nodeRepairInfo->LastRepairTaskId.empty());
    Trace.WriteNoise(ComponentName, "RepairPolicyEngineService::BeginCreateRepairRequest() - Started ..."); 

    // Ignore the repair for now if there's a request in progress
    if (nodeRepairInfo->get_IsRequestInProgress())
    {
        Trace.WriteInfo(ComponentName, "RepairPolicyEngineService::BeginCreateRepairRequest() - Another request is in progress. Returning..."); 
        return S_FALSE;
    }

    // Ignore the repair for now if the RepairsOutstanding is above the threshold.
    if (action->get_NodeRepairActionPolicy().get_MaxOutstandingRepairTasks() <= action->RepairsOutstanding+1)
    {
        Trace.WriteInfo(ComponentName, "RepairPolicyEngineService::BeginCreateRepairRequest() - Maximum number of outstanding tasks reached. Returning..."); 
        return S_FALSE;
    }

    // Target nodes to operate on
    const int size = 1;
    LPCWSTR targets[size];
    targets[0] = nodeRepairInfo->NodeName.c_str();

    FABRIC_STRING_LIST targetsFabricStringList;    
    targetsFabricStringList.Count = static_cast<ULONG>(size);
    targetsFabricStringList.Items = targets;
    
    FABRIC_REPAIR_TARGET_DESCRIPTION targetDescription;
    targetDescription.Kind = FABRIC_REPAIR_TARGET_KIND_NODE;
    targetDescription.Value = &targetsFabricStringList;

    FABRIC_REPAIR_SCOPE_IDENTIFIER scope;
    scope.Kind = FABRIC_REPAIR_SCOPE_IDENTIFIER_KIND_CLUSTER;
    scope.Value = NULL;
    
    FABRIC_REPAIR_TASK task;
    SecureZeroMemory(&task, sizeof(task));

    task.TaskId = nodeRepairInfo->LastRepairTaskId.c_str();
    auto repairName = action->get_NodeRepairActionPolicy().get_RepairName();
    std::wstring repairDescription = L"Policy Engine starting repair :- ";
    repairDescription.append(repairName);
    task.Scope = &scope;
    task.Description =   repairDescription.c_str();
    task.State = FABRIC_REPAIR_TASK_STATE_CREATED;
    task.Action = repairName.c_str();
    task.Target = &targetDescription;

    DWORD timeout = static_cast<int>(repairConfiguration.WindowsFabricQueryTimeoutInterval.TotalPositiveMilliseconds()); 

    ComPointer<RepairAsyncOperationCallback> repairAsyncOperationCallback = make_com<PE::RepairAsyncOperationCallback>();

    // pass in the lambda to process the callback
    repairAsyncOperationCallback->Initialize(_fabricRepairManagementClientCPtr, nodeRepairInfo,
        [](IFabricAsyncOperationContext* context, 
            ComPointer<IFabricRepairManagementClient> fabricRepairManagementClientCPtr,
            std::shared_ptr<NodeRepairInfo> nodeRepairInfo) -> HRESULT 
        {
            Trace.WriteNoise(ComponentName, "RepairPolicyEngineService::EndCreateRepairRequest() - Started ..."); 
            FABRIC_SEQUENCE_NUMBER commitVersion;
            SecureZeroMemory(&commitVersion, sizeof(commitVersion));
            HRESULT result = fabricRepairManagementClientCPtr->EndCreateRepairTask(context, &commitVersion);
            if(FAILED(result))
            {
                Trace.WriteWarning(ComponentName, "RepairPolicyEngineService::EndCreateRepairRequest() - Finished ...(with error {0})", Common::ErrorCode::FromHResult(result)); 
            }
            nodeRepairInfo->set_IsRequestInProgress(false);
            return result;
        });

    nodeRepairInfo->set_IsRequestInProgress(true);
    ComPointer<IFabricAsyncOperationContext> context;
    HRESULT result = _fabricRepairManagementClientCPtr->BeginCreateRepairTask(&task, timeout,
        repairAsyncOperationCallback.GetRawPointer(), context.InitializationAddress());

    if (FAILED(result))
    {
        Trace.WriteError(ComponentName, "RepairPolicyEngineService::BeginCreateRepairRequest() - Finished ...(with error {0})", result); 
        nodeRepairInfo->set_IsRequestInProgress(false);
        return result;
    }

    Trace.WriteNoise(ComponentName, "RepairPolicyEngineService::BeginCreateRepairRequest() - Finished ..."); 
    return S_OK;
}

HRESULT RepairPolicyEngineService::BeginCancelRepairRequest(RepairPolicyEngineConfiguration& repairConfiguration, 
                                                            Common::ComPointer<IFabricHealthClient> fabricHealthClient, 
                                                            std::shared_ptr<NodeRepairInfo> nodeRepairInfo)
{
    CODING_ERROR_ASSERT(!nodeRepairInfo->LastRepairTaskId.empty());
    Trace.WriteNoise(ComponentName, "RepairPolicyEngineService::BeginCancelRepairRequest() - Started ..."); 
    
    // Ignore the cancel for now if there's a request in progress
    if (nodeRepairInfo->get_IsRequestInProgress())
    {
        Trace.WriteInfo(ComponentName, "RepairPolicyEngineService::BeginCancelRepairRequest() - Another request is in progress. Returning..."); 
        return S_FALSE;
    }

    DWORD timeout = static_cast<int>(repairConfiguration.WindowsFabricQueryTimeoutInterval.TotalPositiveMilliseconds()); 

    ComPointer<RepairAsyncOperationCallback> repairAsyncOperationCallback = make_com<PE::RepairAsyncOperationCallback>();

    FABRIC_SEQUENCE_NUMBER commitVersion;
    SecureZeroMemory(&commitVersion, sizeof(commitVersion));

    // Pass the lambda that will be called back on end
    repairAsyncOperationCallback->Initialize(_fabricRepairManagementClientCPtr, nodeRepairInfo,
        [](IFabricAsyncOperationContext* context, 
            ComPointer<IFabricRepairManagementClient> fabricRepairManagementClientCPtr,
            std::shared_ptr<NodeRepairInfo> nodeRepairInfo) -> HRESULT 
        {
            Trace.WriteNoise(ComponentName, "RepairPolicyEngineService::EndCancelRepairRequest() - Started ..."); 
            FABRIC_SEQUENCE_NUMBER commitVersion;
            SecureZeroMemory(&commitVersion, sizeof(commitVersion));
            HRESULT result = fabricRepairManagementClientCPtr->EndCancelRepairTask(context, &commitVersion);
            if(FAILED(result))
            {
                Trace.WriteWarning(ComponentName, "RepairPolicyEngineService::EndCancelRepairRequest() - Finished ...(with error {0})", Common::ErrorCode::FromHResult(result)); 
            }
            nodeRepairInfo->set_IsRequestInProgress(false);
            return result;
        });

    nodeRepairInfo->set_IsRequestInProgress(true);
    ComPointer<IFabricAsyncOperationContext> context;

    FABRIC_REPAIR_SCOPE_IDENTIFIER scope;
    scope.Kind = FABRIC_REPAIR_SCOPE_IDENTIFIER_KIND_CLUSTER;
    scope.Value = NULL;

    FABRIC_REPAIR_CANCEL_DESCRIPTION cancelDescription = { 0 };
    cancelDescription.Scope = &scope;
    cancelDescription.RepairTaskId = nodeRepairInfo->LastRepairTaskId.c_str();

    HRESULT result = _fabricRepairManagementClientCPtr->BeginCancelRepairTask(
        &cancelDescription, 
        timeout,
        repairAsyncOperationCallback.GetRawPointer(),
        context.InitializationAddress());

    if (FAILED(result))
    {
        Trace.WriteError(ComponentName, "RepairPolicyEngineService::BeginCancelRepairRequest() - Finished ...(with error {0})", result); 
        nodeRepairInfo->set_IsRequestInProgress(false);
        return result;
    }

    Trace.WriteNoise(ComponentName, "RepairPolicyEngineService::BeginCancelRepairRequest() - Finished ..."); 
    return S_OK;
}

bool RepairPolicyEngineService::ScheduleRepairs(RepairPolicyEngineConfiguration& repairConfiguration, Common::ComPointer<IFabricHealthClient> fabricHealthClient)
{
    Trace.WriteInfo(ComponentName, "RepairPolicyEngineService::ScheduleRepairs() Started ... (LastRepairScheduledAt:{0})", _lastExecutedRepairAt);

    DateTime currentTime = DateTime::Now();

    bool recoveredNodeRepairInfo = false;

    std::vector<std::shared_ptr<NodeRepairInfo>> nodesCollection;

    // Make a copy of the repairinfo list and look for recovered node repairinfo
    for (auto n = _nodesRepairInfo.begin(); n != _nodesRepairInfo.end(); n++)
    {
        nodesCollection.push_back(*n);
        bool recovered = true;
        if(RepairWorkflow::RecoverFromInvalidState(*n, repairConfiguration, fabricHealthClient, keyValueStorePtr_, recovered))
        {
            recoveredNodeRepairInfo |= recovered;
        }
        else 
        {
            return false;
        }
    }

    std::sort(nodesCollection.begin(), nodesCollection.end(), NodeWithOlderRepairFirst);

    if (recoveredNodeRepairInfo)
    {
        SetHealthState(FABRIC_HEALTH_STATE::FABRIC_HEALTH_STATE_WARNING);
        _healthMessageWriter.Write("A node repair info has been recovered from invalid state.");
    }

    bool repairSchedulingIntervalReached = false;
    for (auto np = nodesCollection.begin(); np != nodesCollection.end(); np++)
    {
        NodeRepairInfo& nodeRepairInfo = **np;

        if (repairConfiguration.RepairActionSchedulingInterval.TotalMilliseconds() > 0 &&
            _lastExecutedRepairAt + repairConfiguration.RepairActionSchedulingInterval > currentTime)
        {
            repairSchedulingIntervalReached = true;
            Trace.WriteInfo(
                    ComponentName,
                    "RepairPolicyEngineService::ScheduleRepairs() will not create repairs in this iteraion due to recent LastRepairScheduledAt:{0} and RepairActionSchedulingInterval=={1}ms",
                    _lastExecutedRepairAt,
                    repairConfiguration.RepairActionSchedulingInterval.TotalMilliseconds());
        }

        if (!repairSchedulingIntervalReached && 
            nodeRepairInfo.HasPendingRepair() && 
            !nodeRepairInfo.IsRepairCreated())
        {
            if (nodeRepairInfo.IsRepairNotStarted())
            {
                CODING_ERROR_ASSERT(nodeRepairInfo.LastRepairTaskId.empty());
                // Generate new GUID for repair task ID
                std::wstring guidTask = Guid::NewGuid().ToString();
                nodeRepairInfo.LastRepairTaskId.append(RepairPolicyEngineName);
                nodeRepairInfo.LastRepairTaskId.append(guidTask);
                if (FAILED(nodeRepairInfo.ReportState(NotStartedToCreatingRepair, repairConfiguration, keyValueStorePtr_)))
                {
                    Trace.WriteError(ComponentName, "RepairPolicyEngineService::ScheduleRepairs() Failed to report state from NotStarted to CreatingRepair");
                    continue;                        
                }
            }

            HRESULT result = BeginCreateRepairRequest(repairConfiguration, fabricHealthClient, nodeRepairInfo.NodeRepairToSchedule, *np);
            if(SUCCEEDED(result))
            {
                Trace.WriteInfo(ComponentName, "Repair {0} of the node '{1}' successfully sent to the RepairManager Service.", nodeRepairInfo.NodeRepairToSchedule->get_NodeRepairActionPolicy().get_RepairName(), nodeRepairInfo.NodeName);
                _lastExecutedRepairAt = currentTime;
            }
            else
            {
                Trace.WriteError(ComponentName, "Repair {0} request for the node '{1}' failed to be sent to the RepairManager Service.", nodeRepairInfo.NodeRepairToSchedule->get_NodeRepairActionPolicy().get_RepairName(), nodeRepairInfo.NodeName);
                SetHealthState(FABRIC_HEALTH_STATE::FABRIC_HEALTH_STATE_ERROR);
                _healthMessageWriter.Write("Scheduling of Repair failed.");
            }
        }
        else if (nodeRepairInfo.IsRepairCreated() && nodeRepairInfo.get_shouldScheduleCancel())
        {
            nodeRepairInfo.set_ShouldScheduleCancel(false);
            HRESULT result = BeginCancelRepairRequest(repairConfiguration, fabricHealthClient, *np);
            if(SUCCEEDED(result))
            {
                Trace.WriteInfo(ComponentName, "Cancel Request of Repair {0} of the node '{1}' successfully sent to the RepairManager Service.", nodeRepairInfo.NodeRepairToSchedule->get_NodeRepairActionPolicy().get_RepairName(), nodeRepairInfo.NodeName);
            }
            else
            {
                Trace.WriteError(ComponentName, "Cancel Request of Repair {0} request for the node '{1}' failed to be sent to the RepairManager Service.", nodeRepairInfo.NodeRepairToSchedule->get_NodeRepairActionPolicy().get_RepairName(), nodeRepairInfo.NodeName);
                _healthMessageWriter.Write("Scheduling of Cancel failed.");
            }
        }
    }

    Trace.WriteNoise(ComponentName, "RepairPolicyEngineService::ScheduleRepairs() Finished ... (LastRepairScheduledAt:{0})", _lastExecutedRepairAt);
    return true;
}
