// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace TestCommon;
using namespace FabricTest;

#define CHECK_INJECT_FAILURE(a, b) \
    if (service_.ShouldFailOn(a, b)) return E_FAIL

#define CHECK_INJECT_SIGNAL(a) \
    if (service_.IsSignalSet(a)) return E_FAIL

//
// Constructor/Destructor.
//
SGComStatefulService::SGComStatefulService(
    SGStatefulService & service)
    : root_(service.shared_from_this())
    , service_(service)
{
    TestSession::WriteNoise(
        TraceSource, 
        "SGComStatefulService::SGComStatefulService ({0}) - ctor - service({1})",
        this,
        &service);
}

SGComStatefulService::~SGComStatefulService()
{
    TestSession::WriteNoise(
        TraceSource, 
        "SGComStatefulService::~SGComStatefulService ({0}) - dtor",
        this);
}

//
// IFabricStatefulServiceReplica methods. 
//
HRESULT STDMETHODCALLTYPE SGComStatefulService::BeginOpen(
    __in FABRIC_REPLICA_OPEN_MODE openMode,
    __in IFabricStatefulServicePartition* partition,
    __in IFabricAsyncOperationCallback* callback,
    __out IFabricAsyncOperationContext** context)
{
    UNREFERENCED_PARAMETER(openMode);

    if (NULL == partition || NULL == callback || NULL == context) { return E_POINTER; }

    TestSession::WriteNoise(
        TraceSource, 
        "SGComStatefulService::BeginOpen ({0}) - partition({1})",
        this,
        partition);

    CHECK_INJECT_FAILURE(ApiFaultHelper::Service, L"BeginOpen");

    if (service_.ShouldFailOn(ApiFaultHelper::Service, L"BeginOpen", ApiFaultHelper::ReportFaultPermanent))
    {
        auto rfHr = partition->ReportFault(FABRIC_FAULT_TYPE_PERMANENT);
        TestSession::FailTestIf(
            FAILED(rfHr), 
            "SGComStatefulService::BeginOpen ({0}) - ReportFault(FABRIC_FAULT_TYPE_PERMANENT) failed with {1}",
            this,
            rfHr);
    }
    if (service_.ShouldFailOn(ApiFaultHelper::Service, L"BeginOpen", ApiFaultHelper::ReportFaultTransient))
    {
        auto rfHr = partition->ReportFault(FABRIC_FAULT_TYPE_TRANSIENT);
        TestSession::FailTestIf(
            FAILED(rfHr), 
            "SGComStatefulService::BeginOpen ({0}) - ReportFault(FABRIC_FAULT_TYPE_TRANSIENT) failed with {1}",
            this,
            rfHr);
    }

    HRESULT hr = E_FAIL;

    if (!service_.ShouldFailOn(ApiFaultHelper::Service, L"EndOpen"))
    {
        Common::ComPointer<IFabricStatefulServicePartition> statefulPartition; 
        statefulPartition.SetAndAddRef(partition);
        Common::ComPointer<IFabricStateReplicator> stateReplicator;
        hr = statefulPartition->CreateReplicator((IFabricStateProvider*)this, NULL, replicator_.InitializationAddress(), stateReplicator.InitializationAddress());

        hr = service_.OnOpen(std::move(statefulPartition), std::move(stateReplicator));
    }

    ComPointer<SGComCompletedAsyncOperationContext> operation = make_com<SGComCompletedAsyncOperationContext>();
    hr = operation->Initialize(hr, root_, callback);
    TestSession::FailTestIf(FAILED(hr), "operation->Initialize should not fail");
    return ComAsyncOperationContext::StartAndDetach<SGComCompletedAsyncOperationContext>(std::move(operation), context);
}

HRESULT STDMETHODCALLTYPE SGComStatefulService::EndOpen(
    __in IFabricAsyncOperationContext* context,
    __out IFabricReplicator** replicator)
{
    if (NULL == context || NULL == replicator) { return E_POINTER; }
    *replicator = replicator_.DetachNoRelease();

    if (service_.ShouldFailOn(ApiFaultHelper::Service, L"EndOpen", ApiFaultHelper::ReportFaultPermanent))
    {
        auto rfHr = service_.Partition->ReportFault(FABRIC_FAULT_TYPE_PERMANENT);
        TestSession::FailTestIf(
            FAILED(rfHr), 
            "SGComStatefulService::EndOpen ({0}) - ReportFault(FABRIC_FAULT_TYPE_PERMANENT) failed with {1}",
            this,
            rfHr);
    }
    if (service_.ShouldFailOn(ApiFaultHelper::Service, L"EndOpen", ApiFaultHelper::ReportFaultTransient))
    {
        auto rfHr = service_.Partition->ReportFault(FABRIC_FAULT_TYPE_TRANSIENT);
        TestSession::FailTestIf(
            FAILED(rfHr), 
            "SGComStatefulService::EndOpen ({0}) - ReportFault(FABRIC_FAULT_TYPE_TRANSIENT) failed with {1}",
            this,
            rfHr);
    }

    HRESULT hr = SGComCompletedAsyncOperationContext::End(context);

    TestSession::WriteNoise(
        TraceSource, 
        "SGComStatefulService::EndOpen ({0}) - replicator({1})",
        this,
        *replicator);

    return hr;
}

HRESULT STDMETHODCALLTYPE SGComStatefulService::BeginChangeRole(
    __in FABRIC_REPLICA_ROLE newRole,
    __in IFabricAsyncOperationCallback* callback,
    __out IFabricAsyncOperationContext** context)
{
    if (NULL == callback || NULL == context) { return E_POINTER; }

    TestSession::WriteNoise(
        TraceSource, 
        "SGComStatefulService::BeginChangeRole ({0}) - newRole({1})",
        this,
        newRole);

    CHECK_INJECT_FAILURE(ApiFaultHelper::Service, L"beginchangerole");

    if (service_.ShouldFailOn(ApiFaultHelper::Service, L"BeginChangeRole", ApiFaultHelper::ReportFaultPermanent))
    {
        auto rfHr = service_.Partition->ReportFault(FABRIC_FAULT_TYPE_PERMANENT);
        TestSession::FailTestIf(
            FAILED(rfHr), 
            "SGComStatefulService::BeginChangeRole ({0}) - ReportFault(FABRIC_FAULT_TYPE_PERMANENT) failed with {1}",
            this,
            rfHr);
    }
    if (service_.ShouldFailOn(ApiFaultHelper::Service, L"BeginChangeRole", ApiFaultHelper::ReportFaultTransient))
    {
        auto rfHr = service_.Partition->ReportFault(FABRIC_FAULT_TYPE_TRANSIENT);
        TestSession::FailTestIf(
            FAILED(rfHr), 
            "SGComStatefulService::BeginChangeRole ({0}) - ReportFault(FABRIC_FAULT_TYPE_TRANSIENT) failed with {1}",
            this,
            rfHr);
    }

    HRESULT hr = E_FAIL;

    if (!service_.ShouldFailOn(ApiFaultHelper::Service, L"EndChangeRole"))
    {
        hr = service_.OnChangeRole(newRole);
    }

    ComPointer<SGComCompletedAsyncOperationContext> operation = make_com<SGComCompletedAsyncOperationContext>();
    hr = operation->Initialize(hr, root_, callback);
    TestSession::FailTestIf(FAILED(hr), "operation->Initialize should not fail");
    return ComAsyncOperationContext::StartAndDetach<SGComCompletedAsyncOperationContext>(std::move(operation), context);
}

HRESULT STDMETHODCALLTYPE SGComStatefulService::EndChangeRole(
    __in IFabricAsyncOperationContext* context,
    __out IFabricStringResult** serviceEndpoint)
{
    if (NULL == context || NULL == serviceEndpoint) { return E_POINTER; }
    *serviceEndpoint = NULL;

    if (service_.ShouldFailOn(ApiFaultHelper::Service, L"EndChangeRole", ApiFaultHelper::ReportFaultPermanent))
    {
        auto rfHr = service_.Partition->ReportFault(FABRIC_FAULT_TYPE_PERMANENT);
        TestSession::FailTestIf(
            FAILED(rfHr), 
            "SGComStatefulService::EndChangeRole ({0}) - ReportFault(FABRIC_FAULT_TYPE_PERMANENT) failed with {1}",
            this,
            rfHr);
    }
    if (service_.ShouldFailOn(ApiFaultHelper::Service, L"EndChangeRole", ApiFaultHelper::ReportFaultTransient))
    {
        auto rfHr = service_.Partition->ReportFault(FABRIC_FAULT_TYPE_TRANSIENT);
        TestSession::FailTestIf(
            FAILED(rfHr), 
            "SGComStatefulService::EndChangeRole ({0}) - ReportFault(FABRIC_FAULT_TYPE_TRANSIENT) failed with {1}",
            this,
            rfHr);
    }

    HRESULT hr = SGComCompletedAsyncOperationContext::End(context);

    TestSession::WriteNoise(
        TraceSource, 
        "SGComStatefulService::EndChangeRole ({0}) - serviceEndpoint({1})",
        this,
        *serviceEndpoint);

    if (SUCCEEDED(hr))
    {
        ComPointer<IFabricStringResult> serviceName = make_com<ComStringResult, IFabricStringResult>(service_.ServiceName + L"##%%");
        hr = serviceName->QueryInterface(IID_IFabricStringResult, reinterpret_cast<void**>(serviceEndpoint));
        TestSession::FailTestIf(FAILED(hr), "QueryInterface");
    }
    return hr;
}

HRESULT STDMETHODCALLTYPE SGComStatefulService::BeginClose(
    __in IFabricAsyncOperationCallback* callback,
    __out IFabricAsyncOperationContext** context)
{
    if (NULL == callback || NULL == context) { return E_POINTER; }

    TestSession::WriteNoise(
        TraceSource, 
        "SGComStatefulService::BeginClose ({0})",
        this);

    CHECK_INJECT_FAILURE(ApiFaultHelper::Service, L"BeginClose");

    if (service_.ShouldFailOn(ApiFaultHelper::Service, L"BeginClose", ApiFaultHelper::ReportFaultPermanent))
    {
        auto rfHr = service_.Partition->ReportFault(FABRIC_FAULT_TYPE_PERMANENT);
        TestSession::FailTestIf(
            FAILED(rfHr), 
            "SGComStatefulService::BeginClose ({0}) - ReportFault(FABRIC_FAULT_TYPE_PERMANENT) failed with {1}",
            this,
            rfHr);
    }
    if (service_.ShouldFailOn(ApiFaultHelper::Service, L"BeginClose", ApiFaultHelper::ReportFaultTransient))
    {
        auto rfHr = service_.Partition->ReportFault(FABRIC_FAULT_TYPE_TRANSIENT);
        TestSession::FailTestIf(
            FAILED(rfHr), 
            "SGComStatefulService::BeginClose ({0}) - ReportFault(FABRIC_FAULT_TYPE_TRANSIENT) failed with {1}",
            this,
            rfHr);
    }

    HRESULT hr = E_FAIL;

    if (!service_.ShouldFailOn(ApiFaultHelper::Service, L"EndClose"))
    {
        hr = service_.OnClose();
    }

    ComPointer<SGComCompletedAsyncOperationContext> operation = make_com<SGComCompletedAsyncOperationContext>();
    hr = operation->Initialize(hr, root_, callback);
    TestSession::FailTestIf(FAILED(hr), "operation->Initialize should not fail");
    return ComAsyncOperationContext::StartAndDetach<SGComCompletedAsyncOperationContext>(std::move(operation), context);
}

HRESULT STDMETHODCALLTYPE SGComStatefulService::EndClose(
    __in IFabricAsyncOperationContext* context)
{
    if (NULL == context) { return E_POINTER; }

     if (service_.ShouldFailOn(ApiFaultHelper::Service, L"EndClose", ApiFaultHelper::ReportFaultPermanent))
    {
        auto rfHr = service_.Partition->ReportFault(FABRIC_FAULT_TYPE_PERMANENT);
        TestSession::FailTestIf(
            FAILED(rfHr), 
            "SGComStatefulService::EndClose ({0}) - ReportFault(FABRIC_FAULT_TYPE_PERMANENT) failed with {1}",
            this,
            rfHr);
    }
    if (service_.ShouldFailOn(ApiFaultHelper::Service, L"EndClose", ApiFaultHelper::ReportFaultTransient))
    {
        auto rfHr = service_.Partition->ReportFault(FABRIC_FAULT_TYPE_TRANSIENT);
        TestSession::FailTestIf(
            FAILED(rfHr), 
            "SGComStatefulService::EndClose ({0}) - ReportFault(FABRIC_FAULT_TYPE_TRANSIENT) failed with {1}",
            this,
            rfHr);
    }

    HRESULT hr = SGComCompletedAsyncOperationContext::End(context);

    TestSession::WriteNoise(
        TraceSource, 
        "SGComStatefulService::EndClose ({0})",
        this);

    return hr;
}

void STDMETHODCALLTYPE SGComStatefulService::Abort()
{
    TestSession::WriteNoise(
        TraceSource, 
        "SGComStatefulService::Abort ({0})",
        this);

    service_.OnAbort();
}

//
// IFabricStateProvider methods.
//
HRESULT STDMETHODCALLTYPE SGComStatefulService::BeginUpdateEpoch( 
    __in ::FABRIC_EPOCH const * epoch,
    __in ::FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber,
    __in IFabricAsyncOperationCallback *callback,
    __out IFabricAsyncOperationContext **context)
{
    if (epoch == NULL || callback == NULL || context == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

    TestSession::WriteNoise(
        TraceSource, 
        "SGComStatefulService::BeginUpdateEpoch ({0}) - epoch({1}.{2}.{3}) previousEpochLastSequenceNumber({4})",
        this,
        epoch->DataLossNumber,
        epoch->ConfigurationNumber >> 32,
        epoch->ConfigurationNumber & 0xffffffff,
        previousEpochLastSequenceNumber);

    CHECK_INJECT_FAILURE(ApiFaultHelper::Provider, L"BeginUpdateEpoch");

    HRESULT hr = E_FAIL;
    if (!service_.ShouldFailOn(ApiFaultHelper::Provider, L"EndUpdateEpoch"))
    {
        hr = service_.UpdateEpoch(epoch, previousEpochLastSequenceNumber);
    }

    ComPointer<SGComCompletedAsyncOperationContext> operation = make_com<SGComCompletedAsyncOperationContext>();
    hr = operation->Initialize(hr, root_, callback);
    TestSession::FailTestIf(FAILED(hr), "operation->Initialize should not fail");
    return ComAsyncOperationContext::StartAndDetach<SGComCompletedAsyncOperationContext>(std::move(operation), context);
}

HRESULT STDMETHODCALLTYPE SGComStatefulService::EndUpdateEpoch( 
    __in IFabricAsyncOperationContext *context)
{
    if (context == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }
    return SGComCompletedAsyncOperationContext::End(context);
}

HRESULT STDMETHODCALLTYPE SGComStatefulService::GetLastCommittedSequenceNumber(
    __out FABRIC_SEQUENCE_NUMBER* sequenceNumber)
{
    TestSession::WriteNoise(
        TraceSource, 
        "SGComStatefulService::GetLastCommittedSequenceNumber ({0})",
        this);

    CHECK_INJECT_FAILURE(ApiFaultHelper::Provider, L"GetLastCommittedSequenceNumber");
    return service_.GetCurrentProgress(sequenceNumber);
}

HRESULT STDMETHODCALLTYPE SGComStatefulService::BeginOnDataLoss(
    __in IFabricAsyncOperationCallback* callback,
    __out IFabricAsyncOperationContext** context
    )
{
    if (NULL == callback || NULL == context) { return E_POINTER; }

    TestSession::WriteNoise(
        TraceSource, 
        "SGComStatefulService::BeginOnDataLoss ({0})",
        this);

    CHECK_INJECT_FAILURE(ApiFaultHelper::Provider, L"BeginOnDataLoss");

    HRESULT hr = E_FAIL;

    if (!service_.ShouldFailOn(ApiFaultHelper::Provider, L"EndOnDataLoss"))
    {
        hr = service_.OnDataLoss();
    }

    ComPointer<SGComCompletedAsyncOperationContext> operation = make_com<SGComCompletedAsyncOperationContext>();
    hr = operation->Initialize(hr, root_, callback);
    TestSession::FailTestIf(FAILED(hr), "operation->Initialize should not fail");
    return ComAsyncOperationContext::StartAndDetach<SGComCompletedAsyncOperationContext>(std::move(operation), context);
}

HRESULT STDMETHODCALLTYPE SGComStatefulService::EndOnDataLoss(
    __in IFabricAsyncOperationContext* context,
    __out BOOLEAN* isStateChanged
    )
{
    if (NULL == context || NULL == isStateChanged) { return E_POINTER; }
    *isStateChanged = FALSE;
    HRESULT hr = SGComCompletedAsyncOperationContext::End(context);

    TestSession::WriteNoise(
        TraceSource, 
        "SGComStatefulService::EndOnDataLoss ({0}) - isStateChanged({1})",
        this,
        *isStateChanged);

    return hr;
}

HRESULT STDMETHODCALLTYPE SGComStatefulService::GetCopyContext(
    __out IFabricOperationDataStream** copyContextEnumerator)
{
    if (NULL == copyContextEnumerator) { return E_POINTER; }

    TestSession::WriteNoise(
        TraceSource, 
        "SGComStatefulService::GetCopyContext ({0})",
        this);

    CHECK_INJECT_FAILURE(ApiFaultHelper::Provider, L"GetCopyContext");

    return service_.GetCopyContext(copyContextEnumerator);
}

HRESULT STDMETHODCALLTYPE SGComStatefulService::GetCopyState(
    __in FABRIC_SEQUENCE_NUMBER uptoSequenceNumber,
    __in IFabricOperationDataStream* copyContextEnumerator,
    __out IFabricOperationDataStream** copyStateEnumerator)
{
    if (NULL == copyStateEnumerator) { return E_POINTER; }

    TestSession::WriteNoise(
        TraceSource, 
        "SGComStatefulService::GetCopyState ({0})",
        this);

    CHECK_INJECT_FAILURE(ApiFaultHelper::Provider, L"GetCopyState");

    return service_.GetCopyState(uptoSequenceNumber, copyContextEnumerator, copyStateEnumerator);
}

HRESULT STDMETHODCALLTYPE SGComStatefulService::BeginAtomicGroupCommit( 
    __in FABRIC_ATOMIC_GROUP_ID atomicGroupId,
    __in FABRIC_SEQUENCE_NUMBER commitSequenceNumber,
    __in IFabricAsyncOperationCallback *callback,
    __out IFabricAsyncOperationContext **context) 
{
    if (NULL == callback || NULL == context) { return E_POINTER; }

    TestSession::WriteNoise(
        TraceSource, 
        "SGComStatefulService::BeginAtomicGroupCommit ({0})",
        this);

    CHECK_INJECT_SIGNAL(AtomicGroupStateProviderBeginCommit);        

    HRESULT hr = E_FAIL;

    if (!service_.IsSignalSet(AtomicGroupStateProviderEndCommit))
    {
        hr = service_.OnAtomicGroupCommit(atomicGroupId, commitSequenceNumber);
    }

    ComPointer<SGComCompletedAsyncOperationContext> operation = make_com<SGComCompletedAsyncOperationContext>();
    hr = operation->Initialize(hr, root_, callback);
    TestSession::FailTestIf(FAILED(hr), "operation->Initialize should not fail");
    return ComAsyncOperationContext::StartAndDetach<SGComCompletedAsyncOperationContext>(std::move(operation), context);
}

HRESULT STDMETHODCALLTYPE SGComStatefulService::EndAtomicGroupCommit( 
    __in IFabricAsyncOperationContext *context) 
{
    if (NULL == context) { return E_POINTER; }
    HRESULT hr = SGComCompletedAsyncOperationContext::End(context);

    TestSession::WriteNoise(
        TraceSource, 
        "SGComStatefulService::EndAtomicGroupCommit ({0})",
        this);

    return hr;
}

HRESULT STDMETHODCALLTYPE SGComStatefulService::BeginAtomicGroupRollback( 
    __in FABRIC_ATOMIC_GROUP_ID atomicGroupId,
    __in FABRIC_SEQUENCE_NUMBER rollbackequenceNumber,
    __in IFabricAsyncOperationCallback *callback,
    __out IFabricAsyncOperationContext **context) 
{
    if (NULL == callback || NULL == context) { return E_POINTER; }

    TestSession::WriteNoise(
        TraceSource, 
        "SGComStatefulService::BeginAtomicGroupRollback ({0})",
        this);

    CHECK_INJECT_SIGNAL(AtomicGroupStateProviderBeginRollback);

    HRESULT hr = E_FAIL;

    if (!service_.IsSignalSet(AtomicGroupStateProviderEndRollback))
    {
        hr = service_.OnAtomicGroupRollback(atomicGroupId, rollbackequenceNumber);
    }

    ComPointer<SGComCompletedAsyncOperationContext> operation = make_com<SGComCompletedAsyncOperationContext>();
    hr = operation->Initialize(hr, root_, callback);
    TestSession::FailTestIf(FAILED(hr), "operation->Initialize should not fail");
    return ComAsyncOperationContext::StartAndDetach<SGComCompletedAsyncOperationContext>(std::move(operation), context);
}

HRESULT STDMETHODCALLTYPE SGComStatefulService::EndAtomicGroupRollback( 
    __in IFabricAsyncOperationContext *context) 
{
    if (NULL == context) { return E_POINTER; }
    HRESULT hr = SGComCompletedAsyncOperationContext::End(context);

    TestSession::WriteNoise(
        TraceSource, 
        "SGComStatefulService::EndAtomicGroupRollback ({0})",
        this);

    return hr;
}

HRESULT STDMETHODCALLTYPE SGComStatefulService::BeginUndoProgress( 
    __in FABRIC_SEQUENCE_NUMBER fromCommitSequenceNumber,
    __in IFabricAsyncOperationCallback *callback,
    __out IFabricAsyncOperationContext **context) 
{
    if (NULL == callback || NULL == context) { return E_POINTER; }

    TestSession::WriteNoise(
        TraceSource, 
        "SGComStatefulService::BeginUndoProgress ({0})",
        this);

    CHECK_INJECT_SIGNAL(AtomicGroupStateProviderBeginUndoProgress);

    HRESULT hr = E_FAIL;

    if (!service_.IsSignalSet(AtomicGroupStateProviderEndUndoProgress))
    {
        hr = service_.OnUndoProgress(fromCommitSequenceNumber);
    }

    ComPointer<SGComCompletedAsyncOperationContext> operation = make_com<SGComCompletedAsyncOperationContext>();
    hr = operation->Initialize(hr, root_, callback);
    TestSession::FailTestIf(FAILED(hr), "operation->Initialize should not fail");
    return ComAsyncOperationContext::StartAndDetach<SGComCompletedAsyncOperationContext>(std::move(operation), context);
}

HRESULT STDMETHODCALLTYPE SGComStatefulService::EndUndoProgress( 
    __in IFabricAsyncOperationContext *context) 
{
    if (NULL == context) { return E_POINTER; }
    HRESULT hr = SGComCompletedAsyncOperationContext::End(context);

    TestSession::WriteNoise(
        TraceSource, 
        "SGComStatefulService::EndUndoProgress ({0})",
        this);

    return hr;
}

StringLiteral const SGComStatefulService::TraceSource("FabricTest.ServiceGroup.SGComStatefulService");
