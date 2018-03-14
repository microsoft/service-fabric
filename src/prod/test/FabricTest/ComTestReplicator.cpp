// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace FabricTest;
using namespace TestHooks;
using namespace std;
using namespace TestHooks;
                                                                            
void ComTestReplicator::CheckForReportFaultsAndDelays(ApiFaultHelper::ComponentName compName, std::wstring operationName)
{
    if (partition_.GetRawPointer() != nullptr && ShouldFailOn(compName, operationName, ApiFaultHelper::ReportFaultPermanent))                    
    {                                                                                   
        partition_->ReportFault(FABRIC_FAULT_TYPE::FABRIC_FAULT_TYPE_PERMANENT);        
    };                                                                                  
    if (partition_.GetRawPointer() != nullptr && ShouldFailOn(compName, operationName, ApiFaultHelper::ReportFaultTransient))                    
    {                                                                                   
        partition_->ReportFault(FABRIC_FAULT_TYPE::FABRIC_FAULT_TYPE_TRANSIENT);        
    };                                                                                  
    if (ShouldFailOn(compName, operationName, ApiFaultHelper::Delay))                        
    {                                                                                   
        ::Sleep(static_cast<DWORD>(ApiFaultHelper::Get().GetApiDelayInterval().TotalMilliseconds()));
    }        
}
     
#define WAIT_FOR_SIGNAL_RESET(a)                                                        \
    WaitForSignalReset(a);                                                              \

ComTestReplicator::ComTestReplicator(
    ComPointer<IFabricReplicator> const& replicator, 
    ComPointer<IFabricStatefulServicePartition> const& partition, 
    bool disableCatchupSpecificQuorum,
    std::wstring const& serviceName, 
    Federation::NodeId const& nodeId) 
    : ComUnknownBase(),
    replicator_(replicator),
    primary_(replicator, ::IID_IFabricPrimaryReplicator),
    internal_(replicator, ::IID_IFabricInternalReplicator),
    partition_(partition),
    serviceName_(serviceName),
    nodeId_(nodeId),
    disableCatchupSpecificQuorum_(disableCatchupSpecificQuorum),
    replicaMap_()
{
    TestSession::FailTestIfNot((bool) replicator_, "Replicator didn't implement IFabricReplicator");
    TestSession::FailTestIfNot((bool) primary_, "Replicator didn't implement IFabricPrimaryReplicator");
    TestSession::FailTestIfNot((bool) internal_, "Replicator didn't implement IFabricInternalReplicator");
    readWriteStatusValidator_ = make_unique<ReadWriteStatusValidator>(partition);
}

// *****************************
// IFabricReplicator methods
// *****************************
HRESULT ComTestReplicator::BeginOpen(
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context) 
{
    readWriteStatusValidator_->OnOpen();

    WAIT_FOR_SIGNAL_RESET(ReplicatorBeginOpenBlock)
    CheckForReportFaultsAndDelays(ApiFaultHelper::Replicator, L"BeginOpen");
    
    if (ShouldFailOn(ApiFaultHelper::Replicator, L"BeginOpen")) return E_FAIL;

    if (ShouldFailOn(ApiFaultHelper::Replicator, L"EndOpen")) 
    {
        ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();
        HRESULT hr = operation->Initialize(E_FAIL, ComponentRootSPtr(), callback);
        TestSession::FailTestIf(FAILED(hr), "operation->Initialize should not fail");
        return ComAsyncOperationContext::StartAndDetach<ComCompletedAsyncOperationContext>(std::move(operation), context);
    }
    else
    {
       
        return replicator_->BeginOpen(callback, context);
    }
}

HRESULT ComTestReplicator::EndOpen( 
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricStringResult **replicationEndpoint)
{
    WAIT_FOR_SIGNAL_RESET(ReplicatorEndOpenBlock)
    CheckForReportFaultsAndDelays(ApiFaultHelper::Replicator, L"EndOpen");

    auto castedContext = dynamic_cast<ComCompletedAsyncOperationContext*>(context);
    if(castedContext != nullptr)
    {
        replicator_->Abort();
        return ComCompletedAsyncOperationContext::End(context);
    }
    else
    {
        return replicator_->EndOpen(context, replicationEndpoint);
    }
}

HRESULT ComTestReplicator::BeginChangeRole( 
    /* [in] */ FABRIC_EPOCH const * epoch,
    /* [in] */ FABRIC_REPLICA_ROLE role,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context) 
{
    readWriteStatusValidator_->OnChangeRole(role);

    WAIT_FOR_SIGNAL_RESET(ReplicatorBeginChangeRoleBlock)
    CheckForReportFaultsAndDelays(ApiFaultHelper::Replicator, L"BeginChangeRole");
    
    if (ShouldFailOn(ApiFaultHelper::Replicator, L"BeginChangeRole")) return E_FAIL;

    if (ShouldFailOn(ApiFaultHelper::Replicator, L"EndChangeRole")) 
    {
        ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();
        HRESULT hr = operation->Initialize(E_FAIL, ComponentRootSPtr(), callback);
        TestSession::FailTestIf(FAILED(hr), "operation->Initialize should not fail");
        return ComAsyncOperationContext::StartAndDetach<ComCompletedAsyncOperationContext>(std::move(operation), context);
    }
    else
    {
        auto hr = replicator_->BeginChangeRole(epoch, role, callback, context);

        if (role == FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY)
        {
            replicaMap_.ClearReplicaMap();
        }
        return hr;
    }
}

HRESULT ComTestReplicator::EndChangeRole( 
    /* [in] */ IFabricAsyncOperationContext *context)
{ 
    WAIT_FOR_SIGNAL_RESET(ReplicatorEndChangeRoleBlock)
    CheckForReportFaultsAndDelays(ApiFaultHelper::Replicator, L"EndChangeRole");
    
    auto castedContext = dynamic_cast<ComCompletedAsyncOperationContext*>(context);
    if(castedContext != nullptr)
    {
        return ComCompletedAsyncOperationContext::End(context);
    }
    else
    {
        return replicator_->EndChangeRole(context);
    }
}

HRESULT ComTestReplicator::BeginUpdateEpoch(
    /* [in] */ FABRIC_EPOCH const * epoch,
    /* [in] */ IFabricAsyncOperationCallback * callback,
    /* [out, retval] */ IFabricAsyncOperationContext ** context)
{
    WAIT_FOR_SIGNAL_RESET(ReplicatorBeginUpdateEpochBlock)
    CheckForReportFaultsAndDelays(ApiFaultHelper::Replicator, L"BeginUpdateEpoch");
    
    if (ShouldFailOn(ApiFaultHelper::Replicator, L"BeginUpdateEpoch")) return E_FAIL;

    HRESULT completionHR = S_OK;

    if (ShouldFailOn(ApiFaultHelper::Replicator, L"EndUpdateEpoch")) 
    {
        completionHR = E_FAIL;
    }
    else if (ApiSignalHelper::IsSignalSet(nodeId_, serviceName_, ReplicatorReturnZeroProgress))
    {
        completionHR = S_OK;
    }
    else
    {
        return replicator_->BeginUpdateEpoch(epoch, callback, context);
    }

    ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();
    HRESULT hr = operation->Initialize(completionHR, ComponentRootSPtr(), callback);
    TestSession::FailTestIf(FAILED(hr), "operation->Initialize should not fail");
    return ComAsyncOperationContext::StartAndDetach<ComCompletedAsyncOperationContext>(std::move(operation), context);
}

HRESULT ComTestReplicator::EndUpdateEpoch(
    /* [in] */ IFabricAsyncOperationContext * context)
{
    WAIT_FOR_SIGNAL_RESET(ReplicatorEndUpdateEpochBlock)
    CheckForReportFaultsAndDelays(ApiFaultHelper::Replicator, L"EndUpdateEpoch");
    
    auto castedContext = dynamic_cast<ComCompletedAsyncOperationContext*>(context);
    if(castedContext != nullptr)
    {
        return ComCompletedAsyncOperationContext::End(context);
    }
    else
    {
        return replicator_->EndUpdateEpoch(context);
    }
}

HRESULT ComTestReplicator::BeginClose( 
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context) 
{
    readWriteStatusValidator_->OnClose();

    WAIT_FOR_SIGNAL_RESET(ReplicatorBeginCloseBlock)
    CheckForReportFaultsAndDelays(ApiFaultHelper::Replicator, L"BeginClose");
    
    if (ShouldFailOn(ApiFaultHelper::Replicator, L"BeginClose")) return E_FAIL;

    if (ShouldFailOn(ApiFaultHelper::Replicator, L"EndClose")) 
    {
        ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();
        HRESULT hr = operation->Initialize(E_FAIL, ComponentRootSPtr(), callback);
        TestSession::FailTestIf(FAILED(hr), "operation->Initialize should not fail");
        return ComAsyncOperationContext::StartAndDetach<ComCompletedAsyncOperationContext>(std::move(operation), context);
    }
    else
    {
        return replicator_->BeginClose(callback, context);
    }
}

HRESULT ComTestReplicator::EndClose( 
    /* [in] */ IFabricAsyncOperationContext *context) 
{
    WAIT_FOR_SIGNAL_RESET(ReplicatorEndCloseBlock)
    CheckForReportFaultsAndDelays(ApiFaultHelper::Replicator, L"EndClose");
    
    auto castedContext = dynamic_cast<ComCompletedAsyncOperationContext*>(context);
    if(castedContext != nullptr)
    {
        return ComCompletedAsyncOperationContext::End(context);
    }
    else
    {
        return replicator_->EndClose(context);
    }
}

void ComTestReplicator::Abort()
{
    WAIT_FOR_SIGNAL_RESET(ReplicatorAbortBlock)
    CheckForReportFaultsAndDelays(ApiFaultHelper::Replicator, L"Abort");
    
    replicator_->Abort();
}

HRESULT ComTestReplicator::GetCurrentProgress( 
    /* [out] */ FABRIC_SEQUENCE_NUMBER *lastSequenceNumber)
{
    CheckForReportFaultsAndDelays(ApiFaultHelper::Replicator, L"GetCurrentProgress");
    
    if (ShouldFailOn(ApiFaultHelper::Replicator, L"GetCurrentProgress")) return E_FAIL;

    if (ApiSignalHelper::IsSignalSet(nodeId_, serviceName_,ReplicatorReturnZeroProgress))
    {
        *lastSequenceNumber = 0;
        return S_OK;
    }

    return replicator_->GetCurrentProgress(lastSequenceNumber);
}

HRESULT ComTestReplicator::GetCatchUpCapability( 
    /* [out] */ FABRIC_SEQUENCE_NUMBER *firstSequenceNumber)
{
    CheckForReportFaultsAndDelays(ApiFaultHelper::Replicator, L"GetCatchUpCapability");
    
    if (ShouldFailOn(ApiFaultHelper::Replicator, L"GetCatchUpCapability")) return E_FAIL;

    if (ApiSignalHelper::IsSignalSet(nodeId_, serviceName_, ReplicatorReturnZeroProgress))
    {
        *firstSequenceNumber = 0;
        return S_OK;
    }

    return replicator_->GetCatchUpCapability(firstSequenceNumber);
}

void ComTestReplicator::VerifyReplicaAndIncrementMustCatchupCounter(
    FABRIC_REPLICA_INFORMATION const & replica,
    FABRIC_REPLICA_ROLE expectedRole,
    bool isMustCatchupAllowed,
    int & counter) const
{
    ASSERT_IF(replica.Id == 0, "Replica Id is 0");
    ASSERT_IF(replica.Role != expectedRole, "Fabric should only pass secondaries");
    ASSERT_IF(replica.Status != FABRIC_REPLICA_STATUS_UP, "Only up replicas should be passed to replicator");
    ASSERT_IF(replica.Reserved == NULL, "Reserved field should be set on replica info");

    auto casted = reinterpret_cast<FABRIC_REPLICA_INFORMATION_EX1 const *>(replica.Reserved);
    ASSERT_IF(casted->Reserved != NULL, "EX1 should have null reserved field");
    if (casted->MustCatchup == TRUE && !isMustCatchupAllowed)
    {
        TestSession::FailTest("MustCatchupNotALlowed and found replica with must catchup true {0} {1} {2}", nodeId_, serviceName_, replica.Id);
    }

    if (casted->MustCatchup)
    {
        counter++;
    }
}

bool ComTestReplicator::VerifyReplicaSetConfigurationAndReturnIfMustCatchupFound(
    FABRIC_REPLICA_SET_CONFIGURATION const * configuration,
    bool isMustCatchupAllowed) const 
{
    int counter = 0;

    for (ULONG i = 0; i < configuration->ReplicaCount; i++)
    {
        VerifyReplicaAndIncrementMustCatchupCounter(configuration->Replicas[i], FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, isMustCatchupAllowed, counter);
    }

    TestSession::FailTestIf(counter > 1, "More than 1 must catchup replica found {0} {1}", nodeId_, serviceName_);

    return counter != 0;
}

void ComTestReplicator::UpdateNewReplicas(FABRIC_REPLICA_SET_CONFIGURATION const * configuration, map<FABRIC_REPLICA_ID, FABRIC_SEQUENCE_NUMBER> & replicas)
{
    for (ULONG i = 0; i < configuration->ReplicaCount; i++)
    {
        auto replicaId = configuration->Replicas[i].Id;
        auto replicaLSN = configuration->Replicas[i].CurrentProgress;

        replicas[replicaId] = replicaLSN;

        replicaMap_.UpdateReplicaMap(replicaId);
    }
}

// *****************************
// IFabricPrimaryReplicator methods
// *****************************
HRESULT ComTestReplicator::UpdateCatchUpReplicaSetConfiguration( 
    /* [in] */ FABRIC_REPLICA_SET_CONFIGURATION const * currentConfiguration,
    /* [in] */ FABRIC_REPLICA_SET_CONFIGURATION const * previousConfiguration)
{
    WAIT_FOR_SIGNAL_RESET(ReplicatorUpdateCatchUpReplicaSetConfigurationBlock)
    CheckForReportFaultsAndDelays(ApiFaultHelper::Replicator, L"UpdateCatchUpReplicaSetConfiguration");
    
    if (ShouldFailOn(ApiFaultHelper::Replicator, L"UpdateCatchUpReplicaSetConfiguration")) return E_FAIL;

    VerifyReplicaSetConfigurationAndReturnIfMustCatchupFound(currentConfiguration, true);
    VerifyReplicaSetConfigurationAndReturnIfMustCatchupFound(previousConfiguration, false);

    std::map<FABRIC_REPLICA_ID, FABRIC_SEQUENCE_NUMBER> ccReplicas;
    std::map<FABRIC_REPLICA_ID, FABRIC_SEQUENCE_NUMBER> pcReplicas;
    std::map<FABRIC_REPLICA_ID, FABRIC_SEQUENCE_NUMBER> pcAndccReplicas;

    UpdateNewReplicas(currentConfiguration, ccReplicas);
    UpdateNewReplicas(previousConfiguration, pcReplicas);

    // Get replicas in both CC and PC
    for (auto replicaEntity : ccReplicas)
    {
        auto replicaId = replicaEntity.first;
        if (pcReplicas.find(replicaId) != pcReplicas.end())
        {
            pcAndccReplicas[replicaId] = ccReplicas[replicaId];
        }
    }

    replicaMap_.UpdateCatchupReplicaSet(ccReplicas, pcReplicas, pcAndccReplicas);
    
    return primary_->UpdateCatchUpReplicaSetConfiguration(currentConfiguration, previousConfiguration);
}

HRESULT ComTestReplicator::BeginWaitForCatchUpQuorum(
    /* [in] */ FABRIC_REPLICA_SET_QUORUM_MODE catchUpMode,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    WAIT_FOR_SIGNAL_RESET(ReplicatorBeginWaitForCatchupQuorumBlock);
        
    CheckForReportFaultsAndDelays(ApiFaultHelper::Replicator, L"BeginWaitForCatchUpQuorum");
    
    if (ShouldFailOn(ApiFaultHelper::Replicator, L"BeginWaitForCatchUpQuorum")) return E_FAIL;

    if (ShouldFailOn(ApiFaultHelper::Replicator, L"EndWaitForCatchUpQuorum")) 
    {
        ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();
        HRESULT hr = operation->Initialize(E_FAIL, ComponentRootSPtr(), callback);
        TestSession::FailTestIf(FAILED(hr), "operation->Initialize should not fail");
        return ComAsyncOperationContext::StartAndDetach<ComCompletedAsyncOperationContext>(std::move(operation), context);
    }
    else
    {
        return primary_->BeginWaitForCatchUpQuorum(catchUpMode, callback, context);
    }
}

HRESULT ComTestReplicator::EndWaitForCatchUpQuorum( 
    /* [in] */ IFabricAsyncOperationContext *context)
{
    CheckForReportFaultsAndDelays(ApiFaultHelper::Replicator, L"EndWaitForCatchUpQuorum");
    
    auto castedContext = dynamic_cast<ComCompletedAsyncOperationContext*>(context);
    if(castedContext != nullptr)
    {
        return ComCompletedAsyncOperationContext::End(context);
    }
    else
    {
        return primary_->EndWaitForCatchUpQuorum(context);
    }
}

HRESULT ComTestReplicator::UpdateCurrentReplicaSetConfiguration( 
    /* [in] */ FABRIC_REPLICA_SET_CONFIGURATION const * currentConfiguration)
{
    WAIT_FOR_SIGNAL_RESET(ReplicatorUpdateCurrentReplicaSetConfigurationBlock);
    CheckForReportFaultsAndDelays(ApiFaultHelper::Replicator, L"UpdateCurrentReplicaSetConfiguration");
    
    if (ShouldFailOn(ApiFaultHelper::Replicator, L"UpdateCurrentReplicaSetConfiguration")) return E_FAIL;

    std::map<FABRIC_REPLICA_ID, FABRIC_SEQUENCE_NUMBER> ccReplicas;

    VerifyReplicaSetConfigurationAndReturnIfMustCatchupFound(currentConfiguration, false);
    UpdateNewReplicas(currentConfiguration, ccReplicas);

    replicaMap_.UpdateCurrentReplicaSet(ccReplicas);

    return primary_->UpdateCurrentReplicaSetConfiguration(currentConfiguration);
}

HRESULT ComTestReplicator::BeginBuildReplica( 
    /* [in] */ FABRIC_REPLICA_INFORMATION const *idleReplica,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    CheckForReportFaultsAndDelays(ApiFaultHelper::Replicator, L"BeginBuildReplica");
    
    if (ShouldFailOn(ApiFaultHelper::Replicator, L"BeginBuildReplica")) return E_FAIL;

    int counter = 0;
    VerifyReplicaAndIncrementMustCatchupCounter(
        *idleReplica,
        FABRIC_REPLICA_ROLE_IDLE_SECONDARY,
        false,
        counter);

    if (ShouldFailOn(ApiFaultHelper::Replicator, L"EndBuildReplica")) 
    {
        ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();
        HRESULT hr = operation->Initialize(E_FAIL, ComponentRootSPtr(), callback);
        TestSession::FailTestIf(FAILED(hr), "operation->Initialize should not fail");
        return ComAsyncOperationContext::StartAndDetach<ComCompletedAsyncOperationContext>(std::move(operation), context);
    }
    else
    {
        replicaMap_.BeginBuildReplica(idleReplica->Id);
        HRESULT hr = primary_->BeginBuildReplica(idleReplica, callback, context);
        
        if (FAILED(hr))
        {
            replicaMap_.BuildReplicaOnComplete(idleReplica->Id, hr);
        }
        else
        {
            replicaMap_.AsyncMapAdd(*context, idleReplica->Id);
        }

        return hr;
    }
}

HRESULT ComTestReplicator::EndBuildReplica( 
    /* [in] */ IFabricAsyncOperationContext *context)
{
    CheckForReportFaultsAndDelays(ApiFaultHelper::Replicator, L"EndBuildReplica");
    
    auto castedContext = dynamic_cast<ComCompletedAsyncOperationContext*>(context);
    if(castedContext != nullptr)
    {
        return ComCompletedAsyncOperationContext::End(context);
    }
    else
    {
        auto hr = primary_->EndBuildReplica(context);
        replicaMap_.EndBuildReplica(context, hr);

        return hr;
    }
}

HRESULT ComTestReplicator::RemoveReplica(
    /* [in] */ FABRIC_REPLICA_ID replicaId)
{
    CheckForReportFaultsAndDelays(ApiFaultHelper::Replicator, L"RemoveReplica");
    
    if (ShouldFailOn(ApiFaultHelper::Replicator, L"RemoveReplica")) return E_FAIL;

    replicaMap_.RemoveReplica(replicaId);
    
    return primary_->RemoveReplica(replicaId);
}

HRESULT ComTestReplicator::BeginOnDataLoss(
    /* [in] */ IFabricAsyncOperationCallback * callback,
    /* [out, retval] */ IFabricAsyncOperationContext ** context)
{
    readWriteStatusValidator_->OnOnDataLoss();
    WAIT_FOR_SIGNAL_RESET(ReplicatorBeginOnDataLossBlock);
    CheckForReportFaultsAndDelays(ApiFaultHelper::Replicator, L"BeginOnDataLoss");
    
    if (ShouldFailOn(ApiFaultHelper::Replicator, L"BeginOnDataLoss")) return E_FAIL;

    if (ShouldFailOn(ApiFaultHelper::Replicator, L"EndOnDataLoss")) 
    {
        ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();
        HRESULT hr = operation->Initialize(E_FAIL, ComponentRootSPtr(), callback);
        TestSession::FailTestIf(FAILED(hr), "operation->Initialize should not fail");
        return ComAsyncOperationContext::StartAndDetach<ComCompletedAsyncOperationContext>(std::move(operation), context);
    }
    else
    {
        return primary_->BeginOnDataLoss(callback, context);
    }
}

HRESULT ComTestReplicator::EndOnDataLoss(
    /* [in] */ IFabricAsyncOperationContext * context,
    /* [out, retval] */ BOOLEAN * isStateChanged)
{
    CheckForReportFaultsAndDelays(ApiFaultHelper::Replicator, L"EndOnDataLoss");
    
    auto castedContext = dynamic_cast<ComCompletedAsyncOperationContext*>(context);
    if(castedContext != nullptr)
    {
        return ComCompletedAsyncOperationContext::End(context);
    }
    else
    {
        return primary_->EndOnDataLoss(context, isStateChanged);
    }
}

HRESULT  ComTestReplicator::GetReplicatorStatus(IFabricGetReplicatorStatusResult **replicatorStatus)
{
    WAIT_FOR_SIGNAL_RESET(ReplicatorGetQueryBlock)
    return internal_->GetReplicatorStatus(replicatorStatus);
}
