// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include "FabricRuntime_.h"

#include "ServiceGroup.Constants.h"
#include "ServiceGroup.ServiceBaseUnit.h"
#include "ServiceGroup.StatelessServiceUnit.h"

using namespace ServiceGroup;
using namespace ServiceModel;

//
// Constructor/Destructor.
//
CStatelessServiceUnit::CStatelessServiceUnit(__in FABRIC_PARTITION_ID partitionId, __in FABRIC_INSTANCE_ID instanceId) : 
    CBaseServiceUnit(partitionId)
{
    WriteNoise(TraceSubComponentStatelessServiceGroupMember, "this={0} partitionId={1} instanceId={2} - ctor", this, partitionId_, instanceId);

    instanceId_ = instanceId;
    outerStatelessServicePartition_ = NULL;
    innerStatelessServiceInstance_ = NULL;
    outerServiceGroupPartition_ = NULL;
}

CStatelessServiceUnit::~CStatelessServiceUnit(void)
{
    WriteNoise(TraceSubComponentStatelessServiceGroupMember, "this={0} partitionId={1} instanceId={2} - dtor", this, partitionId_, instanceId_);
}

//
// Initialize/Cleanup the service unit.
//
STDMETHODIMP CStatelessServiceUnit::FinalConstruct(
    __in CServiceGroupMemberDescription* serviceActivationInfo,
    __in IFabricStatelessServiceFactory* factory
    )
{
    ServiceGroupStatelessEventSource::GetEvents().Info_0_StatelessServiceGroupMember(
        reinterpret_cast<uintptr_t>(this),
        partitionId_,
        instanceId_
        );

    ULONG initializationDataSize = 0;
    HRESULT hr = ::SizeTToULong(serviceActivationInfo->ServiceGroupMemberInitializationData.size(), &initializationDataSize);
    if (FAILED(hr))
    {
        ServiceGroupStatelessEventSource::GetEvents().Error_0_StatelessServiceGroupMember(
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            instanceId_,
            hr
            );

        return hr;
    }

    try { serviceName_ = serviceActivationInfo->ServiceName; }
    catch (std::bad_alloc const &) { hr = E_OUTOFMEMORY; }
    catch (...) { hr = E_FAIL; }
    if (FAILED(hr))
    {
        ServiceGroupStatelessEventSource::GetEvents().Error_1_StatelessServiceGroupMember(
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            instanceId_
            );

        return hr;
    }

    hr = factory->CreateInstance(
        serviceActivationInfo->ServiceType.c_str(),
        serviceActivationInfo->ServiceName.c_str(),
        initializationDataSize,
        serviceActivationInfo->ServiceGroupMemberInitializationData.data(),
        partitionId_.AsGUID(),
        instanceId_,
        &innerStatelessServiceInstance_
        );
        
    if (FAILED(hr))
    {
        ServiceGroupStatelessEventSource::GetEvents().Error_2_StatelessServiceGroupMember(
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            instanceId_,
            hr
            );
    }

    ServiceGroupStatelessEventSource::GetEvents().Info_1_StatelessServiceGroupMember(
        reinterpret_cast<uintptr_t>(this),
        partitionId_,
        instanceId_
        );

    return hr;
}

STDMETHODIMP CStatelessServiceUnit::FinalDestruct(void)
{
    Common::AcquireWriteLock lock(lock_);

    ServiceGroupStatelessEventSource::GetEvents().Info_2_StatelessServiceGroupMember(
        reinterpret_cast<uintptr_t>(this),
        partitionId_,
        instanceId_
        );

    if (NULL != innerStatelessServiceInstance_)
    {
        innerStatelessServiceInstance_->Release();
        innerStatelessServiceInstance_ = NULL;
    }
    if (NULL != outerStatelessServicePartition_)
    {
        outerStatelessServicePartition_->Release();
        outerStatelessServicePartition_ = NULL;
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
    return S_OK;
}

HRESULT CStatelessServiceUnit::InnerQueryInterface(__in REFIID riid, __out void ** ppvObject)
{
    IFabricInternalBrokeredService * broker = NULL;
    
    HRESULT hr = innerStatelessServiceInstance_->QueryInterface(IID_IFabricInternalBrokeredService, reinterpret_cast<void**>(&broker));
    if (SUCCEEDED(hr))
    {
        ServiceGroupStatelessEventSource::GetEvents().Info_3_StatelessServiceGroupMember(
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            instanceId_
            );
        
        IUnknown * brokeredServiceInstance = NULL;
        hr = broker->GetBrokeredService(&brokeredServiceInstance);
        
        if (FAILED(hr))
        {
            ServiceGroupStatelessEventSource::GetEvents().Info_4_StatelessServiceGroupMember(
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                instanceId_,
                hr
                );
        
            broker->Release();
            return hr;
        }
        
        hr = brokeredServiceInstance->QueryInterface(riid, ppvObject);
        
        broker->Release();
        brokeredServiceInstance->Release();
        
        return hr;
    }
    
    return innerStatelessServiceInstance_->QueryInterface(riid, ppvObject);
}

//
// IFabricStatelessServiceInstance methods.
//
STDMETHODIMP CStatelessServiceUnit::BeginOpen(
    __in IFabricStatelessServicePartition* partition,
    __in IFabricAsyncOperationCallback* callback,
    __out IFabricAsyncOperationContext** context
    )
{
    ASSERT_IF(NULL == partition, "Unexpected partition");
    ASSERT_IF(NULL == callback, "Unexpected callback");
    ASSERT_IF(NULL == context, "Unexpected context");

    partition->AddRef();

    HRESULT hr = partition->QueryInterface(IID_IFabricStatelessServicePartition3, (LPVOID*)&outerStatelessServicePartition_);
    ASSERT_IF(FAILED(hr), "Unexpected stateless service partition");

    hr = outerStatelessServicePartition_->QueryInterface(IID_IFabricServiceGroupPartition, (LPVOID*)&outerServiceGroupPartition_);
    ASSERT_IF(FAILED(hr), "Unexpected stateless service partition");

    hr = outerStatelessServicePartition_->QueryInterface(IID_IServiceGroupReport, (LPVOID*)&outerServiceGroupReport_);
    ASSERT_IF(FAILED(hr), "Unexpected stateless service partition");

    ServiceGroupStatelessMemberEventSource::GetEvents().Info_0_StatelessServiceGroupMember(
        serviceName_,
        reinterpret_cast<uintptr_t>(this),
        partitionId_,
        instanceId_
        );

    hr = innerStatelessServiceInstance_->BeginOpen(this, callback, context);
    if (FAILED(hr))
    {
        ServiceGroupStatelessMemberEventSource::GetEvents().Error_0_StatelessServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            instanceId_,
            hr
            );
        //
        // Release all resources allocated in BeginOpen.
        //
        FixUpFailedOpen();
    }
    return hr;
}

void CStatelessServiceUnit::FixUpFailedOpen(void)
{
    ServiceGroupStatelessMemberEventSource::GetEvents().Warning_0_StatelessServiceGroupMember(
        serviceName_,
        reinterpret_cast<uintptr_t>(this),
        partitionId_,
        instanceId_
        );
    //
    // Release all resources allocated in BeginOpen.
    //
    Common::AcquireWriteLock lockPartition(lock_);
    innerStatelessServiceInstance_->Release();
    innerStatelessServiceInstance_ = NULL;
    outerStatelessServicePartition_->Release();
    outerStatelessServicePartition_ = NULL;
    outerServiceGroupPartition_->Release();
    outerServiceGroupPartition_ = NULL;
    outerServiceGroupReport_->Release();
    outerServiceGroupReport_ = NULL;
}

STDMETHODIMP CStatelessServiceUnit::EndOpen(
    __in IFabricAsyncOperationContext* context,
    __out IFabricStringResult** serviceEndpoint
    )
{
    ASSERT_IF(NULL == context, "Unexpected context");
    ASSERT_IF(NULL == serviceEndpoint, "Unexpected service endpoint");

    ServiceGroupStatelessMemberEventSource::GetEvents().Info_1_StatelessServiceGroupMember(
        serviceName_,
        reinterpret_cast<uintptr_t>(this),
        partitionId_,
        instanceId_
        );

    HRESULT hr = innerStatelessServiceInstance_->EndOpen(context, serviceEndpoint);
    opened_ = SUCCEEDED(hr);
    if (SUCCEEDED(hr))
    {
        ServiceGroupStatelessMemberEventSource::GetEvents().Info_2_StatelessServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            instanceId_
            );
    }
    else
    {
        ServiceGroupStatelessMemberEventSource::GetEvents().Error_1_StatelessServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            instanceId_,
            hr
            );
        //
        // Release all resources allocated in BeginOpen.
        //
        FixUpFailedOpen();
    }
    return hr;
}

STDMETHODIMP CStatelessServiceUnit::BeginClose(
    __in IFabricAsyncOperationCallback* callback,
    __out IFabricAsyncOperationContext** context
    )
{
    ASSERT_IF(NULL == callback, "Unexpected callback");
    ASSERT_IF(NULL == context, "Unexpected context");

    if (!opened_)
    {
        ServiceGroupStatelessMemberEventSource::GetEvents().Info_3_StatelessServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            instanceId_
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

    ServiceGroupStatelessMemberEventSource::GetEvents().Info_4_StatelessServiceGroupMember(
        serviceName_,
        reinterpret_cast<uintptr_t>(this),
        partitionId_,
        instanceId_
        );

    HRESULT hr = innerStatelessServiceInstance_->BeginClose(callback, context);
    if (FAILED(hr))
    {
        ServiceGroupStatelessMemberEventSource::GetEvents().Error_2_StatelessServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            instanceId_,
            hr
            );
    }
    return hr;
}

STDMETHODIMP CStatelessServiceUnit::EndClose(__in IFabricAsyncOperationContext* context)
{
    ASSERT_IF(NULL == context, "Unexpected context");

    if (!opened_)
    {
        ServiceGroupStatelessMemberEventSource::GetEvents().Info_5_StatelessServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            instanceId_
            );

        return S_OK;
    }

    ServiceGroupStatelessMemberEventSource::GetEvents().Info_6_StatelessServiceGroupMember(
        serviceName_,
        reinterpret_cast<uintptr_t>(this),
        partitionId_,
        instanceId_
        );

    HRESULT hr = innerStatelessServiceInstance_->EndClose(context);
    if (FAILED(hr))
    {
        ServiceGroupStatelessMemberEventSource::GetEvents().Error_3_StatelessServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            instanceId_,
            hr
            );
    }

    opened_ = FAILED(hr);
    return hr;
}

STDMETHODIMP_(void) CStatelessServiceUnit::Abort()
{
    if (!opened_)
    {
        ServiceGroupStatelessMemberEventSource::GetEvents().Info_7_StatelessServiceGroupMember(
            serviceName_,
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            instanceId_
            );

        return;
    }

    ServiceGroupStatelessMemberEventSource::GetEvents().Info_8_StatelessServiceGroupMember(
        serviceName_,
        reinterpret_cast<uintptr_t>(this),
        partitionId_,
        instanceId_
        );

    innerStatelessServiceInstance_->Abort();
   
    opened_ = FALSE;
}

//
// IFabricStatelessServicePartition methods.
//
STDMETHODIMP CStatelessServiceUnit::GetPartitionInfo(__out const FABRIC_SERVICE_PARTITION_INFORMATION** bufferedValue)
{
    if (NULL == bufferedValue)
    {
        return Common::ComUtility::OnPublicApiReturn(E_POINTER);
    }

    IFabricStatelessServicePartition* partition = NULL;
    {
        Common::AcquireReadLock lockPartition(lock_);

        if (NULL == outerStatelessServicePartition_)
        {
            ServiceGroupStatelessMemberEventSource::GetEvents().Error_4_StatelessServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                instanceId_
                );

            return Common::ComUtility::OnPublicApiReturn(Common::ErrorCodeValue::ObjectClosed);
        }
        else
        {
            partition = outerStatelessServicePartition_;
            partition->AddRef();
        }
    }

    WriteNoise(
        TraceSubComponentStatelessServiceGroupMember, 
        serviceName_,
        "this={0} partitionId={1} instanceId={2} - Calling simulated stateless service partition GetPartitionInfo",
        this,
        partitionId_,
        instanceId_
        );

    HRESULT hr = partition->GetPartitionInfo(bufferedValue);
    partition->Release();

    return Common::ComUtility::OnPublicApiReturn(hr);
}

STDMETHODIMP CStatelessServiceUnit::ReportLoad(__in ULONG metricCount, __in const FABRIC_LOAD_METRIC* metrics)
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
            ServiceGroupStatelessMemberEventSource::GetEvents().Error_4_StatelessServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                instanceId_
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
        TraceSubComponentStatelessServiceGroupMember, 
        serviceName_,
        "this={0} partitionId={1} instanceId={2} - Calling simulated stateless service partition ReportLoad",
        this,
        partitionId_,
        instanceId_
        );

    HRESULT hr = report->ReportLoad(partitionId_.AsGUID(), metricCount, metrics);
    report->Release();

    return Common::ComUtility::OnPublicApiReturn(hr);
}

STDMETHODIMP CStatelessServiceUnit::ReportFault(__in FABRIC_FAULT_TYPE faultType)
{
    IServiceGroupReport* report = NULL;
    {
        Common::AcquireReadLock lockPartition(lock_);

        if (NULL == outerServiceGroupReport_)
        {
            ServiceGroupStatelessMemberEventSource::GetEvents().Error_4_StatelessServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                instanceId_
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
        TraceSubComponentStatelessReportFault, 
        serviceName_,
        "this={0} partitionId={1} instanceId={2} - Calling simulated stateless service partition ReportFault",
        this,
        partitionId_,
        instanceId_
        );

    HRESULT hr = report->ReportFault(partitionId_.AsGUID(), faultType);
    report->Release();

    return Common::ComUtility::OnPublicApiReturn(hr);
}

//
// IFabricStatelessServicePartition1 methods
//
STDMETHODIMP CStatelessServiceUnit::ReportMoveCost(__in FABRIC_MOVE_COST moveCost)
{
    IServiceGroupReport* report = NULL;
    {
        Common::AcquireReadLock lockPartition(lock_);

        if (NULL == outerServiceGroupReport_)
        {
            ServiceGroupStatelessMemberEventSource::GetEvents().Error_4_StatelessServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                instanceId_
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
        TraceSubComponentStatelessReportMoveCost,
        serviceName_,
        "this={0} partitionId={1} instanceId={2} - Calling simulated stateless service partition ReportMoveCost",
        this,
        partitionId_,
        instanceId_
        );

    HRESULT hr = report->ReportMoveCost(partitionId_.AsGUID(), moveCost);
    report->Release();

    return Common::ComUtility::OnPublicApiReturn(hr);
}

//
// IFabricStatelessServicePartition2 methods
//
STDMETHODIMP CStatelessServiceUnit::ReportInstanceHealth(__in const FABRIC_HEALTH_INFORMATION* healthInformation)
{
    return ReportInstanceHealth2(healthInformation, nullptr);
}

STDMETHODIMP CStatelessServiceUnit::ReportPartitionHealth(__in const FABRIC_HEALTH_INFORMATION* healthInformation)
{
    return ReportPartitionHealth2(healthInformation, nullptr);
}

//
// IFabricStatelessServicePartition3 methods
//
STDMETHODIMP CStatelessServiceUnit::ReportInstanceHealth2(
    __in const FABRIC_HEALTH_INFORMATION* healthInformation,
    __in const FABRIC_HEALTH_REPORT_SEND_OPTIONS *sendOptions)
{
    if (NULL == healthInformation)
    {
        return Common::ComUtility::OnPublicApiReturn(E_POINTER);
    }

    IFabricStatelessServicePartition3* partition = NULL;
    {
        Common::AcquireReadLock lockPartition(lock_);

        if (NULL == outerStatelessServicePartition_)
        {
            ServiceGroupStatelessMemberEventSource::GetEvents().Error_4_StatelessServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                instanceId_
            );

            return Common::ComUtility::OnPublicApiReturn(Common::ErrorCodeValue::ObjectClosed);
        }
        else
        {
            partition = outerStatelessServicePartition_;
            partition->AddRef();
        }
    }

    WriteNoise(
        TraceSubComponentStatelessServiceGroupMember,
        serviceName_,
        "this={0} partitionId={1} instanceId={2} - Calling simulated stateless service partition ReportInstanceHealth",
        this,
        partitionId_,
        instanceId_
    );

    HRESULT hr = partition->ReportInstanceHealth2(healthInformation, sendOptions);
    partition->Release();

    return Common::ComUtility::OnPublicApiReturn(hr);
}

STDMETHODIMP CStatelessServiceUnit::ReportPartitionHealth2(
    __in const FABRIC_HEALTH_INFORMATION* healthInformation,
    __in const FABRIC_HEALTH_REPORT_SEND_OPTIONS *sendOptions)
{
    if (NULL == healthInformation)
    {
        return Common::ComUtility::OnPublicApiReturn(E_POINTER);
    }

    IFabricStatelessServicePartition3* partition = NULL;
    {
        Common::AcquireReadLock lockPartition(lock_);

        if (NULL == outerStatelessServicePartition_)
        {
            ServiceGroupStatelessMemberEventSource::GetEvents().Error_4_StatelessServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                instanceId_
            );

            return Common::ComUtility::OnPublicApiReturn(Common::ErrorCodeValue::ObjectClosed);
        }
        else
        {
            partition = outerStatelessServicePartition_;
            partition->AddRef();
        }
    }

    WriteNoise(
        TraceSubComponentStatelessServiceGroupMember,
        serviceName_,
        "this={0} partitionId={1} instanceId={2} - Calling simulated stateless service partition ReportPartitionHealth",
        this,
        partitionId_,
        instanceId_
    );

    HRESULT hr = partition->ReportPartitionHealth2(healthInformation, sendOptions);
    partition->Release();

    return Common::ComUtility::OnPublicApiReturn(hr);
}

//
// IFabricServiceGroupPartition methods.
//
STDMETHODIMP CStatelessServiceUnit::ResolveMember(
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
            ServiceGroupStatelessMemberEventSource::GetEvents().Error_4_StatelessServiceGroupMember(
                serviceName_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                instanceId_
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
        TraceSubComponentStatelessServiceGroupMember, 
        serviceName_,
        "this={0} partitionId={1} instanceId={2} - Calling simulated service group partition ResolveMember",
        this,
        partitionId_,
        instanceId_
        );

    HRESULT hr = group->ResolveMember(name, riid, member);
    group->Release();

    return Common::ComUtility::OnPublicApiReturn(hr);
}
