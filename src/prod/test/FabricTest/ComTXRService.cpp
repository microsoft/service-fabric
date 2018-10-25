// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Common;
using namespace FabricTest;
using namespace std;

void ComTXRService::CheckForReportFaultsAndDelays(ComPointer<IFabricStatefulServicePartition> partition, ApiFaultHelper::ComponentName compName, std::wstring operationName)
{
    if (innerService_.ShouldFailOn(compName, operationName, ApiFaultHelper::ReportFaultPermanent))          
    {
        if (partition.GetRawPointer() != nullptr) 
        {
            partition->ReportFault(FABRIC_FAULT_TYPE::FABRIC_FAULT_TYPE_PERMANENT);
        } 
    };                                                                                          
    if (innerService_.ShouldFailOn(compName, operationName, ApiFaultHelper::ReportFaultTransient))          
    {     
        if (partition.GetRawPointer() != nullptr) 
        {
            partition->ReportFault(FABRIC_FAULT_TYPE::FABRIC_FAULT_TYPE_TRANSIENT);
        }
    };                                                                                          
    if (innerService_.ShouldFailOn(compName, operationName, ApiFaultHelper::Delay))               
    {         
        ::Sleep(static_cast<DWORD>(ApiFaultHelper::Get().GetApiDelayInterval().TotalMilliseconds()));
    }
}

ComPointer<IFabricStateProvider2Factory> ComTXRService::CreateStateProvider2Factory(
    __in std::wstring const & nodeId,
    __in std::wstring const & serviceName,
    __in KAllocator & allocator)
{
    return ComTestStateProvider2Factory::Create(
        nodeId,
        serviceName,
        allocator);
}

ComPointer<IFabricDataLossHandler> ComTXRService::CreateDataLossHandler(
    __in KAllocator & allocator,
    __out TxnReplicator::TestCommon::TestDataLossHandler::SPtr & dataLossHandler)
{
    // Using the DataLoss Handler in TxnReplicator::TestCommon::TestDataLossHandler, set the restore path and policy here.
    wstring currentFolder = Common::Directory::GetCurrentDirectoryW();
    KString::SPtr backupFolderPath = Data::KPath::CreatePath(currentFolder.c_str(), allocator);
    KPath::CombineInPlace(*backupFolderPath, L"BackupRestoreTest");

    dataLossHandler = TxnReplicator::TestCommon::TestDataLossHandler::Create(
        allocator,
        backupFolderPath.RawPtr(),
        FABRIC_RESTORE_POLICY_SAFE);

    return TxnReplicator::TestCommon::TestComProxyDataLossHandler::Create(
        allocator,
        dataLossHandler.RawPtr());
}

#define WAIT_FOR_SIGNAL_RESET(a) \
    innerService_.WaitForSignalReset(a); \

ComTXRService::ComTXRService(TXRService & innerService)
    : root_(innerService.shared_from_this())
    , innerService_(innerService)
    , primaryReplicator_()
    , isClosedCalled_(false)
    , isDecrementSequenceNumberNeeded_(false)
{
}

HRESULT ComTXRService::BeginOpen( 
    /* [in] */ FABRIC_REPLICA_OPEN_MODE openMode,
    /* [in] */ IFabricStatefulServicePartition *statefulServicePartition,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    UNREFERENCED_PARAMETER(openMode);
    HRESULT hr = E_FAIL;

    if (statefulServicePartition == NULL || callback == NULL || context == NULL) { return E_POINTER; }

    ComPointer<IFabricStatefulServicePartition2> partition;
    ComPointer<IFabricInternalStatefulServicePartition> internalPartition;

    ComPointer<IFabricStatefulServicePartition> tempPartition;
    partition.SetAndAddRef((IFabricStatefulServicePartition2*)statefulServicePartition);
    tempPartition.SetAndAddRef(statefulServicePartition);

    CheckForReportFaultsAndDelays(tempPartition, ApiFaultHelper::Service, L"BeginOpen");

    if (innerService_.ShouldFailOn(ApiFaultHelper::Service, L"BeginOpen")) 
    {
        return E_FAIL;
    }

    hr = statefulServicePartition->QueryInterface(IID_IFabricInternalStatefulServicePartition, internalPartition.VoidInitializationAddress());
    TestSession::FailTestIfNot(hr == S_OK, "TxnReplicator interface QI did not return success");

    ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();

    if (innerService_.ShouldFailOn(ApiFaultHelper::Service, L"EndOpen")) 
    {
        hr = operation->Initialize(E_FAIL, ApiFaultHelper::Get().GetAsyncCompleteDelayTime(), root_, callback);
    }
    else
    {
        Common::ScopedHeap heap;
        FABRIC_REPLICATOR_SETTINGS replicatorSettings = { 0 };
        FABRIC_REPLICATOR_SETTINGS_EX1 replicatorSettingsEx1 = { 0 };
        FABRIC_REPLICATOR_SETTINGS_EX2 replicatorSettingsEx2 = { 0 };
        FABRIC_REPLICATOR_SETTINGS_EX3 replicatorSettingsEx3 = { 0 };

        replicatorSettings.Reserved = &replicatorSettingsEx1;
        replicatorSettingsEx1.Reserved = &replicatorSettingsEx2;
        replicatorSettingsEx2.Reserved = &replicatorSettingsEx3;
        replicatorSettingsEx3.Reserved = NULL;

        TRANSACTIONAL_REPLICATOR_SETTINGS txnReplicatorSettings = { 0 };
        TransactionalReplicatorSettingServiceInitDataParser trsParser(innerService_.InitDataString);
        auto inputTxReplicatorSettings = trsParser.CreateTransactionalReplicatorSettings(innerService_.InitDataString);
        inputTxReplicatorSettings->ToPublicApi(txnReplicatorSettings);

        ReplicatorSettingServiceInitDataParser rsParser(innerService_.InitDataString);
        auto inputReplicatorSettings = rsParser.CreateReplicatorSettingsForTransactionalReplicator(
            innerService_.InitDataString,
            innerService_.GetPartitionId());

        inputReplicatorSettings->ToPublicApi(heap, replicatorSettings);

        HANDLE txReplicator = nullptr;
        TxnReplicator::ITransactionalReplicator::SPtr txReplicatorSPtr;

        // Get the KtlSystem from the system.
        void * ktlSystemRaw;
        hr = internalPartition->GetKtlSystem(&ktlSystemRaw);
        TestSession::FailTestIfNot(SUCCEEDED(hr), "GetKtlSystem did not return success");

        // Note: since we do not run in kernel today, using paged allocator since non-paged is limited.
        KtlSystem * ktlSystem = static_cast<KtlSystem *>(ktlSystemRaw);
        TestSession::FailTestIf(ktlSystem == nullptr, "GetKtlSystem is a nullptr");

        KAllocator & allocator = ktlSystem->PagedAllocator();
        
        Data::TestCommon::TestComCodePackageActivationContext::SPtr runtimeFolders = Data::TestCommon::TestComCodePackageActivationContext::Create(innerService_.WorkDirectory.c_str(), allocator);

        ComPointer<IFabricStateProvider2Factory> comFactory = CreateStateProvider2Factory(
            innerService_.NodeId.ToString(), 
            innerService_.ServiceName, 
            allocator);

        TxnReplicator::TestCommon::TestDataLossHandler::SPtr dataLossHandler = nullptr;
        ComPointer<IFabricDataLossHandler> comDataLossHandler = CreateDataLossHandler(allocator, dataLossHandler);

        hr = internalPartition->CreateTransactionalReplicatorInternal(
            runtimeFolders.RawPtr(),
            comFactory.GetRawPointer(),
            comDataLossHandler.GetRawPointer(),
            &replicatorSettings,
            &txnReplicatorSettings,
            NULL,
            primaryReplicator_.InitializationAddress(),
            &txReplicator);

        TestSession::FailTestIfNot(hr == S_OK, "CreateTxReplicator did not return success");
        TestSession::FailTestIfNot((bool) primaryReplicator_, "primaryreplicator is null");
        TestSession::FailTestIf(txReplicator == nullptr, "txreplicator is null");

        txReplicatorSPtr.Attach((TxnReplicator::ITransactionalReplicator *)txReplicator);

        ErrorCode error = innerService_.OnOpen(
            tempPartition,
            *txReplicatorSPtr,
            primaryReplicator_,
            allocator);

        TestSession::FailTestIfNot(error.IsSuccess(), "innerService_.OnOpen failed with error {0}", error);

        dataLossHandler->Initialize(*txReplicatorSPtr);
        hr = operation->Initialize(root_, callback);
    }

    TestSession::FailTestIf(FAILED(hr), "operation->Initialize should not fail");
    return ComAsyncOperationContext::StartAndDetach<ComCompletedAsyncOperationContext>(std::move(operation), context);
}

HRESULT ComTXRService::EndOpen(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [out][retval] */ IFabricReplicator **replicationEngine)
{
    if (context == NULL || replicationEngine == NULL) { return E_POINTER; }

    CheckForReportFaultsAndDelays(innerService_.GetPartition(), ApiFaultHelper::Service, L"EndOpen");
 
    HRESULT hr = ComCompletedAsyncOperationContext::End(context);
    if (FAILED(hr)) { return hr; }
    *replicationEngine = primaryReplicator_.DetachNoRelease();
    return hr;
}

HRESULT ComTXRService::BeginChangeRole( 
    /* [in] */ FABRIC_REPLICA_ROLE newRole,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (callback == NULL || context == NULL) { return E_POINTER; }

    WAIT_FOR_SIGNAL_RESET(ServiceBeginChangeRoleBlock)
    CheckForReportFaultsAndDelays(innerService_.GetPartition(), ApiFaultHelper::Service, L"BeginChangeRole");

    if (innerService_.ShouldFailOn(ApiFaultHelper::Service, L"BeginChangeRole")) return E_FAIL;

    ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();
    HRESULT hr = E_FAIL;
    if (innerService_.ShouldFailOn(ApiFaultHelper::Service, L"EndChangeRole")) 
    {
        hr = operation->Initialize(E_FAIL, ApiFaultHelper::Get().GetAsyncCompleteDelayTime(), root_, callback);
    }
    else
    {
        ErrorCode error = innerService_.OnChangeRole(newRole);
        TestSession::FailTestIfNot(error.IsSuccess(), "innerService_.OnChangeRole failed with error {0}", error);

        hr = operation->Initialize(root_, callback);
    }

    TestSession::FailTestIf(FAILED(hr), "operation->Initialize should not fail");
    return ComAsyncOperationContext::StartAndDetach<ComCompletedAsyncOperationContext>(std::move(operation), context);
}

HRESULT ComTXRService::EndChangeRole( 
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricStringResult **serviceEndpoint)
{
    if (context == NULL || serviceEndpoint == NULL) { return E_POINTER; }

    CheckForReportFaultsAndDelays(innerService_.GetPartition(), ApiFaultHelper::Service, L"endchangerole");

    HRESULT hr = ComCompletedAsyncOperationContext::End(context);
    if (FAILED(hr)) return hr;

    ComPointer<IFabricStringResult> stringResult = make_com<ComStringResult,IFabricStringResult>(innerService_.GetServiceLocation());
    *serviceEndpoint = stringResult.DetachNoRelease();
    return S_OK;
}

HRESULT ComTXRService::BeginClose( 
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL) { return E_POINTER; }

    CheckForReportFaultsAndDelays(innerService_.GetPartition(), ApiFaultHelper::Service, L"BeginClose");

    HRESULT beginResult = innerService_.ShouldFailOn(ApiFaultHelper::Service, L"BeginClose") ? E_FAIL : S_OK;
    HRESULT endResult = innerService_.ShouldFailOn(ApiFaultHelper::Service, L"EndClose") ? E_FAIL : S_OK;

    if(!isClosedCalled_)
    {
        isClosedCalled_ = true;
        ErrorCode error = innerService_.OnClose();
        TestSession::FailTestIfNot(error.IsSuccess(), "innerService_.OnClose failed with error {0}", error);
    }

    if(FAILED(beginResult)) return E_FAIL;

    ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();
    HRESULT hr = operation->Initialize(endResult, root_, callback);
    TestSession::FailTestIf(FAILED(hr), "operation->Initialize should not fail");
    return ComAsyncOperationContext::StartAndDetach<ComCompletedAsyncOperationContext>(std::move(operation), context);
}

HRESULT ComTXRService::EndClose( 
    /* [in] */ IFabricAsyncOperationContext *context)
{
    if (context == NULL) { return E_POINTER; }

    CheckForReportFaultsAndDelays(innerService_.GetPartition(), ApiFaultHelper::Service, L"EndClose");

    return ComCompletedAsyncOperationContext::End(context);
}

void ComTXRService::Abort()
{
    CheckForReportFaultsAndDelays(innerService_.GetPartition(), ApiFaultHelper::Service, L"abort");
    
    innerService_.OnAbort();
}
