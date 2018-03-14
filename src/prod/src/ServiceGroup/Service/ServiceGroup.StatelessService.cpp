// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include "ServiceGroup.Constants.h"
#include "ServiceGroup.ServiceBaseUnit.h"
#include "ServiceGroup.StatelessServiceUnit.h"
#include "ServiceGroup.ServiceBase.h"
#include "ServiceGroup.StatelessService.h"
#include "ServiceGroup.StatelessServiceFactory.h"
#include "ServiceGroup.AsyncOperations.h"

using namespace ServiceGroup;
using namespace ServiceModel;

//
// Constructor/Destructor.
//
CStatelessServiceGroupFactory::CStatelessServiceGroupFactory()
{
    WriteNoise(TraceSubComponentStatelessServiceGroupFactory, "this={0} - ctor", this);
}

CStatelessServiceGroupFactory::~CStatelessServiceGroupFactory()
{
    WriteNoise(TraceSubComponentStatelessServiceGroupFactory, "this={0} - dtor", this);

    std::map<std::wstring, IFabricStatelessServiceFactory*>::iterator iterateStateless;
    for (iterateStateless = statelessServiceFactories_.begin(); statelessServiceFactories_.end() != iterateStateless; iterateStateless++)
    {
        iterateStateless->second->Release();
    }
}

//
// IStatelessServiceReplica methods.
//
STDMETHODIMP CStatelessServiceGroupFactory::CreateInstance(
    __in LPCWSTR serviceType,
    __in LPCWSTR serviceName,
    __in ULONG initializationDataLength,
    __in const byte * initializationData,
    __in FABRIC_PARTITION_ID partitionId,
    __in FABRIC_INSTANCE_ID instanceId,
    __out IFabricStatelessServiceInstance** serviceInstance
    )
{
    HRESULT hr = S_OK;

    UNREFERENCED_PARAMETER(partitionId);

    ASSERT_IF(NULL == serviceInstance, "Unexpected service instance");
    ASSERT_IF(NULL == serviceType, "Unexpected service type");
    ASSERT_IF(NULL == serviceName, "Unexpected service name");

    CStatelessServiceGroup* statelessServiceGroupInstance = new (std::nothrow) CStatelessServiceGroup(instanceId);
    if (NULL == statelessServiceGroupInstance)
    {
        ServiceGroupStatelessEventSource::GetEvents().Error_0_StatelessServiceGroupFactory(reinterpret_cast<uintptr_t>(this));
        return E_OUTOFMEMORY;
    }

    CServiceGroupDescription serviceGroupDescr;
    try
    {
        Common::ErrorCode error = Common::FabricSerializer::Deserialize(serviceGroupDescr, initializationDataLength, const_cast<byte*>(initializationData));
        if (!error.IsSuccess())
        {
            hr = error.ToHResult();
        }
    }
    catch (std::bad_alloc const &) { hr = E_OUTOFMEMORY; }
    catch (...) { hr = E_FAIL; }
    if (FAILED(hr))
    {
        ServiceGroupStatelessEventSource::GetEvents().Error_1_StatelessServiceGroupFactory(
            reinterpret_cast<uintptr_t>(this),
            instanceId,
            hr
            );

        statelessServiceGroupInstance->Release();
        return hr;
    }

    hr = statelessServiceGroupInstance->FinalConstruct(&serviceGroupDescr, statelessServiceFactories_);
    if (FAILED(hr))
    {
        ServiceGroupStatelessEventSource::GetEvents().Error_2_StatelessServiceGroupFactory(
            reinterpret_cast<uintptr_t>(this),
            instanceId,
            hr
            );

        statelessServiceGroupInstance->Release();
    }
    else 
    {
        ServiceGroupStatelessEventSource::GetEvents().Info_0_StatelessServiceGroupFactory(
            reinterpret_cast<uintptr_t>(this),
            instanceId
            );

        *serviceInstance = (IFabricStatelessServiceInstance*)statelessServiceGroupInstance;
    }
    return hr;
}

//
// Initialize.
//
HRESULT CStatelessServiceGroupFactory::FinalConstruct(__in const std::map<std::wstring, IFabricStatelessServiceFactory*>& statelessServiceFactories)
{
    if (statelessServiceFactories.empty())
    {
        ServiceGroupStatelessEventSource::GetEvents().Error_3_StatelessServiceGroupFactory(reinterpret_cast<uintptr_t>(this));

        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;
    try
    { statelessServiceFactories_ = statelessServiceFactories; }
    catch (std::bad_alloc const &) { hr = E_OUTOFMEMORY; }
    catch (...) { hr = E_FAIL; }
    if (FAILED(hr))
    {
        ServiceGroupStatelessEventSource::GetEvents().Error_4_StatelessServiceGroupFactory(reinterpret_cast<uintptr_t>(this));

        return hr;
    }
    std::map<std::wstring, IFabricStatelessServiceFactory*>::iterator iterateStateless;
    for (iterateStateless = statelessServiceFactories_.begin(); statelessServiceFactories_.end() != iterateStateless; iterateStateless++)
    {
        iterateStateless->second->AddRef();
    }
    return hr;
}

//
// Constructor/Destructor.
//
CStatelessServiceGroup::CStatelessServiceGroup(FABRIC_INSTANCE_ID instanceId)
{
    WriteNoise(TraceSubComponentStatelessServiceGroup, "this={0} instanceId={1} - ctor", this, instanceId);
    
    instanceId_ = instanceId;
    outerStatelessServicePartition_ = NULL;
}

CStatelessServiceGroup::~CStatelessServiceGroup(void)
{
    WriteNoise(TraceSubComponentStatelessServiceGroup, "this={0} instanceId={1} - dtor", this, instanceId_);

    PartitionId_StatelessServiceUnit_Iterator iterate;
    for (iterate = innerStatelessServiceUnits_.begin(); 
           innerStatelessServiceUnits_.end() != iterate; 
           iterate++)
    {
        iterate->second->Release();
    }
}

//
// Initialize the service group.
//
STDMETHODIMP CStatelessServiceGroup::FinalConstruct(
    __in CServiceGroupDescription* serviceGroupActivationInfo,
    __in const std::map<std::wstring, IFabricStatelessServiceFactory*>& statelessServiceFactories
    )
{
    HRESULT hr = S_OK;

    ServiceGroupStatelessEventSource::GetEvents().Info_0_StatelessServiceGroup(
        reinterpret_cast<uintptr_t>(this),
        instanceId_
        );

    PartitionId_StatelessServiceUnit_Iterator iterate;
    PartitionId_StatelessServiceUnit_Insert insert;
    ServiceInstanceName_ServiceId_Insert serviceInsert;

    for (size_t idxSvcActivationInfo = 0; idxSvcActivationInfo < serviceGroupActivationInfo->ServiceGroupMemberData.size(); idxSvcActivationInfo++)
    {
        CServiceGroupMemberDescription* serviceActivationInfo = &serviceGroupActivationInfo->ServiceGroupMemberData[idxSvcActivationInfo];
        Common::Guid partitionId(serviceActivationInfo->Identifier);

        CStatelessServiceUnit* statelessServiceUnit = new (std::nothrow) CStatelessServiceUnit(serviceActivationInfo->Identifier, instanceId_);
        if (NULL == statelessServiceUnit)
        {
            ServiceGroupStatelessEventSource::GetEvents().Error_0_StatelessServiceGroup(
                reinterpret_cast<uintptr_t>(this),
                instanceId_,
                serviceActivationInfo->ServiceType.c_str()
                );

            hr = E_OUTOFMEMORY;
            break;
        }

        std::map<std::wstring, IFabricStatelessServiceFactory*>::const_iterator factoryIterator = statelessServiceFactories.find(serviceActivationInfo->ServiceType);
        if (statelessServiceFactories.end() == factoryIterator)
        {
            ServiceGroupStatelessEventSource::GetEvents().Error_1_StatelessServiceGroup(
                reinterpret_cast<uintptr_t>(this),
                instanceId_,
                serviceActivationInfo->ServiceType.c_str()
                );

            hr = E_UNEXPECTED;
            break;
        }

        hr = statelessServiceUnit->FinalConstruct(serviceActivationInfo, factoryIterator->second);
        if (FAILED(hr))
        {
            ServiceGroupStatelessEventSource::GetEvents().Error_2_StatelessServiceGroup(
                reinterpret_cast<uintptr_t>(this),
                instanceId_,
                partitionId,
                hr
                );

            statelessServiceUnit->Release();
            break;
        }

        ServiceGroupStatelessEventSource::GetEvents().Info_1_StatelessServiceGroup(
            reinterpret_cast<uintptr_t>(this),
            instanceId_,
            partitionId
            );

        try 
        {
            innerStatelessServiceUnits_.insert(PartitionId_StatelessServiceUnit_Pair(partitionId, statelessServiceUnit)); 
            serviceInsert = serviceInstanceNames_.insert(ServiceInstanceName_ServiceId_Pair(serviceActivationInfo->ServiceName, partitionId));
            ASSERT_IFNOT(serviceInsert.second, "Unexpected service name");
        }
        catch (std::bad_alloc const &) { hr = E_OUTOFMEMORY; }
        catch (...) { hr = E_FAIL; }
        if (FAILED(hr))
        {
            ServiceGroupStatelessEventSource::GetEvents().Error_3_StatelessServiceGroup(
                reinterpret_cast<uintptr_t>(this),
                instanceId_,
                partitionId
                );
            
            statelessServiceUnit->Release();
            break;
        }

        ServiceGroupStatelessEventSource::GetEvents().Info_2_StatelessServiceGroup(
            reinterpret_cast<uintptr_t>(this),
            instanceId_,
            partitionId
            );
    }
    if (FAILED(hr))
    {
        ServiceGroupStatelessEventSource::GetEvents().Error_4_StatelessServiceGroup(
            reinterpret_cast<uintptr_t>(this),
            instanceId_,
            hr
            );
    
        FinalDestruct();
        return hr;
    }

    ServiceGroupStatelessEventSource::GetEvents().Info_3_StatelessServiceGroup(
        reinterpret_cast<uintptr_t>(this),
        instanceId_
        );
    return S_OK;
}

STDMETHODIMP CStatelessServiceGroup::FinalDestruct(void)
{
    Common::AcquireWriteLock lock(lock_);

    ServiceGroupStatelessEventSource::GetEvents().Info_4_StatelessServiceGroup(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        instanceId_
        );

    PartitionId_StatelessServiceUnit_Iterator iterate;
    for (iterate = innerStatelessServiceUnits_.begin(); 
           innerStatelessServiceUnits_.end() != iterate; 
           iterate++)
    {
        iterate->second->FinalDestruct();
        iterate->second->Release();
    }
    innerStatelessServiceUnits_.clear();
    if (NULL != outerStatelessServicePartition_)
    {
        outerStatelessServicePartition_->Release();
        outerStatelessServicePartition_ = NULL;
    }
    return S_OK;
}

//
// IFabricStatelessServicePartition methods.
STDMETHODIMP CStatelessServiceGroup::GetPartitionInfo(__out const FABRIC_SERVICE_PARTITION_INFORMATION** bufferedValue)
{
    if (NULL == bufferedValue)
    {
        return E_POINTER;
    }

    Common::AcquireReadLock lockPartition(lock_);

    if (NULL != outerStatelessServicePartition_)
    {
        WriteNoise(
            TraceSubComponentStatelessServiceGroup, 
            partitionIdString_,
            "this={0} instanceId={1} - Calling system stateless service partition GetPartitionInfo", 
            this,
            instanceId_
            );

        return outerStatelessServicePartition_->GetPartitionInfo(bufferedValue);
    }
    
    ServiceGroupStatelessEventSource::GetEvents().Error_5_StatelessServiceGroup(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        instanceId_
        );

    return Common::ErrorCodeValue::ObjectClosed;
}

STDMETHODIMP CStatelessServiceGroup::ReportLoad(__in ULONG metricCount, __in const FABRIC_LOAD_METRIC* metrics)
{
    if (NULL == metrics)
    {
        return E_POINTER;
    }
    if (metrics->Reserved != NULL)
    {
        return E_INVALIDARG;
    }

    Common::AcquireReadLock lockPartition(lock_);

    if (NULL != outerStatelessServicePartition_)
    {
        WriteNoise(
            TraceSubComponentStatelessServiceGroup, 
            partitionIdString_,
            "this={0} instanceId={1} - Calling system stateless service partition ReportLoad",
            this,
            instanceId_
            );

        return outerStatelessServicePartition_->ReportLoad(metricCount, metrics);
    }
    
    ServiceGroupStatelessEventSource::GetEvents().Error_5_StatelessServiceGroup(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        instanceId_
        );

    return Common::ErrorCodeValue::ObjectClosed;
}

STDMETHODIMP CStatelessServiceGroup::ReportFault(__in FABRIC_FAULT_TYPE faultType)
{
    Common::AcquireReadLock lockPartition(lock_);

    if (NULL != outerStatelessServicePartition_)
    {
        WriteNoise(
            TraceSubComponentStatelessReportFault, 
            partitionIdString_,
            "this={0} instanceId={1} - Calling system stateless service partition ReportFault",
            this,
            instanceId_
            );

        return outerStatelessServicePartition_->ReportFault(faultType);
    }
    
    ServiceGroupStatelessEventSource::GetEvents().Error_0_StatelessReportFault(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        instanceId_
        );

    return Common::ErrorCodeValue::ObjectClosed;
}

//
// IFabricStatelessServicePartition1 members
//
STDMETHODIMP CStatelessServiceGroup::ReportMoveCost(__in FABRIC_MOVE_COST moveCost)
{
    Common::AcquireReadLock lockPartition(lock_);

    if (NULL != outerStatelessServicePartition_)
    {
        WriteNoise(
            TraceSubComponentStatelessReportMoveCost,
            partitionIdString_,
            "this={0} instanceId={1} - Calling system stateless service partition ReportMoveCost",
            this,
            instanceId_
            );

        return outerStatelessServicePartition_->ReportMoveCost(moveCost);
    }

    ServiceGroupStatelessEventSource::GetEvents().Error_0_StatelessReportMoveCost(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        instanceId_
        );


    return Common::ErrorCodeValue::ObjectClosed;
}

//
// IFabricStatelessServicePartition2 members
//
STDMETHODIMP CStatelessServiceGroup::ReportInstanceHealth(__in const FABRIC_HEALTH_INFORMATION* healthInformation)
{
    return ReportInstanceHealth2(healthInformation, nullptr);
}

STDMETHODIMP CStatelessServiceGroup::ReportPartitionHealth(__in const FABRIC_HEALTH_INFORMATION* healthInformation)
{
    return ReportPartitionHealth2(healthInformation, nullptr);
}

//
// IFabricStatelessServicePartition3 members
//
STDMETHODIMP CStatelessServiceGroup::ReportInstanceHealth2(
    __in const FABRIC_HEALTH_INFORMATION* healthInformation,
    __in const FABRIC_HEALTH_REPORT_SEND_OPTIONS *sendOptions)
{
    Common::AcquireReadLock lockPartition(lock_);

    if (NULL != outerStatelessServicePartition_)
    {
        WriteNoise(
            TraceSubComponentStatelessReportInstanceHealth,
            partitionIdString_,
            "this={0} instanceId={1} - Calling system stateless service partition ReportInstanceHealth",
            this,
            instanceId_
        );

        return outerStatelessServicePartition_->ReportInstanceHealth2(healthInformation, sendOptions);
    }

    ServiceGroupStatelessEventSource::GetEvents().Error_0_StatelessReportInstanceHealth(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        instanceId_
    );

    return Common::ErrorCodeValue::ObjectClosed;
}

STDMETHODIMP CStatelessServiceGroup::ReportPartitionHealth2(
    __in const FABRIC_HEALTH_INFORMATION* healthInformation,
    __in const FABRIC_HEALTH_REPORT_SEND_OPTIONS *sendOptions)
{
    Common::AcquireReadLock lockPartition(lock_);

    if (NULL != outerStatelessServicePartition_)
    {
        WriteNoise(
            TraceSubComponentStatelessReportPartitionHealth,
            partitionIdString_,
            "this={0} instanceId={1} - Calling system stateless service partition ReportPartitionHealth",
            this,
            instanceId_
        );

        return outerStatelessServicePartition_->ReportPartitionHealth2(healthInformation, sendOptions);
    }

    ServiceGroupStatelessEventSource::GetEvents().Error_0_StatelessReportPartitionHealth(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        instanceId_
    );

    return Common::ErrorCodeValue::ObjectClosed;
}

//
// IFabricServiceGroupPartition methods.
//
STDMETHODIMP CStatelessServiceGroup::ResolveMember(
    __in FABRIC_URI name,
    __in REFIID riid,
    __out void ** member)
{
    if (NULL == name || NULL == member)
    {
        return E_POINTER;
    }

    Common::AcquireReadLock lockPartition(lock_);

    HRESULT hr = S_OK;
    if (NULL != outerStatelessServicePartition_)
    {
        WriteNoise(
            TraceSubComponentStatelessServiceGroup, 
            partitionIdString_,
            "this={0} partitionId={1} - Resolving member", 
            this,
            partitionId_
            );

        auto error = Common::ParameterValidator::IsValid(name, Common::ParameterValidator::MinStringSize, Common::ParameterValidator::MaxNameSize);
        if (!error.IsSuccess())
        {
            ServiceGroupStatelessEventSource::GetEvents().Error_6_StatelessServiceGroup(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_
                );

            return error.ToHResult();
        }

        try 
        {
            std::wstring key(name);
            Common::NamingUri uri;
            if (!Common::NamingUri::TryParse(key, uri))
            {
                ServiceGroupStatelessEventSource::GetEvents().Error_7_StatelessServiceGroup(
                    partitionIdString_,
                    reinterpret_cast<uintptr_t>(this),
                    partitionId_,
                    key.c_str()
                    );

                return FABRIC_E_INVALID_NAME_URI;
            }

            std::map<std::wstring, Common::Guid>::iterator lookupName = serviceInstanceNames_.find(uri.get_Name());
            if (serviceInstanceNames_.end() == lookupName)
            {
                ServiceGroupStatelessEventSource::GetEvents().Error_8_StatelessServiceGroup(
                    partitionIdString_,
                    reinterpret_cast<uintptr_t>(this),
                    partitionId_,
                    uri.get_Name().c_str()
                    );

                return E_INVALIDARG;
            }
            PartitionId_StatelessServiceUnit_Iterator lookupInstance = innerStatelessServiceUnits_.find(lookupName->second);
            ASSERT_IF(innerStatelessServiceUnits_.end() == lookupInstance, "Stateless service group unit not found");

            return lookupInstance->second->InnerQueryInterface(riid, member);
        }
        catch (std::bad_alloc const &) { hr = E_OUTOFMEMORY; }
        catch (...) { hr = E_FAIL; }
        if (FAILED(hr))
        {
            ServiceGroupStatelessEventSource::GetEvents().Error_9_StatelessServiceGroup(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                partitionId_
                );

            return hr;
        }
        return S_OK;
    }
    
    ServiceGroupStatelessEventSource::GetEvents().Error_10_StatelessServiceGroup(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        partitionId_
        );

    return Common::ErrorCodeValue::ObjectClosed;
}

//
// IServiceGroupReport methods.
//
STDMETHODIMP CStatelessServiceGroup::ReportLoad(__in FABRIC_PARTITION_ID partitionId, __in ULONG metricCount, __in const FABRIC_LOAD_METRIC* metrics)
{
    FABRIC_LOAD_METRIC* computedMetrics = NULL;
    HRESULT hr = UpdateLoadMetrics(partitionId, metricCount, metrics, &computedMetrics);
    if (SUCCEEDED(hr))
    {
        hr = ReportLoad(metricCount, computedMetrics);
        delete[] computedMetrics;
    }
    return hr;
}

STDMETHODIMP CStatelessServiceGroup::ReportFault(__in FABRIC_PARTITION_ID partitionId, __in FABRIC_FAULT_TYPE faultType)
{
    UNREFERENCED_PARAMETER(partitionId);

    HRESULT hr = ReportFault(faultType);
    if (FAILED(hr))
    {
        return hr;
    }
    return S_OK;
}

STDMETHODIMP CStatelessServiceGroup::ReportMoveCost(__in FABRIC_PARTITION_ID partitionId, __in FABRIC_MOVE_COST moveCost)
{
    UNREFERENCED_PARAMETER(partitionId);

    HRESULT hr = ReportMoveCost(moveCost);
    if (FAILED(hr))
    {
        return hr;
    }
    return S_OK;
}

//
// IFabricStatelessServiceInstance methods.
//
STDMETHODIMP CStatelessServiceGroup::BeginOpen(
    __in IFabricStatelessServicePartition* partition,
    __in IFabricAsyncOperationCallback* callback,
    __out IFabricAsyncOperationContext** context
    )
{
    HRESULT hr = S_OK;
    const FABRIC_SERVICE_PARTITION_INFORMATION* spi = NULL;

    if (NULL == partition || NULL == callback || NULL == context)
    {
        return E_POINTER;
    }

    hr = partition->GetPartitionInfo(&spi);
    if (FAILED(hr))
    {
        ServiceGroupStatelessEventSource::GetEvents().Error_0_StatelessServiceGroupOpen(
            reinterpret_cast<uintptr_t>(this),
            instanceId_,
            hr
            );
        return hr;
    }
    
    partitionId_ = Common::Guid(Common::ServiceInfoUtility::GetPartitionId(spi));

    try { partitionIdString_ = partitionId_.ToString(); }
    catch (std::bad_alloc const &) { hr = E_OUTOFMEMORY; }
    catch (...) { hr = E_FAIL; }
    if (FAILED(hr))
    {
        ServiceGroupStatelessEventSource::GetEvents().Error_1_StatelessServiceGroupOpen(
            reinterpret_cast<uintptr_t>(this),
            instanceId_
            );

        return hr;
    }

    partition->AddRef();

    hr = partition->QueryInterface(IID_IFabricStatelessServicePartition3, (LPVOID*)&outerStatelessServicePartition_);
    ASSERT_IF(FAILED(hr), "Unexpected stateless service partition");

    //
    // Perform open on service group members.
    //
    hr = DoOpen(callback, context);
    if (FAILED(hr))
    {
        FixUpFailedOpen();
    }
    return hr;
}

void CStatelessServiceGroup::FixUpFailedOpen(void)
{
    ServiceGroupStatelessEventSource::GetEvents().Warning_0_StatelessServiceGroupOpen(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        instanceId_
        );
    //
    // Release all resources allocated in BeginOpen.
    //
    Common::AcquireWriteLock lockPartition(lock_);
    outerStatelessServicePartition_->Release();
    outerStatelessServicePartition_ = NULL;
}

STDMETHODIMP CStatelessServiceGroup::EndOpen(
    __in IFabricAsyncOperationContext* context,
    __out IFabricStringResult** serviceEndpoint
    )
{
    if (NULL == context || NULL == serviceEndpoint)
    {
        return E_POINTER;
    }

    ServiceGroupStatelessEventSource::GetEvents().Info_0_StatelessServiceGroupOpen(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        instanceId_
        );

    CStatelessCompositeOpenCloseAsyncOperationAsync* parentOperation = (CStatelessCompositeOpenCloseAsyncOperationAsync*)context;
    HRESULT hr = parentOperation->End();
    if (FAILED(hr))
    {
        ServiceGroupStatelessEventSource::GetEvents().Error_2_StatelessServiceGroupOpen(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            instanceId_,
            hr
            );

        parentOperation->Release();

        PartitionId_StatelessServiceUnit_Iterator iterate;
        for (iterate = innerStatelessServiceUnits_.begin(); innerStatelessServiceUnits_.end() != iterate; iterate++)
        {
            //
            // If any service member failed to open (or close) abort.
            // Abort will not call the member if it successfully closed or never opened.
            //
            iterate->second->Abort();
        }

        //
        // Release all resources allocated in BeginOpen.
        //
        FixUpFailedOpen();

        return hr;
    }

    hr = parentOperation->get_ServiceEndpoint(serviceEndpoint);
    parentOperation->Release();
    if (FAILED(hr))
    {
        ServiceGroupStatelessEventSource::GetEvents().Error_3_StatelessServiceGroupOpen(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            instanceId_,
            hr
            );
        //
        // Release all resources allocated in BeginOpen.
        //
        FixUpFailedOpen();
        return hr;
    }
    
    ServiceGroupStatelessEventSource::GetEvents().Info_1_StatelessServiceGroupOpen(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        instanceId_
        );
    ServiceGroupStatelessEventSource::GetEvents().Info_2_StatelessServiceGroupOpen(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        instanceId_
        );

    return S_OK;
}
    
STDMETHODIMP CStatelessServiceGroup::BeginClose(
    __in IFabricAsyncOperationCallback* callback,
    __out IFabricAsyncOperationContext** context
    )
{
    if (NULL == callback || NULL == context)
    {
        return E_POINTER;
    }
    return DoClose(callback, context);
}

STDMETHODIMP CStatelessServiceGroup::EndClose(__in IFabricAsyncOperationContext* context)
{
    UNREFERENCED_PARAMETER(context);

    ServiceGroupStatelessEventSource::GetEvents().Info_0_StatelessServiceGroupClose(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        instanceId_
        );

    CCompositeAsyncOperationAsync* parentOperation = (CCompositeAsyncOperationAsync*)context;
    HRESULT hr = parentOperation->End();
    parentOperation->Release();
    if (FAILED(hr))
    {
        ServiceGroupStatelessEventSource::GetEvents().Error_0_StatelessServiceGroupClose(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            instanceId_,
            hr
            );

        return hr;

    }

    ServiceGroupStatelessEventSource::GetEvents().Info_1_StatelessServiceGroupClose(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        instanceId_
        );

    FinalDestruct();

    return S_OK;
}

STDMETHODIMP_(void) CStatelessServiceGroup::Abort()
{
    ServiceGroupStatelessEventSource::GetEvents().Info_0_StatelessServiceGroupAbort(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        instanceId_
        );

    PartitionId_StatelessServiceUnit_Iterator iterate;
    for (iterate = innerStatelessServiceUnits_.begin(); 
         innerStatelessServiceUnits_.end() != iterate; 
         iterate++)
    {
        iterate->second->Abort();
    }

    FinalDestruct();
}

//
// Helper methods.
//
STDMETHODIMP CStatelessServiceGroup::DoOpen(
    __in IFabricAsyncOperationCallback* callback,
    __out IFabricAsyncOperationContext** context
    )
{
    ServiceGroupStatelessEventSource::GetEvents().Info_3_StatelessServiceGroupOpen(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        instanceId_
        );

    HRESULT hr = S_OK;
    PartitionId_StatelessServiceUnit_Iterator iterate;
    CStatelessCompositeOpenOperation* mopenAsyncOperation = NULL;
    CCompositeAsyncOperationAsync* mcloseAsyncOperation = NULL;
    CStatelessCompositeOpenCloseAsyncOperationAsync* parentOperation = NULL;

    parentOperation = new (std::nothrow) CStatelessCompositeOpenCloseAsyncOperationAsync(NULL, partitionId_.AsGUID());
    if (NULL == parentOperation)
    {
        return E_OUTOFMEMORY;
    }
    hr = parentOperation->FinalConstruct();
    if (FAILED(hr))
    {
        parentOperation->Release();
        return hr;
    }

    mopenAsyncOperation = new (std::nothrow) CStatelessCompositeOpenOperation(parentOperation, partitionId_.AsGUID());
    if (NULL == mopenAsyncOperation)
    {
        parentOperation->Release();
        return E_OUTOFMEMORY;
    }
    hr = mopenAsyncOperation->FinalConstruct();
    if (FAILED(hr))
    {
        mopenAsyncOperation->Release();
        parentOperation->Release();
        return hr;
    }
    
    mcloseAsyncOperation = new (std::nothrow) CCompositeAsyncOperationAsync(parentOperation, partitionId_.AsGUID());
    if (NULL == mcloseAsyncOperation)
    {
        mopenAsyncOperation->Release();
        parentOperation->Release();
        return E_OUTOFMEMORY;
    }
    hr = mcloseAsyncOperation->FinalConstruct();
    if (FAILED(hr))
    {
        mcloseAsyncOperation->Release();
        mopenAsyncOperation->Release();
        parentOperation->Release();
        return hr;
    }
    
    for (iterate = innerStatelessServiceUnits_.begin(); innerStatelessServiceUnits_.end() != iterate; iterate++)
    {
        CStatelessOpenOperation* sopenAsyncOperation = new (std::nothrow) CStatelessOpenOperation(
            mopenAsyncOperation,
            iterate->second->ServiceName,
            (IFabricStatelessServiceInstance*)iterate->second,
            (IFabricStatelessServicePartition*)this
            );
        if (NULL == sopenAsyncOperation)
        {
            hr = E_OUTOFMEMORY;
            break;
        }
        hr = sopenAsyncOperation->FinalConstruct();
        if (FAILED(hr))
        {
            sopenAsyncOperation->Release();
            break;
        }
        hr = mopenAsyncOperation->Compose(sopenAsyncOperation);
        if (FAILED(hr))
        {
            sopenAsyncOperation->Release();
            break;
        }
    }
    if (FAILED(hr))
    {
        mcloseAsyncOperation->Release();
        mopenAsyncOperation->Release();
        parentOperation->Release();
        return hr;
    }

    for (iterate = innerStatelessServiceUnits_.begin(); innerStatelessServiceUnits_.end() != iterate; iterate++)
    {
        CStatelessCloseOperation* scloseAsyncOperation = new (std::nothrow) CStatelessCloseOperation(
            mcloseAsyncOperation,
            (IFabricStatelessServiceInstance*)iterate->second
            );
        if (NULL == scloseAsyncOperation)
        {
            hr = E_OUTOFMEMORY;
            break;
        }
        hr = scloseAsyncOperation->FinalConstruct();
        if (FAILED(hr))
        {
            scloseAsyncOperation->Release();
            break;
        }
        hr = mcloseAsyncOperation->Compose(scloseAsyncOperation);
        if (FAILED(hr))
        {
            scloseAsyncOperation->Release();
            break;
        }
    }
    if (FAILED(hr))
    {
        mcloseAsyncOperation->Release();
        mopenAsyncOperation->Release();
        parentOperation->Release();
        return hr;
    }

    WriteNoise(
        TraceSubComponentStatelessServiceGroupOpen, 
        partitionIdString_,
        "this={0} instanceId={1} - Starting {2} IFabricStatelessServiceInstance::BeginOpen calls",
        this,
        instanceId_,
        innerStatelessServiceUnits_.size()
        );

    parentOperation->Register(mopenAsyncOperation, mcloseAsyncOperation);
    parentOperation->set_Callback(callback);
    parentOperation->AddRef();
    hr = parentOperation->Begin();

    if (FAILED(hr))
    {
        ServiceGroupStatelessEventSource::GetEvents().Error_4_StatelessServiceGroupOpen(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            instanceId_,
            hr
            );
    }
    else
    {
        ServiceGroupStatelessEventSource::GetEvents().Info_4_StatelessServiceGroupOpen(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            instanceId_
            );
    }

    //
    // The open operation becomes the context returned to the system.
    //
    *context = parentOperation;

    return S_OK;
}

STDMETHODIMP CStatelessServiceGroup::DoClose(
    __in IFabricAsyncOperationCallback* callback,
    __out IFabricAsyncOperationContext** context
    )
{
    ServiceGroupStatelessEventSource::GetEvents().Info_2_StatelessServiceGroupClose(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        instanceId_
        );

    HRESULT hr = S_OK;
    PartitionId_StatelessServiceUnit_Iterator iterate;
    CCompositeAsyncOperationAsync* parentOperation = new (std::nothrow) CCompositeAsyncOperationAsync(NULL, partitionId_.AsGUID());
    if (NULL == parentOperation)
    {
        return E_OUTOFMEMORY;
    }
    hr = parentOperation->FinalConstruct();
    if (FAILED(hr))
    {
        parentOperation->Release();
        return hr;
    }

    for (iterate = innerStatelessServiceUnits_.begin(); 
           innerStatelessServiceUnits_.end() != iterate; 
           iterate++)
    {
        CStatelessCloseOperation* scloseAsyncOperation = new (std::nothrow) CStatelessCloseOperation(
            parentOperation,
            (IFabricStatelessServiceInstance*)iterate->second
            );
        if (NULL == scloseAsyncOperation)
        {
            hr = E_OUTOFMEMORY;
            break;
        }
        hr = scloseAsyncOperation->FinalConstruct();
        if (FAILED(hr))
        {
            scloseAsyncOperation->Release();
            break;
        }
        hr = parentOperation->Compose(scloseAsyncOperation);
        if (FAILED(hr))
        {
            scloseAsyncOperation->Release();
            break;
        }
    }
    if (FAILED(hr))
    {
        ServiceGroupStatelessEventSource::GetEvents().Error_0_StatelessServiceGroupClose(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            instanceId_,
            hr
            );

        parentOperation->Release();
        return hr;
    }

    WriteNoise(
        TraceSubComponentStatelessServiceGroupClose, 
        partitionIdString_,
        "this={0} instanceId={1} - Starting {2} IFabricStatelessServiceInstance::BeginClose calls",
        this,
        instanceId_,
        innerStatelessServiceUnits_.size()
        );

    parentOperation->set_Callback(callback);
    parentOperation->AddRef();
    hr = parentOperation->Begin();
    if (FAILED(hr) && !parentOperation->HasStartedSuccessfully())
    {
        ServiceGroupStatelessEventSource::GetEvents().Error_0_StatelessServiceGroupClose(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            instanceId_,
            hr
            );
        //
        // This might look like a double release, but it is meant to be just that. 
        // The operation has a ref count of 1 when created.
        // And then we have the AddRef from just above the if statement.
        //
        parentOperation->Release();
        parentOperation->Release();
        return hr;
    }

    ServiceGroupStatelessEventSource::GetEvents().Info_3_StatelessServiceGroupClose(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        instanceId_
        );

    //
    // The close operation becomes the context returned to the system.
    //
    *context = parentOperation;

    return S_OK;
}

