// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace FabricTest;
using namespace std;

void ComTestStoreService::CheckForReportFaultsAndDelays(ComPointer<IFabricStatefulServicePartition> partition, ApiFaultHelper::ComponentName compName, std::wstring operationName)
{
    if (testStoreService_.ShouldFailOn(compName, operationName, ApiFaultHelper::ReportFaultPermanent))          
    {
        if (partition.GetRawPointer() != nullptr) 
        {
            partition->ReportFault(FABRIC_FAULT_TYPE::FABRIC_FAULT_TYPE_PERMANENT);
        } 
    };                                                                                          
    if (testStoreService_.ShouldFailOn(compName, operationName, ApiFaultHelper::ReportFaultTransient))          
    {     
        if (partition.GetRawPointer() != nullptr) 
        {
            partition->ReportFault(FABRIC_FAULT_TYPE::FABRIC_FAULT_TYPE_TRANSIENT);
        }
    };                                                                                          
    if (testStoreService_.ShouldFailOn(compName, operationName, ApiFaultHelper::Delay))               
    {         
        ::Sleep(static_cast<DWORD>(ApiFaultHelper::Get().GetApiDelayInterval().TotalMilliseconds()));
    }
}

#define WAIT_FOR_SIGNAL_RESET(a)                                                        \
    testStoreService_.WaitForSignalReset(a);                                                     \

ComTestStoreService::ComTestStoreService(TestStoreService & testStoreService)
    : root_(testStoreService.shared_from_this()),
    testStoreService_(testStoreService),
    replicationEngine_(),
    isClosedCalled_(false),
    isDecrementSequenceNumberNeeded_(false)
{
}

ULONG callCount = 0;
ULONG delayCount = 0;
ULONG tempCount = 0;
ULONG permCount = 0;
ULONG failCount = 0;

HRESULT ComTestStoreService::BeginOpen( 
    /* [in] */ FABRIC_REPLICA_OPEN_MODE openMode,
    /* [in] */ IFabricStatefulServicePartition *statefulServicePartition,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    UNREFERENCED_PARAMETER(openMode);

    if (statefulServicePartition == NULL || callback == NULL || context == NULL) { return E_POINTER; }

    TestSession::FailTestIfNot(openMode == FABRIC_REPLICA_OPEN_MODE::FABRIC_REPLICA_OPEN_MODE_NEW, "Wrong open mode is passed in for volatile service");

    ComPointer<IFabricStatefulServicePartition3> partition;
    // TODO: tempPartition below is used for replicator only, remove when everything is switched to IFabricStatefulServicePartition1 interface (RDBug 3114076)
    ComPointer<IFabricStatefulServicePartition> tempPartition;
    partition.SetAndAddRef((IFabricStatefulServicePartition3*)statefulServicePartition);
    tempPartition.SetAndAddRef(statefulServicePartition);

    CheckForReportFaultsAndDelays(tempPartition, ApiFaultHelper::Service, L"BeginOpen");


    if (testStoreService_.ShouldFailOn(ApiFaultHelper::Service, L"BeginOpen")) 
    {
        return E_FAIL;
    }

    ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();
    HRESULT hr = E_FAIL;
    bool streamFaultsAndRequireServiceAckEnabled = false;

    if (testStoreService_.ShouldFailOn(ApiFaultHelper::Service, L"EndOpen")) 
    {
        hr = operation->Initialize(E_FAIL, ApiFaultHelper::Get().GetAsyncCompleteDelayTime(), root_, callback);
    }
    else
    {
        Common::ScopedHeap heap;
        ComPointer<IFabricStateReplicator> stateReplicator;
        ComPointer<IFabricReplicator> replicationEngine;
        FABRIC_REPLICATOR_SETTINGS replicatorSettings = { 0 };
        FABRIC_REPLICATOR_SETTINGS_EX1 replicatorSettingsEx1 = { 0 };
        FABRIC_REPLICATOR_SETTINGS_EX2 replicatorSettingsEx2 = { 0 };
        FABRIC_REPLICATOR_SETTINGS_EX3 replicatorSettingsEx3 = { 0 };
        FABRIC_REPLICATOR_SETTINGS_EX4 replicatorSettingsEx4 = { 0 };

        replicatorSettings.Flags = FABRIC_REPLICATOR_SETTINGS_NONE; // no memory limit, default number of items limit = 1024.
        replicatorSettings.Reserved = &replicatorSettingsEx1;
        replicatorSettingsEx1.Reserved = &replicatorSettingsEx2;
        replicatorSettingsEx2.Reserved = &replicatorSettingsEx3;
        replicatorSettingsEx3.Reserved = &replicatorSettingsEx4;
        replicatorSettingsEx4.Reserved = NULL;

		ReplicatorSettingServiceInitDataParser rsParser(testStoreService_.InitDataString);
        auto inputReplicatorSettings = rsParser.CreateReplicatorSettings(testStoreService_.InitDataString, testStoreService_.GetPartitionId());
        inputReplicatorSettings->ToPublicApi(heap, replicatorSettings);

        HRESULT result = partition->CreateReplicator(this, &replicatorSettings, replicationEngine.InitializationAddress(), stateReplicator.InitializationAddress());
        TestSession::FailTestIfNot(result == S_OK, "GetReplicator did not return success");
        TestSession::FailTestIfNot((bool) stateReplicator, "stateReplicator is null");
        TestSession::FailTestIfNot((bool) replicationEngine, "replicationEngine is null");

        ComPointer<ComTestReplicator> testReplicator = make_com<ComTestReplicator>(replicationEngine, tempPartition, false, testStoreService_.ServiceName, testStoreService_.NodeId);
        result = testReplicator->QueryInterface(IID_IFabricReplicator, reinterpret_cast<void**>(replicationEngine_.InitializationAddress()));
        TestSession::FailTestIfNot(result == S_OK, "testReplicator->QueryInterface did not return success");

        ErrorCode error = testStoreService_.OnOpen(partition, stateReplicator, streamFaultsAndRequireServiceAckEnabled);
        TestSession::FailTestIfNot(error.IsSuccess(), "testStoreService_.OnOpen failed with error {0}", error);

        hr = operation->Initialize(root_, callback);
    }

    TestSession::FailTestIf(FAILED(hr), "operation->Initialize should not fail");
    return ComAsyncOperationContext::StartAndDetach<ComCompletedAsyncOperationContext>(std::move(operation), context);
}

HRESULT ComTestStoreService::EndOpen(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [out][retval] */ IFabricReplicator **replicationEngine)
{
    if (context == NULL || replicationEngine == NULL) { return E_POINTER; }

    CheckForReportFaultsAndDelays(testStoreService_.GetPartition(), ApiFaultHelper::Service, L"EndOpen");
 
    HRESULT hr = ComCompletedAsyncOperationContext::End(context);
    if (FAILED(hr)) { return hr; }
    *replicationEngine = replicationEngine_.DetachNoRelease();
    return hr;
}

HRESULT ComTestStoreService::BeginChangeRole( 
    /* [in] */ FABRIC_REPLICA_ROLE newRole,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (callback == NULL || context == NULL) { return E_POINTER; }

    WAIT_FOR_SIGNAL_RESET(ServiceBeginChangeRoleBlock)
    CheckForReportFaultsAndDelays(testStoreService_.GetPartition(), ApiFaultHelper::Service, L"BeginChangeRole");

    if (testStoreService_.ShouldFailOn(ApiFaultHelper::Service, L"BeginChangeRole")) return E_FAIL;

    ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();
    HRESULT hr = E_FAIL;
    if (testStoreService_.ShouldFailOn(ApiFaultHelper::Service, L"EndChangeRole")) 
    {
        hr = operation->Initialize(E_FAIL, ApiFaultHelper::Get().GetAsyncCompleteDelayTime(), root_, callback);
    }
    else
    {
        ErrorCode error = testStoreService_.OnChangeRole(newRole);
        TestSession::FailTestIfNot(error.IsSuccess(), "testStoreService_.OnChangeRole failed with error {0}", error);

        hr = operation->Initialize(root_, callback);
    }

    TestSession::FailTestIf(FAILED(hr), "operation->Initialize should not fail");
    return ComAsyncOperationContext::StartAndDetach<ComCompletedAsyncOperationContext>(std::move(operation), context);
}

HRESULT ComTestStoreService::EndChangeRole( 
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricStringResult **serviceEndpoint)
{
    if (context == NULL || serviceEndpoint == NULL) { return E_POINTER; }

    CheckForReportFaultsAndDelays(testStoreService_.GetPartition(), ApiFaultHelper::Service, L"endchangerole");

    HRESULT hr = ComCompletedAsyncOperationContext::End(context);
    if (FAILED(hr)) return hr;

    ComPointer<IFabricStringResult> stringResult = make_com<ComStringResult,IFabricStringResult>(testStoreService_.ServiceLocation);
    *serviceEndpoint = stringResult.DetachNoRelease();
    return S_OK;
}

HRESULT ComTestStoreService::BeginClose( 
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL) { return E_POINTER; }

    CheckForReportFaultsAndDelays(testStoreService_.GetPartition(), ApiFaultHelper::Service, L"BeginClose");

    HRESULT beginResult = testStoreService_.ShouldFailOn(ApiFaultHelper::Service, L"BeginClose") ? E_FAIL : S_OK;
    HRESULT endResult = testStoreService_.ShouldFailOn(ApiFaultHelper::Service, L"EndClose") ? E_FAIL : S_OK;

    if(!isClosedCalled_)
    {
        isClosedCalled_ = true;
        ErrorCode error = testStoreService_.OnClose();
        TestSession::FailTestIfNot(error.IsSuccess(), "testStoreService_.OnClose failed with error {0}", error);
    }

    if(FAILED(beginResult)) return E_FAIL;

    ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();
    HRESULT hr = operation->Initialize(endResult, root_, callback);
    TestSession::FailTestIf(FAILED(hr), "operation->Initialize should not fail");
    return ComAsyncOperationContext::StartAndDetach<ComCompletedAsyncOperationContext>(std::move(operation), context);
}

HRESULT ComTestStoreService::EndClose( 
    /* [in] */ IFabricAsyncOperationContext *context)
{
    if (context == NULL) { return E_POINTER; }

    CheckForReportFaultsAndDelays(testStoreService_.GetPartition(), ApiFaultHelper::Service, L"EndClose");

    return ComCompletedAsyncOperationContext::End(context);
}

void STDMETHODCALLTYPE ComTestStoreService::Abort()
{
    CheckForReportFaultsAndDelays(testStoreService_.GetPartition(), ApiFaultHelper::Service, L"abort");
    
    testStoreService_.OnAbort();
}

HRESULT STDMETHODCALLTYPE ComTestStoreService::BeginUpdateEpoch( 
    /* [in] */ FABRIC_EPOCH const * epoch,
    /* [in] */ FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (epoch == NULL || callback == NULL || context == NULL) { return E_POINTER; }
   
    WAIT_FOR_SIGNAL_RESET(StateProviderBeginUpdateEpochBlock) 
    CheckForReportFaultsAndDelays(testStoreService_.GetPartition(), ApiFaultHelper::Provider, L"BeginUpdateEpoch");

    if (testStoreService_.ShouldFailOn(ApiFaultHelper::Provider, L"BeginUpdateEpoch")) return E_FAIL;

    HRESULT hr;
    if (testStoreService_.ShouldFailOn(ApiFaultHelper::Provider, L"EndUpdateEpoch")) 
    {
        hr = E_FAIL;
    }
    else
    {
        hr = testStoreService_.UpdateEpoch(epoch, previousEpochLastSequenceNumber);
    }

    ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();
    hr = operation->Initialize(hr, root_, callback);
    TestSession::FailTestIf(FAILED(hr), "operation->Initialize should not fail");
    return ComAsyncOperationContext::StartAndDetach<ComCompletedAsyncOperationContext>(std::move(operation), context);
}

HRESULT ComTestStoreService::EndUpdateEpoch( 
    /* [in] */ IFabricAsyncOperationContext *context)
{
    if (context == NULL) { return E_POINTER; }

    WAIT_FOR_SIGNAL_RESET(StateProviderEndUpdateEpochBlock) 
    CheckForReportFaultsAndDelays(testStoreService_.GetPartition(), ApiFaultHelper::Provider, L"EndUpdateEpoch");
 
    return ComCompletedAsyncOperationContext::End(context);
}

HRESULT ComTestStoreService::GetLastCommittedSequenceNumber( 
    /* [retval][out] */ FABRIC_SEQUENCE_NUMBER *sequenceNumber)
{
    if (sequenceNumber == NULL) { return E_POINTER; }

    CheckForReportFaultsAndDelays(testStoreService_.GetPartition(), ApiFaultHelper::Provider, L"GetLastCommittedSequenceNumber");
 
    if (testStoreService_.ShouldFailOn(ApiFaultHelper::Provider, L"GetLastCommittedSequenceNumber")) return E_FAIL;
    
    HRESULT result = testStoreService_.GetCurrentProgress(sequenceNumber);
    if (isDecrementSequenceNumberNeeded_ && *sequenceNumber > FABRIC_INVALID_SEQUENCE_NUMBER)
    {
        (*sequenceNumber)--;
        isDecrementSequenceNumberNeeded_ = false;
    }

    return result;
}

//The test does not recover from data loss
HRESULT ComTestStoreService::BeginOnDataLoss( 
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL) { return E_POINTER; }

    CheckForReportFaultsAndDelays(testStoreService_.GetPartition(), ApiFaultHelper::Provider, L"BeginOnDataLoss");
 
    if (testStoreService_.ShouldFailOn(ApiFaultHelper::Provider, L"BeginOnDataLoss")) return E_FAIL;

    if (testStoreService_.ShouldFailOn(ApiFaultHelper::Provider, L"EndOnDataLoss")) 
    {
        ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();
        HRESULT hr = operation->Initialize(E_FAIL, ApiFaultHelper::Get().GetAsyncCompleteDelayTime(), root_, callback);
        TestSession::FailTestIf(FAILED(hr), "operation->Initialize should not fail");
        return ComAsyncOperationContext::StartAndDetach<ComCompletedAsyncOperationContext>(std::move(operation), context);
    }
    else
    {
        return testStoreService_.BeginOnDataLoss(callback, context);
    }
}

HRESULT ComTestStoreService::EndOnDataLoss( 
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][string][out] */ BOOLEAN * isStateChanged)
{
    if (context == NULL || isStateChanged == NULL) { return E_POINTER; }
 
    CheckForReportFaultsAndDelays(testStoreService_.GetPartition(), ApiFaultHelper::Provider, L"EndOnDataLoss");

    auto castedContext = dynamic_cast<ComCompletedAsyncOperationContext*>(context);
    if(castedContext == nullptr)
    {
      return testStoreService_.EndOnDataLoss(context, isStateChanged);
    }
    else
    {
        if (testStoreService_.IsSignalSet(ProviderStateChangedOnDataLoss)) 
        {
            *isStateChanged = TRUE;
        }
        else if (testStoreService_.IsSignalSet(ProviderStateChangedOnDataLossDecrementProgress)) 
        {
            *isStateChanged = TRUE;
            isDecrementSequenceNumberNeeded_ = true;
        }

        return ComCompletedAsyncOperationContext::End(context);
    }
}

HRESULT ComTestStoreService::GetCopyContext(
    /*[out, retval]*/ IFabricOperationDataStream ** operationDataAsyncEnumerator)
{
    if (operationDataAsyncEnumerator == NULL) { return E_POINTER; }

    CheckForReportFaultsAndDelays(testStoreService_.GetPartition(), ApiFaultHelper::Provider, L"GetCopyContext");
 
    if (testStoreService_.ShouldFailOn(ApiFaultHelper::Provider, L"GetCopyContext")) return E_FAIL;
    return testStoreService_.GetCopyContext(operationDataAsyncEnumerator);
}

HRESULT ComTestStoreService::GetCopyState(
    /*[in]*/ FABRIC_SEQUENCE_NUMBER uptoSequenceNumber,
    /*[in]*/ IFabricOperationDataStream * operationDataAsyncEnumerator,
    /*[out, retval]*/ IFabricOperationDataStream ** copyStateEnumerator)
{
    if (copyStateEnumerator == NULL) { return E_POINTER; }

    CheckForReportFaultsAndDelays(testStoreService_.GetPartition(), ApiFaultHelper::Provider, L"GetCopyState");
 
    if (testStoreService_.ShouldFailOn(ApiFaultHelper::Provider, L"GetCopyState")) return E_FAIL;
    return testStoreService_.GetCopyState(uptoSequenceNumber, operationDataAsyncEnumerator, copyStateEnumerator);
}
