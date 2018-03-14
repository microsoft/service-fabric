// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Api;
using namespace Common;
using namespace FabricTest;
using namespace std;

StringLiteral const TraceSource("ComTestPersistedService");

class ComTestPersistedStoreService::ComOpenOperationContext : public Common::ComAsyncOperationContext
{
    DENY_COPY(ComOpenOperationContext);

public:
    ComOpenOperationContext(
        __in IStatefulServiceReplica & replica,
        __in FABRIC_REPLICA_OPEN_MODE openMode,
        __in Common::ComPointer<IFabricStatefulServicePartition> && partition) 
        : ComAsyncOperationContext(true)
        , replica_(replica) 
        , openMode_(move(openMode))
        , partition_(partition)
        , replicatorResult_()
    { 
    }

    HRESULT STDMETHODCALLTYPE End(__out ComPointer<IFabricReplicator> & result)
    {
        if (SUCCEEDED(this->Result))
        {
            result = this->replicatorResult_;
        }

        return this->Result;
    }

protected:
    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = replica_.BeginOpen(
            openMode_, 
            partition_,
            [this](AsyncOperationSPtr const & operation) { this->OnOpenComplete(operation, false); },
            proxySPtr);
        this->OnOpenComplete(operation, true);
    }

private:

    void OnOpenComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto error = replica_.EndOpen(operation, replicatorResult_);

        this->TryComplete(operation->Parent, error);
    }

    IStatefulServiceReplica & replica_;
    FABRIC_REPLICA_OPEN_MODE openMode_;
    Common::ComPointer<IFabricStatefulServicePartition> partition_; 
    Common::ComPointer<IFabricReplicator> replicatorResult_;
};

class ComTestPersistedStoreService::ComChangeRoleOperationContext : public Common::ComAsyncOperationContext
{
    DENY_COPY(ComChangeRoleOperationContext);

public:
    ComChangeRoleOperationContext(
        __in IStatefulServiceReplica & replica,
        __in FABRIC_REPLICA_ROLE role)
        : ComAsyncOperationContext(true)
        , replica_(replica) 
        , role_(role)
        , serviceLocation_()
    { 
    }

    HRESULT STDMETHODCALLTYPE End(__out wstring & serviceLocation)
    {
        if (SUCCEEDED(this->Result))
        {
            serviceLocation = serviceLocation_;
        }

        return this->Result;
    }

protected:
    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = replica_.BeginChangeRole(
            role_, 
            [this](AsyncOperationSPtr const & operation) { this->OnChangeRoleComplete(operation, false); },
            proxySPtr);
        this->OnChangeRoleComplete(operation, true);
    }

private:

    void OnChangeRoleComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto error = replica_.EndChangeRole(operation, serviceLocation_);

        this->TryComplete(operation->Parent, error);
    }

    IStatefulServiceReplica & replica_;
    FABRIC_REPLICA_ROLE role_;
    wstring serviceLocation_;
};

class ComTestPersistedStoreService::ComCloseOperationContext : public Common::ComAsyncOperationContext
{
    DENY_COPY(ComCloseOperationContext);

public:
    ComCloseOperationContext(
        __in IStatefulServiceReplica & replica)
        : ComAsyncOperationContext(true)
        , replica_(replica) 
    { 
    }

    HRESULT STDMETHODCALLTYPE End()
    {
        return this->Result;
    }

protected:
    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = replica_.BeginClose(
            [this](AsyncOperationSPtr const & operation) { this->OnCloseComplete(operation, false); },
            proxySPtr);
        this->OnCloseComplete(operation, true);
    }

private:

    void OnCloseComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto error = replica_.EndClose(operation);

        this->TryComplete(operation->Parent, error);
    }

    IStatefulServiceReplica & replica_;
};

class ComTestPersistedStoreService::ComOnDataLossOperationContext : public Common::ComAsyncOperationContext
{
    DENY_COPY(ComOnDataLossOperationContext);

public:
    ComOnDataLossOperationContext()
        : ComAsyncOperationContext(true)
        , isStateChanged_(false)
    { 
    }

    HRESULT STDMETHODCALLTYPE End(__out bool & result)
    {
        if (SUCCEEDED(this->Result))
        {
            result = isStateChanged_;
        }

        return this->Result;
    }

protected:
    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        this->TryComplete(proxySPtr, ErrorCodeValue::Success);
    }

private:

    bool isStateChanged_;
};

//
// ComTestPersistedStoreService
//

ComTestPersistedStoreService::ComTestPersistedStoreService(__in TestPersistedStoreService & service)
    : root_(service.CreateComponentRoot())
    , service_(service)
    , isClosedCalled_(false)
    , isDecrementSequenceNumberNeeded_(false)
    , role_(FABRIC_REPLICA_ROLE_UNKNOWN)
{
    TestSession::WriteNoise(TraceSource, "{0} ctor", service_.TraceId);

    service_.SetStateProvider(this);
}

ComTestPersistedStoreService::~ComTestPersistedStoreService()
{
    TestSession::WriteNoise(TraceSource, "{0} ~dtor", service_.TraceId);
}

void ComTestPersistedStoreService::CheckForReportFaultsAndDelays(ComPointer<IFabricStatefulServicePartition> partition, ApiFaultHelper::ComponentName compName, std::wstring operationName)
{
    if (service_.ShouldFailOn(compName, operationName, ApiFaultHelper::ReportFaultPermanent))          
    {
        if (partition.GetRawPointer() != nullptr) 
        {
            partition->ReportFault(FABRIC_FAULT_TYPE::FABRIC_FAULT_TYPE_PERMANENT);
        } 
    };                                                                                          
    if (service_.ShouldFailOn(compName, operationName, ApiFaultHelper::ReportFaultTransient))          
    {     
        if (partition.GetRawPointer() != nullptr) 
        {
            partition->ReportFault(FABRIC_FAULT_TYPE::FABRIC_FAULT_TYPE_TRANSIENT);
        }
    };                                                                                          
    if (service_.ShouldFailOn(compName, operationName, ApiFaultHelper::Delay))               
    {         
        ::Sleep(static_cast<DWORD>(ApiFaultHelper::Get().GetApiDelayInterval().TotalMilliseconds()));
    }
}

#define WAIT_FOR_SIGNAL_RESET(a)                                                        \
    service_.WaitForSignalReset(a);                                                     \

HRESULT STDMETHODCALLTYPE ComTestPersistedStoreService::BeginOpen(
    __in FABRIC_REPLICA_OPEN_MODE openMode,
    __in IFabricStatefulServicePartition * partition, 
    __in IFabricAsyncOperationCallback * callback,
    __out IFabricAsyncOperationContext ** context)
{
    if (partition == NULL || context == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

    if (service_.IsSignalSet(ServiceBeginOpenExpectNewOpenMode))
    {
        TestSession::FailTestIfNot(openMode == FABRIC_REPLICA_OPEN_MODE::FABRIC_REPLICA_OPEN_MODE_NEW, "Wrong open mode is passed in");
    }
    else if (service_.IsSignalSet(ServiceBeginOpenExpectExistingOpenMode))
    {
        TestSession::FailTestIfNot(openMode == FABRIC_REPLICA_OPEN_MODE::FABRIC_REPLICA_OPEN_MODE_EXISTING, "Wrong open mode is passed in");
    }

    ComPointer<IFabricStatefulServicePartition> partitionCPtr(partition, IID_IFabricStatefulServicePartition);
    readWriteStatusValidator_ = make_unique<ReadWriteStatusValidator>(partitionCPtr);
    readWriteStatusValidator_->OnOpen();

    WAIT_FOR_SIGNAL_RESET(ServiceBeginOpenBlock)
    CheckForReportFaultsAndDelays(partitionCPtr, ApiFaultHelper::Service, L"BeginOpen");

    if (service_.ShouldFailOn(ApiFaultHelper::Service, L"BeginOpen"))
    {
        // to test the fault in begin open
        DWORD threadId = 0;
        ::FabricSetLastErrorMessage(L"testerror", &threadId);
        return E_FAIL;
    }

    if (service_.ShouldFailOn(ApiFaultHelper::Service, L"EndOpen")) 
    {
        ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();
        HRESULT hr = operation->Initialize(E_FAIL, ApiFaultHelper::Get().GetAsyncCompleteDelayTime(), root_, callback);
        TestSession::FailTestIf(FAILED(hr), "operation->Initialize should not fail");
        return ComAsyncOperationContext::StartAndDetach<ComCompletedAsyncOperationContext>(std::move(operation), context);
    }
    else
    {
        auto openContext = make_com<ComOpenOperationContext, ComAsyncOperationContext>(
            service_,
            openMode,
            move(partitionCPtr));

        return ComUtility::OnPublicApiReturn(InitializeAndStart(openContext, callback, context));
    }
}

HRESULT STDMETHODCALLTYPE ComTestPersistedStoreService::EndOpen(
    __in IFabricAsyncOperationContext * context,
    __out IFabricReplicator ** replication)
{
    if (context == NULL || replication == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

    WAIT_FOR_SIGNAL_RESET(ServiceEndOpenBlock)
        CheckForReportFaultsAndDelays(service_.GetPartition(), ApiFaultHelper::Service, L"EndOpen");
    
    auto castedContext = dynamic_cast<ComCompletedAsyncOperationContext*>(context);
    if (castedContext != nullptr)
    {
        return ComCompletedAsyncOperationContext::End(context);
    }
    else
    {
        auto openContext = dynamic_cast<ComOpenOperationContext*>(context);
        TestSession::FailTestIf(openContext == nullptr, "ComOpenOperationContext cast failed");

        ComPointer<IFabricReplicator> replicationCPtr;
        auto hr = openContext->End(replicationCPtr);

//LINUXTODO
#if !defined(PLATFORM_UNIX)
        if(hr == ErrorCodeValue::StoreInUse)
        {
            TestSession::WriteNoise(TraceSource, "[{0}:{1}] EndOpen failed with StoreInUse. Retrying....", service_.PartitionId, service_.ReplicaId);
            ErrorCode error;
            TimeoutHelper timeoutHelper(TimeSpan::FromSeconds(30));
            do
            {
                Sleep(2000);
                wstring dbDirectory = reinterpret_cast<Store::EseReplicatedStore *>(service_.ReplicatedStorePrivate)->Directory;

                TestSession::FailTestIfNot(Directory::Exists(dbDirectory), "ESE dir should exist since open failed with StoreInUse");
                unique_ptr<Store::EseLocalStore> store = make_unique<Store::EseLocalStore>(
                    dbDirectory, 
                    TestPersistedStoreService::EseDatabaseFilename,
                    Store::EseLocalStore::LOCAL_STORE_FLAG_USE_LSN_COLUMN);

                error = store->Initialize(service_.ReplicatedStorePrivate->LocalStoreInstanceName);
                TestSession::WriteNoise(TraceSource, "[{0}:{1}] Store initialize retry completed with {2}", service_.PartitionId, service_.ReplicaId, error);
            }
            while(error.ReadValue() == ErrorCodeValue::StoreInUse && !timeoutHelper.IsExpired);

            TestSession::FailTestIf(error.ReadValue() == ErrorCodeValue::StoreInUse, "Test persisted service open failed with StoreInUse on retry");
        }
#endif

        if (!SUCCEEDED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

        ComPointer<ComTestReplicator> testReplicator = make_com<ComTestReplicator>(replicationCPtr, service_.GetPartition(), service_.DisableReplicatorCatchupSpecificReplica, service_.ServiceName, service_.NodeId);
        hr = testReplicator->QueryInterface(IID_IFabricReplicator, reinterpret_cast<void**>(replication));
        return ComUtility::OnPublicApiReturn(hr);
    }
}


HRESULT STDMETHODCALLTYPE ComTestPersistedStoreService::BeginChangeRole(
    __in ::FABRIC_REPLICA_ROLE newRole,
    __in IFabricAsyncOperationCallback * callback,
    __out IFabricAsyncOperationContext ** context)
{
    if (context == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }
    readWriteStatusValidator_->OnChangeRole(newRole);

    WAIT_FOR_SIGNAL_RESET(ServiceBeginChangeRoleBlock)
        CheckForReportFaultsAndDelays(service_.GetPartition(), ApiFaultHelper::Service, L"beginchangerole");
    
    if (service_.ShouldFailOn(ApiFaultHelper::Service, L"beginchangerole")) return E_FAIL;

    if (service_.IsSignalSet(ServiceBeginChangeRoleEnableSecondaryPump))
    {
        service_.ResetSignal(StateProviderSecondaryPumpBlock);
    }

    if (service_.ShouldFailOn(ApiFaultHelper::Service, L"endchangerole")) 
    {
        ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();
        HRESULT hr = operation->Initialize(E_FAIL, ApiFaultHelper::Get().GetAsyncCompleteDelayTime(), root_, callback);
        TestSession::FailTestIf(FAILED(hr), "operation->Initialize should not fail");
        return ComAsyncOperationContext::StartAndDetach<ComCompletedAsyncOperationContext>(std::move(operation), context);
    }    
    else
    {
        role_ = newRole;
        auto changeRoleContext = make_com<ComChangeRoleOperationContext, ComAsyncOperationContext>(service_, role_);
        return ComUtility::OnPublicApiReturn(InitializeAndStart(changeRoleContext, callback, context));
    }
}

HRESULT STDMETHODCALLTYPE ComTestPersistedStoreService::EndChangeRole(
    __in IFabricAsyncOperationContext * context,
    __out IFabricStringResult ** serviceLocation)
{
    if (context == NULL || serviceLocation == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

    WAIT_FOR_SIGNAL_RESET(ServiceEndChangeRoleBlock)
    CheckForReportFaultsAndDelays(service_.GetPartition(), ApiFaultHelper::Service, L"endchangerole");
    
    auto castedContext = dynamic_cast<ComCompletedAsyncOperationContext*>(context);
    if(castedContext != nullptr)
    {
        return ComCompletedAsyncOperationContext::End(context);
    }
    else
    {
        auto changeRoleContext = dynamic_cast<ComChangeRoleOperationContext*>(context);
        TestSession::FailTestIf(changeRoleContext == nullptr, "ComChangeRoleOperationContext cast failed");

        std::wstring changeRoleResult;
        auto hr = changeRoleContext->End(changeRoleResult);
        if (!SUCCEEDED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

        std::wstring currentLocation;
        if (!service_.ShouldFailOn(ApiFaultHelper::Service, L"endchangerole", ApiFaultHelper::SetLocationWhenActive) ||
            role_ == FABRIC_REPLICA_ROLE::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY || role_ == FABRIC_REPLICA_ROLE::FABRIC_REPLICA_ROLE_PRIMARY)
        {
            currentLocation = changeRoleResult;
        }
        
        ComPointer<IFabricStringResult> stringResult = make_com<ComStringResult, IFabricStringResult>(currentLocation);
        hr = stringResult->QueryInterface(IID_IFabricStringResult, reinterpret_cast<void**>(serviceLocation));
        return ComUtility::OnPublicApiReturn(hr);
    }
}

HRESULT STDMETHODCALLTYPE ComTestPersistedStoreService::BeginClose(
    __in IFabricAsyncOperationCallback * callback,
    __out IFabricAsyncOperationContext ** context)
{
    if (context == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

    service_.SetStateProvider(nullptr);

    readWriteStatusValidator_->OnClose();

    WAIT_FOR_SIGNAL_RESET(ServiceBeginCloseBlock)
    CheckForReportFaultsAndDelays(service_.GetPartition(), ApiFaultHelper::Service, L"BeginClose");
    
    if (service_.IsSignalSet(ServiceBeginCloseEnableSecondaryPump))
    {
        service_.ResetSignal(StateProviderSecondaryPumpBlock);
    }

    HRESULT beginResult = service_.ShouldFailOn(ApiFaultHelper::Service, L"BeginClose") ? E_FAIL : S_OK;

    if(!isClosedCalled_)
    {
        isClosedCalled_ = true;

        if(FAILED(beginResult))
        {
            // Do sync cleanup
            shared_ptr<ManualResetEvent> resetEvent = make_shared<ManualResetEvent>(false);
            auto testCallback = make_com<ComAsyncOperationCallbackTestHelper>(
                [resetEvent](IFabricAsyncOperationContext *)
            {
                resetEvent->Set();
            });

            auto closeContext = make_com<ComCloseOperationContext, ComAsyncOperationContext>(service_);
            HRESULT hr = InitializeAndStart(closeContext, testCallback.GetRawPointer(), context);

            TestSession::FailTestIfNot(SUCCEEDED(hr), "service_.BeginClose InitializeAndStart failed. hresult = {0}", hr);
            // 5 minutes is a reasonable high timeout for close to complete. If it does not finish in 5 minutes something might be stuck
            TimeSpan closeTimeout(TimeSpan::FromMinutes(5));
            TestSession::FailTestIfNot(resetEvent->WaitOne(closeTimeout), "Wait for service_.BeginClose to end returned false");
            hr = closeContext->End();
            TestSession::FailTestIfNot(SUCCEEDED(hr), "contextCPtr->End failed. hresult = {0}", hr);
            return E_FAIL;
        }
        else
        {
            auto closeContext = make_com<ComCloseOperationContext, ComAsyncOperationContext>(service_);
            return ComUtility::OnPublicApiReturn(InitializeAndStart(closeContext, callback, context));
        }
    }

    ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();
    HRESULT hr = operation->Initialize(root_, callback);
    TestSession::FailTestIf(FAILED(hr), "operation->Initialize should not fail");
    return ComAsyncOperationContext::StartAndDetach<ComCompletedAsyncOperationContext>(std::move(operation), context);
}

HRESULT STDMETHODCALLTYPE ComTestPersistedStoreService::EndClose(
    __in IFabricAsyncOperationContext * context)
{
    if (context == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

    WAIT_FOR_SIGNAL_RESET(ServiceEndCloseBlock)
    CheckForReportFaultsAndDelays(service_.GetPartition(), ApiFaultHelper::Service, L"EndClose");
    
    auto castedContext = dynamic_cast<ComCompletedAsyncOperationContext*>(context);
    if(castedContext != nullptr)
    {
        return ComCompletedAsyncOperationContext::End(context);
    }
    else
    {
        auto closeContext = dynamic_cast<ComCloseOperationContext*>(context);
        TestSession::FailTestIf(closeContext == nullptr, "ComCloseOperationContext cast failed");

        return service_.ShouldFailOn(ApiFaultHelper::Service, L"EndClose") 
            ? ComUtility::OnPublicApiReturn(E_FAIL) 
            : ComUtility::OnPublicApiReturn(closeContext->End());
    }
}

void STDMETHODCALLTYPE ComTestPersistedStoreService::Abort()
{
    service_.SetStateProvider(nullptr);

    readWriteStatusValidator_->OnAbort();

    WAIT_FOR_SIGNAL_RESET(ServiceAbortBlock)
    CheckForReportFaultsAndDelays(service_.GetPartition(), ApiFaultHelper::Service, L"abort");
    
    if (service_.IsSignalSet(ServiceAbortEnableSecondaryPump))
    {
        service_.ResetSignal(StateProviderSecondaryPumpBlock);
    }

    service_.Abort();
}

// **************
// IFabricStateProvider
// **************

HRESULT STDMETHODCALLTYPE ComTestPersistedStoreService::BeginOnDataLoss(
    __in IFabricAsyncOperationCallback * callback,
    __out IFabricAsyncOperationContext ** context)
{
    if (callback == NULL || context == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

    CheckForReportFaultsAndDelays(service_.GetPartition(), ApiFaultHelper::Provider, L"BeginOnDataLoss");
    
    if (service_.ShouldFailOn(ApiFaultHelper::Provider, L"BeginOnDataLoss")) return E_FAIL;

    if (service_.ShouldFailOn(ApiFaultHelper::Provider, L"EndOnDataLoss")) 
    {
        ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();
        HRESULT hr = operation->Initialize(E_FAIL, ApiFaultHelper::Get().GetAsyncCompleteDelayTime(), root_, callback);
        TestSession::FailTestIf(FAILED(hr), "operation->Initialize should not fail");
        return ComAsyncOperationContext::StartAndDetach<ComCompletedAsyncOperationContext>(std::move(operation), context);
    }
    else
    {
        auto dataLossContext = make_com<ComOnDataLossOperationContext, ComAsyncOperationContext>();
        return ComUtility::OnPublicApiReturn(InitializeAndStart(dataLossContext, callback, context));
    }
}

HRESULT STDMETHODCALLTYPE ComTestPersistedStoreService::EndOnDataLoss(
    __in IFabricAsyncOperationContext * context,
    __out BOOLEAN * isStateChangedResult)
{
    if (context == NULL || isStateChangedResult == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

    CheckForReportFaultsAndDelays(service_.GetPartition(), ApiFaultHelper::Provider, L"EndOnDataLoss");
    
    auto castedContext = dynamic_cast<ComCompletedAsyncOperationContext*>(context);
    if(castedContext != nullptr)
    {
        return ComCompletedAsyncOperationContext::End(context);
    }
    else
    {
        ComAsyncOperationContextCPtr contextCPtr(context, IID_IFabricAsyncOperationContext);
        HRESULT hr = contextCPtr->End();
        if (!SUCCEEDED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

        bool isStateChanged; 
        auto dataLossContext = dynamic_cast<ComOnDataLossOperationContext*>(context);
        hr = dataLossContext->End(isStateChanged);
        if (!SUCCEEDED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

        if (service_.IsSignalSet(ProviderStateChangedOnDataLoss)) 
        {
            *isStateChangedResult = TRUE;
        }
        else if (service_.IsSignalSet(ProviderStateChangedOnDataLossDecrementProgress)) 
        {
            *isStateChangedResult = TRUE;
            isDecrementSequenceNumberNeeded_ = true;
        }
        else
        {
            *isStateChangedResult = (isStateChanged ? TRUE : FALSE);
        }

        return ComUtility::OnPublicApiReturn(hr);
    }
}

HRESULT STDMETHODCALLTYPE ComTestPersistedStoreService::BeginUpdateEpoch( 
    __in ::FABRIC_EPOCH const * epoch,
    __in ::FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber,
    __in IFabricAsyncOperationCallback *callback,
    __out IFabricAsyncOperationContext **context)
{
    UNREFERENCED_PARAMETER(epoch);
    UNREFERENCED_PARAMETER(previousEpochLastSequenceNumber);
    UNREFERENCED_PARAMETER(callback);
    UNREFERENCED_PARAMETER(context);

    Assert::CodingError("Not expected to receive BeginUpdateEpoch at the ComTestPersistedStoreService layer");
}

HRESULT STDMETHODCALLTYPE ComTestPersistedStoreService::EndUpdateEpoch(
    __in IFabricAsyncOperationContext * context)
{
    UNREFERENCED_PARAMETER(context);
    Assert::CodingError("Not expected to receive EndUpdateEpoch at the ComTestPersistedStoreService layer");
}

HRESULT STDMETHODCALLTYPE ComTestPersistedStoreService::GetLastCommittedSequenceNumber(
    __out ::FABRIC_SEQUENCE_NUMBER * sequenceNumber)
{
    UNREFERENCED_PARAMETER(sequenceNumber);
    Assert::CodingError("Not expected to receive GetLastCommittedSequenceNumber at the ComTestPersistedStoreService layer");
}

HRESULT STDMETHODCALLTYPE ComTestPersistedStoreService::GetCopyContext(__out IFabricOperationDataStream ** context)
{
    UNREFERENCED_PARAMETER(context);
    Assert::CodingError("Not expected to receive GetCopyContext at the ComTestPersistedStoreService layer");
}

HRESULT STDMETHODCALLTYPE ComTestPersistedStoreService::GetCopyState(
    __in::FABRIC_SEQUENCE_NUMBER uptoOperationLSN,
    __in IFabricOperationDataStream * context,
    __out IFabricOperationDataStream ** enumerator)
{
    UNREFERENCED_PARAMETER(uptoOperationLSN);
    UNREFERENCED_PARAMETER(context);
    UNREFERENCED_PARAMETER(enumerator);

    Assert::CodingError("Not expected to receive GetCopyState at the ComTestPersistedStoreService layer");
}

HRESULT ComTestPersistedStoreService::InitializeAndStart(
    __in ComAsyncOperationContextCPtr & contextCPtr,
    __in IFabricAsyncOperationCallback * callback,
    __out IFabricAsyncOperationContext ** context)
{
    HRESULT hr = contextCPtr->Initialize(root_->CreateComponentRoot(), callback);
    if (!SUCCEEDED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    hr = contextCPtr->Start(contextCPtr);
    if (!SUCCEEDED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    hr = contextCPtr->QueryInterface(IID_IFabricAsyncOperationContext, reinterpret_cast<void**>(context));
    return ComUtility::OnPublicApiReturn(hr);
}
