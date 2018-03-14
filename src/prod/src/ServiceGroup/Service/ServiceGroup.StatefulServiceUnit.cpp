// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include "FabricRuntime_.h"

#include "ServiceGroup.Constants.h"
#include "ServiceGroup.ServiceBaseUnit.h"
#include "ServiceGroup.StatefulServiceUnit.h"
#include "ServiceGroup.AtomicGroup.h"
#include "ServiceGroup.OperationExtendedBuffer.h"

using namespace ServiceGroup;
using namespace ServiceModel;

//
// Constructor/Destructor.
//
CStatefulServiceUnit::CStatefulServiceUnit(__in FABRIC_PARTITION_ID partitionId, __in FABRIC_REPLICA_ID replicaId) : 
    CBaseServiceUnit(partitionId)
{
    WriteNoise(TraceSubComponentStatefulServiceGroupMember, "this={0} partitionId={1} replicaId={2} - ctor", this, partitionId_, replicaId);

    replicaId_ = replicaId;
    outerStatefulServicePartition_ = NULL;
    outerStateReplicator_ = NULL;
    outerAtomicGroupStateReplicator_ = NULL;
    innerStatefulServiceReplica_ = NULL;
    innerStateProvider_ = NULL;
    innerAtomicGroupStateProvider_ = NULL;
    serviceEndpoint_ = NULL;
    currentReplicaRole_ = FABRIC_REPLICA_ROLE_UNKNOWN;
    proposedReplicaRole_ = FABRIC_REPLICA_ROLE_UNKNOWN;
    outerStateReplicatorDestructed_ = FALSE;
    copyOperationStream_ = NULL;
    copyStreamDrainingStarted_ = FALSE;
    replicationOperationStream_ = NULL;
    replicationStreamDrainingStarted_ = FALSE;
    outerServiceGroupPartition_ = NULL;
    isClosing_ = FALSE;
    isFaulted_ = FALSE;
}

CStatefulServiceUnit::~CStatefulServiceUnit(void)
{
    WriteNoise(TraceSubComponentStatefulServiceGroupMember, "this={0} partitionId={1} replicaId={2} - dtor", this, partitionId_, replicaId_);
}

//
// Initialize/Cleanup the service unit.
//
STDMETHODIMP CStatefulServiceUnit::FinalConstruct(
    __in CServiceGroupMemberDescription* serviceActivationInfo,
    __in IFabricStatefulServiceFactory* factory
    )
{
    ServiceGroupStatefulEventSource::GetEvents().Info_0_StatefulServiceGroupMember(
        reinterpret_cast<uintptr_t>(this),
        partitionId_,
        replicaId_
        );

    ULONG initializationDataSize = 0;
    HRESULT hr = ::SizeTToULong(serviceActivationInfo->ServiceGroupMemberInitializationData.size(), &initializationDataSize);
    if (FAILED(hr))
    {
        ServiceGroupStatefulEventSource::GetEvents().Error_0_StatefulServiceGroupMember(
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_,
            hr
            );

        return hr;
    }

    try { serviceName_ = serviceActivationInfo->ServiceName; }
    catch (std::bad_alloc const &) { hr = E_OUTOFMEMORY; }
    catch (...) { hr = E_FAIL; }
    if (FAILED(hr))
    {
        ServiceGroupStatefulEventSource::GetEvents().Error_1_StatefulServiceGroupMember(
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_
            );

        return hr;
    }

    hr = factory->CreateReplica(
        serviceActivationInfo->ServiceType.c_str(),
        serviceActivationInfo->ServiceName.c_str(),
        initializationDataSize,
        serviceActivationInfo->ServiceGroupMemberInitializationData.data(),
        partitionId_.AsGUID(),
        replicaId_,
        &innerStatefulServiceReplica_
        );
        
    if (FAILED(hr))
    {
        ServiceGroupStatefulMemberEventSource::GetEvents().Error_0_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_,
            hr
            );
    }

    ServiceGroupStatefulMemberEventSource::GetEvents().Info_0_StatefulServiceGroupMember(
        serviceName_,
        reinterpret_cast<uintptr_t>(this),
        partitionId_,
        replicaId_
        );
    return hr;
}

//
// Called during close replica.
//
STDMETHODIMP CStatefulServiceUnit::FinalDestruct(void)
{
    Common::AcquireWriteLock lock(lock_);

    ServiceGroupStatefulEventSource::GetEvents().Info_1_StatefulServiceGroupMember(
        reinterpret_cast<uintptr_t>(this),
        partitionId_,
        replicaId_
        );

    if (NULL != outerStatefulServicePartition_)
    {
        outerStatefulServicePartition_->Release();
        outerStatefulServicePartition_ = NULL;
    }
    if (NULL != outerServiceGroupPartition_)
    {
        outerServiceGroupPartition_->Release();
        outerServiceGroupPartition_ = NULL;
    }
    if (NULL != outerServiceGroupReport_)
    {
        outerServiceGroupReport_->Release();
        outerServiceGroupReport_ = NULL;
    }
    if (!outerStateReplicatorDestructed_)
    {
        //
        // Don't set pointers to NULL. There may be inflight operations in progress. Callers must AddRef.
        //
        if (NULL != outerStateReplicator_)
        {
            outerStateReplicator_->Release();
        }
        if (NULL != outerAtomicGroupStateReplicator_)
        {
            outerAtomicGroupStateReplicator_->Release();
        }
        outerStateReplicatorDestructed_ = TRUE;
    }
    if (NULL != copyOperationStream_)
    {
        copyOperationStream_->Release();
        copyOperationStream_ = NULL;
    }
    if (NULL != replicationOperationStream_)
    {
        replicationOperationStream_->Release();
        replicationOperationStream_ = NULL;
    }
    if (NULL != innerStatefulServiceReplica_)
    {
        innerStatefulServiceReplica_->Release();
        innerStatefulServiceReplica_ = NULL;
    }
    if (NULL != innerStateProvider_)
    {
        innerStateProvider_->Release();
        innerStateProvider_ = NULL;
    }
    if (NULL != innerAtomicGroupStateProvider_)
    {
        innerAtomicGroupStateProvider_->Release();
        innerAtomicGroupStateProvider_ = NULL;
    }
    if (NULL != serviceEndpoint_)
    {
        serviceEndpoint_->Release();
        serviceEndpoint_ = NULL;
    }

    currentReplicaRole_ = FABRIC_REPLICA_ROLE_UNKNOWN;
    proposedReplicaRole_ = FABRIC_REPLICA_ROLE_UNKNOWN;
    return S_OK;
}

//
// IFabricStatefulServiceReplica methods.
//
STDMETHODIMP CStatefulServiceUnit::BeginOpen(
    __in FABRIC_REPLICA_OPEN_MODE openMode,
    __in IFabricStatefulServicePartition* partition,
    __in IFabricAsyncOperationCallback* callback,
    __out IFabricAsyncOperationContext** context
    )
{
    ASSERT_IF(NULL == partition, "Unexpected partition");
    ASSERT_IF(NULL == callback, "Unexpected callback");
    ASSERT_IF(NULL == context, "Unexpected context");

    partition->AddRef();
    HRESULT hr = partition->QueryInterface(IID_IFabricStatefulServicePartition3, (LPVOID*)&outerStatefulServicePartition_);
    ASSERT_IF(FAILED(hr), "Unexpected stateful service partition");
 
    hr = outerStatefulServicePartition_->QueryInterface(IID_IFabricServiceGroupPartition, (LPVOID*)&outerServiceGroupPartition_);
    ASSERT_IF(FAILED(hr), "Unexpected stateful service partition");

    hr = outerStatefulServicePartition_->QueryInterface(IID_IServiceGroupReport, (LPVOID*)&outerServiceGroupReport_);
    ASSERT_IF(FAILED(hr), "Unexpected stateful service partition");

    ServiceGroupStatefulMemberEventSource::GetEvents().Info_1_StatefulServiceGroupMember(
        serviceName_,
        reinterpret_cast<uintptr_t>(this),
        partitionId_,
        replicaId_
        );

    hr = innerStatefulServiceReplica_->BeginOpen(openMode, (IFabricStatefulServicePartition*)this, callback, context);
    if (FAILED(hr))
    {
        ServiceGroupStatefulMemberEventSource::GetEvents().Error_1_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_,
            hr
            );
        //
        // Release all resources allocated in BeginOpen.
        //
        FixUpFailedOpen();
    }
    return hr;
}

void CStatefulServiceUnit::FixUpFailedOpen(void)
{

        ServiceGroupStatefulMemberEventSource::GetEvents().Warning_0_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_
            );
        //
        // Release all resources allocated in BeginOpen.
        //
        Common::AcquireWriteLock lockPartition(lock_);
        outerStatefulServicePartition_->Release();
        outerStatefulServicePartition_ = NULL;
        outerServiceGroupPartition_->Release();
        outerServiceGroupPartition_ = NULL;
        outerServiceGroupReport_->Release();
        outerServiceGroupReport_ = NULL;
        innerStatefulServiceReplica_->Release();
        innerStatefulServiceReplica_ = NULL;
        if (NULL != outerStateReplicator_)
        {
            outerStateReplicator_->Release(); 
            outerStateReplicator_ = NULL;
        }
        if (NULL != outerAtomicGroupStateReplicator_)
        {
            outerAtomicGroupStateReplicator_->Release();
            outerAtomicGroupStateReplicator_ = NULL;
        }
        if (NULL != innerStateProvider_)
        {
            innerStateProvider_->Release();
            innerStateProvider_ = NULL;
        }
        if (NULL != innerAtomicGroupStateProvider_)
        {
            innerAtomicGroupStateProvider_->Release();
            innerAtomicGroupStateProvider_ = NULL;
        }
        outerStateReplicatorDestructed_ = TRUE;
}

STDMETHODIMP CStatefulServiceUnit::EndOpen(
    __in IFabricAsyncOperationContext* context,
    __out IFabricReplicator** replicator
    )
{
    ASSERT_IF(NULL == context, "Unexpected context");
    ASSERT_IF(NULL == replicator, "Unexpected replicator");

    ServiceGroupStatefulMemberEventSource::GetEvents().Info_2_StatefulServiceGroupMember(
        serviceName_,
        reinterpret_cast<uintptr_t>(this),
        partitionId_,
        replicaId_
        );

    HRESULT hr = innerStatefulServiceReplica_->EndOpen(context, replicator);

    opened_ = SUCCEEDED(hr);
    if (SUCCEEDED(hr))
    {
        //
        // The inner state-provider is initialized on CreateReplicator. CreateReplicator must be called during open.
        // Custom replicators are not supported in service groups. 
        //
        if (NULL == innerStateProvider_)
        {
            ServiceGroupStatefulMemberEventSource::GetEvents().Error_2_StatefulServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                replicaId_
                );

            //
            // Fail the otherwise successful open. Do not call FixUpFailedOpen. The member assumes it successfully opened and 
            // it should be closed before deconstruction. ServiceGroup will call close after the failed open to ensure this.
            //
            hr = E_UNEXPECTED;
        }
        else
        {
            ServiceGroupStatefulMemberEventSource::GetEvents().Info_3_StatefulServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                replicaId_
                );
        }
    }
    else
    {
        ServiceGroupStatefulMemberEventSource::GetEvents().Error_3_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_,
            hr
            );
        //
        // Release all resources allocated in BeginOpen.
        //
        FixUpFailedOpen();
    }
    return hr;
}

STDMETHODIMP CStatefulServiceUnit::BeginChangeRole(
    __in FABRIC_REPLICA_ROLE newRole,
    __in IFabricAsyncOperationCallback* callback,
    __out IFabricAsyncOperationContext** context
    )
{
    ASSERT_IF(NULL == callback, "Unexpected callback");
    ASSERT_IF(NULL == context, "Unexpected context");

    HRESULT hr = S_OK;
    if (newRole == currentReplicaRole_)
    {
        ASSERT_IF(currentReplicaRole_ != proposedReplicaRole_, "Unexpected member replica role");

        ServiceGroupStatefulMemberEventSource::GetEvents().Info_4_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_,
            ROLE_TEXT(newRole)
            );

        COperationContext* contextDone = new (std::nothrow) COperationContext();
        if (NULL == contextDone)
        {
            return E_OUTOFMEMORY;

        }
        hr = contextDone->FinalConstruct();
        if (FAILED(hr))
        {
            contextDone->Release();
            return hr;

        }
        contextDone->set_Callback(callback);
        contextDone->set_CompletedSynchronously(TRUE);
        contextDone->set_IsCompleted();
        *context = contextDone;
        return hr;
    }

    ServiceGroupStatefulMemberEventSource::GetEvents().Info_5_StatefulServiceGroupMember(
        serviceName_,
        reinterpret_cast<uintptr_t>(this),
        partitionId_,
        replicaId_,
        ROLE_TEXT(currentReplicaRole_),
        ROLE_TEXT(newRole)
        );

    proposedReplicaRole_ = newRole;

    hr = innerStatefulServiceReplica_->BeginChangeRole(newRole, callback, context);
    if (FAILED(hr))
    {
        proposedReplicaRole_ = currentReplicaRole_;

        ServiceGroupStatefulMemberEventSource::GetEvents().Error_4_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_,
            ROLE_TEXT(currentReplicaRole_),
            ROLE_TEXT(newRole),
            hr
            );
    }
    return hr;
}

STDMETHODIMP CStatefulServiceUnit::EndChangeRole(
    __in IFabricAsyncOperationContext* context,
    __out IFabricStringResult** serviceEndpoint
    )
{
    ASSERT_IF(NULL == context, "Unexpected context");
    ASSERT_IF(NULL == serviceEndpoint, "Unexpected service endpoint");

    if (proposedReplicaRole_ == currentReplicaRole_)
    {
        ServiceGroupStatefulMemberEventSource::GetEvents().Info_6_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_,
            ROLE_TEXT(currentReplicaRole_)
            );

        if (NULL != serviceEndpoint_)
        {
            serviceEndpoint_->AddRef();
        }
        (*serviceEndpoint) = serviceEndpoint_;
         return S_OK;
    }
    
    ServiceGroupStatefulMemberEventSource::GetEvents().Info_7_StatefulServiceGroupMember(
        serviceName_,
        reinterpret_cast<uintptr_t>(this),
        partitionId_,
        replicaId_,
        ROLE_TEXT(currentReplicaRole_),
        ROLE_TEXT(proposedReplicaRole_)
        );

    HRESULT hr = innerStatefulServiceReplica_->EndChangeRole(context, serviceEndpoint);
    if (SUCCEEDED(hr))
    {
        ServiceGroupStatefulMemberEventSource::GetEvents().Info_8_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_,
            ROLE_TEXT(currentReplicaRole_),
            ROLE_TEXT(proposedReplicaRole_)
            );

        currentReplicaRole_ = proposedReplicaRole_;
        if (NULL != serviceEndpoint_)
        {
            serviceEndpoint_->Release();
        }
        if (NULL != (*serviceEndpoint))
        {
            (*serviceEndpoint)->AddRef();
        }
        serviceEndpoint_ = (*serviceEndpoint);
    }
    else
    {
        ServiceGroupStatefulMemberEventSource::GetEvents().Error_5_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_,
            ROLE_TEXT(currentReplicaRole_),
            ROLE_TEXT(proposedReplicaRole_),
            hr
            );

        proposedReplicaRole_ = currentReplicaRole_;
    }
    return hr;
}

STDMETHODIMP CStatefulServiceUnit::BeginClose(
    __in IFabricAsyncOperationCallback* callback,
    __out IFabricAsyncOperationContext** context
    )
{
    ASSERT_IF(NULL == callback, "Unexpected callback");
    ASSERT_IF(NULL == context, "Unexpected context");

    if (!opened_)
    {
        ServiceGroupStatefulMemberEventSource::GetEvents().Info_9_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_
            );

        HRESULT hr = S_OK;
        COperationContext* contextDone = new (std::nothrow) COperationContext();
        if (NULL == contextDone)
        {
            return E_OUTOFMEMORY;

        }
        hr = contextDone->FinalConstruct();
        if (FAILED(hr))
        {
            contextDone->Release();
            return hr;

        }
        contextDone->set_Callback(callback);
        contextDone->set_CompletedSynchronously(TRUE);
        contextDone->set_IsCompleted();
        *context = contextDone;
        return hr;
    }

    ServiceGroupStatefulMemberEventSource::GetEvents().Info_10_StatefulServiceGroupMember(
        serviceName_,
        reinterpret_cast<uintptr_t>(this),
        partitionId_,
        replicaId_
        );

    HRESULT hr = innerStatefulServiceReplica_->BeginClose(callback, context);
    if (FAILED(hr))
    {
        ServiceGroupStatefulMemberEventSource::GetEvents().Error_6_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_,
            hr
            );
    }
    return hr;
}

STDMETHODIMP CStatefulServiceUnit::EndClose(__in IFabricAsyncOperationContext* context)
{
    ASSERT_IF(NULL == context, "Unexpected context");

    if (!opened_)
    {
        ServiceGroupStatefulMemberEventSource::GetEvents().Info_11_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_
            );

        return S_OK;
    }

    ServiceGroupStatefulMemberEventSource::GetEvents().Info_12_StatefulServiceGroupMember(
        serviceName_,
        reinterpret_cast<uintptr_t>(this),
        partitionId_,
        replicaId_
        );

    HRESULT hr = innerStatefulServiceReplica_->EndClose(context);
    if (FAILED(hr))
    {
        ServiceGroupStatefulMemberEventSource::GetEvents().Error_7_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_,
            hr
            );
    }

    opened_ = FAILED(hr);
    return hr;
}

STDMETHODIMP_(void) CStatefulServiceUnit::Abort()
{
    if (!opened_)
    {
        ServiceGroupStatefulMemberEventSource::GetEvents().Info_13_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_
            );

        return;
    }

    ServiceGroupStatefulMemberEventSource::GetEvents().Info_14_StatefulServiceGroupMember(
        serviceName_,
        reinterpret_cast<uintptr_t>(this),
        partitionId_,
        replicaId_
        );

    innerStatefulServiceReplica_->Abort();

    opened_ = FALSE;
}

//
// IFabricStatefulServicePartition methods.
//
STDMETHODIMP CStatefulServiceUnit::GetPartitionInfo(__out const FABRIC_SERVICE_PARTITION_INFORMATION** bufferedValue)
{
    if (NULL == bufferedValue)
    {
        return Common::ComUtility::OnPublicApiReturn(E_POINTER);
    }

    IFabricStatefulServicePartition* partition = NULL;
    {
        Common::AcquireReadLock lockPartition(lock_);

        if (NULL == outerStatefulServicePartition_)
        {
            ServiceGroupStatefulMemberEventSource::GetEvents().Error_8_StatefulServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                replicaId_
                );

            return Common::ComUtility::OnPublicApiReturn(Common::ErrorCodeValue::ObjectClosed);
        }
        else
        {
            partition = outerStatefulServicePartition_;
            partition->AddRef();
        }
    }

    WriteNoise(
        TraceSubComponentStatefulServiceGroupMember, 
        serviceName_,
        "this={0} partitionId={1} replicaId={2} - Calling simulated stateful service partition GetPartitionInfo",
        this,
        partitionId_,
        replicaId_
        );

    HRESULT hr = partition->GetPartitionInfo(bufferedValue);
    partition->Release();

    return Common::ComUtility::OnPublicApiReturn(hr);
}

STDMETHODIMP CStatefulServiceUnit::ReportLoad(__in ULONG metricCount, __in const FABRIC_LOAD_METRIC* metrics)
{
    if (NULL == metrics)
    {
        return Common::ComUtility::OnPublicApiReturn(E_POINTER);
    }
    FAIL_IF_RESERVED_FIELD_NOT_NULL(metrics);
    if (0 == metricCount)
    {
        return Common::ComUtility::OnPublicApiReturn(E_INVALIDARG);
    }

    IServiceGroupReport* report = NULL;
    {
        Common::AcquireReadLock lockPartition(lock_);

        if (NULL == outerServiceGroupReport_)
        {
            ServiceGroupStatefulMemberEventSource::GetEvents().Error_8_StatefulServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                replicaId_
                );

            return Common::ComUtility::OnPublicApiReturn(Common::ErrorCodeValue::ObjectClosed);
        }
        else
        {
            report = outerServiceGroupReport_;
            report->AddRef();
        }
    }
    WriteNoise(
        TraceSubComponentStatefulServiceGroupMember, 
        serviceName_,
        "this={0} partitionId={1} replicaId={2} - Calling simulated stateful service partition ReportLoad",
        this,
        partitionId_,
        replicaId_
        );

    HRESULT hr = report->ReportLoad(partitionId_.AsGUID(), metricCount, metrics);
    report->Release();

    return Common::ComUtility::OnPublicApiReturn(hr);
}

STDMETHODIMP CStatefulServiceUnit::ReportFault(FABRIC_FAULT_TYPE faultType)
{
    IServiceGroupReport* report = NULL;

    {
        Common::AcquireReadLock lockPartition(lock_);

        if (NULL == outerServiceGroupReport_)
        {
            ServiceGroupStatefulMemberEventSource::GetEvents().Error_8_StatefulServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                replicaId_
                );

            return Common::ComUtility::OnPublicApiReturn(Common::ErrorCodeValue::ObjectClosed);
        }
        else
        {
            report = outerServiceGroupReport_;
            report->AddRef();
        }
    }

    WriteNoise(
        TraceSubComponentStatefulReportFault, 
        serviceName_,
        "this={0} partitionId={1} replicaId={2} - Calling simulated stateful service partition ReportFault",
        this,
        partitionId_,
        replicaId_
        );

    HRESULT hr = report->ReportFault(partitionId_.AsGUID(), faultType);
    report->Release();

    return Common::ComUtility::OnPublicApiReturn(hr);
}

STDMETHODIMP CStatefulServiceUnit::ReportMoveCost(FABRIC_MOVE_COST moveCost)
{
    IServiceGroupReport* report = NULL;

    {
        Common::AcquireReadLock lockPartition(lock_);

        if (NULL == outerServiceGroupReport_)
        {
            ServiceGroupStatefulMemberEventSource::GetEvents().Error_8_StatefulServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                replicaId_
                );

            return Common::ComUtility::OnPublicApiReturn(Common::ErrorCodeValue::ObjectClosed);
        }
        else
        {
            report = outerServiceGroupReport_;
            report->AddRef();
        }
    }

    WriteNoise(
        TraceSubComponentStatefulReportMoveCost,
        serviceName_,
        "this={0} partitionId={1} replicaId={2} - Calling simulated stateful service partition ReportMoveCost",
        this,
        partitionId_,
        replicaId_
        );

    HRESULT hr = report->ReportMoveCost(partitionId_.AsGUID(), moveCost);
    report->Release();

    return Common::ComUtility::OnPublicApiReturn(hr);
}

STDMETHODIMP CStatefulServiceUnit::GetReadStatus(__out FABRIC_SERVICE_PARTITION_ACCESS_STATUS* readStatus)
{
    if (NULL == readStatus)
    {
        return Common::ComUtility::OnPublicApiReturn(E_POINTER);
    }

    IFabricStatefulServicePartition* partition = NULL;
    {
        Common::AcquireReadLock lockPartition(lock_);

        if (NULL == outerStatefulServicePartition_)
        {
            ServiceGroupStatefulMemberEventSource::GetEvents().Error_8_StatefulServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                replicaId_
                );

            return Common::ComUtility::OnPublicApiReturn(Common::ErrorCodeValue::ObjectClosed);
        }
        else
        {
            partition = outerStatefulServicePartition_;
            partition->AddRef();
        }
    }

    WriteNoise(
        TraceSubComponentStatefulServiceGroupMember, 
        serviceName_,
        "this={0} partitionId={1} replicaId={2} - Calling simulated stateful service partition GetReadStatus",
        this,
        partitionId_,
        replicaId_
        );

    HRESULT hr = partition->GetReadStatus(readStatus);
    partition->Release();

    return Common::ComUtility::OnPublicApiReturn(hr);
}

STDMETHODIMP CStatefulServiceUnit::GetWriteStatus(__out FABRIC_SERVICE_PARTITION_ACCESS_STATUS* writeStatus)
{
    if (NULL == writeStatus)
    {
        return Common::ComUtility::OnPublicApiReturn(E_POINTER);
    }

    IFabricStatefulServicePartition* partition = NULL;
    {
        Common::AcquireReadLock lockPartition(lock_);

        if (NULL == outerStatefulServicePartition_)
        {
            ServiceGroupStatefulMemberEventSource::GetEvents().Error_8_StatefulServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                replicaId_
                );

            return Common::ComUtility::OnPublicApiReturn(Common::ErrorCodeValue::ObjectClosed);
        }
        else
        {
            partition = outerStatefulServicePartition_;
            partition->AddRef();
        }
    }

    WriteNoise(
        TraceSubComponentStatefulServiceGroupMember, 
        serviceName_,
        "this={0} partitionId={1} replicaId={2} - Calling simulated stateful service partition GetWriteStatus",
        this,
        partitionId_,
        replicaId_
        );

    HRESULT hr = partition->GetWriteStatus(writeStatus);
    partition->Release();

    return Common::ComUtility::OnPublicApiReturn(hr);
}

STDMETHODIMP CStatefulServiceUnit::CreateReplicator(
    __in IFabricStateProvider* stateProvider, 
    __in_opt FABRIC_REPLICATOR_SETTINGS const* replicatorSettings,
    __out IFabricReplicator** replicator, 
    __out IFabricStateReplicator** stateReplicator
    )
{
    WriteNoise(
        TraceSubComponentStatefulServiceGroupMember, 
        serviceName_,
        "this={0} partitionId={1} replicaId={2} - CreateReplicator",
        this,
        partitionId_,
        replicaId_
        );

    if (NULL == stateProvider || NULL == replicator)
    {
        WriteError(
            TraceSubComponentStatefulServiceGroupMember, 
            serviceName_,
            "this={0} partitionId={1} replicaId={2} - CreateReplicator called with invalid arguments - stateProvider = {3}, replicator = {4}",
            this,
            partitionId_,
            replicaId_,
            stateProvider,
            replicator
            );

        return Common::ComUtility::OnPublicApiReturn(E_POINTER);
    }

    if (NULL != replicatorSettings)
    {
        if (0 != replicatorSettings->Flags)
        {
            WriteError(
                TraceSubComponentStatefulServiceGroupMember, 
                serviceName_,
                "this={0} partitionId={1} replicaId={2} - CreateReplicator called with invalid arguments - replicatorSettings = {3}, flags = {4}",
                this,
                partitionId_,
                replicaId_,
                replicatorSettings,
                replicatorSettings ? replicatorSettings->Flags : 0
                );

            return Common::ComUtility::OnPublicApiReturn(E_INVALIDARG);
        }

        FABRIC_REPLICATOR_SETTINGS_EX1 * replicatorSettingsEx1 = (FABRIC_REPLICATOR_SETTINGS_EX1 *) replicatorSettings->Reserved;
        if ((NULL != replicatorSettingsEx1) && (NULL != replicatorSettingsEx1->Reserved))
        {
            FABRIC_REPLICATOR_SETTINGS_EX2 * replicatorSettingsEx2 = (FABRIC_REPLICATOR_SETTINGS_EX2 *) replicatorSettingsEx1->Reserved;

            if ((NULL != replicatorSettingsEx2) && (NULL != replicatorSettingsEx2->Reserved))
            {
                WriteError(
                    TraceSubComponentStatefulServiceGroupMember,
                    serviceName_,
                    "this={0} partitionId={1} replicaId={2} - CreateReplicator called with invalid arguments - The 'Reserved' field of FABRIC_REPLICATOR_SETTINGS_EX2 is not NULL.",
                    this,
                    partitionId_,
                    replicaId_
                    );

                return Common::ComUtility::OnPublicApiReturn(E_INVALIDARG);
            }
        }
        
    }

    Common::AcquireWriteLock lockPartition(lock_);

    if (NULL == outerStatefulServicePartition_)
    {
        ServiceGroupStatefulMemberEventSource::GetEvents().Error_8_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_
            );

        return Common::ComUtility::OnPublicApiReturn(Common::ErrorCodeValue::ObjectClosed);
    }

    if (NULL != innerStateProvider_)
    {
        ServiceGroupStatefulMemberEventSource::GetEvents().Error_9_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_
            );

        return Common::ComUtility::OnPublicApiReturn(Common::ErrorCodeValue::InvalidOperation);
    }

    ServiceGroupStatefulMemberEventSource::GetEvents().Info_15_StatefulServiceGroupMember(
        serviceName_,
        reinterpret_cast<uintptr_t>(this),
        partitionId_,
        replicaId_
        );

    HRESULT hr = outerStatefulServicePartition_->CreateReplicator(NULL, replicatorSettings, replicator, (IFabricStateReplicator**)&outerStateReplicator_);
    if (SUCCEEDED(hr))
    {
        ServiceGroupStatefulMemberEventSource::GetEvents().Info_16_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_
            );

        stateProvider->AddRef();
        innerStateProvider_ = stateProvider;

        hr = innerStateProvider_->QueryInterface(IID_IFabricAtomicGroupStateProvider, (LPVOID*)&innerAtomicGroupStateProvider_);
        if (FAILED(hr))
        {
            ServiceGroupStatefulMemberEventSource::GetEvents().Info_17_StatefulServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                replicaId_
                );
        }
        else
        {
            ServiceGroupStatefulMemberEventSource::GetEvents().Info_18_StatefulServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                replicaId_
                );
            ASSERT_IF(NULL == innerAtomicGroupStateProvider_, "Atomic group state provider should be available");
        }

        AddRef();
        *stateReplicator = (IFabricStateReplicator*)this;

        hr = outerStateReplicator_->QueryInterface(IID_IFabricAtomicGroupStateReplicator, (LPVOID*)&outerAtomicGroupStateReplicator_);
        ASSERT_IF(FAILED(hr), "Atomic group replicator should be available");
    }
    return Common::ComUtility::OnPublicApiReturn(hr);
}

//
// IFabricStatefulServicePartition2 methods
//
STDMETHODIMP CStatefulServiceUnit::ReportReplicaHealth(__in const FABRIC_HEALTH_INFORMATION* healthInformation)
{
    return ReportReplicaHealth2(healthInformation, nullptr);
}

STDMETHODIMP CStatefulServiceUnit::ReportPartitionHealth(__in const FABRIC_HEALTH_INFORMATION* healthInformation)
{
    return ReportPartitionHealth2(healthInformation, nullptr);
}

//
// IFabricStatefulServicePartition3 methods
//
STDMETHODIMP CStatefulServiceUnit::ReportReplicaHealth2(
    __in const FABRIC_HEALTH_INFORMATION* healthInformation,
    __in const FABRIC_HEALTH_REPORT_SEND_OPTIONS *sendOptions)
{
    if (NULL == healthInformation)
    {
        return Common::ComUtility::OnPublicApiReturn(E_POINTER);
    }

    IFabricStatefulServicePartition3* partition = NULL;
    {
        Common::AcquireReadLock lockPartition(lock_);

        if (NULL == outerStatefulServicePartition_)
        {
            ServiceGroupStatefulMemberEventSource::GetEvents().Error_8_StatefulServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                replicaId_
            );

            return Common::ComUtility::OnPublicApiReturn(Common::ErrorCodeValue::ObjectClosed);
        }
        else
        {
            partition = outerStatefulServicePartition_;
            partition->AddRef();
        }
    }

    WriteNoise(
        TraceSubComponentStatefulServiceGroupMember,
        serviceName_,
        "this={0} partitionId={1} replicaId={2} - Calling simulated stateful service partition ReportReplicaHealth",
        this,
        partitionId_,
        replicaId_
    );

    HRESULT hr = partition->ReportReplicaHealth2(healthInformation, sendOptions);
    partition->Release();

    return Common::ComUtility::OnPublicApiReturn(hr);
}

STDMETHODIMP CStatefulServiceUnit::ReportPartitionHealth2(
    __in const FABRIC_HEALTH_INFORMATION* healthInformation,
    __in const FABRIC_HEALTH_REPORT_SEND_OPTIONS *sendOptions)
{
    if (NULL == healthInformation)
    {
        return Common::ComUtility::OnPublicApiReturn(E_POINTER);
    }

    IFabricStatefulServicePartition3* partition = NULL;
    {
        Common::AcquireReadLock lockPartition(lock_);

        if (NULL == outerStatefulServicePartition_)
        {
            ServiceGroupStatefulMemberEventSource::GetEvents().Error_8_StatefulServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                replicaId_
            );

            return Common::ComUtility::OnPublicApiReturn(Common::ErrorCodeValue::ObjectClosed);
        }
        else
        {
            partition = outerStatefulServicePartition_;
            partition->AddRef();
        }
    }

    WriteNoise(
        TraceSubComponentStatefulServiceGroupMember,
        serviceName_,
        "this={0} partitionId={1} replicaId={2} - Calling simulated stateful service partition ReportPartitionHealth",
        this,
        partitionId_,
        replicaId_
    );

    HRESULT hr = partition->ReportPartitionHealth2(healthInformation, sendOptions);
    partition->Release();

    return Common::ComUtility::OnPublicApiReturn(hr);
}

//
// IFabricServiceGroupPartition methods.
//
STDMETHODIMP CStatefulServiceUnit::ResolveMember(
    __in FABRIC_URI name,
    __in REFIID riid,
    __out void ** member)
{
    if (NULL == name || NULL == member)
    {
        return Common::ComUtility::OnPublicApiReturn(E_POINTER);
    }

    IFabricServiceGroupPartition* group = NULL;
    {
        Common::AcquireReadLock lockPartition(lock_);

        if (NULL == outerServiceGroupPartition_)
        {
            ServiceGroupStatefulMemberEventSource::GetEvents().Error_8_StatefulServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                replicaId_
                );

            return Common::ComUtility::OnPublicApiReturn(Common::ErrorCodeValue::ObjectClosed);
        }
        else
        {
            group = outerServiceGroupPartition_;
            group->AddRef();
        }
    }

    WriteNoise(
        TraceSubComponentStatefulServiceGroupMember, 
        serviceName_,
        "this={0} partitionId={1} replicaId={2} - Calling simulated service group partition ResolveMember",
        this,
        partitionId_,
        replicaId_
        );

    HRESULT hr = group->ResolveMember(name, riid, member);
    group->Release();

    return Common::ComUtility::OnPublicApiReturn(hr);
}

//
// IFabricStateReplicator methods.
//
STDMETHODIMP CStatefulServiceUnit::BeginReplicate(
    __in IFabricOperationData* operationData,
    __in IFabricAsyncOperationCallback* callback,
    __out FABRIC_SEQUENCE_NUMBER* sequenceNumber,
    __out IFabricAsyncOperationContext** context
    )
{
    if (NULL == operationData || NULL == sequenceNumber || NULL == context)
    {
        return Common::ComUtility::OnPublicApiReturn(E_POINTER);
    }

    COutgoingOperationDataExtendedBuffer* extendedBufferOperation = new (std::nothrow) COutgoingOperationDataExtendedBuffer();
    if (NULL == extendedBufferOperation)
    {
        ServiceGroupStatefulMemberEventSource::GetEvents().Error_10_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_
            );

        return Common::ComUtility::OnPublicApiReturn(E_OUTOFMEMORY);
    }
    HRESULT hr = extendedBufferOperation->FinalConstruct(
        FABRIC_OPERATION_TYPE_NORMAL,
        FABRIC_INVALID_ATOMIC_GROUP_ID,
        operationData,
        partitionId_,
        TRUE
        );
    if (FAILED(hr))
    {
        ServiceGroupStatefulMemberEventSource::GetEvents().Error_11_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_
            );

        extendedBufferOperation->Release();
        return Common::ComUtility::OnPublicApiReturn(hr);
    }

    hr = extendedBufferOperation->SerializeExtendedOperationDataBuffer();
    if (FAILED(hr))
    {
        ServiceGroupStatefulMemberEventSource::GetEvents().Error_12_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_,
            hr
            );

        extendedBufferOperation->Release();
        return Common::ComUtility::OnPublicApiReturn(hr);
    }

    {
        Common::AcquireReadLock lockReplicator(lock_);

        if (outerStateReplicatorDestructed_)
        {
            ServiceGroupStatefulMemberEventSource::GetEvents().Error_13_StatefulServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                replicaId_
                );
        
            return Common::ComUtility::OnPublicApiReturn(Common::ErrorCodeValue::ObjectClosed);
        }
        outerStateReplicator_->AddRef();
    }

    hr = outerStateReplicator_->BeginReplicate(
        extendedBufferOperation, 
        callback, 
        sequenceNumber, 
        context
        );
    extendedBufferOperation->Release();
    outerStateReplicator_->Release();

    WriteNoise(
        TraceSubComponentStatefulServiceGroupMember, 
        serviceName_,
        "this={0} partitionId={1} replicaId={2} - Simulated stateful replicator BeginReplicate returned with hr={3}, sequenceNumber={4}",
        this,
        partitionId_,
        replicaId_,
        hr,
        *sequenceNumber
        );

    return Common::ComUtility::OnPublicApiReturn(hr);
}

STDMETHODIMP CStatefulServiceUnit::BeginReplicate2(
    /* [in] */ ULONG bufferCount,
    /* [in] */ FABRIC_OPERATION_DATA_BUFFER_EX1 const * buffers,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [out, retval] */ FABRIC_SEQUENCE_NUMBER * sequenceNumber,
    /* [out, retval] */ IFabricAsyncOperationContext ** context)
{
    for (ULONG count = 0; count < bufferCount; count++)
    {
        if (buffers[count].Buffer == NULL || buffers[count].Count == 0)
        {
            return Common::ComUtility::OnPublicApiReturn(E_POINTER);
        }
    }

    Common::ComPointer<IFabricOperationData> operationData;
    if (FAILED(CreateOperationData(bufferCount, buffers, operationData.InitializationAddress())))
    {
        return Common::ComUtility::OnPublicApiReturn(E_OUTOFMEMORY);
    }

    HRESULT hr = BeginReplicate(
        operationData.GetRawPointer(),
        callback,
        sequenceNumber,
        context);

    return Common::ComUtility::OnPublicApiReturn(hr);
}


STDMETHODIMP CStatefulServiceUnit::EndReplicate(__in IFabricAsyncOperationContext* context, __out FABRIC_SEQUENCE_NUMBER* sequenceNumber)
{
    if (NULL == context)
    {
        return Common::ComUtility::OnPublicApiReturn(E_POINTER);
    }

    {
        Common::AcquireReadLock lockReplicator(lock_);

        if (outerStateReplicatorDestructed_)
        {
            ServiceGroupStatefulMemberEventSource::GetEvents().Error_13_StatefulServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                replicaId_
                );
        
            return Common::ComUtility::OnPublicApiReturn(Common::ErrorCodeValue::ObjectClosed);
        }
        outerStateReplicator_->AddRef();
    }
    
    HRESULT hr = outerStateReplicator_->EndReplicate(context, sequenceNumber);
    outerStateReplicator_->Release();

    WriteNoise(
        TraceSubComponentStatefulServiceGroupMember, 
        serviceName_,
        "this={0} partitionId={1} replicaId={2} - Simulated stateful replicator EndReplicate returned with hr={3}, sequenceNumber={4}",
        this,
        partitionId_,
        replicaId_,
        hr,
        NULL == sequenceNumber ? FABRIC_INVALID_SEQUENCE_NUMBER : *sequenceNumber
        );

    return Common::ComUtility::OnPublicApiReturn(hr);
}

STDMETHODIMP CStatefulServiceUnit::GetCopyStream(__out IFabricOperationStream** operationStream)
{
    if (NULL == operationStream)
    {
        return Common::ComUtility::OnPublicApiReturn(E_POINTER);
    }

    BOOLEAN startCopyStream = FALSE;
    {
        Common::AcquireWriteLock lockReplicator(lock_);

        if (outerStateReplicatorDestructed_ || isClosing_)
        {
            ServiceGroupStatefulMemberEventSource::GetEvents().Error_13_StatefulServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                replicaId_
                );

            return Common::ComUtility::OnPublicApiReturn(Common::ErrorCodeValue::ObjectClosed);
        }

        if (NULL == copyOperationStream_)
        {
            ServiceGroupStatefulMemberEventSource::GetEvents().Error_14_StatefulServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                replicaId_
                );

            return Common::ComUtility::OnPublicApiReturn(Common::ErrorCodeValue::ObjectClosed);
        }
        
        startCopyStream = !copyStreamDrainingStarted_;
        if (startCopyStream)
        {
            outerStateReplicator_->AddRef();
        }
    }

    ServiceGroupStatefulMemberEventSource::GetEvents().Info_19_StatefulServiceGroupMember(
        serviceName_,
        reinterpret_cast<uintptr_t>(this),
        partitionId_,
        replicaId_
        );

    HRESULT hr = S_OK;
    if (startCopyStream)
    {
        ServiceGroupStatefulMemberEventSource::GetEvents().Info_20_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_
            );

        // 
        // This is idempotent and can be called outside the lock
        //
        hr = outerStateReplicator_->GetCopyStream(NULL);
        outerStateReplicator_->Release();
    }
    if (SUCCEEDED(hr))
    {
        Common::AcquireWriteLock lockReplicator(lock_);
        if (NULL != copyOperationStream_)
        {
            copyOperationStream_->AddRef();
            *operationStream = copyOperationStream_;
            copyStreamDrainingStarted_ = TRUE;
        }
        else
        {
            //
            // Replica has been closed in the meantime.
            //
            hr = FABRIC_E_OBJECT_CLOSED;
        }
    }
    else
    {
        ServiceGroupStatefulMemberEventSource::GetEvents().Error_15_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_,
            hr
            );
    }
    return Common::ComUtility::OnPublicApiReturn(hr);
}

STDMETHODIMP CStatefulServiceUnit::GetReplicationStream(__out IFabricOperationStream** operationStream)
{
    if (NULL == operationStream)
    {
        return Common::ComUtility::OnPublicApiReturn(E_POINTER);
    }

    BOOLEAN startReplicationStream = FALSE;
    {
        Common::AcquireWriteLock lockReplicator(lock_);

        if (outerStateReplicatorDestructed_ || isClosing_)
        {
            ServiceGroupStatefulMemberEventSource::GetEvents().Error_13_StatefulServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                replicaId_
                );

            return Common::ComUtility::OnPublicApiReturn(Common::ErrorCodeValue::ObjectClosed);
        }

        if (NULL == replicationOperationStream_)
        {
            ServiceGroupStatefulMemberEventSource::GetEvents().Error_16_StatefulServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                replicaId_
                );

            return Common::ComUtility::OnPublicApiReturn(Common::ErrorCodeValue::ObjectClosed);
        }

        startReplicationStream = !replicationStreamDrainingStarted_;
        if (startReplicationStream)
        {
            outerStateReplicator_->AddRef();
        }
    }

    HRESULT hr = S_OK;
    if (startReplicationStream)
    {
        ServiceGroupStatefulMemberEventSource::GetEvents().Info_21_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_
            );

        //
        // This is idempotent and can be called outside the lock
        //
        hr = outerStateReplicator_->GetReplicationStream(NULL);
        outerStateReplicator_->Release();
    }

    if (SUCCEEDED(hr))
    {
        Common::AcquireWriteLock lockReplicator(lock_);

        if (NULL != replicationOperationStream_)
        {
            replicationOperationStream_->AddRef();
            *operationStream = replicationOperationStream_;
            replicationStreamDrainingStarted_ = TRUE;
        }
        else
        {
            //
            // Replica may have been closed in the meantime.
            //
            hr = FABRIC_E_OBJECT_CLOSED;
        }
    }
    return Common::ComUtility::OnPublicApiReturn(hr);
}

STDMETHODIMP CStatefulServiceUnit::UpdateReplicatorSettings(__in FABRIC_REPLICATOR_SETTINGS const * replicatorSettings)
{
    UNREFERENCED_PARAMETER(replicatorSettings);
    return Common::ComUtility::OnPublicApiReturn(E_NOTIMPL);
}

STDMETHODIMP CStatefulServiceUnit::GetReplicatorSettings(__out IFabricReplicatorSettingsResult** replicatorSettings)
{
    return Common::ComUtility::OnPublicApiReturn(outerStateReplicator_->GetReplicatorSettings(replicatorSettings));
}

//
// IFabricInternalStateReplicator implementation
//
STDMETHODIMP CStatefulServiceUnit::ReserveSequenceNumber(
    __in BOOLEAN alwaysReserveWhenPrimary,
    __out FABRIC_SEQUENCE_NUMBER * sequenceNumber)
{
    UNREFERENCED_PARAMETER(alwaysReserveWhenPrimary);
    UNREFERENCED_PARAMETER(sequenceNumber);
    return Common::ComUtility::OnPublicApiReturn(E_NOTIMPL);
}

STDMETHODIMP CStatefulServiceUnit::BeginReplicateBatch(
    __in IFabricBatchOperationData* operationData,
    __in IFabricAsyncOperationCallback* callback,
    __out IFabricAsyncOperationContext** context
    )
{
    UNREFERENCED_PARAMETER(operationData);
    UNREFERENCED_PARAMETER(callback);
    UNREFERENCED_PARAMETER(context);
    return Common::ComUtility::OnPublicApiReturn(E_NOTIMPL);
}

STDMETHODIMP CStatefulServiceUnit::EndReplicateBatch(__in IFabricAsyncOperationContext* context)
{
    UNREFERENCED_PARAMETER(context);
    return Common::ComUtility::OnPublicApiReturn(E_NOTIMPL);
}

STDMETHODIMP CStatefulServiceUnit::GetBatchReplicationStream(__out IFabricBatchOperationStream** operationStream)
{
    UNREFERENCED_PARAMETER(operationStream);
    return Common::ComUtility::OnPublicApiReturn(E_NOTIMPL);
}

STDMETHODIMP CStatefulServiceUnit::GetReplicationQueueCounters(__out FABRIC_INTERNAL_REPLICATION_QUEUE_COUNTERS* counters)
{
    UNREFERENCED_PARAMETER(counters);
    return Common::ComUtility::OnPublicApiReturn(E_NOTIMPL);
}
//
// Helper methods.
//
HRESULT CStatefulServiceUnit::EnqueueCopyOperation(__in_opt IFabricOperation* operation)
{
    const FABRIC_OPERATION_METADATA* operationMetadata = NULL;
    if (NULL != operation)
    {
        operationMetadata = operation->get_Metadata();
        ASSERT_IF(NULL == operationMetadata, "get_Metadata");
    }

    ServiceGroupStatefulMemberEventSource::GetEvents().Info_22_StatefulServiceGroupMember(
        serviceName_,
        reinterpret_cast<uintptr_t>(this),
        partitionId_,
        replicaId_,
        reinterpret_cast<uintptr_t>(operation),
        (NULL == operation) ? -1 : operationMetadata->SequenceNumber
        );

    ASSERT_IF(NULL == copyOperationStream_, "Unexpected copy operation stream");
    return copyOperationStream_->EnqueueOperation(operation);
}

HRESULT CStatefulServiceUnit::EnqueueReplicationOperation(__in_opt IFabricOperation* operation)
{
    const FABRIC_OPERATION_METADATA* operationMetadata = NULL;
    if (NULL != operation)
    {
        operationMetadata = operation->get_Metadata();
        ASSERT_IF(NULL == operationMetadata, "get_Metadata");
    }

    ServiceGroupStatefulMemberEventSource::GetEvents().Info_23_StatefulServiceGroupMember(
        serviceName_,
        reinterpret_cast<uintptr_t>(this),
        partitionId_,
        replicaId_,
        reinterpret_cast<uintptr_t>(operation),
        (NULL == operation) ? -1 : operationMetadata->SequenceNumber
        );

    ASSERT_IF(NULL == replicationOperationStream_, "Unexpected replication operation stream");
    return replicationOperationStream_->EnqueueOperation(operation);
}

HRESULT CStatefulServiceUnit::ForceDrainReplicationStream(__in BOOLEAN waitForNullDispatch)
{
    ServiceGroupStatefulMemberEventSource::GetEvents().Warning_1_StatefulServiceGroupMember(
        serviceName_,
        reinterpret_cast<uintptr_t>(this),
        partitionId_,
        replicaId_,
        waitForNullDispatch
        );

    ASSERT_IF(NULL == replicationOperationStream_, "Unexpected replication operation stream");
    return replicationOperationStream_->ForceDrain(E_UNEXPECTED, waitForNullDispatch);
}

HRESULT CStatefulServiceUnit::ClearOperationStreams(void)
{
    ServiceGroupStatefulMemberEventSource::GetEvents().Info_24_StatefulServiceGroupMember(
        serviceName_,
        reinterpret_cast<uintptr_t>(this),
        partitionId_,
        replicaId_
        );

    Common::AcquireWriteLock lockReplicator(lock_);

    if (NULL != copyOperationStream_)
    {
        copyOperationStream_->ForceDrain(S_OK, FALSE);
        copyOperationStream_->Release();
        copyOperationStream_ = NULL;
        copyStreamDrainingStarted_ = FALSE;
    }
    if (NULL != replicationOperationStream_)
    {
        replicationOperationStream_->ForceDrain(S_OK, FALSE);
        replicationOperationStream_->Release();
        replicationOperationStream_ = NULL;
        replicationStreamDrainingStarted_ = FALSE;
    }
    return S_OK;
}

HRESULT CStatefulServiceUnit::InnerQueryInterface(__in REFIID riid, __out void ** ppvObject)
{
    IFabricInternalBrokeredService * broker = NULL;

    HRESULT hr = innerStatefulServiceReplica_->QueryInterface(IID_IFabricInternalBrokeredService, reinterpret_cast<void**>(&broker));
    if (SUCCEEDED(hr))
    {
        ServiceGroupStatefulMemberEventSource::GetEvents().Info_25_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_
            );

        IUnknown * brokeredServiceReplica = NULL;
        hr = broker->GetBrokeredService(&brokeredServiceReplica);

        if (FAILED(hr))
        {
            ServiceGroupStatefulMemberEventSource::GetEvents().Info_26_StatefulServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                replicaId_,
                hr
                );
            
            broker->Release();
            return hr;
        }

        hr = brokeredServiceReplica->QueryInterface(riid, ppvObject);

        broker->Release();
        brokeredServiceReplica->Release();

        return hr;
    }

    return innerStatefulServiceReplica_->QueryInterface(riid, ppvObject);
}

HRESULT CStatefulServiceUnit::StartOperationStreams(void)
{
    HRESULT hr = S_OK;

    ServiceGroupStatefulMemberEventSource::GetEvents().Info_27_StatefulServiceGroupMember(
        serviceName_,
        reinterpret_cast<uintptr_t>(this),
        partitionId_,
        replicaId_
        );

    ASSERT_IFNOT(NULL == copyOperationStream_, "Unexpected copy operation stream");
    ASSERT_IFNOT(NULL == replicationOperationStream_, "Unexpected replicated operation stream");

    Common::AcquireReadLock lockReplicator(lock_);

    if (isFaulted_)
    {
        ServiceGroupStatefulMemberEventSource::GetEvents().Error_17_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_
            );

        return E_FAIL;
    }

    copyOperationStream_ = new (std::nothrow) COperationStream(partitionId_, TRUE);
    if (NULL == copyOperationStream_)
    {
        ServiceGroupStatefulMemberEventSource::GetEvents().Error_18_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_
            );

        return E_OUTOFMEMORY;
    }

    hr = copyOperationStream_->FinalConstruct();
    if (FAILED(hr))
    {
        ServiceGroupStatefulMemberEventSource::GetEvents().Error_19_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_
            );

        copyOperationStream_->Release();
        copyOperationStream_ = NULL;
        return hr;
    }

    replicationOperationStream_ = new (std::nothrow) COperationStream(partitionId_, FALSE);
    if (NULL == replicationOperationStream_)
    {
        ServiceGroupStatefulMemberEventSource::GetEvents().Error_20_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_
            );

        copyOperationStream_->Release();
        copyOperationStream_ = NULL;
        return E_OUTOFMEMORY;
    }


    hr = replicationOperationStream_->FinalConstruct();
    if (FAILED(hr))
    {
        ServiceGroupStatefulMemberEventSource::GetEvents().Error_21_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_
            );

        copyOperationStream_->Release();
        copyOperationStream_ = NULL;
        replicationOperationStream_->Release();
        replicationOperationStream_ = NULL;
        return hr;
    }

    //
    // In the special case where a primary is demoted to a secondary, the copy stream does not contain anything
    // since the replica is already up-to-date. We mark the copy operation stream as being completed successfully.
    //
    if (FABRIC_REPLICA_ROLE_PRIMARY == currentReplicaRole_)
    {
        ServiceGroupStatefulMemberEventSource::GetEvents().Info_28_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_
            );
        
        hr = EnqueueCopyOperation(NULL);
        if (SUCCEEDED(hr))
        {
            copyStreamDrainingStarted_ = TRUE;
            copyOperationStream_->ForceDrain(S_OK, FALSE);
        }
    }

    return hr;
}

HRESULT CStatefulServiceUnit::DrainUpdateEpoch(void)
{
    ServiceGroupStatefulMemberEventSource::GetEvents().Info_29_StatefulServiceGroupMember(
        serviceName_,
        reinterpret_cast<uintptr_t>(this),
        partitionId_,
        replicaId_,
        ROLE_TEXT(currentReplicaRole_)
        );

    HRESULT hrCopyDrain = S_OK;
    HRESULT hrReplicationDrain = S_OK;
    if (FABRIC_REPLICA_ROLE_IDLE_SECONDARY == currentReplicaRole_ || FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY == currentReplicaRole_)
    {
        ASSERT_IF(NULL == copyOperationStream_, "Unexpected copy operation stream");
        ASSERT_IF(NULL == replicationOperationStream_, "Unexpected replicated operation stream");

        ServiceGroupStatefulMemberEventSource::GetEvents().Info_30_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_
            );
        
        hrCopyDrain = copyOperationStream_->Drain();
        if (FAILED(hrCopyDrain))
        {
            ServiceGroupStatefulMemberEventSource::GetEvents().Warning_2_StatefulServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                replicaId_,
                hrCopyDrain
                );
        }
        else
        {
            ServiceGroupStatefulMemberEventSource::GetEvents().Info_31_StatefulServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                replicaId_
                );
        }

        CUpdateEpochOperation* operationUpdateEpoch = new (std::nothrow) CUpdateEpochOperation();
        if (NULL == operationUpdateEpoch)
        {
            ServiceGroupStatefulMemberEventSource::GetEvents().Error_23_StatefulServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                replicaId_
                );

            return E_OUTOFMEMORY;
        }

        hrReplicationDrain = operationUpdateEpoch->FinalConstruct();
        if (FAILED(hrReplicationDrain))
        {
            ServiceGroupStatefulMemberEventSource::GetEvents().Error_24_StatefulServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                replicaId_
                );

            operationUpdateEpoch->Release();
            return hrReplicationDrain;
        }

        hrReplicationDrain = replicationOperationStream_->EnqueueUpdateEpochOperation(operationUpdateEpoch);
        if (FAILED(hrReplicationDrain))
        {
            ServiceGroupStatefulMemberEventSource::GetEvents().Error_25_StatefulServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                replicaId_
                );

            operationUpdateEpoch->Release();
            return hrReplicationDrain;
        }

        ServiceGroupStatefulMemberEventSource::GetEvents().Info_32_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_
            );

        hrReplicationDrain = operationUpdateEpoch->Barrier();
        operationUpdateEpoch->Release();
        if (FAILED(hrReplicationDrain))
        {
            ServiceGroupStatefulMemberEventSource::GetEvents().Error_26_StatefulServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                replicaId_,
                hrReplicationDrain
                );
        }
        else
        {
            ServiceGroupStatefulMemberEventSource::GetEvents().Info_33_StatefulServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                replicaId_
                );
        }
    }

    ServiceGroupStatefulMemberEventSource::GetEvents().Info_34_StatefulServiceGroupMember(
        serviceName_,
        reinterpret_cast<uintptr_t>(this),
        partitionId_,
        replicaId_
        );

    if (FAILED(hrCopyDrain))
    {
        return hrCopyDrain;
    }
    if (FAILED(hrReplicationDrain))
    {
        return hrReplicationDrain;
    }
    return S_OK;
}

//
// IFabricStateProvider methods.
//
STDMETHODIMP CStatefulServiceUnit::BeginUpdateEpoch(
    __in FABRIC_EPOCH const* epoch, 
    __in FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber,
    __in IFabricAsyncOperationCallback* callback,
    __out IFabricAsyncOperationContext** context
    )
{
    ASSERT_IF(NULL == epoch, "Unexpected epoch");
    if (epoch->Reserved != NULL)
    {
        return E_INVALIDARG; 
    }

    if (NULL == callback || NULL == context)
    {
        return E_POINTER;
    }

    ServiceGroupStatefulMemberEventSource::GetEvents().Info_35_StatefulServiceGroupMember(
        serviceName_,
        reinterpret_cast<uintptr_t>(this),
        partitionId_,
        replicaId_
        );

    HRESULT hr = innerStateProvider_->BeginUpdateEpoch(epoch, previousEpochLastSequenceNumber, callback, context);
    if (FAILED(hr))
    {
        ServiceGroupStatefulMemberEventSource::GetEvents().Error_27_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_,
            hr
            );
    }
    return hr;
}

STDMETHODIMP CStatefulServiceUnit::EndUpdateEpoch(
    __in IFabricAsyncOperationContext* context
    )
{
    if (NULL == context)
    {
        return E_POINTER;
    }

    ServiceGroupStatefulMemberEventSource::GetEvents().Info_36_StatefulServiceGroupMember(
        serviceName_,
        reinterpret_cast<uintptr_t>(this),
        partitionId_,
        replicaId_
        );

    HRESULT hr = innerStateProvider_->EndUpdateEpoch(context);
    if (FAILED(hr))
    {
        ServiceGroupStatefulMemberEventSource::GetEvents().Error_28_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_,
            hr
            );
    }
    return hr;
}

STDMETHODIMP CStatefulServiceUnit::GetLastCommittedSequenceNumber(__out FABRIC_SEQUENCE_NUMBER* sequenceNumber)
{
    if (NULL == sequenceNumber)
    {
        return E_POINTER;
    }

    ServiceGroupStatefulMemberEventSource::GetEvents().Info_37_StatefulServiceGroupMember(
        serviceName_,
        reinterpret_cast<uintptr_t>(this),
        partitionId_,
        replicaId_
        );

    HRESULT hr = innerStateProvider_->GetLastCommittedSequenceNumber(sequenceNumber);
    if (FAILED(hr))
    {
        ServiceGroupStatefulMemberEventSource::GetEvents().Error_29_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_,
            hr
            );
    }
    return hr;
}

STDMETHODIMP CStatefulServiceUnit::BeginOnDataLoss(
    __in IFabricAsyncOperationCallback* callback,
    __out IFabricAsyncOperationContext** context
    )
{
    if (NULL == callback || NULL == context)
    {
        return E_POINTER;
    }

    ServiceGroupStatefulMemberEventSource::GetEvents().Info_38_StatefulServiceGroupMember(
        serviceName_,
        reinterpret_cast<uintptr_t>(this),
        partitionId_,
        replicaId_
        );

    HRESULT hr = innerStateProvider_->BeginOnDataLoss(callback, context);
    if (FAILED(hr))
    {
        ServiceGroupStatefulMemberEventSource::GetEvents().Error_30_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_,
            hr
            );
    }
    return hr;
}

STDMETHODIMP CStatefulServiceUnit::EndOnDataLoss(
    __in IFabricAsyncOperationContext* context,
    __out BOOLEAN* isStateChanged
    )
{
    if (NULL == context || NULL == isStateChanged)
    {
        return E_POINTER;
    }

    ServiceGroupStatefulMemberEventSource::GetEvents().Info_39_StatefulServiceGroupMember(
        serviceName_,
        reinterpret_cast<uintptr_t>(this),
        partitionId_,
        replicaId_
        );

    HRESULT hr = innerStateProvider_->EndOnDataLoss(context, isStateChanged);
    if (FAILED(hr))
    {
        ServiceGroupStatefulMemberEventSource::GetEvents().Error_31_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_,
            hr
            );
    }
    return hr;
}

STDMETHODIMP CStatefulServiceUnit::GetCopyContext(__out IFabricOperationDataStream** copyContextEnumerator)
{
    if (NULL == copyContextEnumerator)
    {
        return E_POINTER;
    }

    ServiceGroupStatefulMemberEventSource::GetEvents().Info_40_StatefulServiceGroupMember(
        serviceName_,
        reinterpret_cast<uintptr_t>(this),
        partitionId_,
        replicaId_
        );

    HRESULT hr = innerStateProvider_->GetCopyContext(copyContextEnumerator);
    if (FAILED(hr))
    {
        ServiceGroupStatefulMemberEventSource::GetEvents().Error_32_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_,
            hr
            );
    }
    return hr;
}

STDMETHODIMP CStatefulServiceUnit::GetCopyState(
    __in FABRIC_SEQUENCE_NUMBER uptoSequenceNumber,
    __in IFabricOperationDataStream* copyContextEnumerator,
    __out IFabricOperationDataStream** copyStateEnumerator
    )
{
    if (NULL == copyStateEnumerator)
    {
        return E_POINTER;
    }

    ServiceGroupStatefulMemberEventSource::GetEvents().Info_41_StatefulServiceGroupMember(
        serviceName_,
        reinterpret_cast<uintptr_t>(this),
        partitionId_,
        replicaId_
        );

    HRESULT hr = innerStateProvider_->GetCopyState(
        uptoSequenceNumber, 
        copyContextEnumerator, 
        copyStateEnumerator
        );
    if (FAILED(hr))
    {
        ServiceGroupStatefulMemberEventSource::GetEvents().Error_33_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_,
            hr
            );
    }
    return hr;
}

//
// IOperationDataFactory methods.
//
STDMETHODIMP CStatefulServiceUnit::CreateOperationData(
    __in_ecount( segmentSizesCount ) ULONG * segmentSizes,
    __in ULONG segmentSizesCount,
    __out IFabricOperationData** operationData
    )
{
    ASSERT_IF(segmentSizesCount == 0, "Unexpected segmentSizesCount");
    ASSERT_IF(NULL == segmentSizes, "Unexpected segmentSizes");
    
    COperationDataFactory* operationDataFactory = new (std::nothrow) COperationDataFactory();
    if (NULL == operationDataFactory)
    {
        return E_OUTOFMEMORY;
    }
    if (FAILED(operationDataFactory->FinalConstruct(segmentSizesCount, segmentSizes)))
    {
        operationDataFactory->Release();
        return E_OUTOFMEMORY;
    }
    *operationData = operationDataFactory;
    return S_OK;
}
 
STDMETHODIMP CStatefulServiceUnit::CreateOperationData(
    __in ULONG bufferCount,
    __in FABRIC_OPERATION_DATA_BUFFER_EX1 const * buffers,
    __out IFabricOperationData** operationData
    )
{
    ASSERT_IF(bufferCount == 0, "Unexpected bufferCount");
    ASSERT_IF(NULL == buffers, "Unexpected buffers");
    
    COperationDataFactory* operationDataFactory = new (std::nothrow) COperationDataFactory();
    if (NULL == operationDataFactory)
    {
        return E_OUTOFMEMORY;
    }
    if (FAILED(operationDataFactory->FinalConstruct(bufferCount, buffers)))
    {
        operationDataFactory->Release();
        return E_OUTOFMEMORY;
    }
    *operationData = operationDataFactory;
    return S_OK;
}

//
// IFabricAtomicGroupStateReplicator methods.
//
STDMETHODIMP CStatefulServiceUnit::CreateAtomicGroup(__out FABRIC_ATOMIC_GROUP_ID* atomicGroupId)
{
    {
        Common::AcquireReadLock lockReplicator(lock_);

        if (!outerStateReplicatorDestructed_)
        {
            if (NULL == innerAtomicGroupStateProvider_)
            {
                return Common::ComUtility::OnPublicApiReturn(Common::ErrorCodeValue::InvalidOperation);
            }

            outerAtomicGroupStateReplicator_->AddRef();
        }
        else
        {
            ServiceGroupStatefulMemberEventSource::GetEvents().Error_34_StatefulServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                replicaId_
                );

            return Common::ComUtility::OnPublicApiReturn(Common::ErrorCodeValue::ObjectClosed);
        }
    }

    WriteNoise(
        TraceSubComponentStatefulServiceGroupMember, 
        serviceName_,
        "this={0} partitionId={1} replicaId={2} - Calling simulated stateful replicator CreateAtomicGroup",
        this,
        partitionId_,
        replicaId_
        );

    HRESULT hr = outerAtomicGroupStateReplicator_->CreateAtomicGroup(atomicGroupId);
    outerAtomicGroupStateReplicator_->Release();

    return Common::ComUtility::OnPublicApiReturn(hr);
}

STDMETHODIMP CStatefulServiceUnit::BeginReplicateAtomicGroupOperation(
    __in FABRIC_ATOMIC_GROUP_ID atomicGroupId,
    __in IFabricOperationData* operationData,
    __in IFabricAsyncOperationCallback* callback,
    __out FABRIC_SEQUENCE_NUMBER* operationSequenceNumber,
    __out IFabricAsyncOperationContext** context
    )
{
    if (NULL == operationSequenceNumber || NULL == context || NULL == operationData)
    {
        return Common::ComUtility::OnPublicApiReturn(E_POINTER);
    }
    if (0 > atomicGroupId)
    {
        return Common::ComUtility::OnPublicApiReturn(E_INVALIDARG);
    }

    COutgoingOperationDataExtendedBuffer* extendedBufferOperation = new (std::nothrow) COutgoingOperationDataExtendedBuffer();
    if (NULL == extendedBufferOperation)
    {
        ServiceGroupStatefulMemberEventSource::GetEvents().Error_10_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_
            );

        return Common::ComUtility::OnPublicApiReturn(E_OUTOFMEMORY);
    }
    HRESULT hr = extendedBufferOperation->FinalConstruct(
        FABRIC_OPERATION_TYPE_ATOMIC_GROUP_OPERATION,
        atomicGroupId,
        operationData,
        partitionId_,
        TRUE
        );
    if (FAILED(hr))
    {
        ServiceGroupStatefulMemberEventSource::GetEvents().Error_11_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_
            );

        extendedBufferOperation->Release();
        return Common::ComUtility::OnPublicApiReturn(hr);
    }

    {
        Common::AcquireReadLock lockReplicator(lock_);

        if (outerStateReplicatorDestructed_)
        {
            ServiceGroupStatefulMemberEventSource::GetEvents().Error_34_StatefulServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                replicaId_
                );
        
            extendedBufferOperation->Release();
            return Common::ComUtility::OnPublicApiReturn(Common::ErrorCodeValue::ObjectClosed);
        }

        if (NULL == innerAtomicGroupStateProvider_)
        {
            extendedBufferOperation->Release();
            return Common::ComUtility::OnPublicApiReturn(Common::ErrorCodeValue::InvalidOperation);
        }

        outerAtomicGroupStateReplicator_->AddRef();
    }
    
    WriteNoise(
        TraceSubComponentStatefulServiceGroupMember, 
        serviceName_,
        "this={0} partitionId={1} replicaId={2} - Calling simulated atomic group stateful replicator BeginReplicateAtomicGroupOperation",
        this,
        partitionId_,
        replicaId_
        );

    hr = outerAtomicGroupStateReplicator_->BeginReplicateAtomicGroupOperation(
        atomicGroupId, 
        extendedBufferOperation, 
        callback, 
        operationSequenceNumber, 
        context
        );
    extendedBufferOperation->Release();
    outerAtomicGroupStateReplicator_->Release();
    return Common::ComUtility::OnPublicApiReturn(hr);
}

STDMETHODIMP CStatefulServiceUnit::EndReplicateAtomicGroupOperation(__in IFabricAsyncOperationContext* context, __out FABRIC_SEQUENCE_NUMBER* operationSequenceNumber)
{
    if (NULL == context)
    {
        return Common::ComUtility::OnPublicApiReturn(E_POINTER);
    }

    {
        Common::AcquireReadLock lockReplicator(lock_);

        if (outerStateReplicatorDestructed_)
        {
            ServiceGroupStatefulMemberEventSource::GetEvents().Error_34_StatefulServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                replicaId_
                );
        
            return Common::ComUtility::OnPublicApiReturn(Common::ErrorCodeValue::ObjectClosed);
        }

        if (NULL == innerAtomicGroupStateProvider_)
        {
            return Common::ComUtility::OnPublicApiReturn(Common::ErrorCodeValue::InvalidOperation);
        }

        outerAtomicGroupStateReplicator_->AddRef();
    }

    WriteNoise(
        TraceSubComponentStatefulServiceGroupMember, 
        serviceName_,
        "this={0} partitionId={1} replicaId={2} - Calling simulated atomic group stateful replicator EndReplicateAtomicGroupOperation",
        this,
        partitionId_,
        replicaId_
        );

    HRESULT hr = outerAtomicGroupStateReplicator_->EndReplicateAtomicGroupOperation(context, operationSequenceNumber);
    outerAtomicGroupStateReplicator_->Release();
    return Common::ComUtility::OnPublicApiReturn(hr);
}

STDMETHODIMP CStatefulServiceUnit::BeginReplicateAtomicGroupCommit(
    __in FABRIC_ATOMIC_GROUP_ID atomicGroupId,
    __in IFabricAsyncOperationCallback* callback,
    __out FABRIC_SEQUENCE_NUMBER* commitSequenceNumber,
    __out IFabricAsyncOperationContext** context
    )
{
    if (NULL == commitSequenceNumber || NULL == context)
    {
        return Common::ComUtility::OnPublicApiReturn(E_POINTER);
    }
    if (0 > atomicGroupId)
    {
        return Common::ComUtility::OnPublicApiReturn(E_INVALIDARG);
    }

    COutgoingOperationDataExtendedBuffer* commitAtomicGroupOperation = new (std::nothrow) COutgoingOperationDataExtendedBuffer();
    if (NULL == commitAtomicGroupOperation)
    {
        commitAtomicGroupOperation->Release();
        return Common::ComUtility::OnPublicApiReturn(E_OUTOFMEMORY);
    }
    HRESULT hr = commitAtomicGroupOperation->FinalConstruct(FABRIC_OPERATION_TYPE_COMMIT_ATOMIC_GROUP, atomicGroupId, NULL, partitionId_, TRUE);
    if (FAILED(hr))
    {
        commitAtomicGroupOperation->Release();
        return Common::ComUtility::OnPublicApiReturn(hr);
    }

    {
        Common::AcquireReadLock lockReplicator(lock_);

        if (outerStateReplicatorDestructed_)
        {
            ServiceGroupStatefulMemberEventSource::GetEvents().Error_34_StatefulServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                replicaId_
                );

            commitAtomicGroupOperation->Release();
            return Common::ComUtility::OnPublicApiReturn(Common::ErrorCodeValue::ObjectClosed);
        }

        if (NULL == innerAtomicGroupStateProvider_)
        {
            commitAtomicGroupOperation->Release();
            return Common::ComUtility::OnPublicApiReturn(Common::ErrorCodeValue::InvalidOperation);
        }

        outerAtomicGroupStateReplicator_->AddRef();
    }
    
    WriteNoise(
        TraceSubComponentStatefulServiceGroupMember, 
        serviceName_,
        "this={0} partitionId={1} replicaId={2} - Calling simulated atomic group stateful replicator BeginReplicateAtomicGroupCommit",
        this,
        partitionId_,
        replicaId_
        );

    hr = outerAtomicGroupStateReplicator_->BeginReplicateAtomicGroupOperation(
        atomicGroupId, 
        commitAtomicGroupOperation, 
        callback, 
        commitSequenceNumber, 
        context
        );
    outerAtomicGroupStateReplicator_->Release();
    commitAtomicGroupOperation->Release();
    return Common::ComUtility::OnPublicApiReturn(hr);
}

STDMETHODIMP CStatefulServiceUnit::EndReplicateAtomicGroupCommit(__in IFabricAsyncOperationContext* context, __out FABRIC_SEQUENCE_NUMBER* commitSequenceNumber)
{
    if (NULL == context)
    {
        return Common::ComUtility::OnPublicApiReturn(E_POINTER);
    }

    {
        Common::AcquireReadLock lockReplicator(lock_);

        if (outerStateReplicatorDestructed_)
        {
            ServiceGroupStatefulMemberEventSource::GetEvents().Error_34_StatefulServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                replicaId_
                );

            return Common::ComUtility::OnPublicApiReturn(Common::ErrorCodeValue::ObjectClosed);
        }

        if (NULL == innerAtomicGroupStateProvider_)
        {

            return Common::ComUtility::OnPublicApiReturn(Common::ErrorCodeValue::InvalidOperation);
        }

        outerAtomicGroupStateReplicator_->AddRef();
    }
    
    WriteNoise(
        TraceSubComponentStatefulServiceGroupMember, 
        serviceName_,
        "this={0} partitionId={1} replicaId={2} - Calling simulated atomic group stateful replicator EndReplicateAtomicGroupCommit",
        this,
        partitionId_,
        replicaId_
        );

    HRESULT hr = outerAtomicGroupStateReplicator_->EndReplicateAtomicGroupCommit(context, commitSequenceNumber);
    outerAtomicGroupStateReplicator_->Release();
    return Common::ComUtility::OnPublicApiReturn(hr);
}

STDMETHODIMP CStatefulServiceUnit::BeginReplicateAtomicGroupRollback(
    __in FABRIC_ATOMIC_GROUP_ID atomicGroupId,
    __in IFabricAsyncOperationCallback* callback,
    __out FABRIC_SEQUENCE_NUMBER* rollbackSequenceNumber,
    __out IFabricAsyncOperationContext** context
    )
{
    if (NULL == rollbackSequenceNumber || NULL == context)
    {
        return Common::ComUtility::OnPublicApiReturn(E_POINTER);
    }
    if (0 > atomicGroupId)
    {
        return Common::ComUtility::OnPublicApiReturn(E_INVALIDARG);
    }

    COutgoingOperationDataExtendedBuffer* rollbackAtomicGroupOperation = new (std::nothrow) COutgoingOperationDataExtendedBuffer();
    if (NULL == rollbackAtomicGroupOperation)
    {
        rollbackAtomicGroupOperation->Release();
        return Common::ComUtility::OnPublicApiReturn(E_OUTOFMEMORY);
    }

    HRESULT hr = rollbackAtomicGroupOperation->FinalConstruct(FABRIC_OPERATION_TYPE_ROLLBACK_ATOMIC_GROUP, atomicGroupId, NULL, partitionId_, TRUE);
    if (FAILED(hr))
    {
        rollbackAtomicGroupOperation->Release();
        return Common::ComUtility::OnPublicApiReturn(hr);
    }

    {
        Common::AcquireReadLock lockReplicator(lock_);

        if (outerStateReplicatorDestructed_)
        {
            ServiceGroupStatefulMemberEventSource::GetEvents().Error_34_StatefulServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                replicaId_
                );
        
            rollbackAtomicGroupOperation->Release();
            return Common::ComUtility::OnPublicApiReturn(Common::ErrorCodeValue::ObjectClosed);
        }

        if (NULL == innerAtomicGroupStateProvider_)
        {
            rollbackAtomicGroupOperation->Release();
            return Common::ComUtility::OnPublicApiReturn(Common::ErrorCodeValue::InvalidOperation);
        }

        outerAtomicGroupStateReplicator_->AddRef();
    }
    
    WriteNoise(
        TraceSubComponentStatefulServiceGroupMember, 
        serviceName_,
        "this={0} partitionId={1} replicaId={2} - Calling simulated atomic group stateful replicator BeginReplicateAtomicGroupRollback",
        this,
        partitionId_,
        replicaId_
        );

    hr = outerAtomicGroupStateReplicator_->BeginReplicateAtomicGroupOperation(
        atomicGroupId, 
        rollbackAtomicGroupOperation, 
        callback, 
        rollbackSequenceNumber, 
        context
        );
    outerAtomicGroupStateReplicator_->Release();
    rollbackAtomicGroupOperation->Release();
    return Common::ComUtility::OnPublicApiReturn(hr);
}

STDMETHODIMP CStatefulServiceUnit::EndReplicateAtomicGroupRollback(__in IFabricAsyncOperationContext* context, __out FABRIC_SEQUENCE_NUMBER* rollbackSequenceNumber)
{

    if (NULL == context)
    {
        return Common::ComUtility::OnPublicApiReturn(E_POINTER);
    }

    {
        Common::AcquireReadLock lockReplicator(lock_);

        if (outerStateReplicatorDestructed_)
        {
            ServiceGroupStatefulMemberEventSource::GetEvents().Error_34_StatefulServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                replicaId_
                );
        
            return Common::ComUtility::OnPublicApiReturn(Common::ErrorCodeValue::ObjectClosed);
        }

        if (NULL == innerAtomicGroupStateProvider_)
        {
            return Common::ComUtility::OnPublicApiReturn(Common::ErrorCodeValue::InvalidOperation);
        }

        outerAtomicGroupStateReplicator_->AddRef();
    }
    
    WriteNoise(
        TraceSubComponentStatefulServiceGroupMember, 
        serviceName_,
        "this={0} partitionId={1} replicaId={2} - Calling simulated atomic group stateful replicator IFabricAtomicGroupStateReplicator::EndReplicateAtomicGroupRollback",
        this,
        partitionId_,
        replicaId_
        );

    HRESULT hr = outerAtomicGroupStateReplicator_->EndReplicateAtomicGroupRollback(context, rollbackSequenceNumber);
    outerAtomicGroupStateReplicator_->Release();
    return Common::ComUtility::OnPublicApiReturn(hr);
}

//
// IFabricAtomicGroupStateProvider methods.
//
STDMETHODIMP CStatefulServiceUnit::BeginAtomicGroupCommit(
    __in FABRIC_ATOMIC_GROUP_ID atomicGroupId,
    __in FABRIC_SEQUENCE_NUMBER commitSequenceNumber,
    __in IFabricAsyncOperationCallback* callback,
    __out IFabricAsyncOperationContext** context
    )
{
    ASSERT_IF(NULL == innerAtomicGroupStateProvider_, "Unexpected call");

    ServiceGroupStatefulMemberEventSource::GetEvents().Info_42_StatefulServiceGroupMember(
        serviceName_,
        reinterpret_cast<uintptr_t>(this),
        partitionId_,
        replicaId_,
        atomicGroupId,
        commitSequenceNumber
        );

    HRESULT hr = innerAtomicGroupStateProvider_->BeginAtomicGroupCommit(atomicGroupId, commitSequenceNumber, callback, context);
    if (FAILED(hr))
    {
        ServiceGroupStatefulMemberEventSource::GetEvents().Error_35_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_,
            atomicGroupId,
            hr
            );
    }
    return hr;
}

STDMETHODIMP CStatefulServiceUnit::EndAtomicGroupCommit(__in IFabricAsyncOperationContext* context)
{
    ASSERT_IF(NULL == innerAtomicGroupStateProvider_, "Unexpected call");

    ServiceGroupStatefulMemberEventSource::GetEvents().Info_43_StatefulServiceGroupMember(
        serviceName_,
        reinterpret_cast<uintptr_t>(this),
        partitionId_,
        replicaId_
        );

    HRESULT hr = innerAtomicGroupStateProvider_->EndAtomicGroupCommit(context);
    if (FAILED(hr))
    {
        ServiceGroupStatefulMemberEventSource::GetEvents().Error_36_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_,
            hr
            );
    }
    return hr;
}

STDMETHODIMP CStatefulServiceUnit::BeginAtomicGroupRollback(
    __in FABRIC_ATOMIC_GROUP_ID atomicGroupId,
    __in FABRIC_SEQUENCE_NUMBER rollbackSequenceNumber,
    __in IFabricAsyncOperationCallback* callback,
    __out IFabricAsyncOperationContext** context
    )
{
    ASSERT_IF(NULL == innerAtomicGroupStateProvider_, "Unexpected call");

    ServiceGroupStatefulMemberEventSource::GetEvents().Info_44_StatefulServiceGroupMember(
        serviceName_,
        reinterpret_cast<uintptr_t>(this),
        partitionId_,
        replicaId_,
        atomicGroupId,
        rollbackSequenceNumber
        );

    HRESULT hr = innerAtomicGroupStateProvider_->BeginAtomicGroupRollback(atomicGroupId, rollbackSequenceNumber, callback, context);
    if (FAILED(hr))
    {
        ServiceGroupStatefulMemberEventSource::GetEvents().Error_37_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_,
            atomicGroupId,
            hr
            );
    }
    return hr;
}

STDMETHODIMP CStatefulServiceUnit::EndAtomicGroupRollback(__in IFabricAsyncOperationContext* context)
{
    ASSERT_IF(NULL == innerAtomicGroupStateProvider_, "Unexpected call");

    ServiceGroupStatefulMemberEventSource::GetEvents().Info_45_StatefulServiceGroupMember(
        serviceName_,
        reinterpret_cast<uintptr_t>(this),
        partitionId_,
        replicaId_
        );

    HRESULT hr = innerAtomicGroupStateProvider_->EndAtomicGroupRollback(context);
    if (FAILED(hr))
    {
        ServiceGroupStatefulMemberEventSource::GetEvents().Error_38_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_,
            hr
            );
    }
    return hr;
}

STDMETHODIMP CStatefulServiceUnit::BeginUndoProgress(
    __in FABRIC_SEQUENCE_NUMBER fromCommitSequenceNumber,
    __in IFabricAsyncOperationCallback* callback,
    __out IFabricAsyncOperationContext** context
    )
{
    ASSERT_IF(NULL == innerAtomicGroupStateProvider_, "Unexpected call");

    ServiceGroupStatefulMemberEventSource::GetEvents().Info_46_StatefulServiceGroupMember(
        serviceName_,
        reinterpret_cast<uintptr_t>(this),
        partitionId_,
        replicaId_,
        fromCommitSequenceNumber
        );

    HRESULT hr = innerAtomicGroupStateProvider_->BeginUndoProgress(fromCommitSequenceNumber, callback, context);
    if (FAILED(hr))
    {
        ServiceGroupStatefulMemberEventSource::GetEvents().Error_39_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_,
            hr
            );
    }
    return hr;
}

STDMETHODIMP CStatefulServiceUnit::EndUndoProgress(__in IFabricAsyncOperationContext* context)
{
    ASSERT_IF(NULL == innerAtomicGroupStateProvider_, "Unexpected call");

    ServiceGroupStatefulMemberEventSource::GetEvents().Info_47_StatefulServiceGroupMember(
        serviceName_,
        reinterpret_cast<uintptr_t>(this),
        partitionId_,
        replicaId_
        );

    HRESULT hr = innerAtomicGroupStateProvider_->EndUndoProgress(context);
    if (FAILED(hr))
    {
        ServiceGroupStatefulMemberEventSource::GetEvents().Error_40_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_,
            hr
            );
    }
    return hr;
}

HRESULT CStatefulServiceUnit::UpdateOperationStreams(__in BOOLEAN isClosing, __in FABRIC_REPLICA_ROLE newReplicaRole)
{
    if (newReplicaRole == currentReplicaRole_)
    {
        ServiceGroupStatefulMemberEventSource::GetEvents().Info_48_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_
            );
        return S_OK;
    }

    ServiceGroupStatefulMemberEventSource::GetEvents().Info_49_StatefulServiceGroupMember(
        serviceName_,
        reinterpret_cast<uintptr_t>(this),
        partitionId_,
        replicaId_,
        ROLE_TEXT(currentReplicaRole_),
        ROLE_TEXT(newReplicaRole)
        );

    HRESULT hrCopyDrain = S_OK;
    HRESULT hrReplicationDrain = S_OK;

    if (FABRIC_REPLICA_ROLE_IDLE_SECONDARY == currentReplicaRole_ && 
       (FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY == newReplicaRole || FABRIC_REPLICA_ROLE_NONE == newReplicaRole || FABRIC_REPLICA_ROLE_PRIMARY == newReplicaRole))
    {
        ASSERT_IF(NULL == copyOperationStream_, "Unexpected copy operation stream");
        
        ServiceGroupStatefulMemberEventSource::GetEvents().Info_30_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_
            );
        
        hrCopyDrain = copyOperationStream_->Drain();
        if (FAILED(hrCopyDrain))
        {
            ServiceGroupStatefulMemberEventSource::GetEvents().Warning_2_StatefulServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                replicaId_,
                hrCopyDrain
                );
        }
        else
        {
            ServiceGroupStatefulMemberEventSource::GetEvents().Info_31_StatefulServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                replicaId_
                );
        }
        
        if (FABRIC_REPLICA_ROLE_PRIMARY == newReplicaRole || FABRIC_REPLICA_ROLE_NONE == newReplicaRole)
        {
            ASSERT_IF(NULL == replicationOperationStream_, "Unexpected replication operation stream");

            ServiceGroupStatefulMemberEventSource::GetEvents().Info_32_StatefulServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                replicaId_
                );
            
            hrReplicationDrain = replicationOperationStream_->Drain();
            if (FAILED(hrReplicationDrain))
            {
                ServiceGroupStatefulMemberEventSource::GetEvents().Warning_3_StatefulServiceGroupMember(
                    serviceName_,
                    reinterpret_cast<uintptr_t>(this),
                    partitionId_,
                    replicaId_,
                    hrReplicationDrain
                    );
            }
            else
            {
                ServiceGroupStatefulMemberEventSource::GetEvents().Info_33_StatefulServiceGroupMember(
                    serviceName_,
                    reinterpret_cast<uintptr_t>(this),
                    partitionId_,
                    replicaId_
                    );
            }
        }
    }
    if (FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY == currentReplicaRole_ && 
       (FABRIC_REPLICA_ROLE_PRIMARY == newReplicaRole || FABRIC_REPLICA_ROLE_NONE == newReplicaRole))
    {
        ASSERT_IF(NULL == copyOperationStream_, "Unexpected copy operation stream");
        ASSERT_IF(NULL == replicationOperationStream_, "Unexpected replication operation stream");
        
        ServiceGroupStatefulMemberEventSource::GetEvents().Info_30_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_
            );
        
        hrCopyDrain = copyOperationStream_->Drain();
        if (FAILED(hrCopyDrain))
        {
            ServiceGroupStatefulMemberEventSource::GetEvents().Warning_2_StatefulServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                replicaId_,
                hrCopyDrain
                );
        }
        else
        {
            ServiceGroupStatefulMemberEventSource::GetEvents().Info_31_StatefulServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                replicaId_
                );
        }
        
        ServiceGroupStatefulMemberEventSource::GetEvents().Info_32_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_
            );
        
        hrReplicationDrain = replicationOperationStream_->Drain();
        if (FAILED(hrReplicationDrain))
        {
            ServiceGroupStatefulMemberEventSource::GetEvents().Warning_3_StatefulServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                replicaId_,
                hrReplicationDrain
                );
        }
        else
        {
            ServiceGroupStatefulMemberEventSource::GetEvents().Info_33_StatefulServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                replicaId_
                );
        }
    }
    if (isClosing)
    {
        ServiceGroupStatefulMemberEventSource::GetEvents().Info_50_StatefulServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            replicaId_
            );

        Common::AcquireWriteLock lockReplicator(lock_);
        isClosing_ = TRUE;
    }
    if (FAILED(hrCopyDrain))
    {
        return hrCopyDrain;
    }
    if (FAILED(hrReplicationDrain))
    {
        return hrReplicationDrain;
    }
    return S_OK;
}

void CStatefulServiceUnit::InternalTerminateOperationStreams(__in BOOLEAN forceDrainStream, __in BOOLEAN isReportFault)
{
    ASSERT_IF(!isReportFault && !forceDrainStream, "Unexpected value for force drain stream on abort during stream termination.");
    Common::StringLiteral const & traceSubComponent = isReportFault ? TraceSubComponentStatefulReportFault : TraceSubComponentStatefulServiceGroupAbort;

    WriteNoise(
        traceSubComponent, 
        serviceName_,
        "this={0} partitionId={1} replicaId={2} - {3} in role {4} with force drain stream flag {5}",
        this,
        partitionId_,
        replicaId_,
        isReportFault ? "Faulting" : "Aborting",
        currentReplicaRole_,
        forceDrainStream
        );
    
    HRESULT errorCode = isReportFault ? E_FAIL : E_ABORT;
    
    {
        Common::AcquireWriteLock lockReplicator(lock_);
        isFaulted_ = TRUE;

        if (forceDrainStream)
        {
            //
            // On fault don't wait for null dispatch.
            //
            BOOLEAN waitForNullDispatchOnReplicationStream = FALSE;
            BOOLEAN waitForNullDispatchOnCopyStream = FALSE;
            if (!isReportFault)
            {
                //
                // During abort only wait for null dispatch only if stream processing has started.
                //
                waitForNullDispatchOnCopyStream = copyStreamDrainingStarted_;
                waitForNullDispatchOnReplicationStream = replicationStreamDrainingStarted_;
            }

            if (isReportFault)
            {
                ServiceGroupStatefulEventSource::GetEvents().Warning_5_StatefulReportFault(
                    serviceName_,
                    reinterpret_cast<uintptr_t>(this),
                    partitionId_,
                    replicaId_,
                    ROLE_TEXT(currentReplicaRole_),
                    waitForNullDispatchOnCopyStream,
                    waitForNullDispatchOnReplicationStream
                    );
            }
            else
            {
                ServiceGroupStatefulEventSource::GetEvents().Warning_2_StatefulServiceGroupAbort(
                    serviceName_,
                    reinterpret_cast<uintptr_t>(this),
                    partitionId_,
                    replicaId_,
                    ROLE_TEXT(currentReplicaRole_),
                    waitForNullDispatchOnCopyStream,
                    waitForNullDispatchOnReplicationStream
                    );
            }

            if (NULL != copyOperationStream_)
            {
                copyOperationStream_->ForceDrain(errorCode, waitForNullDispatchOnCopyStream);
            }
            if (NULL != replicationOperationStream_)
            {
                replicationOperationStream_->ForceDrain(errorCode, waitForNullDispatchOnReplicationStream);
            }
        }
        else
        {
            if (NULL != copyOperationStream_)
            {
                copyOperationStream_->EnqueueOperation(NULL);
            }
            if (NULL != replicationOperationStream_)
            {
                replicationOperationStream_->EnqueueOperation(NULL);
            }
        }
    }
}

