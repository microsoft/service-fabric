// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include "ServiceGroup.Constants.h"
#include "ServiceGroup.ServiceBaseUnit.h"
#include "ServiceGroup.StatefulServiceUnit.h"
#include "ServiceGroup.ServiceBase.h"
#include "ServiceGroup.StatefulService.h"
#include "ServiceGroup.StatefulServiceFactory.h"
#include "ServiceGroup.AsyncOperations.h"
#include "ServiceGroup.AtomicGroup.h"
#include "ServiceGroup.OperationExtendedBuffer.h"
#include "ServiceGroup.ReplicatorSettings.h"

using namespace ServiceGroup;
using namespace ServiceModel;

//
// Constructor/Destructor.
//
COperationDataAsyncEnumeratorWrapper::COperationDataAsyncEnumeratorWrapper(IFabricOperationDataStream* enumerator)
{
    WriteNoise(TraceSubComponentOperationDataAsyncEnumeratorWrapper, "this={0} - ctor", this);
    ASSERT_IF(enumerator == NULL, "enumerator can't be null");
    inner_ = enumerator;
    enumerator->AddRef();
}

COperationDataAsyncEnumeratorWrapper::~COperationDataAsyncEnumeratorWrapper(void)
{
    WriteNoise(TraceSubComponentOperationDataAsyncEnumeratorWrapper, "this={0} - dtor", this);

    HRESULT hr = S_OK;
    IFabricDisposable* disposable = NULL;
    hr = inner_->QueryInterface(__uuidof(IFabricDisposable), reinterpret_cast<void**>(&disposable));
    if (SUCCEEDED(hr) && disposable != NULL)
    {
        disposable->Dispose();
        disposable->Release();
    }

    inner_->Release();
}

STDMETHODIMP COperationDataAsyncEnumeratorWrapper::BeginGetNext(
    __in IFabricAsyncOperationCallback* callback,
    __out IFabricAsyncOperationContext** context
    )
{
    WriteNoise(TraceSubComponentOperationDataAsyncEnumeratorWrapper, "this={0} - BeginGetNext", this);
    return inner_->BeginGetNext(callback, context);
}

STDMETHODIMP COperationDataAsyncEnumeratorWrapper::EndGetNext( 
    __in IFabricAsyncOperationContext* context,
    __out IFabricOperationData** operationData
    )
{
    WriteNoise(TraceSubComponentOperationDataAsyncEnumeratorWrapper, "this={0} - EndGetNext", this);
    return inner_->EndGetNext(context, operationData);
}

//
// Constructor/Destructor.
//
CStatefulServiceGroupFactory::CStatefulServiceGroupFactory() :
    activationContext_(NULL)
{
    WriteNoise(TraceSubComponentStatefulServiceGroupFactory, "this={0} - ctor", this);
}

CStatefulServiceGroupFactory::~CStatefulServiceGroupFactory()
{
    WriteNoise(TraceSubComponentStatefulServiceGroupFactory, "this={0} - dtor", this);

    //
    // Release service factories of service group members.
    //
    std::map<std::wstring, IFabricStatelessServiceFactory*>::iterator iterateStateless;
    for (iterateStateless = statelessServiceFactories_.begin(); statelessServiceFactories_.end() != iterateStateless; iterateStateless++)
    {
        iterateStateless->second->Release();
    }
    std::map<std::wstring, IFabricStatefulServiceFactory*>::iterator iterateStateful;
    for (iterateStateful = statefulServiceFactories_.begin(); statefulServiceFactories_.end() != iterateStateful; iterateStateful++)
    {
        iterateStateful->second->Release();
    }

    if (activationContext_ != NULL)
    {
        activationContext_->Release();
        activationContext_ = NULL;
    }
}

//
// IFabricStatefulServiceFactory methods.
//
STDMETHODIMP CStatefulServiceGroupFactory::CreateReplica(
    __in LPCWSTR serviceType,
    __in LPCWSTR serviceName,
    __in ULONG initializationDataLength,
    __in const byte* initializationData,
    __in FABRIC_PARTITION_ID partitionId,
    __in FABRIC_REPLICA_ID replicaId,
    __out IFabricStatefulServiceReplica** serviceReplica
    )
{
    HRESULT hr = S_OK;

    UNREFERENCED_PARAMETER(partitionId);

    ASSERT_IF(NULL == serviceReplica, "Unexpected service replica");
    ASSERT_IF(NULL == serviceType, "Unexpected service type");
    ASSERT_IF(NULL == serviceName, "Unexpected service name");

    CStatefulServiceGroup* statefulServiceGroupReplica = new (std::nothrow) CStatefulServiceGroup(replicaId);
    if (NULL == statefulServiceGroupReplica)
    {
        ServiceGroupStatefulEventSource::GetEvents().Error_0_StatefulServiceGroupFactory(reinterpret_cast<uintptr_t>(this));

        return E_OUTOFMEMORY;
    }

    //
    // Deserialize the service group initialization data.
    //
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
        ServiceGroupStatefulEventSource::GetEvents().Error_1_StatefulServiceGroupFactory(
            reinterpret_cast<uintptr_t>(this),
            replicaId,
            hr
            );

        statefulServiceGroupReplica->Release();
        return hr;
    }

    hr = statefulServiceGroupReplica->FinalConstruct(&serviceGroupDescr, statefulServiceFactories_, statelessServiceFactories_, activationContext_, serviceType);
    if (FAILED(hr))
    {
        ServiceGroupStatefulEventSource::GetEvents().Error_2_StatefulServiceGroupFactory(
            reinterpret_cast<uintptr_t>(this),
            replicaId,
            hr
            );

        statefulServiceGroupReplica->Release();
    }
    else 
    {
        ServiceGroupStatefulEventSource::GetEvents().Info_0_StatefulServiceGroupFactory(
            reinterpret_cast<uintptr_t>(this),
            replicaId
            );

        *serviceReplica = (IFabricStatefulServiceReplica*)statefulServiceGroupReplica;
    }
    return hr;
}

//
// Initialize.
//
HRESULT CStatefulServiceGroupFactory::FinalConstruct(
    __in const std::map<std::wstring, IFabricStatefulServiceFactory*>& statefulServiceFactories,
    __in const std::map<std::wstring, IFabricStatelessServiceFactory*>& statelessServiceFactories,
    __in IFabricCodePackageActivationContext * activationContext
    )
{
    if (statefulServiceFactories.empty())
    {
        ServiceGroupStatefulEventSource::GetEvents().Error_3_StatefulServiceGroupFactory(reinterpret_cast<uintptr_t>(this));

        return E_INVALIDARG;
    }

    //
    // Keep a copy of the service factories for the service group members.
    //
    HRESULT hr = S_OK;
    try
    {
        statelessServiceFactories_ = statelessServiceFactories; 
        statefulServiceFactories_ = statefulServiceFactories; 
    }
    catch (std::bad_alloc const &) { hr = E_OUTOFMEMORY; }
    catch (...) { hr = E_FAIL; }
    if (FAILED(hr))
    {
        ServiceGroupStatefulEventSource::GetEvents().Error_4_StatefulServiceGroupFactory(reinterpret_cast<uintptr_t>(this));

        return hr;
    }
    std::map<std::wstring, IFabricStatelessServiceFactory*>::iterator iterateStateless;
    for (iterateStateless = statelessServiceFactories_.begin(); statelessServiceFactories_.end() != iterateStateless; iterateStateless++)
    {
        iterateStateless->second->AddRef();
    }
    std::map<std::wstring, IFabricStatefulServiceFactory*>::iterator iterateStateful;
    for (iterateStateful = statefulServiceFactories_.begin(); statefulServiceFactories_.end() != iterateStateful; iterateStateful++)
    {
        iterateStateful->second->AddRef();
    }

    if (activationContext != NULL)
    {
        activationContext_ = activationContext;
        activationContext_->AddRef();
    }

    return hr;
}

//
// Constructor/Destructor.
//
CStatefulServiceGroup::CStatefulServiceGroup(FABRIC_REPLICA_ID replicaId)
{
    WriteNoise(TraceSubComponentStatefulServiceGroup, "this={0} replicaId={1} - ctor", this, replicaId);

    replicaId_ = replicaId;
    currentReplicaRole_ = FABRIC_REPLICA_ROLE_UNKNOWN;
    openMode_ = FABRIC_REPLICA_OPEN_MODE_INVALID;
    outerStatefulServicePartition_ = NULL;
    outerStateReplicator_ = NULL;
    outerReplicator_ = NULL;
    sequenceAtomicGroupId_ = 0;
    hasPersistedState_ = FALSE;
    outerStateReplicatorDestructed_ = FALSE;
    replicationOperationStreamCallback_ = NULL;
    copyOperationStreamCallback_ = NULL;
    outerReplicationOperationStream_ = NULL;
    outerCopyOperationStream_ = NULL;
    lastCopyOperationCount_ = 0;
    replicationStreamDrainingStarted_ = FALSE;
    copyStreamDrainingStarted_ = FALSE;
    streamsEnding_ = FALSE;
    lastCommittedLSN_ = FABRIC_INVALID_SEQUENCE_NUMBER;
    isAtomicGroupCopied_ = FALSE;
    currentEpoch_.DataLossNumber = currentEpoch_.ConfigurationNumber = -1;
    previousEpochLsn_ = _I64_MIN;
    activationContext_ = NULL;
    configurationChangedCallbackHandle_ = - 1;
    useReplicatorSettingsFromConfigPackage_ = FALSE;
    isReplicaFaulted_ = FALSE;
}

CStatefulServiceGroup::~CStatefulServiceGroup(void)
{
    WriteNoise(TraceSubComponentStatefulServiceGroup, "this={0} replicaId={1} - dtor", this, replicaId_);

    if (activationContext_ != NULL)
    {
        activationContext_->Release();
        activationContext_ = NULL;
    }

    PartitionId_StatefulServiceUnit_Iterator iterate;
    for (iterate = innerStatefulServiceUnits_.begin(); innerStatefulServiceUnits_.end() != iterate; iterate++)
    {
        iterate->second->Release();
    }
}

//
// Initialize the service group.
//
STDMETHODIMP CStatefulServiceGroup::FinalConstruct(
    __in CServiceGroupDescription* serviceGroupActivationInfo,
    __in const std::map<std::wstring, IFabricStatefulServiceFactory*>& statefulServiceFactories,
    __in const std::map<std::wstring, IFabricStatelessServiceFactory*>& statelessServiceFactories,
    __in IFabricCodePackageActivationContext * activationContext,
    __in LPCWSTR serviceTypeName
    )
{
    UNREFERENCED_PARAMETER(statelessServiceFactories);

    HRESULT hr = S_OK;

    ServiceGroupStatefulEventSource::GetEvents().Info_0_StatefulServiceGroup(
        reinterpret_cast<uintptr_t>(this),
        replicaId_
        );

    PartitionId_StatefulServiceUnit_Iterator iterate;
    PartitionId_StatefulServiceUnit_Insert insert;
    ServiceInstanceName_ServiceId_Insert serviceInsert;

    //
    // Iterate through the service group members and create the member replicas using the already registered service class factories.
    //
    for (size_t idxSvcActivationInfo = 0; idxSvcActivationInfo < serviceGroupActivationInfo->ServiceGroupMemberData.size(); idxSvcActivationInfo++)
    {
        CServiceGroupMemberDescription* serviceActivationInfo = &serviceGroupActivationInfo->ServiceGroupMemberData[idxSvcActivationInfo];
        Common::Guid partitionId(serviceActivationInfo->Identifier);

        CStatefulServiceUnit* statefulServiceUnit = new (std::nothrow) CStatefulServiceUnit(serviceActivationInfo->Identifier, replicaId_);
        if (NULL == statefulServiceUnit)
        {
            ServiceGroupStatefulEventSource::GetEvents().Error_0_StatefulServiceGroup(
                reinterpret_cast<uintptr_t>(this),
                replicaId_,
                serviceActivationInfo->ServiceType.c_str()
                );

            hr = E_OUTOFMEMORY;
            break;
        }

        std::map<std::wstring, IFabricStatefulServiceFactory*>::const_iterator factoryIterator = statefulServiceFactories.find(serviceActivationInfo->ServiceType);
        if (statefulServiceFactories.end() == factoryIterator)
        {
            ServiceGroupStatefulEventSource::GetEvents().Error_1_StatefulServiceGroup(
                reinterpret_cast<uintptr_t>(this),
                replicaId_,
                serviceActivationInfo->ServiceType.c_str()
                );

            hr = E_UNEXPECTED;
            break;
        }
            
        hr = statefulServiceUnit->FinalConstruct(serviceActivationInfo, factoryIterator->second);
        if (FAILED(hr))
        {
            ServiceGroupStatefulEventSource::GetEvents().Error_2_StatefulServiceGroup(
                reinterpret_cast<uintptr_t>(this),
                replicaId_,
                partitionId,
                hr
                );

            statefulServiceUnit->Release();
            break;
        }

        ServiceGroupStatefulEventSource::GetEvents().Info_1_StatefulServiceGroup(
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            partitionId
            );

        try
        {
            innerStatefulServiceUnits_.insert(PartitionId_StatefulServiceUnit_Pair(partitionId, statefulServiceUnit)); 
            serviceInsert = serviceInstanceNames_.insert(ServiceInstanceName_ServiceId_Pair(serviceActivationInfo->ServiceName, partitionId));
            ASSERT_IFNOT(serviceInsert.second, "Unexpected service name");
        }
        catch (std::bad_alloc const &) { hr = E_OUTOFMEMORY; }
        catch (...) { hr = E_FAIL; }
        if (FAILED(hr))
        {
            ServiceGroupStatefulEventSource::GetEvents().Error_3_StatefulServiceGroup(
                reinterpret_cast<uintptr_t>(this),
                replicaId_,
                partitionId
                );
            
            statefulServiceUnit->Release();
            break;
        }

        try { innerStatefulServiceUnitsCopyCompletion_.insert(PartitionId_CopyCompletion_Pair(partitionId, (BOOLEAN)FALSE)); }
        catch (std::bad_alloc const &) { hr = E_OUTOFMEMORY; }
        catch (...) { hr = E_FAIL; }
        if (FAILED(hr))
        {
            ServiceGroupStatefulEventSource::GetEvents().Error_3_StatefulServiceGroup(
                reinterpret_cast<uintptr_t>(this),
                replicaId_,
                partitionId
                );
            break;
        }

        ServiceGroupStatefulEventSource::GetEvents().Info_2_StatefulServiceGroup(
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            partitionId
            );
    }
    if (FAILED(hr))
    {
        ServiceGroupStatefulEventSource::GetEvents().Error_4_StatefulServiceGroup(
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            hr
            );
    
        FinalDestruct();
        return hr;
    }

    //
    // Depending whether the service group is persisted or not, some behaviors might be different.
    //
    hasPersistedState_ = serviceGroupActivationInfo->HasPersistedState;

    //
    // Activation context and service type name are use to read replicator settings from service manifest 
    // and listen for configuration changes to update the settings if necessary
    //
    if (activationContext != NULL)
    {
        ServiceGroupStatefulEventSource::GetEvents().Info_3_StatefulServiceGroup(
            reinterpret_cast<uintptr_t>(this),
            replicaId_
            );

        activationContext_ = activationContext;
        activationContext_->AddRef();
    }

    try
    {
        serviceTypeName_ = std::wstring(serviceTypeName);
    }
    catch (std::bad_alloc const &) { hr = E_OUTOFMEMORY; }
    catch (...) { hr = E_FAIL; }

    ServiceGroupStatefulEventSource::GetEvents().Info_4_StatefulServiceGroup(
        reinterpret_cast<uintptr_t>(this),
        replicaId_
        );
    return S_OK;
}

//
// Called during close. Release all resources that the service group holds from the system and
// asks the service members to replease their resources that they hold from the service group.
//
STDMETHODIMP CStatefulServiceGroup::FinalDestruct(void)
{
    Common::AcquireWriteLock lock(lock_);

    ServiceGroupStatefulEventSource::GetEvents().Info_5_StatefulServiceGroup(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_
        );

    PartitionId_StatefulServiceUnit_Iterator iterate;
    for (iterate = innerStatefulServiceUnits_.begin(); innerStatefulServiceUnits_.end() != iterate; iterate++)
    {
        iterate->second->FinalDestruct();
        iterate->second->Release();
    }
    innerStatefulServiceUnits_.clear();
    if (!outerStateReplicatorDestructed_)
    {
        if (NULL != outerStateReplicator_)
        {
            outerStateReplicator_->Release();
        }
        if (NULL != outerReplicator_)
        {
            outerReplicator_->Release();
        }
        outerStateReplicatorDestructed_ = TRUE;
    }
    if (NULL != outerStatefulServicePartition_)
    {
        outerStatefulServicePartition_->Release();
        outerStatefulServicePartition_ = NULL;
    }
    currentReplicaRole_ = FABRIC_REPLICA_ROLE_UNKNOWN;

    atomicGroupStateReplicatorCounters_.reset();

    return S_OK;
}

//
// IFabricStatefulServicePartition methods.
//
STDMETHODIMP CStatefulServiceGroup::GetPartitionInfo(__out const FABRIC_SERVICE_PARTITION_INFORMATION** bufferedValue)
{
    if (NULL == bufferedValue)
    {
        return E_POINTER;
    }

    Common::AcquireReadLock lockPartition(lock_);

    if (NULL != outerStatefulServicePartition_)
    {
        WriteNoise(
            TraceSubComponentStatefulServiceGroup, 
            partitionIdString_,
            "this={0} replicaId={1} - Calling system stateful service partition GetPartitionInfo",
            this,
            replicaId_
            );

        return outerStatefulServicePartition_->GetPartitionInfo(bufferedValue);
    }

    ServiceGroupStatefulEventSource::GetEvents().Error_5_StatefulServiceGroup(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_
        );

    return Common::ErrorCodeValue::ObjectClosed;
}

STDMETHODIMP CStatefulServiceGroup::ReportLoad(__in ULONG metricCount, __in const FABRIC_LOAD_METRIC* metrics)
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

    if (NULL != outerStatefulServicePartition_)
    {
        WriteNoise(
            TraceSubComponentStatefulServiceGroup, 
            partitionIdString_,
            "this={0} replicaId={1} - Calling system stateful service partition ReportLoad",
            this,
            replicaId_
            );

        return outerStatefulServicePartition_->ReportLoad(metricCount, metrics);
    }

    ServiceGroupStatefulEventSource::GetEvents().Error_6_StatefulServiceGroup(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_
        );

    return Common::ErrorCodeValue::ObjectClosed;
}

STDMETHODIMP CStatefulServiceGroup::ReportFault(__in FABRIC_FAULT_TYPE faultType)
{
    Common::AcquireReadLock lockPartition(lock_);

    if (NULL != outerStatefulServicePartition_)
    {
        WriteNoise(
            TraceSubComponentStatefulReportFault, 
            partitionIdString_,
            "this={0} replicaId={1} - Calling system stateful service partition ReportFault",
            this,
            replicaId_
            );

        return outerStatefulServicePartition_->ReportFault(faultType);
    }

    ServiceGroupStatefulEventSource::GetEvents().Error_0_StatefulReportFault(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_
        );

    return Common::ErrorCodeValue::ObjectClosed;
}

STDMETHODIMP CStatefulServiceGroup::ReportMoveCost(__in FABRIC_MOVE_COST moveCost)
{
    Common::AcquireReadLock lockPartition(lock_);

    if (NULL != outerStatefulServicePartition_)
    {
        WriteNoise(
            TraceSubComponentStatefulReportMoveCost,
            partitionIdString_,
            "this={0} replicaId={1} - Calling system stateful service partition ReportMoveCost",
            this,
            replicaId_
            );

        return outerStatefulServicePartition_->ReportMoveCost(moveCost);
    }

    ServiceGroupStatefulEventSource::GetEvents().Error_0_StatefulReportMoveCost(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_
        );

    return Common::ErrorCodeValue::ObjectClosed;
}

STDMETHODIMP CStatefulServiceGroup::GetReadStatus(__out FABRIC_SERVICE_PARTITION_ACCESS_STATUS* readStatus)
{
    if (NULL == readStatus)
    {
        return E_POINTER;
    }

    Common::AcquireReadLock lockPartition(lock_);

    if (NULL != outerStatefulServicePartition_)
    {
        WriteNoise(
            TraceSubComponentStatefulServiceGroup, 
            partitionIdString_,
            "this={0} replicaId={1} - Calling system stateful service partition GetReadStatus",
            this,
            replicaId_
            );

        return outerStatefulServicePartition_->GetReadStatus(readStatus);
    }

    ServiceGroupStatefulEventSource::GetEvents().Error_7_StatefulServiceGroup(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_
        );

    return Common::ErrorCodeValue::ObjectClosed;
}

STDMETHODIMP CStatefulServiceGroup::GetWriteStatus(__out FABRIC_SERVICE_PARTITION_ACCESS_STATUS* writeStatus)
{
    if (NULL == writeStatus)
    {
        return E_POINTER;
    }

    Common::AcquireReadLock lockPartition(lock_);
    
    if (NULL != outerStatefulServicePartition_)
    {
        WriteNoise(
            TraceSubComponentStatefulServiceGroup, 
            partitionIdString_,
            "this={0} replicaId={1} - Calling system stateful service partition GetWriteStatus",
            this,
            replicaId_
            );

        return outerStatefulServicePartition_->GetWriteStatus(writeStatus);
    }

    ServiceGroupStatefulEventSource::GetEvents().Error_8_StatefulServiceGroup(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_
        );

    return Common::ErrorCodeValue::ObjectClosed;
}

STDMETHODIMP CStatefulServiceGroup::CreateReplicator(
    __in IFabricStateProvider* stateProvider, 
    __in_opt FABRIC_REPLICATOR_SETTINGS const* replicatorSettings,
    __out IFabricReplicator** replicator, 
    __out IFabricStateReplicator** stateReplicator
    )
{
    //
    // The replicator settings are ignored since only the default replicator settings can be used for service groups.
    // The state provider is ignored since it is kept by the service group unit.
    //
    UNREFERENCED_PARAMETER(stateProvider);
    UNREFERENCED_PARAMETER(replicatorSettings);

    WriteNoise(
        TraceSubComponentStatefulServiceGroup, 
        partitionIdString_,
        "this={0} replicaId={1} - CreateReplicator", 
        this,
        replicaId_
        );
    
    if (NULL == replicator || NULL == stateReplicator)
    {
        return E_POINTER;
    }

    Common::AcquireReadLock lockPartition(lock_);

    if (outerStateReplicatorDestructed_ || NULL == outerReplicator_)
    {
        ServiceGroupStatefulEventSource::GetEvents().Error_9_StatefulServiceGroup(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_
            );

        return Common::ErrorCodeValue::ObjectClosed;
    }

    ServiceGroupStatefulEventSource::GetEvents().Info_6_StatefulServiceGroup(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_
        );

    //
    // The state replicator is replaced by this object.
    //
    AddRef();
    *stateReplicator = (IFabricStateReplicator*)this;

    //
    // The system replicator is returned from the system.
    //
    outerReplicator_->AddRef();
    *replicator = outerReplicator_;

    return S_OK;
}

//
// IFabricStatefulServicePartition2 methods.
//
STDMETHODIMP CStatefulServiceGroup::ReportReplicaHealth(__in const FABRIC_HEALTH_INFORMATION* healthInformation)
{
    return ReportReplicaHealth2(healthInformation, nullptr);
}

STDMETHODIMP CStatefulServiceGroup::ReportPartitionHealth(__in const FABRIC_HEALTH_INFORMATION* healthInformation)
{
    return ReportPartitionHealth2(healthInformation, nullptr);
}


//
// IFabricStatefulServicePartition3 methods.
//
STDMETHODIMP CStatefulServiceGroup::ReportReplicaHealth2(
    __in const FABRIC_HEALTH_INFORMATION* healthInformation,
    __in const FABRIC_HEALTH_REPORT_SEND_OPTIONS *sendOptions)
{
    Common::AcquireReadLock lockPartition(lock_);

    if (NULL != outerStatefulServicePartition_)
    {
        WriteNoise(
            TraceSubComponentStatefulReportReplicaHealth,
            partitionIdString_,
            "this={0} instanceId={1} - Calling system stateful service partition ReportReplicaHealth",
            this,
            replicaId_
        );

        return outerStatefulServicePartition_->ReportReplicaHealth2(healthInformation, sendOptions);
    }

    ServiceGroupStatefulEventSource::GetEvents().Error_0_StatefulReportReplicaHealth(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_
    );

    return Common::ErrorCodeValue::ObjectClosed;
}

STDMETHODIMP CStatefulServiceGroup::ReportPartitionHealth2(
    __in const FABRIC_HEALTH_INFORMATION* healthInformation,
    __in const FABRIC_HEALTH_REPORT_SEND_OPTIONS *sendOptions)
{
    Common::AcquireReadLock lockPartition(lock_);

    if (NULL != outerStatefulServicePartition_)
    {
        WriteNoise(
            TraceSubComponentStatefulReportPartitionHealth,
            partitionIdString_,
            "this={0} instanceId={1} - Calling system stateful service partition ReportPartitionHealth",
            this,
            replicaId_
        );

        return outerStatefulServicePartition_->ReportPartitionHealth2(healthInformation, sendOptions);
    }

    ServiceGroupStatefulEventSource::GetEvents().Error_0_StatefulReportPartitionHealth(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_
    );

    return Common::ErrorCodeValue::ObjectClosed;
}

//
// IFabricServiceGroupPartition methods.
//

//
// Local (in-proc) name resolution provided by the special service group partition.
//
STDMETHODIMP CStatefulServiceGroup::ResolveMember(
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
    if (NULL != outerStatefulServicePartition_)
    {
        WriteNoise(
            TraceSubComponentStatefulServiceGroup, 
            partitionIdString_,
            "this={0} replicaId={1} - Resolving member", 
            this,
            replicaId_
            );

        auto error = Common::ParameterValidator::IsValid(name, Common::ParameterValidator::MinStringSize, Common::ParameterValidator::MaxNameSize);
        if (!error.IsSuccess())
        {
            ServiceGroupStatefulEventSource::GetEvents().Error_10_StatefulServiceGroup(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_
                );

            return error.ToHResult();
        }

        try 
        {
            std::wstring key(name);
            Common::NamingUri uri;
            if (!Common::NamingUri::TryParse(key, uri))
            {
                ServiceGroupStatefulEventSource::GetEvents().Error_11_StatefulServiceGroup(
                    partitionIdString_,
                    reinterpret_cast<uintptr_t>(this),
                    replicaId_,
                    key.c_str()
                    );
    
                return FABRIC_E_INVALID_NAME_URI;
            }

            std::map<std::wstring, Common::Guid>::iterator lookupName = serviceInstanceNames_.find(uri.get_Name());
            if (serviceInstanceNames_.end() == lookupName)
            {
                ServiceGroupStatefulEventSource::GetEvents().Error_12_StatefulServiceGroup(
                    partitionIdString_,
                    reinterpret_cast<uintptr_t>(this),
                    replicaId_,
                    uri.get_Name().c_str()
                    );

                return E_INVALIDARG;
            }
            PartitionId_StatefulServiceUnit_Iterator lookupReplica = innerStatefulServiceUnits_.find(lookupName->second);
            ASSERT_IF(innerStatefulServiceUnits_.end() == lookupReplica, "Stateful service group member not found");

            return lookupReplica->second->InnerQueryInterface(riid, member);
        }
        catch (std::bad_alloc const &) { hr = E_OUTOFMEMORY; }
        catch (...) { hr = E_FAIL; }
        if (FAILED(hr))
        {
            ServiceGroupStatefulEventSource::GetEvents().Error_13_StatefulServiceGroup(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_
                );

            return hr;
        }
        return S_OK;
    }

    ServiceGroupStatefulEventSource::GetEvents().Error_14_StatefulServiceGroup(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_
        );

    return Common::ErrorCodeValue::ObjectClosed;
}

//
// Helper methods.
//
HRESULT CStatefulServiceGroup::DoOpen(__in IFabricAsyncOperationCallback* callback, __out IFabricAsyncOperationContext** context)
{
    ServiceGroupStatefulEventSource::GetEvents().Info_0_StatefulServiceGroupOpen(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_
        );

    ASSERT_IF(FABRIC_REPLICA_OPEN_MODE_INVALID == openMode_, "Unexpected open mode");

    HRESULT hr = S_OK;
    PartitionId_StatefulServiceUnit_Iterator iterate;
    CCompositeAsyncOperationAsync* mopenAsyncOperation = NULL;
    CCompositeAsyncOperationAsync* mcloseAsyncOperation = NULL;
    CStatefulCompositeUndoProgressOperation* mundoAsyncOperation = NULL;
    CStatefulCompositeOpenUndoCloseAsyncOperationAsync* parentOperation = NULL;

    //
    // Create open, undo, and close operations. The guarantee to be maintained is that if some service members
    // successfully open and some do not, the ones that successfully opened will be closed, and open error
    // will be returned to the system.
    //

    parentOperation = new (std::nothrow) CStatefulCompositeOpenUndoCloseAsyncOperationAsync(NULL, partitionId_.AsGUID());
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
  
    mopenAsyncOperation = new (std::nothrow) CCompositeAsyncOperationAsync(parentOperation, partitionId_.AsGUID());
    if (NULL == mopenAsyncOperation)
    {
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
        mopenAsyncOperation->Release();
        mcloseAsyncOperation->Release();
        parentOperation->Release();
        return hr;
    }

    if (hasPersistedState_)
    {
        mundoAsyncOperation = new (std::nothrow) CStatefulCompositeUndoProgressOperation(parentOperation, partitionId_.AsGUID());
        if (NULL == mundoAsyncOperation)
        {
            mopenAsyncOperation->Release();
            mcloseAsyncOperation->Release();
            parentOperation->Release();
            return E_OUTOFMEMORY;
        }
        hr = mundoAsyncOperation->FinalConstruct();
        if (FAILED(hr))
        {
            mopenAsyncOperation->Release();
            mcloseAsyncOperation->Release();
            mundoAsyncOperation->Release();
            parentOperation->Release();
            return hr;
        }
    }

    //
    // Populate open operation.
    //
    for (iterate = innerStatefulServiceUnits_.begin(); innerStatefulServiceUnits_.end() != iterate; iterate++)
    {
        CStatefulOpenOperation* sopenAsyncOperation = new (std::nothrow) CStatefulOpenOperation(
            mopenAsyncOperation,
            openMode_,
            (IFabricStatefulServiceReplica*)iterate->second,
            (IFabricStatefulServicePartition*)this
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
        mopenAsyncOperation->Release();
        mcloseAsyncOperation->Release();
        if (NULL != mundoAsyncOperation)
        {
            mundoAsyncOperation->Release();
        }
        parentOperation->Release();
        return hr;
    }

    //
    // Populate close operation.
    //
    for (iterate = innerStatefulServiceUnits_.begin(); innerStatefulServiceUnits_.end() != iterate; iterate++)
    {
        CStatefulCloseOperation* scloseAsyncOperation = new (std::nothrow) CStatefulCloseOperation(
            mcloseAsyncOperation,
            (IFabricStatefulServiceReplica*)iterate->second
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
        mopenAsyncOperation->Release();
        mcloseAsyncOperation->Release();
        if (NULL != mundoAsyncOperation)
        {
            mundoAsyncOperation->Release();
        }
        parentOperation->Release();
        return hr;
    }

    //
    // Populate undo operation.
    //
    if (hasPersistedState_)
    {
        for (iterate = innerStatefulServiceUnits_.begin(); innerStatefulServiceUnits_.end() != iterate; iterate++)
        {
            hr = mundoAsyncOperation->Register(iterate->second);
            if (FAILED(hr))
            {
                break;
            }
        }
        if (FAILED(hr))
        {
            mopenAsyncOperation->Release();
            mcloseAsyncOperation->Release();
            if (NULL != mundoAsyncOperation)
            {
                mundoAsyncOperation->Release();
            }
            parentOperation->Release();
            return hr;
        }
    }

    WriteNoise(
        TraceSubComponentStatefulServiceGroupOpen, 
        partitionIdString_,
        "this={0} replicaId={1} - Starting {2} IFabricStatefulServiceReplica::BeginOpen calls",
        this,
        replicaId_,
        innerStatefulServiceUnits_.size()
        );

    parentOperation->Register(mopenAsyncOperation, mundoAsyncOperation, mcloseAsyncOperation);
    parentOperation->set_Callback(callback);
    parentOperation->AddRef();
    hr = parentOperation->Begin();

    if (FAILED(hr))
    {
        ServiceGroupStatefulEventSource::GetEvents().Error_0_StatefulServiceGroupOpen(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            hr
            );
    }
    else
    {
        ServiceGroupStatefulEventSource::GetEvents().Info_1_StatefulServiceGroupOpen(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_
            );
    }

    //
    // The open operation becomes the context returned to the system.
    //
    *context = parentOperation;

    return S_OK;
}

HRESULT CStatefulServiceGroup::DoClose(__in IFabricAsyncOperationCallback* callback, __out IFabricAsyncOperationContext** context)
{
    ServiceGroupStatefulEventSource::GetEvents().Info_0_StatefulServiceGroupClose(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_,
        currentReplicaRole_
        );

    if (FABRIC_REPLICA_ROLE_PRIMARY != currentReplicaRole_)
    {
        //
        // If the replica is idle or active secondary, we will wait for the operation streams to be
        // drained before closing. This allows for the replicas to see all their operations and 
        // allows for proper cleanup of all resources associated with those operation streams.
        //
        UpdateOperationStreams(CLOSE, FABRIC_REPLICA_ROLE_NONE);
    }
    else
    {
        //
        // Drain all in-flight atomic groups.
        //
        DrainAtomicGroupsReplication();
    }

    //
    // Create close operation context
    //
    HRESULT hr = S_OK;
    CStatefulCompositeRollbackCloseOperationAsync* parentOperation = new (std::nothrow) CStatefulCompositeRollbackCloseOperationAsync(
        this,
        NULL,
        partitionId_.AsGUID());
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

    //
    // Create rollback operation
    //
    CCompositeAsyncOperationAsync * mRollbackAsyncOperation = NULL;
    hr = CreateRollbackAtomicGroupsOperation(_I64_MAX, parentOperation, &mRollbackAsyncOperation);
    if (FAILED(hr))
    {
        parentOperation->Release();
        return hr;
    }

    //
    // Create close operation
    //
    PartitionId_StatefulServiceUnit_Iterator iterate;
    CCompositeAsyncOperationAsync* mcloseAsyncOperation = new (std::nothrow) CCompositeAsyncOperationAsync(parentOperation, partitionId_.AsGUID());
    if (NULL == mcloseAsyncOperation)
    {
        if (NULL != mRollbackAsyncOperation)
        {
            mRollbackAsyncOperation->Release();
        }
        parentOperation->Release();
        return E_OUTOFMEMORY;
    }
    hr = mcloseAsyncOperation->FinalConstruct();
    if (FAILED(hr))
    {
        if (NULL != mRollbackAsyncOperation)
        {
            mRollbackAsyncOperation->Release();
        }
        parentOperation->Release();
        mcloseAsyncOperation->Release();
        return hr;
    }

    //
    // Populate close operation.
    //
    for (iterate = innerStatefulServiceUnits_.begin(); 
           innerStatefulServiceUnits_.end() != iterate; 
           iterate++)
    {
        CStatefulCloseOperation* scloseAsyncOperation = new (std::nothrow) CStatefulCloseOperation(
            mcloseAsyncOperation,
            (IFabricStatefulServiceReplica*)iterate->second
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
        ServiceGroupStatefulEventSource::GetEvents().Error_0_StatefulServiceGroupClose(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            hr
            );

        if (NULL != mRollbackAsyncOperation)
        {
            mRollbackAsyncOperation->Release();
        }
        mcloseAsyncOperation->Release();
        parentOperation->Release();
        return hr;
    }
           
    WriteNoise(
        TraceSubComponentStatefulServiceGroupClose, 
        partitionIdString_,
        "this={0} replicaId={1} - Starting {2} IFabricStatefulServiceReplica::BeginClose calls",
        this,
        replicaId_,
        innerStatefulServiceUnits_.size()
        );

    //
    // Compose and start operation
    //
    parentOperation->Register(mRollbackAsyncOperation, mcloseAsyncOperation);
    parentOperation->set_Callback(callback);
    parentOperation->AddRef();
    hr = parentOperation->Begin();
    
    if (FAILED(hr))
    {
        ServiceGroupStatefulEventSource::GetEvents().Error_8_StatefulServiceGroupClose(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            hr
            );
    }
    else
    {
        ServiceGroupStatefulEventSource::GetEvents().Info_7_StatefulServiceGroupClose(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_
            );
    }

    //
    // The close operation becomes the context returned to the system.
    //
    *context = parentOperation;

    return S_OK;
}

HRESULT CStatefulServiceGroup::DoChangeRole(
    __in FABRIC_REPLICA_ROLE newReplicaRole,
    __in IFabricAsyncOperationCallback* callback,
    __out IFabricAsyncOperationContext** context
    )
{
    ServiceGroupStatefulEventSource::GetEvents().Info_0_StatefulServiceGroupChangeRole(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_,
        ROLE_TEXT(currentReplicaRole_),
        ROLE_TEXT(newReplicaRole)
        );

    FABRIC_REPLICA_ROLE oldReplicaRole = currentReplicaRole_;

    if (FABRIC_REPLICA_ROLE_PRIMARY == newReplicaRole)
    {
        //
        // Construct resources that are only required on the primary.
        //
        Common::AcquireWriteLock lockAtomicGroupReplication(lockAtomicGroupReplication_);
        if (NULL == atomicGroupReplicationDoneSignal_)
        {
            atomicGroupReplicationDoneSignal_ = Common::make_unique<Common::ManualResetEvent>(true);
        }
    }

    //
    // Create change role operation context
    //
    CStatefulCompositeRollbackChangeRoleOperationAsync* parentOperation = new (std::nothrow) CStatefulCompositeRollbackChangeRoleOperationAsync(
        this,
        NULL, 
        partitionId_.AsGUID()
        );
    if (NULL == parentOperation)
    {
        return E_OUTOFMEMORY;
    }
    HRESULT hr = parentOperation->FinalConstruct();
    if (FAILED(hr))
    {
        parentOperation->Release();
        return hr;
    }

    //
    // On change role from primary or active secondary, rollback atomic groups
    //
    BOOLEAN mustRollback = FALSE;

    //
    // Create change role operation
    //
    PartitionId_StatefulServiceUnit_Iterator iterate;
    CStatefulCompositeChangeRoleOperation* mChangeRoleOperation = new (std::nothrow) CStatefulCompositeChangeRoleOperation(parentOperation, partitionId_.AsGUID(), newReplicaRole);
    if (NULL == mChangeRoleOperation)
    {
        parentOperation->Release();
        return E_OUTOFMEMORY;
    }
    hr = mChangeRoleOperation->FinalConstruct();
    if (FAILED(hr))
    {
        mChangeRoleOperation->Release();
        parentOperation->Release();
        return hr;
    }

    //
    // Populate change role operation.
    //
    for (iterate = innerStatefulServiceUnits_.begin(); 
           innerStatefulServiceUnits_.end() != iterate; 
           iterate++)
    {
        CStatefulChangeRoleOperation* schangeAsyncOperation = new (std::nothrow) CStatefulChangeRoleOperation(
            mChangeRoleOperation,
            iterate->second->ServiceName,
            (IFabricStatefulServiceReplica*)iterate->second,
            newReplicaRole
            );
        if (NULL == schangeAsyncOperation)
        {
            hr = E_OUTOFMEMORY;
            break;
        }
        hr = schangeAsyncOperation->FinalConstruct();
        if (FAILED(hr))
        {
            schangeAsyncOperation->Release();
            break;
        }
        hr = mChangeRoleOperation->Compose(schangeAsyncOperation);
        if (FAILED(hr))
        {
            schangeAsyncOperation->Release();
            break;
        }
    }
    if (FAILED(hr))
    {
        ServiceGroupStatefulEventSource::GetEvents().Error_0_StatefulServiceGroupChangeRole(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            currentReplicaRole_,
            newReplicaRole,
            hr
            );

        mChangeRoleOperation->Release();
        parentOperation->Release();
        return hr;
    }

    //
    // If the replica moves to a secondary role, then start the operation streams, if not already started.
    // If the replica moves to a primary role, then wait for the operation streams to drain.
    // The guarantee is that if some service members successfully change role and some do not, an error
    // is returned to the system. State is maintained such that a retry of the change role is idempotent and/or
    // a close call is handled correctly.
    //
    if (newReplicaRole != currentReplicaRole_)
    {
        if (FABRIC_REPLICA_ROLE_IDLE_SECONDARY == newReplicaRole || FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY == newReplicaRole)
        {
            if (FABRIC_REPLICA_ROLE_PRIMARY == currentReplicaRole_)
            {
                //
                // Drain all in-flight atomic groups.
                //
                DrainAtomicGroupsReplication();
            }
            if (FABRIC_REPLICA_ROLE_IDLE_SECONDARY != currentReplicaRole_ && FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY != currentReplicaRole_)
            {
                hr = StartOperationStreams();
                if (FAILED(hr))
                {
                    ServiceGroupStatefulEventSource::GetEvents().Error_1_StatefulServiceGroupChangeRole(
                        partitionIdString_,
                        reinterpret_cast<uintptr_t>(this),
                        replicaId_,
                        hr,
                        currentReplicaRole_,
                        newReplicaRole
                        );
            
                    mChangeRoleOperation->Release();
                    parentOperation->Release();
                    return hr;
                }
            }
            else
            {
                hr = UpdateOperationStreams(CHANGE_ROLE, newReplicaRole);
                if (FAILED(hr))
                {
                    ServiceGroupStatefulEventSource::GetEvents().Error_2_StatefulServiceGroupChangeRole(
                        partitionIdString_,
                        reinterpret_cast<uintptr_t>(this),
                        replicaId_,
                        hr,
                        ROLE_TEXT(currentReplicaRole_),
                        ROLE_TEXT(newReplicaRole)
                        );
            
                    mChangeRoleOperation->Release();
                    parentOperation->Release();
                    return hr;
                }
            }
        }
        if (FABRIC_REPLICA_ROLE_PRIMARY == newReplicaRole || FABRIC_REPLICA_ROLE_NONE == newReplicaRole)
        {
            if (FABRIC_REPLICA_ROLE_IDLE_SECONDARY == currentReplicaRole_ || FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY == currentReplicaRole_)
            {
                hr = UpdateOperationStreams(CHANGE_ROLE, newReplicaRole);
                if (FAILED(hr))
                {
                    ServiceGroupStatefulEventSource::GetEvents().Error_2_StatefulServiceGroupChangeRole(
                        partitionIdString_,
                        reinterpret_cast<uintptr_t>(this),
                        replicaId_,
                        hr,
                        ROLE_TEXT(currentReplicaRole_),
                        ROLE_TEXT(newReplicaRole)
                        );
            
                    mChangeRoleOperation->Release();
                    parentOperation->Release();
                    return hr;
                }
            }
            if (FABRIC_REPLICA_ROLE_PRIMARY == currentReplicaRole_ && FABRIC_REPLICA_ROLE_NONE == newReplicaRole)
            {
                //
                // Drain all in-flight atomic groups.
                //
                DrainAtomicGroupsReplication();
                //
                // Rollback and discard remaining atomic groups.
                //
                mustRollback = TRUE;
            }
            if (FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY == currentReplicaRole_ && FABRIC_REPLICA_ROLE_NONE == newReplicaRole)
            {
                //
                // Rollback and discard remaining atomic groups.
                //
                mustRollback = TRUE;
            }
        }
        if (FABRIC_REPLICA_ROLE_PRIMARY == newReplicaRole)
        {
            UpdateLastCommittedLSN(FABRIC_INVALID_SEQUENCE_NUMBER);
        }
    }

    CCompositeAsyncOperationAsync* mRollbackOperation = NULL;
    if (mustRollback)
    {
        hr = CreateRollbackAtomicGroupsOperation(_I64_MAX, parentOperation, &mRollbackOperation);
        if (FAILED(hr))
        {
            mChangeRoleOperation->Release();
            parentOperation->Release();
            return hr;
        }
    }

    WriteNoise(
        TraceSubComponentStatefulServiceGroupChangeRole, 
        partitionIdString_,
        "this={0} replicaId={1} - Starting {2} IFabricStatefulServiceReplica::BeginChangeRole calls to role {3}",
        this,
        replicaId_,
        innerStatefulServiceUnits_.size(),
        newReplicaRole
        );

    parentOperation->Register(mRollbackOperation, mChangeRoleOperation);
    parentOperation->set_Callback(callback);
    parentOperation->AddRef();
    hr = parentOperation->Begin();

    if (FAILED(hr))
    {
        ServiceGroupStatefulEventSource::GetEvents().Error_3_StatefulServiceGroupChangeRole(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            ROLE_TEXT(currentReplicaRole_),
            ROLE_TEXT(newReplicaRole),
            hr
            );
    }
    else
    {
        ServiceGroupStatefulEventSource::GetEvents().Info_1_StatefulServiceGroupChangeRole(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            ROLE_TEXT(oldReplicaRole),
            ROLE_TEXT(newReplicaRole)
            );
    }

    //
    // The change role operation becomes the context returned to the system.
    //
    *context = parentOperation;

    return S_OK;
}

HRESULT CStatefulServiceGroup::DoUpdateEpoch(
    __in FABRIC_EPOCH const & epoch, 
    __in FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber,
    __in IFabricAsyncOperationCallback* callback,
    __out IFabricAsyncOperationContext** context
    )
{
    HRESULT hr = S_OK;

    ServiceGroupStatefulEventSource::GetEvents().Info_1_StatefulServiceGroupUpdateEpoch(
        reinterpret_cast<uintptr_t>(this),
        replicaId_,
        ROLE_TEXT(currentReplicaRole_),
        previousEpochLastSequenceNumber
        );

    if (FABRIC_REPLICA_ROLE_PRIMARY == currentReplicaRole_ && !(-1 == currentEpoch_.ConfigurationNumber && -1 == currentEpoch_.DataLossNumber))
    {
        FABRIC_SERVICE_PARTITION_ACCESS_STATUS writeStatus;
        hr = GetWriteStatus(&writeStatus);
        if (FAILED(hr))
        {
            ServiceGroupStatefulEventSource::GetEvents().Error_1_StatefulServiceGroupUpdateEpoch(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_,
                hr
                );

            return hr;
        }

        if (writeStatus == FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED)
        {
            ServiceGroupStatefulEventSource::GetEvents().Error_2_StatefulServiceGroupUpdateEpoch(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_
                );

            return E_UNEXPECTED;
        }
    }

    if (FABRIC_REPLICA_ROLE_PRIMARY == currentReplicaRole_)
    {
        DrainAtomicGroupsReplication();
    }

    HRESULT hrDrainUpdateEpoch = S_OK;
    for (PartitionId_StatefulServiceUnit_Iterator iterate = innerStatefulServiceUnits_.begin(); 
         innerStatefulServiceUnits_.end() != iterate; 
         iterate++)
    {
        hr = iterate->second->DrainUpdateEpoch();
        if (FAILED(hr) && SUCCEEDED(hrDrainUpdateEpoch))
        {
            hrDrainUpdateEpoch = hr;
        }
    }
    if (FAILED(hrDrainUpdateEpoch))
    {
        //
        // This drain phase can fail only if ReporFault has been called already.
        //
        ServiceGroupStatefulEventSource::GetEvents().Error_3_StatefulServiceGroupUpdateEpoch(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            hrDrainUpdateEpoch
            );

        return hrDrainUpdateEpoch;
    }

    CStatefulCompositeRollbackUpdateEpochOperationAsync* parentOperation = new (std::nothrow) CStatefulCompositeRollbackUpdateEpochOperationAsync(
        this,
        NULL,
        partitionId_.AsGUID(),
        epoch,
        previousEpochLastSequenceNumber);
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

    CCompositeAsyncOperationAsync * mRollbackAsyncOperation = NULL;

    if (-1 != currentEpoch_.DataLossNumber && -1 != currentEpoch_.ConfigurationNumber)
    {
        if (currentEpoch_.DataLossNumber == epoch.DataLossNumber && 
            currentEpoch_.ConfigurationNumber == epoch.ConfigurationNumber &&
            _I64_MIN != previousEpochLsn_)
        {
            ServiceGroupStatefulEventSource::GetEvents().Info_2_StatefulServiceGroupUpdateEpoch(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_,
                epoch.DataLossNumber,
                epoch.ConfigurationNumber
                );
            
           ASSERT_IF(previousEpochLsn_ > previousEpochLastSequenceNumber, "Unexpected sequence number");
        }
        if (currentEpoch_.DataLossNumber < epoch.DataLossNumber || currentEpoch_.ConfigurationNumber < epoch.ConfigurationNumber ||
            (_I64_MIN != previousEpochLsn_ && previousEpochLastSequenceNumber != previousEpochLsn_))
        {
            ServiceGroupStatefulEventSource::GetEvents().Info_3_StatefulServiceGroupUpdateEpoch(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_,
                previousEpochLastSequenceNumber
                );

            hr = CreateRollbackAtomicGroupsOperation(previousEpochLastSequenceNumber, parentOperation, &mRollbackAsyncOperation);
            if (FAILED(hr))
            {
                parentOperation->Release();
                return hr;
            }
        }
    }

    CCompositeAsyncOperationAsync* mUpdateEpochAsyncOperation = new (std::nothrow) CCompositeAsyncOperationAsync(parentOperation, partitionId_.AsGUID());
    if (NULL == mUpdateEpochAsyncOperation)
    {
        parentOperation->Release();
        if (NULL != mRollbackAsyncOperation)
        {
            mRollbackAsyncOperation->Release();
        }
        return E_OUTOFMEMORY;
    }
    hr = mUpdateEpochAsyncOperation->FinalConstruct();
    if (FAILED(hr))
    {
        parentOperation->Release();
        if (NULL != mRollbackAsyncOperation)
        {
            mRollbackAsyncOperation->Release();
        }
        mUpdateEpochAsyncOperation->Release();
        return hr;
    }

    for (PartitionId_StatefulServiceUnit_Iterator it = begin(innerStatefulServiceUnits_); it != end(innerStatefulServiceUnits_); ++it)
    {
        CUpdateEpochOperationAsync* sUpdateEpochAsyncOperation = new (std::nothrow) CUpdateEpochOperationAsync(
            mUpdateEpochAsyncOperation,
            (IFabricStateProvider*)(it->second),
            epoch,
            previousEpochLastSequenceNumber);

        if (NULL == sUpdateEpochAsyncOperation)
        {
            hr = E_OUTOFMEMORY;
            break;
        }
        hr = sUpdateEpochAsyncOperation->FinalConstruct();
        if (FAILED(hr))
        {
            sUpdateEpochAsyncOperation->Release();
            break;
        }
        hr = mUpdateEpochAsyncOperation->Compose(sUpdateEpochAsyncOperation);
        if (FAILED(hr))
        {
            sUpdateEpochAsyncOperation->Release();
            break;
        }
    }

    if (FAILED(hr))
    {
        parentOperation->Release();
        if (NULL != mRollbackAsyncOperation)
        {
            mRollbackAsyncOperation->Release();
        }
        mUpdateEpochAsyncOperation->Release();
        return hr;
    }

    WriteNoise(
        TraceSubComponentStatefulServiceGroupUpdateEpoch, 
        partitionIdString_,
        "this={0} replicaId={1} - Starting {2} IFabricStateProvider::BeginUpdateEpoch calls",
        this,
        replicaId_,
        innerStatefulServiceUnits_.size()
        );

    parentOperation->Register(mRollbackAsyncOperation, mUpdateEpochAsyncOperation);
    parentOperation->set_Callback(callback);
    parentOperation->AddRef();
    hr = parentOperation->Begin();
    
    if (FAILED(hr))
    {
        ServiceGroupStatefulEventSource::GetEvents().Error_4_StatefulServiceGroupUpdateEpoch(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            hr
            );
    }
    else
    {
        ServiceGroupStatefulEventSource::GetEvents().Info_4_StatefulServiceGroupUpdateEpoch(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_
            );
    }

    //
    // The update epoch operation becomes the context returned to the system.
    //
    *context = parentOperation;

    return S_OK;
}

HRESULT CStatefulServiceGroup::DoOnDataLoss(__in IFabricAsyncOperationCallback* callback, __out IFabricAsyncOperationContext** context)
{
    ServiceGroupStatefulEventSource::GetEvents().Info_0_StatefulServiceGroupOnDataLoss(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_,
        ROLE_TEXT(currentReplicaRole_)
        );

    HRESULT hr = S_OK;
    PartitionId_StatefulServiceUnit_Iterator iterate;

    CStatefulCompositeOnDataLossUndoAsyncOperationAsync* parentOperation = new (std::nothrow) CStatefulCompositeOnDataLossUndoAsyncOperationAsync(NULL, partitionId_.AsGUID());
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

    CStatefulCompositeOnDataLossOperation* mdataLossAsyncOperation = new (std::nothrow) CStatefulCompositeOnDataLossOperation(parentOperation, partitionId_.AsGUID());
    if (NULL == mdataLossAsyncOperation)
    {
        parentOperation->Release();
        return E_OUTOFMEMORY;
    }
    hr = mdataLossAsyncOperation->FinalConstruct();
    if (FAILED(hr))
    {
        mdataLossAsyncOperation->Release();
        parentOperation->Release();
        return hr;
    }
    //
    // Populate data loss operation.
    //
    for (iterate = innerStatefulServiceUnits_.begin(); innerStatefulServiceUnits_.end() != iterate; iterate++)
    {
        CStatefulOnDataLossOperation* sdataLossAsyncOperation = new (std::nothrow) CStatefulOnDataLossOperation(
            mdataLossAsyncOperation,
            (IFabricStateProvider*)iterate->second
            );
        if (NULL == sdataLossAsyncOperation)
        {
            hr = E_OUTOFMEMORY;
            break;
        }
        hr = sdataLossAsyncOperation->FinalConstruct();
        if (FAILED(hr))
        {
            sdataLossAsyncOperation->Release();
            break;
        }
        hr = mdataLossAsyncOperation->Compose(sdataLossAsyncOperation);
        if (FAILED(hr))
        {
            sdataLossAsyncOperation->Release();
            break;
        }
    }
    if (FAILED(hr))
    {
        ServiceGroupStatefulEventSource::GetEvents().Error_0_StatefulServiceGroupOnDataLoss(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            hr
            );
        
        mdataLossAsyncOperation->Release();
        parentOperation->Release();
        return hr;
    }

    CStatefulCompositeUndoProgressOperation* mundoAsyncOperation = NULL;
    if (hasPersistedState_)
    {
        //
        // Populate undo progress operation.
        //
        mundoAsyncOperation = new (std::nothrow) CStatefulCompositeUndoProgressOperation(parentOperation, partitionId_.AsGUID());
        if (NULL == mundoAsyncOperation)
        {
            mdataLossAsyncOperation->Release();
            parentOperation->Release();
            return E_OUTOFMEMORY;
        }
        hr = mundoAsyncOperation->FinalConstruct();
        if (FAILED(hr))
        {
            mundoAsyncOperation->Release();
            mdataLossAsyncOperation->Release();
            parentOperation->Release();
            return hr;
        }
        for (iterate = innerStatefulServiceUnits_.begin(); innerStatefulServiceUnits_.end() != iterate; iterate++)
        {
            hr = mundoAsyncOperation->Register(iterate->second);
            if (FAILED(hr))
            {
                break;
            }
        }
        if (FAILED(hr))
        {
            ServiceGroupStatefulEventSource::GetEvents().Error_0_StatefulServiceGroupOnDataLoss(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_,
                hr
                );

            mundoAsyncOperation->Release();
            mdataLossAsyncOperation->Release();
            parentOperation->Release();
            return hr;
        }
    }

    parentOperation->Register(mdataLossAsyncOperation, mundoAsyncOperation);
    parentOperation->set_Callback(callback);
    parentOperation->AddRef();
    hr = parentOperation->Begin();

    if (FAILED(hr))
    {
        ServiceGroupStatefulEventSource::GetEvents().Error_1_StatefulServiceGroupOnDataLoss(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            hr
            );
    }
    else
    {
        ServiceGroupStatefulEventSource::GetEvents().Info_1_StatefulServiceGroupOnDataLoss(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_
            );
    }

    //
    // The dataloss/undo operation becomes the context returned to the system.
    //
    *context = parentOperation;

    return S_OK;
}

HRESULT CStatefulServiceGroup::GetCopyContextEnumerators(__out std::map<Common::Guid, IFabricOperationDataStream*>& copyContextEnumerators)
{
    HRESULT hr = S_OK;
    BOOLEAN allEmpty = TRUE;

    //
    // Retrieves all the copy contexts enumerators from the service group members. Called for persisted services on secondary
    // that comes up. All the copy contexts are aggregated into a single enumerator. For the service members that return NULL 
    // as their copy context, we create an empty enumerator. If all service members return NULL copy contexts, then NULL is
    // returned to the system.
    //
    for (PartitionId_StatefulServiceUnit_Iterator iterateServiceUnit = innerStatefulServiceUnits_.begin();
         iterateServiceUnit != innerStatefulServiceUnits_.end();
         iterateServiceUnit++)
    {
        IFabricOperationDataStream* copyContextEnum = NULL;
        hr = iterateServiceUnit->second->GetCopyContext(&copyContextEnum);
        if (FAILED(hr))
        {
            ServiceGroupReplicationEventSource::GetEvents().Error_0_StatefulServiceGroupCopyContext(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_,
                iterateServiceUnit->first,
                hr
                );
            break;
        }

        if (NULL == copyContextEnum)
        {
            ServiceGroupReplicationEventSource::GetEvents().Info_0_StatefulServiceGroupCopyContext(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_,
                iterateServiceUnit->first
                );

            CEmptyOperationDataAsyncEnumerator* emptyEnumerator = new (std::nothrow) CEmptyOperationDataAsyncEnumerator();
            if (NULL == emptyEnumerator)
            {
                ServiceGroupReplicationEventSource::GetEvents().Error_1_StatefulServiceGroupCopyContext(
                    partitionIdString_,
                    reinterpret_cast<uintptr_t>(this),
                    replicaId_,
                    iterateServiceUnit->first
                    );
    
                hr = E_OUTOFMEMORY;
                break;
            }
            copyContextEnum = emptyEnumerator;
        }
        else
        {
            IFabricOperationDataStream* wrapper = new (std::nothrow) COperationDataAsyncEnumeratorWrapper(copyContextEnum);
            if (wrapper == NULL)
            {
                ServiceGroupReplicationEventSource::GetEvents().Error_2_StatefulServiceGroupCopyContext(
                    partitionIdString_,
                    reinterpret_cast<uintptr_t>(this),
                    replicaId_,
                    iterateServiceUnit->first
                    );

                hr = E_OUTOFMEMORY;
                copyContextEnum->Release();
                break;
            }

            // wrapper does an addref and now holds onto the copyContextEnum
            copyContextEnum->Release();

            // the function from now-onwards uses copyContextEnum
            copyContextEnum = wrapper;
            wrapper = NULL;

            allEmpty = FALSE;
        }

        try
        {
            copyContextEnumerators.insert(std::pair<Common::Guid, IFabricOperationDataStream*>(iterateServiceUnit->first, copyContextEnum));
        }
        catch (std::bad_alloc const &) { hr = E_OUTOFMEMORY; }
        catch (...) { hr = E_FAIL; }
        if (FAILED(hr))
        {
            ServiceGroupReplicationEventSource::GetEvents().Error_3_StatefulServiceGroupCopyContext(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_,
                iterateServiceUnit->first,
                hr
                );
            copyContextEnum->Release();
            break;
        }
    }

    if (FAILED(hr) || allEmpty)
    {
        for (std::map<Common::Guid, IFabricOperationDataStream*>::iterator iterateEnum = copyContextEnumerators.begin();
             iterateEnum != copyContextEnumerators.end();
             iterateEnum++)
        {
            iterateEnum->second->Release();
        }
        copyContextEnumerators.clear();
    }

    if (FAILED(hr))
    {
        return hr;
    }

    if (copyContextEnumerators.empty())
    {
        ServiceGroupReplicationEventSource::GetEvents().Info_1_StatefulServiceGroupCopyContext(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_
            );
    }
    else
    {
        ServiceGroupReplicationEventSource::GetEvents().Info_2_StatefulServiceGroupCopyContext(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            copyContextEnumerators.size()
            );
    }
    return S_OK;
}

HRESULT CStatefulServiceGroup::DoGetCopyContext(__out IFabricOperationDataStream** copyContextEnumerator)
{
    ServiceGroupReplicationEventSource::GetEvents().Info_3_StatefulServiceGroupCopyContext(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_
        );

    CCopyOperationDataAsyncEnumerator* copyContextEnum = new (std::nothrow) CCopyOperationDataAsyncEnumerator();
    if (NULL == copyContextEnum)
    {
        ServiceGroupReplicationEventSource::GetEvents().Error_4_StatefulServiceGroupCopyContext(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_
            );
        return E_OUTOFMEMORY;
    }

    std::map<Common::Guid, IFabricOperationDataStream*> serviceCopyContextEnumerators;
    HRESULT hr = GetCopyContextEnumerators(serviceCopyContextEnumerators);
    if (FAILED(hr))
    {
        copyContextEnum->Release();
        return hr;
    }

    if (serviceCopyContextEnumerators.empty())
    {
        copyContextEnum->Release();
        *copyContextEnumerator = NULL;
    }
    else
    {
        hr = copyContextEnum->FinalConstruct(partitionId_, serviceCopyContextEnumerators);
        ASSERT_IF(FAILED(hr), "CCopyOperationDataAsyncEnumerator->FinalConstruct");
        *copyContextEnumerator = copyContextEnum;
    }

    return S_OK;
}

HRESULT CStatefulServiceGroup::GetCopyStateEnumerators(
    __in FABRIC_SEQUENCE_NUMBER uptoSequenceNumber,
    __in std::map<Common::Guid, COperationDataStream*>& serviceCopyContextEnumerators,
    __out std::map<Common::Guid, IFabricOperationDataStream*>& serviceCopyStateEnumerators
    )
{
    HRESULT hr = S_OK;
    BOOLEAN hasSequenceNumberChanged = FALSE;

    Common::Guid empty = Common::Guid::Empty();
    CAtomicGroupStateOperationDataAsyncEnumerator* atomicGroupStateEnumerator = new (std::nothrow) CAtomicGroupStateOperationDataAsyncEnumerator();
    if (NULL == atomicGroupStateEnumerator)
    {
        ServiceGroupReplicationEventSource::GetEvents().Error_0_StatefulServiceGroupCopyState(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_
            );
        
        return E_OUTOFMEMORY;
    }

    //
    // First, retrieve the atomic group state. If the copy stream raced with the last acked atomic group replication operation,
    // then replace the up-to-sequence-number to that last acked sequence number. Otherwise phantoms could occur on the
    // newly built replica in case commit/rollback are acked, so there is nothing to copy.
    //
    {
        Common::AcquireReadLock lockAtomicGroups(lockAtomicGroups_);
        if (uptoSequenceNumber < lastCommittedLSN_)
        {
            ServiceGroupReplicationEventSource::GetEvents().Info_0_StatefulServiceGroupCopyState(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_,
                uptoSequenceNumber,
                lastCommittedLSN_
                );
        
            uptoSequenceNumber = lastCommittedLSN_;
            hasSequenceNumberChanged = TRUE;
        }
        if (uptoSequenceNumber < previousEpochLsn_)
        {
            ServiceGroupReplicationEventSource::GetEvents().Info_1_StatefulServiceGroupCopyState(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_,
                uptoSequenceNumber,
                previousEpochLsn_
                );
        
            uptoSequenceNumber = previousEpochLsn_;
            hasSequenceNumberChanged = TRUE;
        }
        if (!hasSequenceNumberChanged)
        {
            ServiceGroupReplicationEventSource::GetEvents().Info_2_StatefulServiceGroupCopyState(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_,
                uptoSequenceNumber
                );
        }

        try
        {
            AtomicGroupCopyState atomicGroupCopyState;
            if (hasSequenceNumberChanged)
            {
                atomicGroupCopyState.lastCommittedLSN = uptoSequenceNumber;
            }
            atomicGroupCopyState.epochDataLossNumber = currentEpoch_.DataLossNumber;
            atomicGroupCopyState.epochConfigurationNumber = currentEpoch_.ConfigurationNumber;
            //
            // Copy only the atomic groups that have started before the given sequence number.
            //
            if (atomicGroups_.empty())
            {
                ServiceGroupReplicationEventSource::GetEvents().Info_3_StatefulServiceGroupCopyState(
                    partitionIdString_,
                    reinterpret_cast<uintptr_t>(this),
                    replicaId_
                    );
            }
            else
            {
                AtomicGroup_AtomicState_Iterator iterateAtomicGroup = atomicGroups_.begin();
                while (atomicGroups_.end() != iterateAtomicGroup)
                {
                    if (iterateAtomicGroup->second.createdSequenceNumber > uptoSequenceNumber)
                    {
                        ServiceGroupReplicationEventSource::GetEvents().Info_4_StatefulServiceGroupCopyState(
                            partitionIdString_,
                            reinterpret_cast<uintptr_t>(this),
                            replicaId_,
                            iterateAtomicGroup->first,
                            iterateAtomicGroup->second.createdSequenceNumber
                            );
                        
                        iterateAtomicGroup++;
                        continue;
                    }
                    
                    ASSERT_IF(iterateAtomicGroup->second.participants.empty(), "Unexpected atomic group");
    
                    ServiceGroupReplicationEventSource::GetEvents().Info_5_StatefulServiceGroupCopyState(
                        partitionIdString_,
                        reinterpret_cast<uintptr_t>(this),
                        replicaId_,
                        iterateAtomicGroup->first,
                        iterateAtomicGroup->second.createdSequenceNumber
                        );
    
                    atomicGroupCopyState.atomicGroups.insert(
                        AtomicGroup_AtomicState_Pair(
                            iterateAtomicGroup->first, 
                            iterateAtomicGroup->second
                            )
                        );
    
                    Participant_ParticipantState_Iterator iterateAtomicGroupParticipant = iterateAtomicGroup->second.participants.begin();
                    while (iterateAtomicGroup->second.participants.end() != iterateAtomicGroupParticipant)
                    {
                        if (iterateAtomicGroupParticipant->second.createdSequenceNumber > uptoSequenceNumber)
                        {
                            ServiceGroupReplicationEventSource::GetEvents().Info_6_StatefulServiceGroupCopyState(
                                partitionIdString_,
                                reinterpret_cast<uintptr_t>(this),
                                replicaId_,
                                innerStatefulServiceUnits_[iterateAtomicGroupParticipant->first]->get_ServiceName(),
                                iterateAtomicGroupParticipant->second.createdSequenceNumber,
                                iterateAtomicGroup->first
                                );
        
                            iterateAtomicGroupParticipant++;
                            continue;
                        }
                    
                        ServiceGroupReplicationEventSource::GetEvents().Info_7_StatefulServiceGroupCopyState(
                            partitionIdString_,
                            reinterpret_cast<uintptr_t>(this),
                            replicaId_,
                            innerStatefulServiceUnits_[iterateAtomicGroupParticipant->first]->get_ServiceName(),
                            iterateAtomicGroupParticipant->second.createdSequenceNumber,
                            iterateAtomicGroup->first
                            );
    
                        atomicGroupCopyState.atomicGroups[iterateAtomicGroup->first].participants.insert(
                            Participant_ParticipantState_Pair(
                                iterateAtomicGroupParticipant->first, 
                                iterateAtomicGroupParticipant->second
                                )
                            );
    
                        iterateAtomicGroupParticipant++;
                    }
    
                    iterateAtomicGroup++;
                }
            }
            hr = atomicGroupStateEnumerator->FinalConstruct(atomicGroupCopyState);
            serviceCopyStateEnumerators.insert(std::pair<Common::Guid, IFabricOperationDataStream*>(empty, atomicGroupStateEnumerator));
        }
        catch (std::bad_alloc const &) { hr = E_OUTOFMEMORY; }
        catch (...) { hr = E_FAIL; }
        if (FAILED(hr))
        {
            ServiceGroupReplicationEventSource::GetEvents().Error_1_StatefulServiceGroupCopyState(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_
                );
    
            atomicGroupStateEnumerator->Release();
            return hr;
        }
    }

    //
    // Enumerate over each service member and retrieve their copy state enumerators. We pass to
    // each service member our own created copy context, which will be populated from the system 
    // copy context. We will demux those copy context operations going out to the appropriate service
    // member and we will mux all copy state operations coming in from the appropriate service member.
    //
    for (PartitionId_StatefulServiceUnit_Iterator iterateServiceUnit = innerStatefulServiceUnits_.begin();
         iterateServiceUnit != innerStatefulServiceUnits_.end();
         iterateServiceUnit++)
    {
        IFabricOperationDataStream* copyStateEnum = NULL;
        hr = iterateServiceUnit->second->GetCopyState(
            uptoSequenceNumber, 
            serviceCopyContextEnumerators.empty() ? NULL : serviceCopyContextEnumerators[iterateServiceUnit->first], 
            &copyStateEnum);
        if (FAILED(hr))
        {
            ServiceGroupReplicationEventSource::GetEvents().Error_2_StatefulServiceGroupCopyState(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_,
                iterateServiceUnit->second->get_ServiceName(),
                hr
                );
            break;
        }

        if (NULL == copyStateEnum)
        {
            ServiceGroupReplicationEventSource::GetEvents().Info_8_StatefulServiceGroupCopyState(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_,
                iterateServiceUnit->first
                );

            CEmptyOperationDataAsyncEnumerator* emptyEnumerator = new (std::nothrow) CEmptyOperationDataAsyncEnumerator();
            if (NULL == emptyEnumerator)
            {
                ServiceGroupReplicationEventSource::GetEvents().Error_1_StatefulServiceGroupCopyContext(
                    partitionIdString_,
                    reinterpret_cast<uintptr_t>(this),
                    replicaId_,
                    iterateServiceUnit->first
                    );

                hr = E_OUTOFMEMORY;
                break;
            }
            copyStateEnum = emptyEnumerator;
        }
        else
        {
            IFabricOperationDataStream* wrapper = new (std::nothrow) COperationDataAsyncEnumeratorWrapper(copyStateEnum);
            if (wrapper == NULL)
            {
                ServiceGroupReplicationEventSource::GetEvents().Error_2_StatefulServiceGroupCopyContext(
                    partitionIdString_,
                    reinterpret_cast<uintptr_t>(this),
                    replicaId_,
                    iterateServiceUnit->first
                    );

                hr = E_OUTOFMEMORY;
                copyStateEnum->Release();
                break;
            }

            // wrapper does an addref and now holds onto the copyContextEnum
            copyStateEnum->Release();

            // the function from now-onwards uses copyContextEnum
            copyStateEnum = wrapper;
            wrapper = NULL;
        }

        try
        {
            serviceCopyStateEnumerators.insert(std::pair<Common::Guid, IFabricOperationDataStream*>(iterateServiceUnit->first, copyStateEnum));
        }
        catch (std::bad_alloc const &) { hr = E_OUTOFMEMORY; }
        catch (...) { hr = E_FAIL; }
        if (FAILED(hr))
        {
            ServiceGroupReplicationEventSource::GetEvents().Error_3_StatefulServiceGroupCopyState(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_,
                iterateServiceUnit->first,
                hr
                );

            copyStateEnum->Release();
            break;
        }
    }
    if (FAILED(hr))
    {
        for (std::map<Common::Guid, IFabricOperationDataStream*>::iterator iterateEnum = serviceCopyStateEnumerators.begin();
             iterateEnum != serviceCopyStateEnumerators.end();
             iterateEnum++)
        {
            iterateEnum->second->Release();
        }
        serviceCopyStateEnumerators.clear();
        return hr;
    }
    else
    {
        ServiceGroupReplicationEventSource::GetEvents().Info_9_StatefulServiceGroupCopyState(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            serviceCopyStateEnumerators.size() - 1
            );

        return S_OK;
    }
}

HRESULT CStatefulServiceGroup::DoGetCopyState(
    __in FABRIC_SEQUENCE_NUMBER uptoSequenceNumber,
    __in IFabricOperationDataStream* copyContextEnumerator,
    __out IFabricOperationDataStream** copyStateEnumerator
    )
{
    ServiceGroupReplicationEventSource::GetEvents().Info_10_StatefulServiceGroupCopyState(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_,
        currentEpoch_.DataLossNumber,
        currentEpoch_.ConfigurationNumber,
        currentReplicaRole_
        );

    HRESULT hr = S_OK;
    std::map<Common::Guid, COperationDataStream*> serviceCopyContextEnumeratorsDispatcher;
    std::map<Common::Guid, COperationDataStream*> serviceCopyContextEnumeratorsGetCopyState;
    CCopyOperationDataAsyncEnumerator* copyStateEnum = new (std::nothrow) CCopyOperationDataAsyncEnumerator();
    if (NULL == copyStateEnum)
    {
        ServiceGroupReplicationEventSource::GetEvents().Error_4_StatefulServiceGroupCopyState(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_
            );

        return E_OUTOFMEMORY;
    }
    
    if (NULL != copyContextEnumerator)
    {
        for (PartitionId_StatefulServiceUnit_Iterator iterateServiceUnit = innerStatefulServiceUnits_.begin();
             iterateServiceUnit != innerStatefulServiceUnits_.end();
             iterateServiceUnit++)
        {
            COperationDataStream* copyContextStream = new (std::nothrow) COperationDataStream(iterateServiceUnit->first);
            if (NULL == copyContextStream)
            {
                ServiceGroupReplicationEventSource::GetEvents().Error_5_StatefulServiceGroupCopyState(
                    partitionIdString_,
                    reinterpret_cast<uintptr_t>(this),
                    replicaId_,
                    iterateServiceUnit->second->get_ServiceName()
                    );
    
                hr = E_OUTOFMEMORY;
                break;
            }
            hr = copyContextStream->FinalConstruct();
            if (FAILED(hr))
            {
                ServiceGroupReplicationEventSource::GetEvents().Error_6_StatefulServiceGroupCopyState(
                    partitionIdString_,
                    reinterpret_cast<uintptr_t>(this),
                    replicaId_,
                    iterateServiceUnit->second->get_ServiceName()
                    );
    
                copyContextStream->Release();
                break;
            }
                
            try { serviceCopyContextEnumeratorsDispatcher.insert(std::pair<Common::Guid, COperationDataStream*>(iterateServiceUnit->first, copyContextStream)); }
            catch (std::bad_alloc const &) { hr = E_OUTOFMEMORY; }
            catch (...) { hr = E_FAIL; }
            if (FAILED(hr))
            {
                ServiceGroupReplicationEventSource::GetEvents().Error_7_StatefulServiceGroupCopyState(
                    partitionIdString_,
                    reinterpret_cast<uintptr_t>(this),
                    replicaId_,
                    iterateServiceUnit->second->get_ServiceName(),
                    hr
                    );
    
                copyContextStream->Release();
                break;
            }
            
            copyContextStream->AddRef();
            try { serviceCopyContextEnumeratorsGetCopyState.insert(std::pair<Common::Guid, COperationDataStream*>(iterateServiceUnit->first, copyContextStream)); }
            catch (std::bad_alloc const &) { hr = E_OUTOFMEMORY; }
            catch (...) { hr = E_FAIL; }
            if (FAILED(hr))
            {
                ServiceGroupReplicationEventSource::GetEvents().Error_7_StatefulServiceGroupCopyState(
                    partitionIdString_,
                    reinterpret_cast<uintptr_t>(this),
                    replicaId_,
                    iterateServiceUnit->second->get_ServiceName(),
                    hr
                    );
            
                copyContextStream->Release();
                break;
            }
        }
        if (FAILED(hr))
        {
            for (std::map<Common::Guid, COperationDataStream*>::iterator iteratorCCEnum = serviceCopyContextEnumeratorsDispatcher.begin();
                iteratorCCEnum != serviceCopyContextEnumeratorsDispatcher.end();
                iteratorCCEnum++)
            {
                iteratorCCEnum->second->Release();
            }
            serviceCopyContextEnumeratorsDispatcher.clear();
            for (std::map<Common::Guid, COperationDataStream*>::iterator iteratorCCEnum = serviceCopyContextEnumeratorsGetCopyState.begin();
                iteratorCCEnum != serviceCopyContextEnumeratorsGetCopyState.end();
                iteratorCCEnum++)
            {
                iteratorCCEnum->second->AddRef();
            }
            serviceCopyContextEnumeratorsGetCopyState.clear();
            copyStateEnum->Release();
            return hr;
        }

        CCopyContextStreamDispatcher* copyContextDispatcher = new (std::nothrow) CCopyContextStreamDispatcher(partitionId_, copyContextEnumerator, serviceCopyContextEnumeratorsDispatcher);
        if (NULL == copyContextDispatcher)
        {
            ServiceGroupReplicationEventSource::GetEvents().Error_8_StatefulServiceGroupCopyState(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_
                );

            for (std::map<Common::Guid, COperationDataStream*>::iterator iteratorCCEnum = serviceCopyContextEnumeratorsDispatcher.begin();
                iteratorCCEnum != serviceCopyContextEnumeratorsDispatcher.end();
                iteratorCCEnum++)
            {
                iteratorCCEnum->second->Release();
            }
            serviceCopyContextEnumeratorsDispatcher.clear();
            for (std::map<Common::Guid, COperationDataStream*>::iterator iteratorCCEnum = serviceCopyContextEnumeratorsGetCopyState.begin();
                iteratorCCEnum != serviceCopyContextEnumeratorsGetCopyState.end();
                iteratorCCEnum++)
            {
                iteratorCCEnum->second->Release();
            }
            serviceCopyContextEnumeratorsGetCopyState.clear();
            copyStateEnum->Release();
            return E_OUTOFMEMORY;
        }
        else
        {
            ServiceGroupReplicationEventSource::GetEvents().Info_11_StatefulServiceGroupCopyState(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_
                );

            copyContextDispatcher->AddRef();
            try
            {
                Common::Threadpool::Post
                (
                    [copyContextDispatcher]
                    {
                        copyContextDispatcher->Dispatch();
                        copyContextDispatcher->Release();
                    }
                );
            }
            catch (std::bad_alloc const &)
            {
                copyContextDispatcher->Release();
                copyStateEnum->Release();
                return E_OUTOFMEMORY;
            }
        }
    }

    std::map<Common::Guid, IFabricOperationDataStream*> serviceCopyStateEnumerators;
    hr = GetCopyStateEnumerators(uptoSequenceNumber, serviceCopyContextEnumeratorsGetCopyState, serviceCopyStateEnumerators);
    for (std::map<Common::Guid, COperationDataStream*>::iterator iteratorCCEnum = serviceCopyContextEnumeratorsGetCopyState.begin();
        iteratorCCEnum != serviceCopyContextEnumeratorsGetCopyState.end();
        iteratorCCEnum++)
    {
        iteratorCCEnum->second->Release();
    }
    serviceCopyContextEnumeratorsGetCopyState.clear();
    if (FAILED(hr))
    {
        copyStateEnum->Release();
        return hr;
    }

    hr = copyStateEnum->FinalConstruct(partitionId_, serviceCopyStateEnumerators);
    ASSERT_IF(FAILED(hr), "copyStateEnum->FinalConstruct");

    *copyStateEnumerator = copyStateEnum;
    return S_OK;
}

//
// IFabricStatefulServiceReplica methods.
//
STDMETHODIMP CStatefulServiceGroup::BeginOpen(
    __in FABRIC_REPLICA_OPEN_MODE openMode,
    __in IFabricStatefulServicePartition* partition,
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

    openMode_ = openMode;

    hr = partition->GetPartitionInfo(&spi);
    if (FAILED(hr))
    {
        ServiceGroupStatefulEventSource::GetEvents().Error_1_StatefulServiceGroupOpen(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
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
        ServiceGroupStatefulEventSource::GetEvents().Error_2_StatefulServiceGroupOpen(
            reinterpret_cast<uintptr_t>(this),
            replicaId_
            );

        return hr;
    }

    try
    {
        std::wstring instanceId;
        Common::StringWriter writer(instanceId);
        writer.Write("{0}/{1}", partitionIdString_, replicaId_);
        
        atomicGroupStateReplicatorCounters_ = AtomicGroupStateReplicatorCounters::CreateInstance(instanceId);
    }
    catch (...) { hr = E_FAIL; }
    if (FAILED(hr))
    {
        return hr;
    }

    //
    // Retrieve system replicators.
    //

    // get replicator settings
    ReplicatorSettings replicatorSettings(partitionId_);
    hr = GetReplicatorSettingsFromActivationContext(replicatorSettings);
    if (FAILED(hr)) { return hr; }
    
    // create replicator
    hr = partition->CreateReplicator((IFabricStateProvider*)this, replicatorSettings.GetRawPointer(), &outerReplicator_, (IFabricStateReplicator**)&outerStateReplicator_);
    if (FAILED(hr))
    {
        ServiceGroupStatefulEventSource::GetEvents().Error_3_StatefulServiceGroupOpen(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            hr
            );

        return hr;
    }
    
    WriteNoise(
        TraceSubComponentStatefulServiceGroupOpen, 
        partitionIdString_,
        "this={0} replicaId={1} - IFabricStatefulServicePartition::CreateReplicator succeeded",
        this,
        replicaId_
        );

    // register for config change notifications
    hr = RegisterConfigurationChangeHandler();
    if (FAILED(hr)) { return hr; }

    //
    // Keep a reference to the system partition.
    //
    partition->AddRef();
    
    hr = partition->QueryInterface(IID_IFabricStatefulServicePartition3, (LPVOID*)&outerStatefulServicePartition_);
    ASSERT_IF(FAILED(hr), "Unexpected stateful service partition");

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

void CStatefulServiceGroup::FixUpFailedOpen(void)
{
    ServiceGroupStatefulEventSource::GetEvents().Warning_0_StatefulServiceGroupOpen(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_
        );
    //
    // Release all resources allocated in BeginOpen.
    //
    Common::AcquireWriteLock lockPartition(lock_);
    outerStatefulServicePartition_->Release();
    outerStatefulServicePartition_ = NULL;
    outerReplicator_->Release();
    outerReplicator_ = NULL;
    outerStateReplicator_->Release();
    outerStateReplicator_ = NULL;
    outerStateReplicatorDestructed_ = TRUE;

    atomicGroupStateReplicatorCounters_.reset();
}

STDMETHODIMP CStatefulServiceGroup::EndOpen(
    __in IFabricAsyncOperationContext* context,
    __out IFabricReplicator** replicator
    )
{
    if (NULL == context || NULL == replicator)
    {
        return E_POINTER;
    }

    ServiceGroupStatefulEventSource::GetEvents().Info_2_StatefulServiceGroupOpen(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_
        );

    CStatefulCompositeOpenUndoCloseAsyncOperationAsync* parentOperation = (CStatefulCompositeOpenUndoCloseAsyncOperationAsync*)context;
    HRESULT hr = parentOperation->End();
    parentOperation->Release();
    if (FAILED(hr))
    {
        ServiceGroupStatefulEventSource::GetEvents().Error_4_StatefulServiceGroupOpen(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            hr
            );

        //
        // Ignore the hr return by Unregister and return the original one.
        //
        UnregisterConfigurationChangeHandler();
        
        PartitionId_StatefulServiceUnit_Iterator iterate;
        for (iterate = innerStatefulServiceUnits_.begin(); iterate != innerStatefulServiceUnits_.end(); ++iterate)
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
    
    ServiceGroupStatefulEventSource::GetEvents().Info_3_StatefulServiceGroupOpen(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_
        );
    ServiceGroupStatefulEventSource::GetEvents().Info_4_StatefulServiceGroupOpen(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_
        );

    //
    // Return as custom replicator the replicator we obtained in BeginOpen from the system.
    //
    outerReplicator_->AddRef();
    *replicator = outerReplicator_;
    return S_OK;
}

STDMETHODIMP CStatefulServiceGroup::BeginChangeRole(
    __in FABRIC_REPLICA_ROLE newRole,
    __in IFabricAsyncOperationCallback* callback,
    __out IFabricAsyncOperationContext** context
    )
{
    if (NULL == callback || NULL == context)
    {
        return E_POINTER;
    }
    return DoChangeRole(newRole, callback, context);
}

STDMETHODIMP CStatefulServiceGroup::EndChangeRole(
    __in IFabricAsyncOperationContext* context,
    __out IFabricStringResult** serviceEndpoint
    )
{
    if (NULL == context || NULL == serviceEndpoint)
    {
        return E_POINTER;
    }

    ServiceGroupStatefulEventSource::GetEvents().Info_2_StatefulServiceGroupChangeRole(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_
        );

    //
    // Now that change role has completed successfully, retrieve the composite service endpoint.
    // The service group endpoint contains the concatenation of service group member endpoints.
    // The interpretation of this composite service endpoint is left up to the service group client.
    //
    CStatefulCompositeRollbackChangeRoleOperationAsync* parentOperation = (CStatefulCompositeRollbackChangeRoleOperationAsync*)context;
    
    FABRIC_REPLICA_ROLE newReplicaRole = FABRIC_REPLICA_ROLE_NONE;
    HRESULT hr = parentOperation->get_ReplicaRole(&newReplicaRole);
    ASSERT_IF(FAILED(hr), "get_ReplicaRole");

    hr = parentOperation->End();
    if (FAILED(hr))
    {
        ServiceGroupStatefulEventSource::GetEvents().Error_4_StatefulServiceGroupChangeRole(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            ROLE_TEXT(currentReplicaRole_),
            ROLE_TEXT(newReplicaRole),
            hr
            );

        parentOperation->Release();
        return hr;
    }

    hr = parentOperation->get_ServiceEndpoint(serviceEndpoint);
    parentOperation->Release();
    if (FAILED(hr))
    {
        ServiceGroupStatefulEventSource::GetEvents().Error_5_StatefulServiceGroupChangeRole(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            hr
            );
        return hr;
    }

    ServiceGroupStatefulEventSource::GetEvents().Info_3_StatefulServiceGroupChangeRole(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_,
        ROLE_TEXT(currentReplicaRole_),
        ROLE_TEXT(newReplicaRole)
        );

    //
    // Destruct resources used for previous role. 
    //
    if (FABRIC_REPLICA_ROLE_PRIMARY != newReplicaRole)
    {
        //
        // Acquire the lock to make sure that no replication attempt is ongoing while the signal is being closed.
        //
        Common::AcquireWriteLock lockAtomicGroups(lockAtomicGroupReplication_);

        if (NULL != atomicGroupReplicationDoneSignal_)
        {
            atomicGroupReplicationDoneSignal_.reset();
        }
    }

    //
    // Switch the replica role of this service group.
    //
    currentReplicaRole_ = newReplicaRole;

    return hr;
}

STDMETHODIMP CStatefulServiceGroup::BeginClose(
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

STDMETHODIMP CStatefulServiceGroup::EndClose(__in IFabricAsyncOperationContext* context)
{
    if (NULL == context)
    {
        return E_POINTER;
    }

    ServiceGroupStatefulEventSource::GetEvents().Info_1_StatefulServiceGroupClose(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_
        );

    CStatefulCompositeRollbackCloseOperationAsync* parentOperation = (CStatefulCompositeRollbackCloseOperationAsync*)context;
    HRESULT hr = parentOperation->End();
    parentOperation->Release();
    if (FAILED(hr))
    {
        ServiceGroupStatefulEventSource::GetEvents().Error_0_StatefulServiceGroupClose(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            hr
            );

        return hr;
    }

    UnregisterConfigurationChangeHandler();

    ServiceGroupStatefulEventSource::GetEvents().Info_2_StatefulServiceGroupClose(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_
        );

    FinalDestruct();

    return S_OK;
}

STDMETHODIMP_(void) CStatefulServiceGroup::Abort()
{
    ServiceGroupStatefulEventSource::GetEvents().Info_0_StatefulServiceGroupAbort(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_,
        ROLE_TEXT(currentReplicaRole_)
        );

    InternalAbort(partitionId_.AsGUID(), FALSE);

    if (FABRIC_REPLICA_ROLE_PRIMARY != currentReplicaRole_)
    {
        //
        // If the replica is idle or active secondary, we will wait for the operation streams to be
        // drained before aborting. This allows for the replicas to see all their operations and 
        // allows for proper cleanup of all resources associated with those operation streams.
        //
        UpdateOperationStreams(ABORT, FABRIC_REPLICA_ROLE_NONE);
    }
    else
    {
        //
        // Drain all in-flight atomic groups (does not roll them back).
        //
        DrainAtomicGroupsReplication();
    }

    UnregisterConfigurationChangeHandler();

    PartitionId_StatefulServiceUnit_Iterator iterate;
    for (iterate = innerStatefulServiceUnits_.begin(); 
           innerStatefulServiceUnits_.end() != iterate; 
           iterate++)
    {
        iterate->second->Abort();
    }

    FinalDestruct();
}

//
// IFabricStateReplicator methods.
//
STDMETHODIMP CStatefulServiceGroup::BeginReplicate(
    __in IFabricOperationData* operationData,
    __in IFabricAsyncOperationCallback* callback,
    __out FABRIC_SEQUENCE_NUMBER* sequenceNumber,
    __out IFabricAsyncOperationContext** context
    )
{
    if (NULL == operationData || NULL == sequenceNumber || NULL == context)
    {
        return E_POINTER;
    }

    {
        Common::AcquireReadLock lockReplicator(lock_);
    
        if (outerStateReplicatorDestructed_)
        {
            ServiceGroupReplicationEventSource::GetEvents().Error_0_StatefulServiceGroupReplicationProcessing(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_
                );

            return Common::ErrorCodeValue::ObjectClosed;
        }
        outerStateReplicator_->AddRef();
    }
    
    WriteNoise(
        TraceSubComponentStatefulServiceGroupReplicationProcessing, 
        partitionIdString_,
        "this={0} replicaId={1} - Calling system stateful replicator IFabricStateReplicator::BeginReplicate",
        this,
        replicaId_
        );

    HRESULT hr = outerStateReplicator_->BeginReplicate(
        operationData, 
        callback, 
        sequenceNumber, 
        context
        );
    outerStateReplicator_->Release();
    return hr;
}

STDMETHODIMP CStatefulServiceGroup::EndReplicate(__in IFabricAsyncOperationContext* context, __out FABRIC_SEQUENCE_NUMBER* sequenceNumber)
{
    if (NULL == context)
    {
        return E_POINTER;
    }

    {
        Common::AcquireReadLock lockReplicator(lock_);

        if (outerStateReplicatorDestructed_)
        {
            ServiceGroupReplicationEventSource::GetEvents().Error_0_StatefulServiceGroupReplicationProcessing(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_
                );
        
            return Common::ErrorCodeValue::ObjectClosed;
        }
        outerStateReplicator_->AddRef();
    }
    
    WriteNoise(
        TraceSubComponentStatefulServiceGroupReplicationProcessing, 
        partitionIdString_,
        "this={0} replicaId={1} - Calling system stateful replicator IFabricStateReplicator::EndReplicate",
        this,
        replicaId_
        );

    HRESULT hr = outerStateReplicator_->EndReplicate(context, sequenceNumber);
    outerStateReplicator_->Release();
    return hr;
}

STDMETHODIMP CStatefulServiceGroup::GetCopyStream(__out IFabricOperationStream**)
{
    {
        Common::AcquireWriteLock lockReplicator(lock_);
    
        if (outerStateReplicatorDestructed_ || streamsEnding_)
        {
            ServiceGroupReplicationEventSource::GetEvents().Error_0_StatefulServiceGroupReplicationProcessing(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_
                );

            return Common::ErrorCodeValue::ObjectClosed;
        }

        if (copyStreamDrainingStarted_)
        {
            ServiceGroupReplicationEventSource::GetEvents().Info_0_StatefulServiceGroupCopyProcessing(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_
                );

            return S_OK;
        }

        if (NULL == outerCopyOperationStream_)
        {
            ServiceGroupReplicationEventSource::GetEvents().Error_0_StatefulServiceGroupCopyProcessing(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_
                );
    
            return Common::ErrorCodeValue::ObjectClosed;
        }

        copyStreamDrainingStarted_ = TRUE;
    }

    ServiceGroupReplicationEventSource::GetEvents().Info_1_StatefulServiceGroupCopyProcessing(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_
        );

    //
    // When this call is coming from the service group unit, we start the draining of the
    // system copy operation stream. Once started, it is the responsibility of the service
    // to drain it. The drain is started only once.
    //
    return DrainOperationStream(TRUE);
}

STDMETHODIMP CStatefulServiceGroup::GetReplicationStream(__out IFabricOperationStream**)
{
    {
        Common::AcquireWriteLock lockReplicator(lock_);
    
        if (outerStateReplicatorDestructed_ || streamsEnding_)
        {
            ServiceGroupReplicationEventSource::GetEvents().Error_0_StatefulServiceGroupReplicationProcessing(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_
                );

            return Common::ErrorCodeValue::ObjectClosed;
        }

        if (replicationStreamDrainingStarted_)
        {
            ServiceGroupReplicationEventSource::GetEvents().Info_0_StatefulServiceGroupReplicationProcessing(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_
                );

            return S_OK;
        }

        if (NULL == outerReplicationOperationStream_)
        {
            ServiceGroupReplicationEventSource::GetEvents().Error_1_StatefulServiceGroupReplicationProcessing(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_
                );
    
            return Common::ErrorCodeValue::ObjectClosed;
        }

        replicationStreamDrainingStarted_ = TRUE;
    }

    ServiceGroupReplicationEventSource::GetEvents().Info_1_StatefulServiceGroupReplicationProcessing(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_
        );

    //
    // When this call is coming from the service group unit, we start the draining of the
    // system replication operation stream. Once started, it is the responsibility of the service
    // to drain it. The drain is started only once.
    //
    AddRef();
    try
    {
        Common::Threadpool::Post
        (
            [this]
            {
                ServiceGroupReplicationEventSource::GetEvents().Info_10_StatefulServiceGroupReplicationProcessing(
                    this->partitionIdString_,
                    reinterpret_cast<uintptr_t>(this),
                    this->replicaId_
                    );

                //
                // This event is set by copy stream processing. This requires that the copy stream is always picked up
                // and either the atomic group state is copied or the copy stream terminates with NULL.
                // (When changing from Primary => Active the event is always set.)
                //
                this->atomicGroupCopyDoneSignal_.WaitOne();
                this->atomicGroupCopyDoneSignal_.Reset();

                //
                // Check whether the replica has been faulted, if so there is no need to start draining.
                // This is an optimization, the pump will stop if there is a fault after an operation has been retrieved.
                //
                BOOLEAN drain = TRUE;
                {
                    Common::AcquireReadLock lock(this->lockFaultReplica_);
                    drain = !this->isReplicaFaulted_;
                }

                if (drain)
                {
                    HRESULT hr = this->DrainOperationStream(FALSE);
                    ASSERT_IF(FAILED(hr), SERVICE_REPLICA_DOWN);
                }
                else
                {
                    this->lastReplicationOperationDispatched_.Set();
                }

                this->Release();
            }
        );
    }
    catch (std::bad_alloc const &)
    {
        Common::Assert::CodingError(SERVICE_REPLICA_DOWN);
    }
    return S_OK;
}

STDMETHODIMP CStatefulServiceGroup::UpdateReplicatorSettings(__in FABRIC_REPLICATOR_SETTINGS const * replicatorSettings)
{
    UNREFERENCED_PARAMETER(replicatorSettings);
    return E_NOTIMPL;
}

STDMETHODIMP CStatefulServiceGroup::GetReplicatorSettings(__out IFabricReplicatorSettingsResult** replicatorSettingsResult)
{
    return outerStateReplicator_->GetReplicatorSettings(replicatorSettingsResult);
}

HRESULT CStatefulServiceGroup::DrainOperationStream(__in BOOLEAN isCopy)
{
    HRESULT hr = S_OK;
    IFabricAsyncOperationContext* context = NULL;
    BOOLEAN completedSynchronously = TRUE;
    //
    // Loop until operations stop completing synchronously or last operation is retrieved.
    //
    while (completedSynchronously)
    {
        if (isCopy)
        {
            WriteNoise(
                TraceSubComponentStatefulServiceGroupCopyProcessing, 
                partitionIdString_,
                "this={0} replicaId={1} - Calling copy IFabricOperationStream::BeginGetOperation",
                this,
                replicaId_
                );
    
            hr = outerCopyOperationStream_->BeginGetOperation(copyOperationStreamCallback_, &context);
            ASSERT_IF(FAILED(hr), "copy BeginGetOperation");
        }
        else
        {
            WriteNoise(
                TraceSubComponentStatefulServiceGroupReplicationProcessing,
                partitionIdString_,
                "this={0} replicaId={1} - Calling replication IFabricOperationStream::BeginGetOperation",
                this,
                replicaId_
                );
    
            hr = outerReplicationOperationStream_->BeginGetOperation(replicationOperationStreamCallback_, &context);
            ASSERT_IF(FAILED(hr), "replication BeginGetOperation");
        }
        if (SUCCEEDED(hr))
        {
            completedSynchronously = context->CompletedSynchronously();
            if (completedSynchronously)
            {
                hr = CompleteStreamOperation(context, isCopy);
                //
                // If the last operation from the stream was seen, then stop draining.
                //
                completedSynchronously = (HR_STOP_OPERATION_PUMP != hr);
            }
            
            context->Release();
        }
        else
        {
            if (isCopy)
            {
                ServiceGroupReplicationEventSource::GetEvents().Error_1_StatefulServiceGroupCopyProcessing(
                    partitionIdString_,
                    reinterpret_cast<uintptr_t>(this),
                    replicaId_,
                    hr
                    );
            
               ASSERT_IF(FAILED(hr), "Copy operation stream is broken");
            }
            else
            {
                ServiceGroupReplicationEventSource::GetEvents().Error_2_StatefulServiceGroupReplicationProcessing(
                    partitionIdString_,
                    reinterpret_cast<uintptr_t>(this),
                    replicaId_,
                    hr
                    );
    
                ASSERT_IF(FAILED(hr), "Replication operation stream is broken");
            }
        }
    }
    return hr;
}

HRESULT CStatefulServiceGroup::CompleteStreamOperation(
    __in IFabricAsyncOperationContext* context, 
    __in BOOLEAN isCopy
    )
{
    if (isCopy)
    {
        HRESULT hr = CompleteCopyOperation(context);
        ASSERT_IF(FAILED(hr), "CompleteCopyOperation");
        if (HR_STOP_OPERATION_PUMP == hr)
        {
            ServiceGroupReplicationEventSource::GetEvents().Info_2_StatefulServiceGroupCopyProcessing(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_
                );

            lastCopyOperationDispatched_.Set();
        }

        return hr;
    }
    else
    {
        HRESULT hr = CompleteReplicationOperation(context);
        ASSERT_IF(FAILED(hr), "CompleteReplicationOperation");
        if (HR_STOP_OPERATION_PUMP == hr)
        {
            ServiceGroupReplicationEventSource::GetEvents().Info_2_StatefulServiceGroupReplicationProcessing(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_
                );

            lastReplicationOperationDispatched_.Set();
        }

        return hr;
    }
}

HRESULT CStatefulServiceGroup::StartInnerOperationStreams(void)
{
    HRESULT hr = S_OK;
    PartitionId_StatefulServiceUnit_Iterator iterate;
    PartitionId_StatefulServiceUnit_Iterator iterateFailed;
    
    ServiceGroupReplicationEventSource::GetEvents().Info_0_StatefulServiceGroupOperationStreams(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_
        );

    for (iterate = innerStatefulServiceUnits_.begin(); innerStatefulServiceUnits_.end() != iterate; iterate++)
    {
        hr = iterate->second->StartOperationStreams();
        if (FAILED(hr))
        {
            break;
        }
    }
    if (FAILED(hr))
    {
        for (iterateFailed = innerStatefulServiceUnits_.begin(); iterateFailed != iterate; iterateFailed++)
        {
            iterateFailed->second->ClearOperationStreams();
        }

        ServiceGroupReplicationEventSource::GetEvents().Error_0_StatefulServiceGroupOperationStreams(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            hr
            );
    }
    return hr;
}

HRESULT CStatefulServiceGroup::StartOperationStreams(void)
{
    ServiceGroupReplicationEventSource::GetEvents().Info_1_StatefulServiceGroupOperationStreams(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_
        );

    HRESULT hr = S_OK;
    
    Common::AcquireWriteLock lockReplicator(lock_);

    //
    // Retrieve the system operation streams and create operation callbacks.
    //
    if (NULL != outerReplicationOperationStream_)
    {
        ASSERT_IF(NULL == replicationOperationStreamCallback_, "Unexpected replication operation stream callback");
        ASSERT_IF(NULL == outerReplicationOperationStream_, "Unexpected replication operation stream");
        ASSERT_IF(NULL == copyOperationStreamCallback_, "Unexpected copy operation stream callback");
        ASSERT_IF(NULL == outerCopyOperationStream_, "Unexpected copy operation stream");

        ServiceGroupReplicationEventSource::GetEvents().Warning_0_StatefulServiceGroupOperationStreams(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_
            );

        return S_FALSE;
    }
    else
    {
        ASSERT_IFNOT(NULL == replicationOperationStreamCallback_, "Unexpected replication operation stream callback");
        ASSERT_IFNOT(NULL == outerReplicationOperationStream_, "Unexpected replication operation stream");
        ASSERT_IFNOT(NULL == copyOperationStreamCallback_, "Unexpected copy operation stream callback");
        ASSERT_IFNOT(NULL == outerCopyOperationStream_, "Unexpected copy operation stream");
    }

    hr = outerStateReplicator_->GetReplicationStream(&outerReplicationOperationStream_);
    if (FAILED(hr))
    {
        ServiceGroupReplicationEventSource::GetEvents().Error_1_StatefulServiceGroupOperationStreams(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_
            );

        return hr;
    }

    hr = outerStateReplicator_->GetCopyStream(&outerCopyOperationStream_);
    if (FAILED(hr))
    {
        ServiceGroupReplicationEventSource::GetEvents().Error_2_StatefulServiceGroupOperationStreams(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_
            );

        outerReplicationOperationStream_->Release();
        outerReplicationOperationStream_ = NULL;
        return hr;
    }

    replicationOperationStreamCallback_ = new (std::nothrow) COperationStreamCallback(this, FALSE);
    if (NULL == replicationOperationStreamCallback_)
    {
        ServiceGroupReplicationEventSource::GetEvents().Error_3_StatefulServiceGroupOperationStreams(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_
            );

        outerReplicationOperationStream_->Release();
        outerReplicationOperationStream_ = NULL;
        outerCopyOperationStream_->Release();
        outerCopyOperationStream_ = NULL;
        return E_OUTOFMEMORY;
    }

    copyOperationStreamCallback_ = new (std::nothrow) COperationStreamCallback(this, TRUE);
    if (NULL == copyOperationStreamCallback_)
    {
        ServiceGroupReplicationEventSource::GetEvents().Error_4_StatefulServiceGroupOperationStreams(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_
            );

        outerReplicationOperationStream_->Release();
        outerReplicationOperationStream_ = NULL;
        outerCopyOperationStream_->Release();
        outerCopyOperationStream_ = NULL;
        replicationOperationStreamCallback_->Release();
        replicationOperationStreamCallback_ = NULL;
        return E_OUTOFMEMORY;
    }

    //
    // Request each service member to start its operation streams. Each service member operation stream
    // will be populated by the service group retrieving operations from the system operation stream and 
    // and dispatching them to the appropriate service member.
    //
    hr = StartInnerOperationStreams();
    if (FAILED(hr))
    {
        ServiceGroupReplicationEventSource::GetEvents().Error_5_StatefulServiceGroupOperationStreams(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            hr
            );

        outerReplicationOperationStream_->Release();
        outerReplicationOperationStream_ = NULL;
        outerCopyOperationStream_->Release();
        outerCopyOperationStream_ = NULL;
        replicationOperationStreamCallback_->Release();
        replicationOperationStreamCallback_ = NULL;
        copyOperationStreamCallback_->Release();
        copyOperationStreamCallback_ = NULL;
        return hr;
    }

    //
    // Reset state related to secondary processing.
    //
    lastCopyOperationCount_ = 0;

    //
    // In the special case of changing the role from primary to secondary, there will be no copy stream
    // available, since the replica is already up-to-date. Replication stream needs to be created though 
    // so that the new secondary can receive replication operations from the new primary.
    //
    if (FABRIC_REPLICA_ROLE_PRIMARY == currentReplicaRole_)
    {
        ServiceGroupReplicationEventSource::GetEvents().Info_2_StatefulServiceGroupOperationStreams(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_
            );

        isAtomicGroupCopied_ = TRUE;
        lastCopyOperationDispatched_.Set();
        copyStreamDrainingStarted_ = TRUE;
        lastCopyOperationCount_ = innerStatefulServiceUnits_.size() + 1;
        atomicGroupCopyDoneSignal_.Set();
    }

    streamsEnding_ = FALSE;
    return S_OK;
}

HRESULT CStatefulServiceGroup::CompleteCopyOperation(
    __in IFabricAsyncOperationContext* context
    )
{
    ASSERT_IF(NULL == context, "Unexpected context");

    IFabricOperation* operation = NULL;
    FABRIC_PARTITION_ID partitionId;
    PartitionId_StatefulServiceUnit_Iterator iterate;

    HRESULT hr = outerCopyOperationStream_->EndGetOperation(context, &operation);
    if (FAILED(hr))
    {
        ServiceGroupReplicationEventSource::GetEvents().Error_2_StatefulServiceGroupCopyProcessing(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            hr
            );

        ASSERT_IF(FAILED(hr), "Copy operation stream is broken");
        return hr;
    }

    //
    // Complete operation pump as soon as report fault was called.
    //
    Common::AcquireReadLock lock(lockFaultReplica_);
    if (isReplicaFaulted_)
    {
        if (NULL != operation)
        {
            operation->Release();
        }

        ServiceGroupReplicationEventSource::GetEvents().Warning_3_StatefulServiceGroupCopyProcessing(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_
            );

        if (!isAtomicGroupCopied_)
        {
            //
            // Unblock replication processing. This only needs to be done if the atomic group state hasn't been copied. Otherwise the event is already set.
            //
            atomicGroupCopyDoneSignal_.Set();
        }

        return HR_STOP_OPERATION_PUMP;
    }

    if (NULL == operation)
    {
        ServiceGroupReplicationEventSource::GetEvents().Info_3_StatefulServiceGroupCopyProcessing(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_
            );

        if (!isAtomicGroupCopied_)
        {
            ServiceGroupReplicationEventSource::GetEvents().Warning_0_StatefulServiceGroupCopyProcessing(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_
                );
        }

        if (lastCopyOperationCount_ != innerStatefulServiceUnits_.size() + 1)
        {
            ServiceGroupReplicationEventSource::GetEvents().Warning_1_StatefulServiceGroupCopyProcessing(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_
                );

            for (iterate = innerStatefulServiceUnits_.begin(); innerStatefulServiceUnits_.end() != iterate; iterate++)
            {
                hr = iterate->second->ForceDrainReplicationStream(TRUE == innerStatefulServiceUnitsCopyCompletion_[iterate->first]);
                ASSERT_IF(FAILED(hr), "Unexpected error");
            }
        }

        for (iterate = innerStatefulServiceUnits_.begin(); innerStatefulServiceUnits_.end() != iterate; iterate++)
        {
            hr = iterate->second->EnqueueCopyOperation(NULL);
            ASSERT_IF(FAILED(hr), "NULL copy operation processing failed");
        }

        if (!isAtomicGroupCopied_)
        {
            //
            // Unblock replication processing. This only needs to be done if the atomic group state hasn't been copied. Otherwise the event is already set.
            //
            atomicGroupCopyDoneSignal_.Set();
        }

        ServiceGroupReplicationEventSource::GetEvents().Info_4_StatefulServiceGroupCopyProcessing(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_
            );

        return HR_STOP_OPERATION_PUMP;
    }

    ULONG bufferCount = 0;
    FABRIC_OPERATION_DATA_BUFFER const * replicaBuffers = NULL;

    hr = operation->GetData(&bufferCount, &replicaBuffers);
    ASSERT_IF(FAILED(hr), "GetData");
    ASSERT_IF(0 == bufferCount, "must have at least 1 buffer");

    const FABRIC_OPERATION_METADATA* operationMetadata =  operation->get_Metadata();
    ASSERT_IF(NULL == operationMetadata, "Unexpected operation metadata");

    ServiceGroupReplicationEventSource::GetEvents().Info_5_StatefulServiceGroupCopyProcessing(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_,
        operationMetadata->SequenceNumber
        );

    CExtendedOperationDataBuffer extendedReplicaBuffer;
    hr = CExtendedOperationDataBuffer::Read(extendedReplicaBuffer, replicaBuffers[0]);
    if (FAILED(hr))
    {
        ServiceGroupReplicationEventSource::GetEvents().Error_3_StatefulServiceGroupCopyProcessing(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_
            );

        operation->Release();
        ReportFault(partitionId_.AsGUID(), FABRIC_FAULT_TYPE_PERMANENT);
        return HR_STOP_OPERATION_PUMP;
    }

    ASSERT_IF(FABRIC_OPERATION_TYPE_NORMAL != extendedReplicaBuffer.OperationType, "operationType");

    partitionId = extendedReplicaBuffer.PartitionId;
    if (IsServiceGroupRelatedOperation(partitionId))
    {
        WriteNoise(
            TraceSubComponentStatefulServiceGroupCopyProcessing, 
            partitionIdString_,
            "this={0} replicaId={1} - Service group processing copy operation {2}",
            this,
            replicaId_,
            operation
            );

        if (1 == bufferCount)
        {
            if (0 != lastCopyOperationCount_)
            {
                ASSERT_IF(copyCompletionIterator_ == innerStatefulServiceUnitsCopyCompletion_.end(), "Unexpected copy iterator");

                ServiceGroupReplicationEventSource::GetEvents().Info_6_StatefulServiceGroupCopyProcessing(
                    partitionIdString_,
                    reinterpret_cast<uintptr_t>(this),
                    replicaId_,
                    copyCompletionIterator_->first
                    );

                innerStatefulServiceUnitsCopyCompletion_[copyCompletionIterator_->first] = (BOOLEAN)TRUE;
                copyCompletionIterator_++;
            }
            else
            {

                ServiceGroupReplicationEventSource::GetEvents().Info_7_StatefulServiceGroupCopyProcessing(
                    partitionIdString_,
                    reinterpret_cast<uintptr_t>(this),
                    replicaId_
                    );

                copyCompletionIterator_ = innerStatefulServiceUnitsCopyCompletion_.begin();
            }
            lastCopyOperationCount_++;
        }
        else if (!isAtomicGroupCopied_)
        {
            try
            {
                std::vector<byte> buffer;
                ASSERT_IFNOT(bufferCount == 2, "buffer Count");
                buffer.insert(buffer.end(), replicaBuffers[1].Buffer, replicaBuffers[1].Buffer + replicaBuffers[1].BufferSize);
                AtomicGroupCopyState atomicGroupCopyState;

                Common::ErrorCode error = Common::FabricSerializer::Deserialize(atomicGroupCopyState, buffer);
                if (error.IsSuccess())
                {
                    lastCommittedLSN_ = atomicGroupCopyState.lastCommittedLSN;
                    currentEpoch_.DataLossNumber = atomicGroupCopyState.epochDataLossNumber;
                    currentEpoch_.ConfigurationNumber = atomicGroupCopyState.epochConfigurationNumber;
                    atomicGroups_ = atomicGroupCopyState.atomicGroups;
                }
                else
                {
                    hr = error.ToHResult();
                }
            }
            catch (std::bad_alloc const &) { hr = E_OUTOFMEMORY; }
            catch (...) { hr = E_FAIL; }
            if (FAILED(hr))
            {
                ServiceGroupReplicationEventSource::GetEvents().Error_3_StatefulServiceGroupCopyProcessing(
                    partitionIdString_,
                    reinterpret_cast<uintptr_t>(this),
                    replicaId_
                    );
    
                operation->Release();
                ReportFault(partitionId_.AsGUID(), FABRIC_FAULT_TYPE_PERMANENT);
                return HR_STOP_OPERATION_PUMP;
            }
            else
            {
                ServiceGroupReplicationEventSource::GetEvents().Info_8_StatefulServiceGroupCopyProcessing(
                    partitionIdString_,
                    reinterpret_cast<uintptr_t>(this),
                    replicaId_,
                    currentEpoch_.DataLossNumber,
                    currentEpoch_.ConfigurationNumber
                    );

                for (AtomicGroup_AtomicState_Iterator iterateAtomicGroup = atomicGroups_.begin(); atomicGroups_.end() != iterateAtomicGroup; iterateAtomicGroup++)
                {
                    ServiceGroupReplicationEventSource::GetEvents().Info_9_StatefulServiceGroupCopyProcessing(
                        partitionIdString_,
                        reinterpret_cast<uintptr_t>(this),
                        replicaId_,
                        iterateAtomicGroup->first,
                        iterateAtomicGroup->second.createdSequenceNumber
                        );

                    for (Participant_ParticipantState_Iterator iterateAtomicGroupParticipant = iterateAtomicGroup->second.participants.begin(); 
                         iterateAtomicGroup->second.participants.end() != iterateAtomicGroupParticipant; 
                         iterateAtomicGroupParticipant++)
                    {
                        ServiceGroupReplicationEventSource::GetEvents().Info_10_StatefulServiceGroupCopyProcessing(
                            partitionIdString_,
                            reinterpret_cast<uintptr_t>(this),
                            replicaId_,
                            innerStatefulServiceUnits_[iterateAtomicGroupParticipant->first]->get_ServiceName(),
                            iterateAtomicGroupParticipant->second.createdSequenceNumber,
                            iterateAtomicGroup->first
                            );
                    }

                    if (sequenceAtomicGroupId_ < iterateAtomicGroup->first)
                    {
                        sequenceAtomicGroupId_ = iterateAtomicGroup->first;
                    }
                }

                ServiceGroupReplicationEventSource::GetEvents().Info_11_StatefulServiceGroupCopyProcessing(
                    partitionIdString_,
                    reinterpret_cast<uintptr_t>(this),
                    replicaId_,
                    sequenceAtomicGroupId_
                    );
                ServiceGroupReplicationEventSource::GetEvents().Info_12_StatefulServiceGroupCopyProcessing(
                    partitionIdString_,
                    reinterpret_cast<uintptr_t>(this),
                    replicaId_,
                    lastCommittedLSN_
                    );

                isAtomicGroupCopied_ = TRUE;

                ServiceGroupReplicationEventSource::GetEvents().Info_13_StatefulServiceGroupCopyProcessing(
                    partitionIdString_,
                    reinterpret_cast<uintptr_t>(this),
                    replicaId_
                    );

                //
                // Unblock replication processing
                //
                atomicGroupCopyDoneSignal_.Set();
            }
        }
        operation->Acknowledge();
        operation->Release();
    }
    else
    {
        Common::Guid partitionGuid(partitionId);
        PartitionId_StatefulServiceUnit_Iterator lookup = innerStatefulServiceUnits_.find(partitionGuid);
        ASSERT_IF(
            innerStatefulServiceUnits_.end() == lookup, 
            "Service group {0} unit should be available", 
            partitionGuid);
       
        hr = CompleteSingleCopyOperation(partitionGuid, operationMetadata->Type, lookup->second, operation);
        if (HR_STOP_OPERATION_PUMP == hr)
        {
            ServiceGroupReplicationEventSource::GetEvents().Error_4_StatefulServiceGroupCopyProcessing(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_,
                reinterpret_cast<uintptr_t>(operation),
                lookup->second->get_ServiceName()
                );

            ReportFault(partitionId_.AsGUID(), FABRIC_FAULT_TYPE_PERMANENT);
        }
    }
    return hr;
}

HRESULT CStatefulServiceGroup::CompleteSingleCopyOperation(
    __in const Common::Guid& serviceGroupUnitPartition,
    __in FABRIC_OPERATION_TYPE operationType,
    __in CStatefulServiceUnit* statefulServiceUnit,
    __in IFabricOperation* operation
    )
{
    CIncomingOperationExtendedBuffer* incomingOperationExtendedBuffer = new (std::nothrow) CIncomingOperationExtendedBuffer();
    if (NULL == incomingOperationExtendedBuffer)
    {
        operation->Release();
        return HR_STOP_OPERATION_PUMP;
    }

    HRESULT hr = incomingOperationExtendedBuffer->FinalConstruct(operationType, FABRIC_INVALID_ATOMIC_GROUP_ID, operation);
    operation->Release();
    if (FAILED(hr))
    {
        incomingOperationExtendedBuffer->Release();
        return HR_STOP_OPERATION_PUMP;
    }
    
    ServiceGroupReplicationEventSource::GetEvents().Info_14_StatefulServiceGroupCopyProcessing(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_,
        reinterpret_cast<uintptr_t>(incomingOperationExtendedBuffer),
        serviceGroupUnitPartition
        );

    hr = statefulServiceUnit->EnqueueCopyOperation(incomingOperationExtendedBuffer);
    if (FAILED(hr))
    {
        incomingOperationExtendedBuffer->Release();
        return HR_STOP_OPERATION_PUMP;
    }
    return S_OK;
}

HRESULT CStatefulServiceGroup::CompleteAtomicGroupReplicationOperation(
    __in const Common::Guid& serviceGroupUnitParticipantPartition,
    __in FABRIC_OPERATION_TYPE operationType,
    __in FABRIC_ATOMIC_GROUP_ID atomicGroupId,
    __in LONGLONG sequenceNumber,
    __in CStatefulServiceUnit* statefulServiceUnit,
    __in IFabricOperation* operation
    )
{
    HRESULT hr = S_OK;

    ServiceGroupReplicationEventSource::GetEvents().Info_0_StatefulServiceGroupAtomicGroupReplicationProcessing(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_,
        atomicGroupId
        );

    if (sequenceAtomicGroupId_ < atomicGroupId)
    {
        sequenceAtomicGroupId_ = atomicGroupId;
        
        ServiceGroupReplicationEventSource::GetEvents().Info_11_StatefulServiceGroupCopyProcessing(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            sequenceAtomicGroupId_
            );
    }

    if (FABRIC_OPERATION_TYPE_CREATE_ATOMIC_GROUP == operationType || FABRIC_OPERATION_TYPE_ATOMIC_GROUP_OPERATION == operationType)
    {
        HRESULT hrRefresh = RefreshAtomicGroup(
            operationType, 
            COPYING, 
            atomicGroupId, 
            serviceGroupUnitParticipantPartition,
            innerStatefulServiceUnits_[serviceGroupUnitParticipantPartition]->get_ServiceName()
            ); 
        if (FAILED(hrRefresh))
        {
            operation->Release();
            return HR_STOP_OPERATION_PUMP;
        }

        CIncomingOperationExtendedBuffer* atomicGroupOperation = new (std::nothrow) CIncomingOperationExtendedBuffer();
        if (NULL == atomicGroupOperation)
        {
            operation->Release();
            return HR_STOP_OPERATION_PUMP;
        }

        hr = atomicGroupOperation->FinalConstruct(operationType, atomicGroupId, operation);
        operation->Release();
        if (FAILED(hr))
        {
            atomicGroupOperation->Release();
            return HR_STOP_OPERATION_PUMP;
        }

        if (FABRIC_OPERATION_TYPE_CREATE_ATOMIC_GROUP == operationType)
        {
            SetAtomicGroupSequenceNumber(atomicGroupId, serviceGroupUnitParticipantPartition, sequenceNumber);

            ServiceGroupReplicationEventSource::GetEvents().Info_1_StatefulServiceGroupAtomicGroupReplicationProcessing(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_,
                atomicGroupId,
                reinterpret_cast<uintptr_t>(atomicGroupOperation),
                serviceGroupUnitParticipantPartition
                );
        }
        else
        {
            //
            // If this is not a create operation, we will have seen a create operation for this participant in this atomic group earlier.
            //
            ASSERT_IF(S_FALSE == hrRefresh, "Unexpected new atomic group or new participant");

            ServiceGroupReplicationEventSource::GetEvents().Info_2_StatefulServiceGroupAtomicGroupReplicationProcessing(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_,
                atomicGroupId,
                reinterpret_cast<uintptr_t>(atomicGroupOperation),
                serviceGroupUnitParticipantPartition
                );
        }
            
        hr = statefulServiceUnit->EnqueueReplicationOperation(atomicGroupOperation);
        if (FAILED(hr))
        {
            atomicGroupOperation->Release();
            return HR_STOP_OPERATION_PUMP;
        }
    }
    else if (FABRIC_OPERATION_TYPE_COMMIT_ATOMIC_GROUP == operationType || FABRIC_OPERATION_TYPE_ROLLBACK_ATOMIC_GROUP == operationType)
    {
        AtomicGroup_AtomicState_Iterator lookupAtomicGroupState = atomicGroups_.find(atomicGroupId);
        ASSERT_IF(atomicGroups_.end() == lookupAtomicGroupState, SERVICE_REPLICA_DOWN);

        CAtomicGroupCommitRollback* atomicGroupCommitRollback = new (std::nothrow) CAtomicGroupCommitRollback(FABRIC_OPERATION_TYPE_COMMIT_ATOMIC_GROUP == operationType);
        if (NULL == atomicGroupCommitRollback)
        {
            operation->Release();
            return HR_STOP_OPERATION_PUMP;
        }

        size_t count = lookupAtomicGroupState->second.participants.size();
        hr = atomicGroupCommitRollback->FinalConstruct(count, operationType, atomicGroupId, operation);
        operation->Release();
        if (FAILED(hr))
        {
            atomicGroupCommitRollback->Release();
            return HR_STOP_OPERATION_PUMP;
        }

        for (Participant_ParticipantState_Iterator iterateParticipants = lookupAtomicGroupState->second.participants.begin(); 
               lookupAtomicGroupState->second.participants.end() != iterateParticipants; 
               iterateParticipants++)
        {
            PartitionId_StatefulServiceUnit_Iterator lookupParticipant = innerStatefulServiceUnits_.find(iterateParticipants->first);
            ASSERT_IF(innerStatefulServiceUnits_.end() == lookupParticipant, "Unknown participant");
                
            CIncomingOperationExtendedBuffer* atomicGroupOperation = new (std::nothrow) CIncomingOperationExtendedBuffer();
            if (NULL == atomicGroupOperation)
            {
                atomicGroupCommitRollback->Release();
                return HR_STOP_OPERATION_PUMP;
            }

            hr = atomicGroupOperation->FinalConstruct(operationType, atomicGroupId, atomicGroupCommitRollback);
            if (FAILED(hr))
            {
                atomicGroupCommitRollback->Release();
                atomicGroupOperation->Release();
                return HR_STOP_OPERATION_PUMP;
            }

            if (FABRIC_OPERATION_TYPE_COMMIT_ATOMIC_GROUP == operationType)
            {
                ServiceGroupReplicationEventSource::GetEvents().Info_3_StatefulServiceGroupAtomicGroupReplicationProcessing(
                    partitionIdString_,
                    reinterpret_cast<uintptr_t>(this),
                    replicaId_,
                    atomicGroupId,
                    reinterpret_cast<uintptr_t>(atomicGroupOperation),
                    iterateParticipants->first
                    );
            }
            else
            {
                ServiceGroupReplicationEventSource::GetEvents().Info_4_StatefulServiceGroupAtomicGroupReplicationProcessing(
                    partitionIdString_,
                    reinterpret_cast<uintptr_t>(this),
                    replicaId_,
                    atomicGroupId,
                    reinterpret_cast<uintptr_t>(atomicGroupOperation),
                    iterateParticipants->first
                    );
            }

            hr = lookupParticipant->second->EnqueueReplicationOperation(atomicGroupOperation);
            if (FAILED(hr))
            {
                atomicGroupCommitRollback->Release();
                atomicGroupOperation->Release();
                return HR_STOP_OPERATION_PUMP;
            }
        }
        RemoveAtomicGroup(atomicGroupId, COPYING);
        atomicGroupCommitRollback->Release();
    }
    return hr;
}

HRESULT CStatefulServiceGroup::CompleteReplicationOperation(
    __in IFabricAsyncOperationContext* context
    )
{
    ASSERT_IF(NULL == context, "Unexpected context");

    IFabricOperation* operation = NULL;
    FABRIC_OPERATION_TYPE operationType = FABRIC_OPERATION_TYPE_INVALID;
    FABRIC_ATOMIC_GROUP_ID atomicGroupId = FABRIC_INVALID_ATOMIC_GROUP_ID;
    FABRIC_PARTITION_ID partitionId;
    PartitionId_StatefulServiceUnit_Iterator iterate;

    HRESULT hr = outerReplicationOperationStream_->EndGetOperation(context, &operation);
    if (FAILED(hr))
    {
        ServiceGroupReplicationEventSource::GetEvents().Error_3_StatefulServiceGroupReplicationProcessing(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            hr
            );

        ASSERT_IF(FAILED(hr), "Replication operation stream is broken");
        return hr;
    }

    //
    // Complete operation pump as soon as report fault was called.
    //
    Common::AcquireReadLock lock(lockFaultReplica_);
    if (isReplicaFaulted_)
    {
        if (NULL != operation)
        {
            operation->Release();
        }

        ServiceGroupReplicationEventSource::GetEvents().Warning_0_StatefulServiceGroupReplicationProcessing(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_
            );

        return HR_STOP_OPERATION_PUMP;
    }

    if (NULL == operation)
    {
        ServiceGroupReplicationEventSource::GetEvents().Info_3_StatefulServiceGroupReplicationProcessing(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_
            );

        for (iterate = innerStatefulServiceUnits_.begin(); innerStatefulServiceUnits_.end() != iterate; iterate++)
        {
            hr = iterate->second->EnqueueReplicationOperation(NULL);
            ASSERT_IF(FAILED(hr), "NULL replication operation processing failed");
        }

        ServiceGroupReplicationEventSource::GetEvents().Info_4_StatefulServiceGroupReplicationProcessing(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_
            );

        return HR_STOP_OPERATION_PUMP;
    }

    ASSERT_IF(FALSE == isAtomicGroupCopied_, "Atomic group state has not been seen");

    ULONG bufferCount = 0;
    FABRIC_OPERATION_DATA_BUFFER const * replicaBuffers = NULL;

    hr = operation->GetData(&bufferCount, &replicaBuffers);
    ASSERT_IF(FAILED(hr), "GetData");

    const FABRIC_OPERATION_METADATA* operationMetadata = operation->get_Metadata();
    ASSERT_IF(NULL == operationMetadata, "Unexpected operation metadata");

    ServiceGroupReplicationEventSource::GetEvents().Info_5_StatefulServiceGroupReplicationProcessing(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_,
        operationMetadata->SequenceNumber
        );

    ASSERT_IF(bufferCount < 1, "bufferCount");

    CExtendedOperationDataBuffer extendedReplicaBuffer;
    hr = CExtendedOperationDataBuffer::Read(extendedReplicaBuffer, replicaBuffers[0]);

    if (FAILED(hr))
    {
        ServiceGroupReplicationEventSource::GetEvents().Error_6_StatefulServiceGroupReplicationProcessing(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            hr
            );

        ReportFault(partitionId_.AsGUID(), FABRIC_FAULT_TYPE_PERMANENT);

        return HR_STOP_OPERATION_PUMP;
    }

    ASSERT_IF(
        (FABRIC_OPERATION_TYPE_NORMAL != extendedReplicaBuffer.OperationType) &&
        (FABRIC_OPERATION_TYPE_CREATE_ATOMIC_GROUP != extendedReplicaBuffer.OperationType) &&
        (FABRIC_OPERATION_TYPE_ATOMIC_GROUP_OPERATION != extendedReplicaBuffer.OperationType) &&
        (FABRIC_OPERATION_TYPE_COMMIT_ATOMIC_GROUP != extendedReplicaBuffer.OperationType) &&
        (FABRIC_OPERATION_TYPE_ROLLBACK_ATOMIC_GROUP != extendedReplicaBuffer.OperationType),
        "operationType");

    operationType = extendedReplicaBuffer.OperationType;
    atomicGroupId = extendedReplicaBuffer.AtomicGroupId;
    partitionId = extendedReplicaBuffer.PartitionId;

    if (operationMetadata->SequenceNumber <= lastCommittedLSN_)
    {
        ServiceGroupReplicationEventSource::GetEvents().Info_6_StatefulServiceGroupReplicationProcessing(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            operationMetadata->SequenceNumber
            );

        if (FABRIC_OPERATION_TYPE_COMMIT_ATOMIC_GROUP == operationType || FABRIC_OPERATION_TYPE_ROLLBACK_ATOMIC_GROUP == operationType)
        {
            AtomicGroup_AtomicState_Iterator lookupAtomicGroupState = atomicGroups_.find(atomicGroupId);
            if (atomicGroups_.end() != lookupAtomicGroupState)
            {
                ASSERT_IF(lookupAtomicGroupState->second.createdSequenceNumber >= operationMetadata->SequenceNumber, "Unexpected created sequence number");

                ServiceGroupReplicationEventSource::GetEvents().Info_7_StatefulServiceGroupReplicationProcessing(
                    partitionIdString_,
                    reinterpret_cast<uintptr_t>(this),
                    replicaId_,
                    atomicGroupId,
                    atomicGroups_.size() - 1
                    );

                atomicGroups_.erase(lookupAtomicGroupState);

                atomicGroupStateReplicatorCounters_->AtomicGroupsInFlight.Value = static_cast<Common::PerformanceCounterValue>(atomicGroups_.size());
            }
        }
        
        operation->Acknowledge();
        operation->Release();
        return S_OK;
    }
    else
    {
        UpdateLastCommittedLSN(FABRIC_INVALID_SEQUENCE_NUMBER);
    }

    if (IsServiceGroupRelatedOperation(partitionId))
    {
        ServiceGroupReplicationEventSource::GetEvents().Info_8_StatefulServiceGroupReplicationProcessing(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            reinterpret_cast<uintptr_t>(operation)
            );

        operation->Acknowledge();
        operation->Release();
    }
    else
    {
        Common::Guid partitionGuid(partitionId);
        PartitionId_StatefulServiceUnit_Iterator lookup = innerStatefulServiceUnits_.find(partitionGuid);
        ASSERT_IF(
            innerStatefulServiceUnits_.end() == lookup, 
            "Service group {0} unit should be available", 
            partitionGuid);

        if (FABRIC_OPERATION_TYPE_HAS_ATOMIC_GROUP_MASK & operationType)
        {
            ASSERT_IFNOT(
                lookup->second->HasInnerAtomicGroupStateProvider(), 
                "Service group {0} unit should support atomic groups", 
                partitionGuid);

            Common::AcquireWriteLock lockAtomicGroups(lockAtomicGroups_);
            hr = CompleteAtomicGroupReplicationOperation(
                partitionGuid, 
                operationType, 
                atomicGroupId, 
                operationMetadata->SequenceNumber,
                lookup->second, 
                operation
                );

            if (HR_STOP_OPERATION_PUMP == hr)
            {
                ServiceGroupReplicationEventSource::GetEvents().Error_4_StatefulServiceGroupReplicationProcessing(
                    partitionIdString_,
                    reinterpret_cast<uintptr_t>(this),
                    replicaId_,
                    atomicGroupId,
                    reinterpret_cast<uintptr_t>(operation),
                    lookup->second->get_ServiceName()
                    );

                ReportFault(partitionId_.AsGUID(), FABRIC_FAULT_TYPE_PERMANENT);
            }
        }
        else
        {
            operationMetadata = operation->get_Metadata();

            hr = CompleteSingleReplicationOperation(partitionGuid, operationMetadata->Type, lookup->second, operation);
            if (HR_STOP_OPERATION_PUMP == hr)
            {
                ServiceGroupReplicationEventSource::GetEvents().Error_5_StatefulServiceGroupReplicationProcessing(
                    partitionIdString_,
                    reinterpret_cast<uintptr_t>(this),
                    replicaId_,
                    reinterpret_cast<uintptr_t>(operation),
                    lookup->second->get_ServiceName()
                    );

                ReportFault(partitionId_.AsGUID(), FABRIC_FAULT_TYPE_PERMANENT);
            }
        }
    }
    return hr;
}

HRESULT CStatefulServiceGroup::CompleteSingleReplicationOperation(
    __in const Common::Guid& serviceGroupUnitPartition,
    __in FABRIC_OPERATION_TYPE operationType,
    __in CStatefulServiceUnit* statefulServiceUnit,
    __in IFabricOperation* operation
    )
{
    CIncomingOperationExtendedBuffer* incomingOperationExtendedBuffer = new (std::nothrow) CIncomingOperationExtendedBuffer();
    if (NULL == incomingOperationExtendedBuffer)
    {
        operation->Release();
        return HR_STOP_OPERATION_PUMP;
    }

    HRESULT hr = incomingOperationExtendedBuffer->FinalConstruct(operationType, FABRIC_INVALID_ATOMIC_GROUP_ID, operation);
    operation->Release();
    if (FAILED(hr))
    {
        incomingOperationExtendedBuffer->Release();
        return HR_STOP_OPERATION_PUMP;
    }
    
    ServiceGroupReplicationEventSource::GetEvents().Info_9_StatefulServiceGroupReplicationProcessing(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_,
        reinterpret_cast<uintptr_t>(incomingOperationExtendedBuffer),
        serviceGroupUnitPartition
        );

    hr = statefulServiceUnit->EnqueueReplicationOperation(incomingOperationExtendedBuffer);
    if (FAILED(hr))
    {
        incomingOperationExtendedBuffer->Release();
        return HR_STOP_OPERATION_PUMP;
    }
    return S_OK;
}

//
// IFabricStateProvider methods.
//
STDMETHODIMP CStatefulServiceGroup::BeginUpdateEpoch(
    __in FABRIC_EPOCH const* epoch, 
    __in FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber,
    __in IFabricAsyncOperationCallback* callback,
    __out IFabricAsyncOperationContext** context
    )
{
    if (epoch == NULL || NULL == callback || NULL == context)
    {
        return E_POINTER;
    }
    if (epoch->Reserved != NULL)
    {
        return E_INVALIDARG; 
    }

    return DoUpdateEpoch(*epoch, previousEpochLastSequenceNumber, callback, context);
}

STDMETHODIMP CStatefulServiceGroup::EndUpdateEpoch(
    __in IFabricAsyncOperationContext* context
    )
{
    if (NULL == context)
    {
        return E_POINTER;
    }

    ServiceGroupStatefulEventSource::GetEvents().Info_5_StatefulServiceGroupUpdateEpoch(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_
        );

    CStatefulCompositeRollbackUpdateEpochOperationAsync* parentOperation = (CStatefulCompositeRollbackUpdateEpochOperationAsync*)context;
    HRESULT hr = parentOperation->End();
    
    if (FAILED(hr))
    {
        parentOperation->Release();

        ServiceGroupStatefulEventSource::GetEvents().Error_5_StatefulServiceGroupUpdateEpoch(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            hr
            );

        return hr;
    }

    FABRIC_EPOCH const & epoch = parentOperation->get_Epoch();
    FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber = parentOperation->get_PreviousEpochLastSequenceNumber();

    ServiceGroupStatefulEventSource::GetEvents().Info_6_StatefulServiceGroupUpdateEpoch(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_,
        currentEpoch_.DataLossNumber,
        currentEpoch_.ConfigurationNumber,
        previousEpochLsn_,
        epoch.DataLossNumber,
        epoch.ConfigurationNumber,
        previousEpochLastSequenceNumber
        );

    currentEpoch_.DataLossNumber = epoch.DataLossNumber;
    currentEpoch_.ConfigurationNumber = epoch.ConfigurationNumber;
    previousEpochLsn_ = previousEpochLastSequenceNumber;

    parentOperation->Release();

    return S_OK;
}

STDMETHODIMP CStatefulServiceGroup::GetLastCommittedSequenceNumber(__out FABRIC_SEQUENCE_NUMBER* sequenceNumber)
{
    if (NULL == sequenceNumber)
    {
        return E_POINTER;
    }

    HRESULT hr = E_FAIL;
    FABRIC_SEQUENCE_NUMBER innerSequenceNumber = _I64_MIN;
    FABRIC_SEQUENCE_NUMBER maxSequenceNumber = _I64_MIN;

    ServiceGroupStatefulEventSource::GetEvents().Info_7_StatefulServiceGroup(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_
        );

    for (PartitionId_StatefulServiceUnit_Iterator iterate = innerStatefulServiceUnits_.begin(); 
         innerStatefulServiceUnits_.end() != iterate; 
         iterate++)
    {
        hr = iterate->second->GetLastCommittedSequenceNumber(&innerSequenceNumber);
        if (FAILED(hr))
        {
            ServiceGroupStatefulEventSource::GetEvents().Error_19_StatefulServiceGroup(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_,
                hr
                );

            break;
        }
        if (maxSequenceNumber < innerSequenceNumber)
        {
            maxSequenceNumber = innerSequenceNumber;
        }
    }
    if (SUCCEEDED(hr))
    {
        ServiceGroupStatefulEventSource::GetEvents().Info_8_StatefulServiceGroup(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            maxSequenceNumber
            );

        *sequenceNumber = maxSequenceNumber;
    }
    return hr;
}

STDMETHODIMP CStatefulServiceGroup::BeginOnDataLoss(
    __in IFabricAsyncOperationCallback* callback,
    __out IFabricAsyncOperationContext** context
    )
{
    if (NULL == callback || NULL == context)
    {
        return E_POINTER;
    }
    return DoOnDataLoss(callback, context);
}

STDMETHODIMP CStatefulServiceGroup::EndOnDataLoss(
    __in IFabricAsyncOperationContext* context,
    __out BOOLEAN* isStateChanged
    )
{
    if (NULL == context || NULL == isStateChanged)
    {
        return E_POINTER;
    }

    ServiceGroupStatefulEventSource::GetEvents().Info_2_StatefulServiceGroupOnDataLoss(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_
        );

    CStatefulCompositeOnDataLossUndoAsyncOperationAsync* parentOperation = (CStatefulCompositeOnDataLossUndoAsyncOperationAsync*)context;
    HRESULT hr = parentOperation->End();
    if (FAILED(hr))
    {
        ServiceGroupStatefulEventSource::GetEvents().Error_2_StatefulServiceGroupOnDataLoss(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            hr
            );

        return hr;
    }

    hr = parentOperation->get_IsStateChanged(isStateChanged);
    parentOperation->Release();
    if (FAILED(hr))
    {
        ServiceGroupStatefulEventSource::GetEvents().Error_2_StatefulServiceGroupOnDataLoss(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            hr
            );

        return hr;
    }

    ServiceGroupStatefulEventSource::GetEvents().Info_3_StatefulServiceGroupOnDataLoss(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_
        );
    ServiceGroupStatefulEventSource::GetEvents().Info_4_StatefulServiceGroupOnDataLoss(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_,
        *isStateChanged
        );

    return hr;
}

STDMETHODIMP CStatefulServiceGroup::GetCopyContext(__out IFabricOperationDataStream** copyContextEnumerator)
{
    if (NULL == copyContextEnumerator)
    {
        return E_POINTER;
    }
    ASSERT_IF(0 == hasPersistedState_, "Unexpected call");
    return DoGetCopyContext(copyContextEnumerator);
}

STDMETHODIMP CStatefulServiceGroup::GetCopyState(
    __in FABRIC_SEQUENCE_NUMBER uptoSequenceNumber,
    __in IFabricOperationDataStream* copyContextEnumerator,
    __out IFabricOperationDataStream** copyStateEnumerator
    )
{
    if (NULL == copyStateEnumerator)
    {
        return E_POINTER;
    }
    return DoGetCopyState(uptoSequenceNumber, copyContextEnumerator, copyStateEnumerator);
}

//
// IFabricAtomicGroupStateReplicator methods.
//
STDMETHODIMP CStatefulServiceGroup::CreateAtomicGroup(__out FABRIC_ATOMIC_GROUP_ID* atomicGroupId)
{
    if (NULL == atomicGroupId)
    {
        return E_POINTER;
    }
    *atomicGroupId = ::InterlockedIncrement64(&sequenceAtomicGroupId_);

    WriteNoise(
        TraceSubComponentStatefulServiceGroupAtomicGroupReplicationProcessing, 
        partitionIdString_,
        "this={0} replicaId={1} - Created atomic group {2}",
        this,
        replicaId_,
        *atomicGroupId
        );

    return S_OK;
}

STDMETHODIMP CStatefulServiceGroup::BeginReplicateAtomicGroupOperation(
    __in FABRIC_ATOMIC_GROUP_ID atomicGroupId,
    __in IFabricOperationData* operationData,
    __in IFabricAsyncOperationCallback* callback,
    __out FABRIC_SEQUENCE_NUMBER* operationSequenceNumber,
    __out IFabricAsyncOperationContext** context
    )
{
    COutgoingOperationDataExtendedBuffer* outgoingOperationData = (COutgoingOperationDataExtendedBuffer*)operationData;

    FABRIC_OPERATION_TYPE operationType = outgoingOperationData->GetOperationType();
    ASSERT_IF(FABRIC_OPERATION_TYPE_CREATE_ATOMIC_GROUP == operationType, "CStatefulServiceGroup::BeginReplicateAtomicGroupOperation - Invalid FABRIC_OPERATION_TYPE");

    FABRIC_ATOMIC_GROUP_ID operationAtomicGroupId = outgoingOperationData->GetAtomicGroupId();
    ASSERT_IF(0 > operationAtomicGroupId, "CStatefulServiceGroup::BeginReplicateAtomicGroupOperation - Invalid FABRIC_ATOMIC_GROUP_ID");
    ASSERT_IFNOT(atomicGroupId == operationAtomicGroupId, "CStatefulServiceGroup::BeginReplicateAtomicGroupOperation - Unexpected FABRIC_ATOMIC_GROUP_ID");

    Common::Guid participant(outgoingOperationData->GetPartitionId());

    IFabricAsyncOperationContext* systemContext = NULL;
    CReplicationAtomicGroupOperationCallback* internalCallback = NULL;
    CAtomicGroupAsyncOperationContext* internalContext = NULL;

    HRESULT hr = S_OK;

    CStatefulServiceUnit* participantUnit = NULL;

    {
        Common::AcquireReadLock lockReplicator(lock_);

        if (outerStateReplicatorDestructed_)
        {
            ServiceGroupReplicationEventSource::GetEvents().Error_0_StatefulServiceGroupReplicationProcessing(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_
                );

            return Common::ErrorCodeValue::ObjectClosed;
        }

        PartitionId_StatefulServiceUnit_Iterator lookup = innerStatefulServiceUnits_.find(participant);
        ASSERT_IF(innerStatefulServiceUnits_.end() == lookup, "CStatefulServiceGroup::BeginReplicateAtomicGroupOperation - Unknown FABRIC_PARTITION_ID");

        ServiceGroupReplicationEventSource::GetEvents().Info_5_StatefulServiceGroupAtomicGroupReplicationProcessing(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            (LONG)operationType,
            operationAtomicGroupId,
            innerStatefulServiceUnits_[participant]->get_ServiceName()
            );

        internalCallback = new (std::nothrow) CReplicationAtomicGroupOperationCallback();
        if (NULL == internalCallback)
        {
            return E_OUTOFMEMORY;
        }

        internalContext = new (std::nothrow) CAtomicGroupAsyncOperationContext();
        if (NULL == internalContext)
        {
            internalCallback->Release();
            return E_OUTOFMEMORY;
        }

        hr = internalCallback->FinalConstruct(callback, internalContext);
        ASSERT_IF(FAILED(hr), "CReplicationAtomicGroupOperationCallback::FinalConstruct");

        hr = internalContext->FinalConstruct(operationType, operationAtomicGroupId, participant);
        ASSERT_IF(FAILED(hr), "CAtomicGroupAsyncOperationContext::FinalConstruct");

        outerStateReplicator_->AddRef();
        outerStatefulServicePartition_->AddRef();

        participantUnit = lookup->second;
        participantUnit->AddRef();
    }

    FABRIC_SERVICE_PARTITION_ACCESS_STATUS writeStatus;
    hr = outerStatefulServicePartition_->GetWriteStatus(&writeStatus);
    outerStatefulServicePartition_->Release();
    if (FAILED(hr) || (FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED != writeStatus))
    {
        ServiceGroupReplicationEventSource::GetEvents().Warning_0_StatefulServiceGroupAtomicGroupReplicationProcessing(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_
            );

        internalCallback->Release();
        internalContext->Release();
        outerStateReplicator_->Release();
        participantUnit->Release();

        if (FAILED(hr))
        {
            return hr;
        }
        if (FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY == writeStatus)
        {
            return FABRIC_E_NOT_PRIMARY; 
        }
        if (FABRIC_SERVICE_PARTITION_ACCESS_STATUS_RECONFIGURATION_PENDING == writeStatus)
        {
            return FABRIC_E_RECONFIGURATION_PENDING; 
        }
        if (FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NO_WRITE_QUORUM == writeStatus)
        {
            return FABRIC_E_NO_WRITE_QUORUM; 
        }
        Common::Assert::CodingError("GetWriteStatus");
    }

    Common::AcquireReadLock lockAtomicGroupReplication(lockAtomicGroupReplication_);

    //
    // The replica may have changed role in the meantime in which case the atomicGroupReplicationDoneSignal_ may be closed.
    // The replica is then no longer a primary. This needs to checked under the atomic group replication lock
    //
    if (NULL == atomicGroupReplicationDoneSignal_)
    {
        ServiceGroupReplicationEventSource::GetEvents().Warning_0_StatefulServiceGroupAtomicGroupReplicationProcessing(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_
            );

        internalCallback->Release();
        internalContext->Release();
        outerStateReplicator_->Release();
        participantUnit->Release();

        return FABRIC_E_NOT_PRIMARY;
    }

    Common::AcquireWriteLock lockAtomicGroups(lockAtomicGroups_);

    hr = RefreshAtomicGroup(operationType, REPLICATING, operationAtomicGroupId, participant, participantUnit->get_ServiceName());
    if (FAILED(hr))
    {
        ServiceGroupReplicationEventSource::GetEvents().Error_0_StatefulServiceGroupAtomicGroupReplicationProcessing(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            hr
            );

        internalCallback->Release();
        internalContext->Release();
        outerStateReplicator_->Release();
        participantUnit->Release();

        return hr;
    }
    if (S_FALSE == hr)
    {
        operationType = FABRIC_OPERATION_TYPE_CREATE_ATOMIC_GROUP;
        outgoingOperationData->SetOperationType(operationType);
    }

    hr = outgoingOperationData->SerializeExtendedOperationDataBuffer();
    if (FAILED(hr))
    {
        ServiceGroupReplicationEventSource::GetEvents().Error_1_StatefulServiceGroupAtomicGroupReplicationProcessing(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            hr
            );

        internalCallback->Release();
        internalContext->Release();
        outerStateReplicator_->Release();
        participantUnit->Release();

        return hr;
    }
    
    hr = outerStateReplicator_->BeginReplicate(operationData, internalCallback, internalContext->get_SequenceNumber(), &systemContext);
    outerStateReplicator_->Release();
    if (SUCCEEDED(hr))
    {
        *operationSequenceNumber = *internalContext->get_SequenceNumber();
        *context = internalContext;
        internalContext->set_SystemContext(systemContext);

        if (systemContext->CompletedSynchronously())
        {
            WriteNoise(TraceSubComponentStatefulServiceGroupAtomicGroupReplicationProcessing, "this={0} - context {1} completed synchronously", this, systemContext);

            if (NULL != callback)
            {
                callback->Invoke(internalContext);
            }
        }
        else
        {
            WriteNoise(TraceSubComponentStatefulServiceGroupAtomicGroupReplicationProcessing, "this={0} - context {1} completed asynchronously", this, systemContext);
        }

        internalCallback->Release();

        ServiceGroupReplicationEventSource::GetEvents().Info_6_StatefulServiceGroupAtomicGroupReplicationProcessing(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            (LONG)operationType,
            operationAtomicGroupId,
            innerStatefulServiceUnits_[participant]->get_ServiceName(),
            *operationSequenceNumber
            );

        if (0 == *operationSequenceNumber)
        {
            ErrorFixupAtomicGroup(operationAtomicGroupId, participant, participantUnit->get_ServiceName());
        }
        else if (FABRIC_OPERATION_TYPE_CREATE_ATOMIC_GROUP == operationType)
        {
            SetAtomicGroupSequenceNumber(operationAtomicGroupId, participant, *operationSequenceNumber);
        }
    }
    else
    {
        ServiceGroupReplicationEventSource::GetEvents().Error_2_StatefulServiceGroupAtomicGroupReplicationProcessing(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            (LONG)operationType,
            operationAtomicGroupId,
            innerStatefulServiceUnits_[participant]->get_ServiceName(),
            hr
            );

        ErrorFixupAtomicGroup(operationAtomicGroupId, participant, participantUnit->get_ServiceName());
   
        internalCallback->Release();
        internalContext->Release();
    }

    participantUnit->Release();

    return hr;
}

STDMETHODIMP CStatefulServiceGroup::EndReplicateAtomicGroupOperation(__in IFabricAsyncOperationContext* context, __out FABRIC_SEQUENCE_NUMBER* outOperationSequenceNumber)
{
    IFabricAsyncOperationContext* systemContext = NULL;
    CAtomicGroupAsyncOperationContext* operationAsyncContext = (CAtomicGroupAsyncOperationContext*)context;
    HRESULT hr = operationAsyncContext->get_SystemContext(&systemContext);
    ASSERT_IF(FAILED(hr), "CAtomicGroupAsyncOperationContext::get_SystemContext");
    ASSERT_IF(NULL == systemContext, "Unexpected system context");

    FABRIC_OPERATION_TYPE operationType = operationAsyncContext->get_OperationType();
    ASSERT_IF(
        FABRIC_OPERATION_TYPE_COMMIT_ATOMIC_GROUP == operationType || FABRIC_OPERATION_TYPE_ROLLBACK_ATOMIC_GROUP == operationType, 
        "FABRIC_OPERATION_TYPE");

    FABRIC_ATOMIC_GROUP_ID atomicGroupId = operationAsyncContext->get_AtomicGroupId();
    Common::Guid participant = operationAsyncContext->get_Participant();
    FABRIC_SEQUENCE_NUMBER operationSequenceNumber = *operationAsyncContext->get_SequenceNumber();

    // TODO: Remove sequence number logic above and use the new out param

    FABRIC_SEQUENCE_NUMBER sequenceNumberOuter;
    
    hr = EndReplicate(systemContext, &sequenceNumberOuter);
    systemContext->Release();

    CStatefulServiceUnit* participantUnit = NULL;
    HRESULT hrLookup = GetStatefulServiceUnit(participant, &participantUnit);

    if (SUCCEEDED(hr))
    {
        ASSERT_IF(FAILED(hrLookup), "Service closed or aborted with pending atomic group replication.");

        if (outOperationSequenceNumber != NULL)
        {
            *outOperationSequenceNumber = sequenceNumberOuter;
        }

        ASSERT_IF(0 == operationSequenceNumber, "Unexpected operation sequence number");
        ASSERT_IFNOT(sequenceNumberOuter == operationSequenceNumber, "Sequence numbers don't match");

        ServiceGroupReplicationEventSource::GetEvents().Info_7_StatefulServiceGroupAtomicGroupReplicationProcessing(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            atomicGroupId,
            participantUnit->get_ServiceName(),
            operationSequenceNumber
            );

        if (FABRIC_OPERATION_TYPE_COMMIT_ATOMIC_GROUP != operationType && FABRIC_OPERATION_TYPE_ROLLBACK_ATOMIC_GROUP != operationType)
        {
            Common::AcquireWriteLock lockAtomicGroups(lockAtomicGroups_);
            hr = RefreshAtomicGroup(operationType, REPLICATED, atomicGroupId, participant, participantUnit->get_ServiceName());
            ASSERT_IF(FAILED(hr), "RefreshAtomicGroup");
        }
    }
    else
    {
        ServiceGroupReplicationEventSource::GetEvents().Error_3_StatefulServiceGroupAtomicGroupReplicationProcessing(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            atomicGroupId,
            SUCCEEDED(hrLookup) ? participantUnit->get_ServiceName() : UNKNOWN_SERVICE_UNIT_REPLICA_CLOSED,
            operationSequenceNumber,
            hr
            );

        if (FABRIC_OPERATION_TYPE_COMMIT_ATOMIC_GROUP != operationType && FABRIC_OPERATION_TYPE_ROLLBACK_ATOMIC_GROUP != operationType)
        {
            if (0 != operationSequenceNumber)
            {
                ASSERT_IF(FAILED(hrLookup), "Service closed or aborted with pending atomic group replication.");

                Common::AcquireWriteLock lockAtomicGroups(lockAtomicGroups_);
                ErrorFixupAtomicGroup(atomicGroupId, participant, participantUnit->get_ServiceName());
            }
        }
    }

    if (SUCCEEDED(hrLookup))
    {
        participantUnit->Release();
    }

    return hr;
}

STDMETHODIMP CStatefulServiceGroup::BeginReplicateAtomicGroupCommit(
    __in FABRIC_ATOMIC_GROUP_ID atomicGroupId,
    __in IFabricAsyncOperationCallback* callback,
    __out FABRIC_SEQUENCE_NUMBER* commitSequenceNumber,
    __out IFabricAsyncOperationContext** context
    )
{
    UNREFERENCED_PARAMETER(atomicGroupId);
    UNREFERENCED_PARAMETER(callback);
    UNREFERENCED_PARAMETER(commitSequenceNumber);
    UNREFERENCED_PARAMETER(context);

    Common::Assert::CodingError("Unexpected call");
}

STDMETHODIMP CStatefulServiceGroup::EndReplicateAtomicGroupCommit(__in IFabricAsyncOperationContext* context, __out FABRIC_SEQUENCE_NUMBER* commitSequenceNumber)
{
    IFabricAsyncOperationContext* systemContext = NULL;
    CAtomicGroupAsyncOperationContext* operationAsyncContext = (CAtomicGroupAsyncOperationContext*)context;
    HRESULT hr = operationAsyncContext->get_SystemContext(&systemContext);
    ASSERT_IF(FAILED(hr), "CAtomicGroupAsyncOperationContext::get_SystemContext");
    ASSERT_IF(NULL == systemContext, "Unexpected system context");

    FABRIC_OPERATION_TYPE operationType = operationAsyncContext->get_OperationType();
    ASSERT_IFNOT(FABRIC_OPERATION_TYPE_COMMIT_ATOMIC_GROUP == operationType, "FABRIC_OPERATION_TYPE");
    
    FABRIC_ATOMIC_GROUP_ID atomicGroupId = operationAsyncContext->get_AtomicGroupId();
    Common::Guid participant = operationAsyncContext->get_Participant();
    FABRIC_SEQUENCE_NUMBER operationSequenceNumber = *operationAsyncContext->get_SequenceNumber();

    FABRIC_SEQUENCE_NUMBER sequenceNumberOuter;
    
    hr = EndReplicate(systemContext, &sequenceNumberOuter);
    systemContext->Release();
    if (SUCCEEDED(hr))
    {
        if (commitSequenceNumber != NULL)
        {
            *commitSequenceNumber = sequenceNumberOuter;
        }

        ASSERT_IF(0 == operationSequenceNumber, "Unexpected operation sequence number");
        ASSERT_IFNOT(sequenceNumberOuter == operationSequenceNumber, "Sequence numbers don't match");

        ServiceGroupReplicationEventSource::GetEvents().Info_8_StatefulServiceGroupAtomicGroupReplicationProcessing(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            atomicGroupId,
            innerStatefulServiceUnits_[participant]->get_ServiceName(),
            operationSequenceNumber
            );

        {
            Common::AcquireWriteLock lockAtomicGroups(lockAtomicGroups_);
            UpdateLastCommittedLSN(operationSequenceNumber);
        }
        {
            hr = DoAtomicGroupCommitRollback(TRUE, atomicGroupId, operationSequenceNumber);
            ASSERT_IF(FAILED(hr), SERVICE_REPLICA_DOWN);
        }
    }
    else
    {
        CStatefulServiceUnit* participantUnit = NULL;
        HRESULT hrLookup = GetStatefulServiceUnit(participant, &participantUnit);

        ServiceGroupReplicationEventSource::GetEvents().Error_4_StatefulServiceGroupAtomicGroupReplicationProcessing(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            atomicGroupId,
            SUCCEEDED(hrLookup) ? participantUnit->get_ServiceName() : UNKNOWN_SERVICE_UNIT_REPLICA_CLOSED,
            operationSequenceNumber,
            hr
            );

        if (0 != operationSequenceNumber)
        {
            ASSERT_IF(FAILED(hrLookup), "Service closed or aborted with pending atomic group replication.");

            Common::AcquireWriteLock lockAtomicGroups(lockAtomicGroups_);
            ErrorFixupAtomicGroup(atomicGroupId, participant, participantUnit->get_ServiceName());
        }

        if (SUCCEEDED(hrLookup))
        {
            participantUnit->Release();
        }
    }
    return hr;
}

STDMETHODIMP CStatefulServiceGroup::BeginReplicateAtomicGroupRollback(
    __in FABRIC_ATOMIC_GROUP_ID atomicGroupId,
    __in IFabricAsyncOperationCallback* callback,
    __out FABRIC_SEQUENCE_NUMBER* rollbackSequenceNumber,
    __out IFabricAsyncOperationContext** context
    )
{
    UNREFERENCED_PARAMETER(atomicGroupId);
    UNREFERENCED_PARAMETER(callback);
    UNREFERENCED_PARAMETER(rollbackSequenceNumber);
    UNREFERENCED_PARAMETER(context);

    Common::Assert::CodingError("Unexpected call");
}

STDMETHODIMP CStatefulServiceGroup::EndReplicateAtomicGroupRollback(__in IFabricAsyncOperationContext* context, __out FABRIC_SEQUENCE_NUMBER* rollbackSequenceNumber)
{
    IFabricAsyncOperationContext* systemContext = NULL;
    CAtomicGroupAsyncOperationContext* operationAsyncContext = (CAtomicGroupAsyncOperationContext*)context;
    HRESULT hr = operationAsyncContext->get_SystemContext(&systemContext);
    ASSERT_IF(FAILED(hr), "CAtomicGroupAsyncOperationContext::get_SystemContext");
    ASSERT_IF(NULL == systemContext, "Unexpected system context");

    FABRIC_OPERATION_TYPE operationType = operationAsyncContext->get_OperationType();
    ASSERT_IFNOT(FABRIC_OPERATION_TYPE_ROLLBACK_ATOMIC_GROUP == operationType, "FABRIC_OPERATION_TYPE");
    
    FABRIC_ATOMIC_GROUP_ID atomicGroupId = operationAsyncContext->get_AtomicGroupId();
    Common::Guid participant = operationAsyncContext->get_Participant();
    FABRIC_SEQUENCE_NUMBER operationSequenceNumber = *operationAsyncContext->get_SequenceNumber();

    FABRIC_SEQUENCE_NUMBER sequenceNumberOuter;
    
    hr = EndReplicate(systemContext, &sequenceNumberOuter);
    systemContext->Release();
    if (SUCCEEDED(hr))
    {
        if (rollbackSequenceNumber != NULL)
        {
            *rollbackSequenceNumber = sequenceNumberOuter;
        }

        ASSERT_IF(0 == operationSequenceNumber, "Unexpected operation sequence number");
        ASSERT_IFNOT(sequenceNumberOuter == operationSequenceNumber, "Sequence numbers don't match");

        ServiceGroupReplicationEventSource::GetEvents().Info_9_StatefulServiceGroupAtomicGroupReplicationProcessing(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            atomicGroupId,
            innerStatefulServiceUnits_[participant]->get_ServiceName(),
            operationSequenceNumber
            );

        {
            Common::AcquireWriteLock lockAtomicGroups(lockAtomicGroups_);
            UpdateLastCommittedLSN(operationSequenceNumber);
        }
        {
            hr = DoAtomicGroupCommitRollback(FALSE, atomicGroupId, operationSequenceNumber);
            ASSERT_IF(FAILED(hr), SERVICE_REPLICA_DOWN);
        }
    }
    else
    {
        CStatefulServiceUnit* participantUnit = NULL;
        HRESULT hrLookup = GetStatefulServiceUnit(participant, &participantUnit);
        
        ServiceGroupReplicationEventSource::GetEvents().Error_5_StatefulServiceGroupAtomicGroupReplicationProcessing(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            atomicGroupId,
            SUCCEEDED(hrLookup) ? participantUnit->get_ServiceName() : UNKNOWN_SERVICE_UNIT_REPLICA_CLOSED,
            operationSequenceNumber,
            hr
            );

        if (0 != operationSequenceNumber)
        {
            ASSERT_IF(FAILED(hrLookup), "Service closed or aborted with pending atomic group replication.");

            Common::AcquireWriteLock lockAtomicGroups(lockAtomicGroups_);
            ErrorFixupAtomicGroup(atomicGroupId, participant, participantUnit->get_ServiceName());
        }

        if (SUCCEEDED(hrLookup))
        {
            participantUnit->Release();
        }
    }
    return hr;
}

//
// Helper routines.
//
HRESULT CStatefulServiceGroup::CreateAtomicGroupCommitRollbackOperation(
    __in BOOLEAN isCommit, 
    __in FABRIC_ATOMIC_GROUP_ID atomicGroupId, 
    __in FABRIC_SEQUENCE_NUMBER sequenceNumber,
    __in_opt CBaseAsyncOperation* parent,
    __out CCompositeAsyncOperationAsync** operation)
{
    // this function needs to be called under lockAtomicGroups_

    ASSERT_IF(NULL == operation, "Unexpected operation");

    HRESULT hr = S_OK;
    
    CCompositeAsyncOperationAsync* mCommitRollbackAsyncOperation = mCommitRollbackAsyncOperation = new (std::nothrow) CCompositeAsyncOperationAsync(parent, partitionId_.AsGUID());
    if (NULL == mCommitRollbackAsyncOperation)
    {
        return E_OUTOFMEMORY;
    }
    hr = mCommitRollbackAsyncOperation->FinalConstruct();
    if (FAILED(hr))
    {
        mCommitRollbackAsyncOperation->Release();
        return hr;
    }

    AtomicGroup_AtomicState_Iterator lookupAtomicGroupState = atomicGroups_.find(atomicGroupId);
    ASSERT_IF(atomicGroups_.end() == lookupAtomicGroupState, "Atomic group does not exist");

    if (AtomicGroupStatus::IN_FLIGHT != lookupAtomicGroupState->second.status)
    {
        ASSERT_IF(1 < lookupAtomicGroupState->second.replicating, "Unexpected atomic group replicating");
    }

    if (isCommit)
    {
        ServiceGroupReplicationEventSource::GetEvents().Info_10_StatefulServiceGroupAtomicGroupReplicationProcessing(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            atomicGroupId,
            sequenceNumber,
            ATOMIC_GROUP_STATUS_TEXT(lookupAtomicGroupState->second.status)
            );
    }
    else
    {
        ServiceGroupReplicationEventSource::GetEvents().Info_11_StatefulServiceGroupAtomicGroupReplicationProcessing(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            atomicGroupId,
            sequenceNumber,
            ATOMIC_GROUP_STATUS_TEXT(lookupAtomicGroupState->second.status)
            );
    }

    for (Participant_ParticipantState_Iterator iterateParticipants = lookupAtomicGroupState->second.participants.begin(); 
        lookupAtomicGroupState->second.participants.end() != iterateParticipants; 
        iterateParticipants++)
    {
        PartitionId_StatefulServiceUnit_Iterator lookupParticipant = innerStatefulServiceUnits_.find(iterateParticipants->first);
        ASSERT_IF(innerStatefulServiceUnits_.end() == lookupParticipant, "Unknown participant");

        Participant_ParticipantState_Iterator lookupParticipantState = lookupAtomicGroupState->second.participants.find(iterateParticipants->first);
        ASSERT_IF(lookupAtomicGroupState->second.participants.end() == lookupParticipantState, "Unknown participant state");

        if (AtomicGroupStatus::IN_FLIGHT != lookupAtomicGroupState->second.status)
        {
            ASSERT_IF(1 < lookupParticipantState->second.replicating, "Unexpected participant replicating");
        }

        CAtomicGroupCommitRollbackOperationAsync* sCommitRollbackAsyncOperation = new (std::nothrow) CAtomicGroupCommitRollbackOperationAsync(
                mCommitRollbackAsyncOperation,
                (IFabricAtomicGroupStateProvider*)lookupParticipant->second,
                isCommit,
                atomicGroupId,
                sequenceNumber
                );
        if (NULL == sCommitRollbackAsyncOperation)
        {
            hr = E_OUTOFMEMORY;
            break;
        }
        hr = sCommitRollbackAsyncOperation->FinalConstruct();
        if (FAILED(hr))
        {
            sCommitRollbackAsyncOperation->Release();
            break;
        }
        hr = mCommitRollbackAsyncOperation->Compose(sCommitRollbackAsyncOperation);
        if (FAILED(hr))
        {
            sCommitRollbackAsyncOperation->Release();
            break;
        }
    }

    if (FAILED(hr))
    {
        mCommitRollbackAsyncOperation->Release();
        return hr;
    }

    *operation = mCommitRollbackAsyncOperation;
    return hr;
}

HRESULT CStatefulServiceGroup::DoAtomicGroupCommitRollback(
    __in BOOLEAN isCommit, 
    __in FABRIC_ATOMIC_GROUP_ID atomicGroupId, 
    __in FABRIC_SEQUENCE_NUMBER sequenceNumber
    )
{
    HRESULT hr = S_OK;
    CCompositeAsyncOperation* mCommitRollbackAsyncOperation = new (std::nothrow) CCompositeAtomicGroupCommitRollbackOperationAsync(this, partitionId_.AsGUID(), atomicGroupId, isCommit);
    if (NULL == mCommitRollbackAsyncOperation)
    {
        return E_OUTOFMEMORY;
    }
    hr = mCommitRollbackAsyncOperation->FinalConstruct();
    if (FAILED(hr))
    {
        mCommitRollbackAsyncOperation->Release();
        return hr;
    }

    {
        Common::AcquireReadLock lockAtomicGroups(lockAtomicGroups_);
    
        AtomicGroup_AtomicState_Iterator lookupAtomicGroupState = atomicGroups_.find(atomicGroupId);
        ASSERT_IF(atomicGroups_.end() == lookupAtomicGroupState, "Atomic group does not exist");
    
        if (AtomicGroupStatus::IN_FLIGHT != lookupAtomicGroupState->second.status)
        {
            ASSERT_IF(1 < lookupAtomicGroupState->second.replicating, "Unexpected atomic group replicating");
        }
    
        if (isCommit)
        {
            ServiceGroupReplicationEventSource::GetEvents().Info_10_StatefulServiceGroupAtomicGroupReplicationProcessing(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_,
                atomicGroupId,
                sequenceNumber,
                ATOMIC_GROUP_STATUS_TEXT(lookupAtomicGroupState->second.status)
                );
        }
        else
        {
            ServiceGroupReplicationEventSource::GetEvents().Info_11_StatefulServiceGroupAtomicGroupReplicationProcessing(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_,
                atomicGroupId,
                sequenceNumber,
                ATOMIC_GROUP_STATUS_TEXT(lookupAtomicGroupState->second.status)
                );
        }
    
        for (Participant_ParticipantState_Iterator iterateParticipants = lookupAtomicGroupState->second.participants.begin(); 
             lookupAtomicGroupState->second.participants.end() != iterateParticipants; 
             iterateParticipants++)
        {
            PartitionId_StatefulServiceUnit_Iterator lookupParticipant = innerStatefulServiceUnits_.find(iterateParticipants->first);
            ASSERT_IF(innerStatefulServiceUnits_.end() == lookupParticipant, "Unknown participant");
    
            Participant_ParticipantState_Iterator lookupParticipantState = lookupAtomicGroupState->second.participants.find(iterateParticipants->first);
            ASSERT_IF(lookupAtomicGroupState->second.participants.end() == lookupParticipantState, "Unknown participant state");
    
            if (AtomicGroupStatus::IN_FLIGHT != lookupAtomicGroupState->second.status)
            {
                ASSERT_IF(1 < lookupParticipantState->second.replicating, "Unexpected participant replicating");
            }

            CSingleAsyncOperation* sCommitRollbackAsyncOperation = new (std::nothrow) CAtomicGroupCommitRollbackOperationAsync(
                    mCommitRollbackAsyncOperation,
                    (IFabricAtomicGroupStateProvider*)lookupParticipant->second,
                    isCommit,
                    atomicGroupId,
                    sequenceNumber
                    );
            
            if (NULL == sCommitRollbackAsyncOperation)
            {
                hr = E_OUTOFMEMORY;
                break;
            }
            hr = sCommitRollbackAsyncOperation->FinalConstruct();
            if (FAILED(hr))
            {
                sCommitRollbackAsyncOperation->Release();
                break;
            }
            hr = mCommitRollbackAsyncOperation->Compose(sCommitRollbackAsyncOperation);
            if (FAILED(hr))
            {
                sCommitRollbackAsyncOperation->Release();
                break;
            }
        }
    }

    if (FAILED(hr))
    {
        mCommitRollbackAsyncOperation->Release();
        return hr;
    }

    ServiceGroupReplicationEventSource::GetEvents().Info_12_StatefulServiceGroupAtomicGroupReplicationProcessing(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_,
        atomicGroupId
        );

    hr = mCommitRollbackAsyncOperation->Begin();
    return hr;
}

HRESULT CStatefulServiceGroup::RefreshAtomicGroup(
    __in FABRIC_OPERATION_TYPE operationType,
    __in OperationStatus operationStatus,
    __in FABRIC_ATOMIC_GROUP_ID atomicGroupId, 
    __in const Common::Guid & participant,
    __in const std::wstring & participantName
    )
{
    HRESULT hr = S_OK;
    AtomicGroup_AtomicState_Iterator iterateAtomicGroup = atomicGroups_.find(atomicGroupId);
    if (atomicGroups_.end() == iterateAtomicGroup)
    {
        //
        // Create new atomic group
        //
        ASSERT_IF(REPLICATED == operationStatus, "OperationStatus");

        if (FABRIC_OPERATION_TYPE_COMMIT_ATOMIC_GROUP == operationType || FABRIC_OPERATION_TYPE_ROLLBACK_ATOMIC_GROUP == operationType)
        {
            ServiceGroupReplicationEventSource::GetEvents().Error_6_StatefulServiceGroupAtomicGroupReplicationProcessing(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_,
                participantName,
                atomicGroupId
                );

            return E_INVALIDARG;
        }

        AtomicGroupState atomicGroupState;
        ParticipantState participantState;
        if (REPLICATING == operationStatus)
        {
            atomicGroupState.replicating = 1;
            participantState.replicating = 1;
        }

        ServiceGroupReplicationEventSource::GetEvents().Info_13_StatefulServiceGroupAtomicGroupReplicationProcessing(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            atomicGroupId,
            atomicGroups_.size() + 1
            );

        try { atomicGroups_.insert(AtomicGroup_AtomicState_Pair(atomicGroupId, atomicGroupState)); }
        catch (std::bad_alloc const &) { hr = E_OUTOFMEMORY; }
        catch (...) { hr = E_FAIL; }
        if (FAILED(hr))
        {
            ServiceGroupReplicationEventSource::GetEvents().Error_7_StatefulServiceGroupAtomicGroupReplicationProcessing(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_,
                atomicGroupId,
                atomicGroups_.size()
                );

            return hr;
        }

        atomicGroupStateReplicatorCounters_->AtomicGroupsInFlight.Value = static_cast<Common::PerformanceCounterValue>(atomicGroups_.size());

        ServiceGroupReplicationEventSource::GetEvents().Info_14_StatefulServiceGroupAtomicGroupReplicationProcessing(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            participantName,
            atomicGroupId
            );

        try { atomicGroups_[atomicGroupId].participants.insert(Participant_ParticipantState_Pair(participant, participantState)); }
        catch (std::bad_alloc const &) { hr = E_OUTOFMEMORY; }
        catch (...) { hr = E_FAIL; }
        if (FAILED(hr))
        {
            ServiceGroupReplicationEventSource::GetEvents().Error_8_StatefulServiceGroupAtomicGroupReplicationProcessing(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_,
                participantName,
                atomicGroupId
                );

            atomicGroups_.erase(atomicGroupId);

            atomicGroupStateReplicatorCounters_->AtomicGroupsInFlight.Value = static_cast<Common::PerformanceCounterValue>(atomicGroups_.size());

            return hr;
        }

        if (REPLICATING == operationStatus)
        {
            atomicGroupReplicationDoneSignal_->Reset();
        }

        hr = S_FALSE;
    }    
    else
    {
        //
        // Update existing atomic group
        //
        ServiceGroupReplicationEventSource::GetEvents().Info_15_StatefulServiceGroupAtomicGroupReplicationProcessing(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            atomicGroupId
            );

        Participant_ParticipantState_Iterator iterateParticipant = iterateAtomicGroup->second.participants.find(participant);
        if (iterateAtomicGroup->second.participants.end() == iterateParticipant)
        {
            //
            // Add new participant
            //
            ASSERT_IF(REPLICATED == operationStatus, "OperationStatus");

            if (FABRIC_OPERATION_TYPE_COMMIT_ATOMIC_GROUP == operationType || FABRIC_OPERATION_TYPE_ROLLBACK_ATOMIC_GROUP == operationType)
            {
                ServiceGroupReplicationEventSource::GetEvents().Error_9_StatefulServiceGroupAtomicGroupReplicationProcessing(
                    partitionIdString_,
                    reinterpret_cast<uintptr_t>(this),
                    replicaId_,
                    participantName,
                    atomicGroupId
                    );

                return E_INVALIDARG;
            }

            if (AtomicGroupStatus::ROLLING_BACK == iterateAtomicGroup->second.status || AtomicGroupStatus::COMMITTING == iterateAtomicGroup->second.status)
            {
                ServiceGroupReplicationEventSource::GetEvents().Error_10_StatefulServiceGroupAtomicGroupReplicationProcessing(
                    partitionIdString_,
                    reinterpret_cast<uintptr_t>(this),
                    replicaId_,
                    participantName,
                    (LONG)operationType,
                    atomicGroupId
                    );

                return E_INVALIDARG;
            }

            ServiceGroupReplicationEventSource::GetEvents().Info_16_StatefulServiceGroupAtomicGroupReplicationProcessing(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_,
                participantName,
                atomicGroupId
                );

            ParticipantState participantState;
            if (REPLICATING == operationStatus)
            {
                participantState.replicating = 1;
            }

            try { iterateAtomicGroup->second.participants.insert(Participant_ParticipantState_Pair(participant, participantState)); }
            catch (std::bad_alloc const &) { hr = E_OUTOFMEMORY; }
            catch (...) { hr = E_FAIL; }
            if (FAILED(hr))
            {
                ServiceGroupReplicationEventSource::GetEvents().Error_11_StatefulServiceGroupAtomicGroupReplicationProcessing(
                    partitionIdString_,
                    reinterpret_cast<uintptr_t>(this),
                    replicaId_,
                    participantName,
                    atomicGroupId
                    );
                
                return hr;
            }

            iterateAtomicGroup->second.replicating++;

            if (REPLICATING == operationStatus)
            {
                atomicGroupReplicationDoneSignal_->Reset();
            }

            hr = S_FALSE;
        }
        else
        {
            //
            // Update existing participant
            //
            ServiceGroupReplicationEventSource::GetEvents().Info_17_StatefulServiceGroupAtomicGroupReplicationProcessing(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_,
                participantName,
                atomicGroupId
                );

            if (AtomicGroupStatus::ROLLING_BACK == iterateAtomicGroup->second.status || AtomicGroupStatus::COMMITTING == iterateAtomicGroup->second.status)
            {
                if (FABRIC_OPERATION_TYPE_COMMIT_ATOMIC_GROUP == operationType || FABRIC_OPERATION_TYPE_ROLLBACK_ATOMIC_GROUP == operationType)
                {
                    //
                    // Check that commit/rollback can only be retried by the participant that made the earlier attempt
                    //
                    if (participant == iterateAtomicGroup->second.terminator)
                    {
                        if (TRUE == iterateAtomicGroup->second.terminationFailed)
                        {
                            //
                            // Check that a previous failed rollback is not converted to a commit and vice versa
                            //
                            if ((AtomicGroupStatus::COMMITTING == iterateAtomicGroup->second.status && FABRIC_OPERATION_TYPE_ROLLBACK_ATOMIC_GROUP == operationType)||
                                (AtomicGroupStatus::ROLLING_BACK == iterateAtomicGroup->second.status && FABRIC_OPERATION_TYPE_COMMIT_ATOMIC_GROUP == operationType))
                            {
                                ServiceGroupReplicationEventSource::GetEvents().Error_12_StatefulServiceGroupAtomicGroupReplicationProcessing(
                                    partitionIdString_,
                                    reinterpret_cast<uintptr_t>(this),
                                    replicaId_,
                                    participantName,
                                    atomicGroupId
                                    );
                                
                                return E_INVALIDARG;
                            }
                            
                            ServiceGroupReplicationEventSource::GetEvents().Info_18_StatefulServiceGroupAtomicGroupReplicationProcessing(
                                partitionIdString_,
                                reinterpret_cast<uintptr_t>(this),
                                replicaId_,
                                participantName,
                                (LONG)operationType,
                                atomicGroupId
                                );
                        }
                        else
                        {
                            ServiceGroupReplicationEventSource::GetEvents().Error_13_StatefulServiceGroupAtomicGroupReplicationProcessing(
                                partitionIdString_,
                                reinterpret_cast<uintptr_t>(this),
                                replicaId_,
                                participantName,
                                atomicGroupId
                                );
                            
                            return E_INVALIDARG;
                        }
                    }
                    else
                    {
                        ServiceGroupReplicationEventSource::GetEvents().Error_14_StatefulServiceGroupAtomicGroupReplicationProcessing(
                            partitionIdString_,
                            reinterpret_cast<uintptr_t>(this),
                            replicaId_,
                            participantName,
                            atomicGroupId,
                            iterateAtomicGroup->second.terminator
                            );

                        return E_INVALIDARG;
                    }
                }
                else
                {
                    ServiceGroupReplicationEventSource::GetEvents().Error_15_StatefulServiceGroupAtomicGroupReplicationProcessing(
                        partitionIdString_,
                        reinterpret_cast<uintptr_t>(this),
                        replicaId_,
                        participantName,
                        (LONG)operationType,
                        atomicGroupId
                        );
                    
                    return E_INVALIDARG;
                }
            }
            if (FABRIC_OPERATION_TYPE_COMMIT_ATOMIC_GROUP == operationType || FABRIC_OPERATION_TYPE_ROLLBACK_ATOMIC_GROUP == operationType)
            {
                if ((REPLICATING == operationStatus) && (0 != iterateAtomicGroup->second.replicating || 0 != iterateParticipant->second.replicating))
                {
                    ServiceGroupReplicationEventSource::GetEvents().Info_19_StatefulServiceGroupAtomicGroupReplicationProcessing(
                        partitionIdString_,
                        reinterpret_cast<uintptr_t>(this),
                        replicaId_,
                        participantName,
                        (LONG)operationType,
                        atomicGroupId,
                        iterateAtomicGroup->second.replicating
                        );
                }
                if (AtomicGroupStatus::IN_FLIGHT == iterateAtomicGroup->second.status)
                {
                    if (FABRIC_OPERATION_TYPE_COMMIT_ATOMIC_GROUP == operationType)
                    {
                        iterateAtomicGroup->second.status = AtomicGroupStatus::COMMITTING;
                    }
                    if (FABRIC_OPERATION_TYPE_ROLLBACK_ATOMIC_GROUP == operationType)
                    {
                        iterateAtomicGroup->second.status = AtomicGroupStatus::ROLLING_BACK;
                    }
                    iterateAtomicGroup->second.terminator.assign(participant);
                }
                else
                {
                    ServiceGroupReplicationEventSource::GetEvents().Warning_1_StatefulServiceGroupAtomicGroupReplicationProcessing(
                        partitionIdString_,
                        reinterpret_cast<uintptr_t>(this),
                        replicaId_,
                        atomicGroupId
                        );
                }
                if (REPLICATING == operationStatus)
                {
                    iterateParticipant->second.replicating++;
                    iterateAtomicGroup->second.replicating++;
                    atomicGroupReplicationDoneSignal_->Reset();
                }
                //
                // Do not check REPLICATED == operationStatus here! On completion, commit/rollback will call RemoveAtomicGroup which updates the atomic group
                //
            }
            else
            {
                //
                // Normal operation (not commit or rollback)
                //
                if (REPLICATING == operationStatus)
                {
                    iterateParticipant->second.replicating++;
                    iterateAtomicGroup->second.replicating++;
                    atomicGroupReplicationDoneSignal_->Reset();
                }
                if (REPLICATED == operationStatus)
                {
                    ASSERT_IF(AtomicGroupStatus::ROLLING_BACK == iterateAtomicGroup->second.status || AtomicGroupStatus::COMMITTING == iterateAtomicGroup->second.status, "status");

                    ASSERT_IFNOT(0 < iterateParticipant->second.replicating, "participant replicating");
                    iterateParticipant->second.replicating--;
                    iterateParticipant->second.replicated++;

                    ServiceGroupReplicationEventSource::GetEvents().Info_20_StatefulServiceGroupAtomicGroupReplicationProcessing(
                        partitionIdString_,
                        reinterpret_cast<uintptr_t>(this),
                        replicaId_,
                        participantName,
                        iterateParticipant->second.replicated,
                        atomicGroupId
                        );

                    ASSERT_IFNOT(0 < iterateAtomicGroup->second.replicating, "atomic group replicating");
                    iterateAtomicGroup->second.replicating--;
                    iterateAtomicGroup->second.replicated++;

                    ServiceGroupReplicationEventSource::GetEvents().Info_21_StatefulServiceGroupAtomicGroupReplicationProcessing(
                        partitionIdString_,
                        reinterpret_cast<uintptr_t>(this),
                        replicaId_,
                        atomicGroupId,
                        iterateAtomicGroup->second.replicated
                        );

                    if (0 == iterateAtomicGroup->second.replicating)
                    {
                        SetAtomicGroupsReplicationDone();
                    }
                }
            }
        }
    }
    return hr;
}

void CStatefulServiceGroup::SetAtomicGroupsReplicationDone(void)
{
    if (atomicGroups_.empty())
    {
        ServiceGroupReplicationEventSource::GetEvents().Info_22_StatefulServiceGroupAtomicGroupReplicationProcessing(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_
            );

        atomicGroupReplicationDoneSignal_->Set();
    }
    else
    {
        AtomicGroup_AtomicState_Iterator iterate;
        for (iterate = atomicGroups_.begin(); atomicGroups_.end() != iterate; iterate++)
        {
            if (0 != iterate->second.replicating)
            {
                ServiceGroupReplicationEventSource::GetEvents().Info_23_StatefulServiceGroupAtomicGroupReplicationProcessing(
                    partitionIdString_,
                    reinterpret_cast<uintptr_t>(this),
                    replicaId_,
                    iterate->first
                    );
                break;
            }
        }
        if (atomicGroups_.end() == iterate)
        {
            ServiceGroupReplicationEventSource::GetEvents().Info_24_StatefulServiceGroupAtomicGroupReplicationProcessing(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_
                );

            atomicGroupReplicationDoneSignal_->Set();
        }
    }
}

void CStatefulServiceGroup::DrainAtomicGroupsReplication(void)
{
    {
        Common::AcquireReadLock lockAtomicGroups(lockAtomicGroups_);
        AtomicGroup_AtomicState_Iterator iterate;
        for (iterate = atomicGroups_.begin(); atomicGroups_.end() != iterate; iterate++)
        {
            if (0 != iterate->second.replicating)
            {
                ServiceGroupReplicationEventSource::GetEvents().Info_25_StatefulServiceGroupAtomicGroupReplicationProcessing(
                    partitionIdString_,
                    reinterpret_cast<uintptr_t>(this),
                    replicaId_,
                    iterate->first
                    );
            }
        }
    }

    ServiceGroupReplicationEventSource::GetEvents().Info_26_StatefulServiceGroupAtomicGroupReplicationProcessing(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_
        );

    auto waitResult = atomicGroupReplicationDoneSignal_->WaitOne();
    ASSERT_IFNOT(waitResult, "Unexpected wait result");

    ServiceGroupReplicationEventSource::GetEvents().Info_27_StatefulServiceGroupAtomicGroupReplicationProcessing(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_
        );
}

void CStatefulServiceGroup::RemoveAllAtomicGroups()
{
    Common::AcquireWriteLock lockAtomicGroups(lockAtomicGroups_);

    ServiceGroupReplicationEventSource::GetEvents().Info_28_StatefulServiceGroupAtomicGroupReplicationProcessing(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_,
        atomicGroups_.size()
        );

    atomicGroups_.clear();

    atomicGroupStateReplicatorCounters_->AtomicGroupsInFlight.Value = static_cast<Common::PerformanceCounterValue>(atomicGroups_.size());
}

void CStatefulServiceGroup::RemoveAtomicGroupsOnUpdateEpoch(FABRIC_EPOCH const & epoch, FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber)
{
    Common::AcquireWriteLock lockAtomicGroups(lockAtomicGroups_);

    AtomicGroup_AtomicState_Iterator iterate = atomicGroups_.begin();
    while (atomicGroups_.end() != iterate)
    {
        if (iterate->second.createdSequenceNumber > previousEpochLastSequenceNumber)
        {
            ServiceGroupStatefulEventSource::GetEvents().Info_7_StatefulServiceGroupUpdateEpoch(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_,
                iterate->first
                );

            iterate++;
        }
        else
        {
            ServiceGroupStatefulEventSource::GetEvents().Info_8_StatefulServiceGroupUpdateEpoch(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_,
                iterate->first
                );

            atomicGroups_.erase(iterate->first);

            atomicGroupStateReplicatorCounters_->AtomicGroupsInFlight.Value = static_cast<Common::PerformanceCounterValue>(atomicGroups_.size());

            iterate = atomicGroups_.begin();
        }
    }

    ServiceGroupStatefulEventSource::GetEvents().Info_9_StatefulServiceGroupUpdateEpoch(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_,
        atomicGroups_.size()
        );

    if (currentEpoch_.DataLossNumber < epoch.DataLossNumber)
    {
        if (!atomicGroups_.empty())
        {
            ServiceGroupStatefulEventSource::GetEvents().Warning_0_StatefulServiceGroupUpdateEpoch(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_
                );

            iterate = atomicGroups_.begin();
            while (atomicGroups_.end() != iterate)
            {
                ServiceGroupStatefulEventSource::GetEvents().Warning_1_StatefulServiceGroupUpdateEpoch(
                    partitionIdString_,
                    reinterpret_cast<uintptr_t>(this),
                    replicaId_,
                    iterate->first
                    );

                iterate++;
            }

            atomicGroups_.clear();

            atomicGroupStateReplicatorCounters_->AtomicGroupsInFlight.Value = static_cast<Common::PerformanceCounterValue>(atomicGroups_.size());
        }
    }
}

HRESULT CStatefulServiceGroup::ErrorFixupAtomicGroup(__in FABRIC_ATOMIC_GROUP_ID atomicGroupId, __in const Common::Guid& participant, __in const std::wstring & participantName)
{
    HRESULT hr = S_OK;

    AtomicGroup_AtomicState_Iterator iterateAtomicGroup = atomicGroups_.find(atomicGroupId);
    ASSERT_IF(atomicGroups_.end() == iterateAtomicGroup, "Unknown atomic group");
    ASSERT_IFNOT(0 < iterateAtomicGroup->second.replicating, "replicating atomic group ");

    Participant_ParticipantState_Iterator iterateParticipant = iterateAtomicGroup->second.participants.find(participant);
    ASSERT_IF(iterateAtomicGroup->second.participants.end() == iterateParticipant, "Unknown participant");
    ASSERT_IFNOT(0 < iterateParticipant->second.replicating, "replicating participant ");

    if (AtomicGroupStatus::COMMITTING == iterateAtomicGroup->second.status || AtomicGroupStatus::ROLLING_BACK == iterateAtomicGroup->second.status)
    {
        iterateAtomicGroup->second.terminationFailed = TRUE;
    }
    iterateParticipant->second.replicating--;
    if (0 == iterateParticipant->second.replicating && 0 == iterateParticipant->second.replicated)
    {
        ServiceGroupReplicationEventSource::GetEvents().Error_16_StatefulServiceGroupAtomicGroupReplicationProcessing(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            participantName,
            atomicGroupId
            );

        iterateAtomicGroup->second.participants.erase(iterateParticipant);
    }
    iterateAtomicGroup->second.replicating--;
    if (iterateAtomicGroup->second.participants.empty())
    {
        ASSERT_IFNOT(0 == iterateAtomicGroup->second.replicating, "replicating participant");

        ServiceGroupReplicationEventSource::GetEvents().Error_17_StatefulServiceGroupAtomicGroupReplicationProcessing(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            atomicGroupId,
            atomicGroups_.size() - 1
            );

        atomicGroups_.erase(iterateAtomicGroup);

        atomicGroupStateReplicatorCounters_->AtomicGroupsInFlight.Value = static_cast<Common::PerformanceCounterValue>(atomicGroups_.size());
    }
    SetAtomicGroupsReplicationDone();
    return hr;
}

HRESULT CStatefulServiceGroup::CreateRollbackAtomicGroupsOperation(
    __in FABRIC_SEQUENCE_NUMBER sequenceNumber, 
    __in_opt CBaseAsyncOperation* parent,
    __out CCompositeAsyncOperationAsync** operation)
{
    ASSERT_IF(NULL == operation, "Unexpected operation");

    HRESULT hr = S_OK;

    CCompositeAsyncOperationAsync* mRollbackOperation = new (std::nothrow) CCompositeAsyncOperationAsync(parent, partitionId_.AsGUID());
    if (NULL == mRollbackOperation)
    {
        return E_OUTOFMEMORY;
    }
    hr = mRollbackOperation->FinalConstruct();
    if (FAILED(hr))
    {
        mRollbackOperation->Release();
        return hr;
    }

    Common::AcquireReadLock lockAtomicGroups(lockAtomicGroups_);

    BOOLEAN hasChildren = FALSE;

    for (AtomicGroup_AtomicState_Iterator it = begin(atomicGroups_); it != end(atomicGroups_); ++it)
    {
        if (it->second.createdSequenceNumber > sequenceNumber)
        {
            ServiceGroupReplicationEventSource::GetEvents().Info_29_StatefulServiceGroupAtomicGroupReplicationProcessing(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_,
                it->first
                );
        }
        else
        {
             ServiceGroupReplicationEventSource::GetEvents().Info_30_StatefulServiceGroupAtomicGroupReplicationProcessing(
                 partitionIdString_,
                 reinterpret_cast<uintptr_t>(this),
                 this->replicaId_,
                 it->first
                 );

            CCompositeAsyncOperationAsync * sRollbackOperation = NULL;
            hr = CreateAtomicGroupCommitRollbackOperation(
                FALSE,                      // rollback
                it->first,                  // atomic group id
                FABRIC_INVALID_SEQUENCE_NUMBER,    // rollback is local
                mRollbackOperation,         // parent operation
                &sRollbackOperation         // new operation
                );
            if (FAILED(hr))
            {
                break;
            }
            
            hr = mRollbackOperation->Compose(sRollbackOperation);
            if (FAILED(hr))
            {
                sRollbackOperation->Release();
                break;
            }

            hasChildren = TRUE;
        }
    }

    if (FAILED(hr))
    {
        mRollbackOperation->Release();
        return hr;
    }

    if (hasChildren)
    {
        *operation = mRollbackOperation;
    }
    else
    {
        // there is nothing to rollback
        mRollbackOperation->Release();
        *operation = NULL;
    }

    return hr;
}

HRESULT CStatefulServiceGroup::RemoveAtomicGroup(__in FABRIC_ATOMIC_GROUP_ID atomicGroupId, __in OperationStatus operationStatus)
{
    //
    // Remove a committed or rolled back atomic group.
    // On the secondary operations are processed sequentially therefore no lock is required.
    // On the primary replication operations can complete indenpendently and the caller must hold lockAtomicGroups_.
    //

    AtomicGroup_AtomicState_Iterator lookupAtomicGroupState = atomicGroups_.find(atomicGroupId);
    ASSERT_IF(atomicGroups_.end() == lookupAtomicGroupState, "Atomic group does not exist");

    if (TRUE == lookupAtomicGroupState->second.terminationFailed)
    {
        ServiceGroupReplicationEventSource::GetEvents().Info_31_StatefulServiceGroupAtomicGroupReplicationProcessing(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            atomicGroupId
            );
    }

    ServiceGroupReplicationEventSource::GetEvents().Info_32_StatefulServiceGroupAtomicGroupReplicationProcessing(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_,
        atomicGroupId,
        atomicGroups_.size() - 1
        );

    atomicGroups_.erase(lookupAtomicGroupState);

    atomicGroupStateReplicatorCounters_->AtomicGroupsInFlight.Value = static_cast<Common::PerformanceCounterValue>(atomicGroups_.size());

    if (REPLICATED == operationStatus)
    {
        SetAtomicGroupsReplicationDone();
    }
    return S_OK;
}

HRESULT CStatefulServiceGroup::RemoveAtomicGroupOnCommitRollback(__in FABRIC_ATOMIC_GROUP_ID atomicGroupId)
{
    Common::AcquireWriteLock lockAtomicGroups(lockAtomicGroups_);
    return RemoveAtomicGroup(atomicGroupId, REPLICATED);
}

void CStatefulServiceGroup::UpdateLastCommittedLSN(__in LONGLONG lsn)
{
    if (lastCommittedLSN_ < lsn)
    {
        ServiceGroupReplicationEventSource::GetEvents().Info_33_StatefulServiceGroupAtomicGroupReplicationProcessing(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            lastCommittedLSN_,
            lsn
            );

        lastCommittedLSN_ = lsn;
    }
    if (FABRIC_INVALID_SEQUENCE_NUMBER != lastCommittedLSN_ && FABRIC_INVALID_SEQUENCE_NUMBER == lsn)
    {
        ServiceGroupReplicationEventSource::GetEvents().Info_34_StatefulServiceGroupAtomicGroupReplicationProcessing(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            lastCommittedLSN_,
            lsn
            );

        lastCommittedLSN_ = lsn;
    }
}

void CStatefulServiceGroup::SetAtomicGroupSequenceNumber(
    __in FABRIC_ATOMIC_GROUP_ID atomicGroupId, 
    __in const Common::Guid& participant,
    __in LONGLONG sequenceNumber
    )
{
    AtomicGroup_AtomicState_Iterator iterateAtomicGroup = atomicGroups_.find(atomicGroupId);
    ASSERT_IF(atomicGroups_.end() == iterateAtomicGroup, "Unknown atomic group");

    Participant_ParticipantState_Iterator iterateParticipant = iterateAtomicGroup->second.participants.find(participant);
    ASSERT_IF(iterateAtomicGroup->second.participants.end() == iterateParticipant, "Unknown participant");

    if (_I64_MAX == iterateAtomicGroup->second.createdSequenceNumber)
    {
        ServiceGroupReplicationEventSource::GetEvents().Info_35_StatefulServiceGroupAtomicGroupReplicationProcessing(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            sequenceNumber,
            atomicGroupId
            );

        iterateAtomicGroup->second.createdSequenceNumber = sequenceNumber;
    }

    ServiceGroupReplicationEventSource::GetEvents().Info_36_StatefulServiceGroupAtomicGroupReplicationProcessing(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_,
        sequenceNumber,
        innerStatefulServiceUnits_[participant]->get_ServiceName()
        );

    ASSERT_IF(_I64_MAX != iterateParticipant->second.createdSequenceNumber, "Incorrect participant");
    iterateParticipant->second.createdSequenceNumber = sequenceNumber;
}

//
// IServiceGroupReport methods.
//
STDMETHODIMP CStatefulServiceGroup::ReportLoad(__in FABRIC_PARTITION_ID partitionId, __in ULONG metricCount, __in const FABRIC_LOAD_METRIC* metrics)
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

STDMETHODIMP CStatefulServiceGroup::ReportFault(__in FABRIC_PARTITION_ID partitionId, __in FABRIC_FAULT_TYPE faultType)
{
    AddRef();
    try
    {
        Common::Threadpool::Post
        (
            [this, partitionId, faultType]
            {
                //
                // If permanent, mark the replica as faulted. From this point on the operation pump will stop, if still in progress.
                // (Ensure that a subsequent transient fault cannot reset the flag)
                //
                {
                    Common::AcquireWriteLock lock(this->lockFaultReplica_);
                    isReplicaFaulted_ = isReplicaFaulted_ || (FABRIC_FAULT_TYPE_PERMANENT == faultType);
                }

                HRESULT hr = this->ReportFault(faultType);
                if (FAILED(hr))
                {
                    ServiceGroupStatefulEventSource::GetEvents().Warning_0_StatefulReportFault(
                        this->partitionIdString_,
                        reinterpret_cast<uintptr_t>(this),
                        replicaId_,
                        hr
                        );

                    if (FABRIC_E_OBJECT_CLOSED == hr)
                    {
                        this->Release();
                        return;
                    }
                }

                ServiceGroupStatefulEventSource::GetEvents().Warning_1_StatefulReportFault(
                    this->partitionIdString_,
                    reinterpret_cast<uintptr_t>(this),
                    replicaId_,
                    (ULONG)faultType
                    );

                if (FABRIC_FAULT_TYPE_PERMANENT == faultType)
                {
                    this->InternalAbort(partitionId, TRUE);
                }

                this->Release();
            }
        );
    }
    catch (std::bad_alloc const &)
    {
        Common::Assert::CodingError(SERVICE_REPLICA_DOWN);
    }
    return S_OK;
}

STDMETHODIMP CStatefulServiceGroup::ReportMoveCost(__in FABRIC_PARTITION_ID partitionId, __in FABRIC_MOVE_COST moveCost)
{
    UNREFERENCED_PARAMETER(partitionId);

    HRESULT hr = ReportMoveCost(moveCost);
    if (FAILED(hr))
    {
        return hr;
    }
    return S_OK;
}

void STDMETHODCALLTYPE CStatefulServiceGroup::OnPackageAdded( 
    __in IFabricCodePackageActivationContext *source,
    __in IFabricConfigurationPackage *configPackage)
{
    ServiceGroupStatefulEventSource::GetEvents().Info_2_StatefulServiceGroupPackage(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_
        );

    ASSERT_IF(NULL == configPackage, "CStatefulServiceGroup::PackageChanged configPackage is null");
    ASSERT_IFNOT(source == activationContext_, "CStatefulServiceGroup::PackageChanged received from unknown source");

    const FABRIC_CONFIGURATION_PACKAGE_DESCRIPTION * packageDescription = configPackage->get_Description();
    ASSERT_IF(NULL == packageDescription, "Unexpected configuration package description");

    try
    {
        if (!Common::StringUtility::AreEqualCaseInsensitive(replicatorSettingsConfigPackageName_.c_str(), packageDescription->Name))
        {
            //
            // Package doesn't contain the replicator settings.
            //
            ServiceGroupStatefulEventSource::GetEvents().Info_3_StatefulServiceGroupPackage(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_,
                packageDescription->Name
                );

            return;
        }
    }
    catch (...)
    {
        ServiceGroupStatefulEventSource::GetEvents().Error_10_StatefulServiceGroupPackage(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_
            );

        return;
    }

    ReplicatorSettings settings(partitionId_);
    HRESULT hr = settings.ReadFromConfigurationPackage(configPackage, source);

    if (FAILED(hr))
    {
        ServiceGroupStatefulEventSource::GetEvents().Error_11_StatefulServiceGroupPackage(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            hr
            );

        return;
    }

    {
        Common::AcquireReadLock lockReplicator(lock_);

        if (outerStateReplicatorDestructed_)
        {
            ServiceGroupStatefulEventSource::GetEvents().Error_12_StatefulServiceGroupPackage(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_
                );

            return;
        }
        outerStateReplicator_->AddRef();
    }

    hr = outerStateReplicator_->UpdateReplicatorSettings(settings.GetRawPointer());
    if (FAILED(hr))
    {
        ServiceGroupStatefulEventSource::GetEvents().Error_13_StatefulServiceGroupPackage(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            hr
            );
    }
    else
    {
        ServiceGroupStatefulEventSource::GetEvents().Info_4_StatefulServiceGroupPackage(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_
            );
    }

    outerStateReplicator_->Release();
}

void STDMETHODCALLTYPE CStatefulServiceGroup::OnPackageRemoved( 
    __in IFabricCodePackageActivationContext *source,
    __in IFabricConfigurationPackage *configPackage)
{
    UNREFERENCED_PARAMETER(source);
    UNREFERENCED_PARAMETER(configPackage);
}

void STDMETHODCALLTYPE CStatefulServiceGroup::OnPackageModified( 
    __in IFabricCodePackageActivationContext *source,
    __in IFabricConfigurationPackage *previousConfigPackage,
    __in IFabricConfigurationPackage *configPackage)
{
    UNREFERENCED_PARAMETER(previousConfigPackage);

    this->OnPackageAdded(source, configPackage);
}

void CStatefulServiceGroup::InternalAbort(__in FABRIC_PARTITION_ID partitionId, __in BOOLEAN isReportFault)
{
    Common::Guid faultingReplicaId(partitionId);

    {
        Common::AcquireWriteLock lock(lock_);

        if (outerStateReplicatorDestructed_)
        {
            ServiceGroupCommonEventSource::GetEvents().ReplicaAlreadyClosed(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_
                );

            return;
        }

        if (isReportFault && partitionId_ != faultingReplicaId)
        {
            ServiceGroupStatefulEventSource::GetEvents().Warning_2_StatefulReportFault(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_,
                ROLE_TEXT(currentReplicaRole_),
                innerStatefulServiceUnits_[faultingReplicaId]->get_ServiceName()
                );
        }
        else
        {
            if (isReportFault)
            {
                ServiceGroupStatefulEventSource::GetEvents().Warning_3_StatefulReportFault(
                    partitionIdString_,
                    reinterpret_cast<uintptr_t>(this),
                    replicaId_,
                    ROLE_TEXT(currentReplicaRole_)
                    );
            }
            else
            {
                ServiceGroupStatefulEventSource::GetEvents().Warning_0_StatefulServiceGroupAbort(
                    partitionIdString_,
                    reinterpret_cast<uintptr_t>(this),
                    replicaId_,
                    ROLE_TEXT(currentReplicaRole_)
                    );
            }
        }

        //
        // Ensure that any subsequent calls to GetCopyStream/GetReplicationStream will fail after Abort (!isReportFault).
        // Do not reset streamsEnding_ if already set.
        //
        streamsEnding_ = streamsEnding_ || !isReportFault;

        //
        // There is no need to unblock the replication processing here. Either the replicator has already or will enqueue NULL
        // in the copy stream which will unblock the replication processing; or we terminate the copy pump on the next copy operation
        // which will unblock the replication processing.
        //
    }

    //
    // Call InternalTerminateOperationStreams on all service members.
    // innerStatefulServiceUnits_ needs to be accessed under lock_ but InternalAbort must not be called under this lock.
    //
    Common::Guid serviceMemberId = Common::Guid::Empty();
    BOOLEAN done = FALSE;
    while (!done)
    {
        CStatefulServiceUnit* serviceMember = NULL;

        {
            Common::AcquireWriteLock lock(lock_);

            PartitionId_StatefulServiceUnit_Iterator iterator;
            if (Common::Guid::Empty() == serviceMemberId)
            {
                iterator = innerStatefulServiceUnits_.begin();
            }
            else
            {
                iterator = innerStatefulServiceUnits_.find(serviceMemberId);
                if (innerStatefulServiceUnits_.end() != iterator)
                {
                    ++iterator;
                }
            }

            if (innerStatefulServiceUnits_.end() == iterator)
            {
                //
                // The service may have been destructed in the meantime, this can happen even if the previous service member is not the last one.
                //
                done = TRUE;
            }
            else
            {
                serviceMemberId = iterator->first;

                serviceMember = iterator->second;
                ASSERT_IF(NULL == serviceMember, "Unexpected service unit");
                serviceMember->AddRef();
            }
        }

        if (NULL != serviceMember)
        {
            //
            // Call InternalTerminateOperationStreams outside the lock to prevent lock inversion.
            //

            BOOLEAN forceDrainStreams = FALSE;
            if (isReportFault)
            {
                //
                // On report fault only force drain streams on the faulting member. Non-faulting members that experience faults must call ReportFault themselves.
                //
                forceDrainStreams = (faultingReplicaId == serviceMemberId);
            }
            else
            {
                //
                // On abort always force drains the streams.
                //
                forceDrainStreams = TRUE;
            }

            serviceMember->InternalTerminateOperationStreams(forceDrainStreams, isReportFault);
            serviceMember->Release();
        }
    }

    if (isReportFault)
    {
        ServiceGroupStatefulEventSource::GetEvents().Warning_4_StatefulReportFault(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            faultingReplicaId
            );
    }
    else
    {
        ServiceGroupStatefulEventSource::GetEvents().Warning_1_StatefulServiceGroupAbort(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            faultingReplicaId
            );
    }
}

HRESULT CStatefulServiceGroup::GetStatefulServiceUnit(__in const Common::Guid & id, __out CStatefulServiceUnit** unit)
{
    if (NULL == unit)
    {
        return E_POINTER;
    }

    Common::AcquireReadLock lock(lock_);

    PartitionId_StatefulServiceUnit_Iterator lookup = innerStatefulServiceUnits_.find(id);
    if (innerStatefulServiceUnits_.end() == lookup)
    {
        return E_INVALIDARG;
    }

    *unit = lookup->second;
    (*unit)->AddRef();

    return S_OK;
}

//
// Constructor/Destructor.
//
CAtomicGroupStateOperationDataAsyncEnumerator::CAtomicGroupStateOperationDataAsyncEnumerator()
{
    WriteNoise(TraceSubComponentAtomicGroupStateOperationDataAsyncEnumerator, "this={0} - ctor", this);

    operationData_ = NULL;
    done_ = FALSE;
}

CAtomicGroupStateOperationDataAsyncEnumerator::~CAtomicGroupStateOperationDataAsyncEnumerator(void)
{
    WriteNoise(TraceSubComponentAtomicGroupStateOperationDataAsyncEnumerator, "this={0} - dtor", this);

    if (NULL != operationData_)
    {
        operationData_->Release();
    }
}

//
// IFabricOperationDataStream methods.
//
STDMETHODIMP CAtomicGroupStateOperationDataAsyncEnumerator::BeginGetNext(
    __in IFabricAsyncOperationCallback* callback,
    __out IFabricAsyncOperationContext** context
    )
{
    if (NULL == callback || NULL == context)
    {
        return Common::ComUtility::OnPublicApiReturn(E_POINTER);
    }

    COperationContext* asyncContext = new (std::nothrow) COperationContext();
    if (NULL == asyncContext)
    {
        ServiceGroupReplicationEventSource::GetEvents().Error_0_EmptyOperationDataAsyncEnumerator(reinterpret_cast<uintptr_t>(this));

        return Common::ComUtility::OnPublicApiReturn(E_OUTOFMEMORY);
    }

    HRESULT hr = asyncContext->FinalConstruct();
    if (FAILED(hr))
    {
        ServiceGroupReplicationEventSource::GetEvents().Error_1_EmptyOperationDataAsyncEnumerator(
            reinterpret_cast<uintptr_t>(this),
            hr
            );

        asyncContext->Release();
        return Common::ComUtility::OnPublicApiReturn(hr);
    }

    WriteNoise(
        TraceSubComponentAtomicGroupStateOperationDataAsyncEnumerator, 
        "this={0} - Operation context {1} created",
        this,
        asyncContext
        );

    asyncContext->set_Callback(callback);
    asyncContext->set_IsCompleted();
    asyncContext->set_CompletedSynchronously(TRUE);
    callback->Invoke(asyncContext);
    *context = asyncContext;
    return S_OK;
}

STDMETHODIMP CAtomicGroupStateOperationDataAsyncEnumerator::EndGetNext( 
    __in IFabricAsyncOperationContext* context,
    __out IFabricOperationData** operationData
    )
{
    if (NULL == context || NULL == operationData)
    {
        return Common::ComUtility::OnPublicApiReturn(E_POINTER);
    }

    HRESULT hr = S_OK;
    COperationContext* asyncContext = (COperationContext*)context;

    WriteNoise(
        TraceSubComponentAtomicGroupStateOperationDataAsyncEnumerator, 
        "this={0} - Waiting for operation context {1} to complete",
        this,
        asyncContext
        );

    asyncContext->Wait(INFINITE);

    if (done_)
    {
        WriteNoise(
            TraceSubComponentAtomicGroupStateOperationDataAsyncEnumerator, 
            "this={0} - Returning NULL operation data",
            this
            );

        *operationData = NULL;
    }
    else
    {
        WriteNoise(
            TraceSubComponentAtomicGroupStateOperationDataAsyncEnumerator, 
            "this={0} - Returning single operation data",
            this
            );

        done_ = TRUE;
        operationData_->AddRef();
        *operationData = operationData_;
    }
    return hr;
}

//
// Initialization.
//
STDMETHODIMP CAtomicGroupStateOperationDataAsyncEnumerator:: FinalConstruct(__in const AtomicGroupCopyState& atomicGroupCopyState)
{
    operationData_ = new (std::nothrow) CAtomicGroupStateOperationData();
    if (NULL == operationData_)
    {
        return E_OUTOFMEMORY;
    }
    HRESULT hr = operationData_->FinalConstruct(atomicGroupCopyState);
    ASSERT_IF(FAILED(hr), "Unexpected result");
    return S_OK;
}

//
// Constructor/Destructor.
//
CAtomicGroupStateOperationData::CAtomicGroupStateOperationData(void)
{
    WriteNoise(TraceSubComponentAtomicGroupStateOperationData, "this={0} - ctor", this);
}

CAtomicGroupStateOperationData::~CAtomicGroupStateOperationData(void)
{
    WriteNoise(TraceSubComponentAtomicGroupStateOperationData, "this={0} - dtor", this);
}

//
// IFabricOperationData methods.
//
STDMETHODIMP CAtomicGroupStateOperationData::GetData(__out ULONG* count, __out const FABRIC_OPERATION_DATA_BUFFER** buffers)
{
    if (NULL == count || NULL == buffers)
    {
        return Common::ComUtility::OnPublicApiReturn(E_POINTER);
    }

    *count = 1;
    *buffers = &replicaBuffer_;

    return S_OK;
}

//
// Initialization for this operation data.
//
STDMETHODIMP CAtomicGroupStateOperationData::FinalConstruct(__in const AtomicGroupCopyState& atomicGroupCopyState)
{
    ULONG length = 0;
    Common::ErrorCode error = Common::FabricSerializer::Serialize(&atomicGroupCopyState, buffer_);
    if (!error.IsSuccess())
    {
        return error.ToHResult();
    }

    HRESULT hr = ::SizeTToULong(buffer_.size(), &length);
    if (FAILED(hr))
    {
        return hr;
    }

    ASSERT_IF(0 == length, "Unexpected length");

    replicaBuffer_.BufferSize = length;
    replicaBuffer_.Buffer = buffer_.data();
    return S_OK;
}

CCopyContextStreamDispatcher::CCopyContextStreamDispatcher(
    __in Common::Guid const & serviceGroupGuid,
    __in IFabricOperationDataStream* systemCopyContextEnumerator,
    __in std::map<Common::Guid, COperationDataStream*> & serviceCopyContextEnumerators)
{
    WriteNoise(TraceSubComponentCopyContextStreamDispatcher, "this={0} - ctor", this);

    ASSERT_IF(NULL == systemCopyContextEnumerator, "systemCopyContextEnumerator");
    serviceGroupGuid_ = serviceGroupGuid;
    systemCopyContextEnumerator->AddRef();
    systemCopyContextEnumerator_ = systemCopyContextEnumerator;
    serviceCopyContextEnumerators_ = std::move(serviceCopyContextEnumerators);
}

CCopyContextStreamDispatcher::~CCopyContextStreamDispatcher()
{
    WriteNoise(TraceSubComponentCopyContextStreamDispatcher, "this={0} - dtor", this);

    ASSERT_IF(NULL == systemCopyContextEnumerator_, "systemCopyContextEnumerator_");
    systemCopyContextEnumerator_->Release();
    systemCopyContextEnumerator_ = NULL;
}

HRESULT CCopyContextStreamDispatcher::Dispatch()
{
    HRESULT hr = S_OK;
    BOOLEAN completedSynchronously = FALSE;
    IFabricAsyncOperationContext* context = NULL;
    BOOLEAN isLast = FALSE;            

    while (SUCCEEDED(hr) && !isLast)
    {
        hr = systemCopyContextEnumerator_->BeginGetNext(this, &context);
        if (FAILED(hr))
        {
            ServiceGroupReplicationEventSource::GetEvents().Warning_2_CopyContextStreamDispatcher(
                reinterpret_cast<uintptr_t>(this),
                hr
                );

            FinishDispatch(hr);
        }
        else
        {
            completedSynchronously = context->CompletedSynchronously();
            if (completedSynchronously)
            {
                WriteNoise(TraceSubComponentCopyContextStreamDispatcher, 
                    "this={0} - Completed synchronously",
                    this);
    
                hr = DispatchOperationData(context, &isLast);
                context->Release();
            }
            else
            {
                WriteNoise(TraceSubComponentCopyContextStreamDispatcher, 
                   "this={0} - Completed Asynchronously - context: {1}",
                   this,
                   context);
                
                context->Release();
                return S_OK;
            }
        }
    }
    Release();

    return hr;
}

void STDMETHODCALLTYPE CCopyContextStreamDispatcher::Invoke(
    __in IFabricAsyncOperationContext* context)
{
    if (!context->CompletedSynchronously())
    {
        BOOLEAN isLast = FALSE;
        HRESULT hr = DispatchOperationData(context, &isLast);
        if (SUCCEEDED(hr) && !isLast)
        {
            hr = Dispatch();
        }
        else
        {
            Release();
        }
    }
}

HRESULT CCopyContextStreamDispatcher::FinishDispatch(HRESULT errorCode)
{
    ServiceGroupReplicationEventSource::GetEvents().Info_0_CopyContextStreamDispatcher(
        reinterpret_cast<uintptr_t>(this),
        errorCode,
        serviceCopyContextEnumerators_.size()
        );

    for (std::map<Common::Guid, COperationDataStream*>::iterator iteratorEnum = serviceCopyContextEnumerators_.begin();
         iteratorEnum != serviceCopyContextEnumerators_.end();
         iteratorEnum++)
    {
        if (SUCCEEDED(errorCode))
        {
            ServiceGroupReplicationEventSource::GetEvents().Info_1_CopyContextStreamDispatcher(
                reinterpret_cast<uintptr_t>(this),
                iteratorEnum->first,
                errorCode
                );

            iteratorEnum->second->EnqueueOperationData(S_OK, NULL);
        }
        else
        {
            ServiceGroupReplicationEventSource::GetEvents().Warning_0_CopyContextStreamDispatcher(
                reinterpret_cast<uintptr_t>(this),
                iteratorEnum->first,
                errorCode
                );

            iteratorEnum->second->ForceDrain(errorCode);
        }

        iteratorEnum->second->Release();
    }
    serviceCopyContextEnumerators_.clear();

    return S_OK;
}

HRESULT CCopyContextStreamDispatcher::DispatchOperationData(
    __in IFabricAsyncOperationContext* context,
    __out BOOLEAN* isLast)
{
    ASSERT_IF(NULL == context, "context");

    HRESULT hr = S_OK;

    IFabricOperationData* operationData = NULL;
    hr = systemCopyContextEnumerator_->EndGetNext(context, &operationData);
    if (FAILED(hr))
    {
        ServiceGroupReplicationEventSource::GetEvents().Warning_1_CopyContextStreamDispatcher(
            reinterpret_cast<uintptr_t>(this),
            hr
            );

        FinishDispatch(hr);
        return hr;
    }

    if (NULL == operationData)
    {
        ServiceGroupReplicationEventSource::GetEvents().Info_2_CopyContextStreamDispatcher(reinterpret_cast<uintptr_t>(this));

        FinishDispatch(S_OK);
        *isLast = TRUE;
        return S_OK;
    }

    *isLast = FALSE;

    ULONG bufferCount = 0;
    FABRIC_OPERATION_DATA_BUFFER const * replicaBuffers = NULL;

    hr = operationData->GetData(&bufferCount, &replicaBuffers);
    ASSERT_IF(FAILED(hr), "GetData");
    ASSERT_IF(bufferCount < 1, "bufferCount");
    
    CExtendedOperationDataBuffer extendedReplicaBuffer;
    hr = CExtendedOperationDataBuffer::Read(extendedReplicaBuffer, replicaBuffers[0]);
    if (FAILED(hr))
    {
        ServiceGroupReplicationEventSource::GetEvents().Error_5_CopyContextStreamDispatcher(reinterpret_cast<uintptr_t>(this), hr);

        FinishDispatch(hr);
        return hr;
    }

    Common::Guid partitionId = Common::Guid(extendedReplicaBuffer.PartitionId);
    if (partitionId == serviceGroupGuid_)
    {
        ASSERT_IF(1 != bufferCount, "Wrong copy context operation data");
        return S_OK;
    }

    std::map<Common::Guid, COperationDataStream*>::iterator iteratorStream = serviceCopyContextEnumerators_.find(partitionId);
    ASSERT_IF(serviceCopyContextEnumerators_.end() == iteratorStream, "guid not existing in service group");

    CCopyContextOperationData* opData = new (std::nothrow) CCopyContextOperationData(operationData);
    if (NULL == opData)
    {
        ServiceGroupReplicationEventSource::GetEvents().Error_3_CopyContextStreamDispatcher(reinterpret_cast<uintptr_t>(this));

        FinishDispatch(E_OUTOFMEMORY);
        return E_OUTOFMEMORY;
    }

    hr = iteratorStream->second->EnqueueOperationData(S_OK, opData);
    if (FAILED(hr))
    {
        ServiceGroupReplicationEventSource::GetEvents().Error_4_CopyContextStreamDispatcher(
            reinterpret_cast<uintptr_t>(this),
            iteratorStream->first
            );

        FinishDispatch(hr);
        return hr;
    }

    return S_OK;
}

HRESULT CStatefulServiceGroup::UpdateOperationStreams(__in UpdateOperationStreamsReason reason, __in FABRIC_REPLICA_ROLE newReplicaRole)
{
    ASSERT_IF(FABRIC_REPLICA_ROLE_PRIMARY == currentReplicaRole_, "Unexpected replica role");
    if (NULL == outerReplicationOperationStream_)
    {
        ASSERT_IFNOT(NULL == replicationOperationStreamCallback_, "Unexpected replication operation stream callback");
        ASSERT_IFNOT(NULL == copyOperationStreamCallback_, "Unexpected copy operation stream callback");
        ASSERT_IFNOT(NULL == outerReplicationOperationStream_, "Unexpected replication operation stream");
        ASSERT_IFNOT(NULL == outerCopyOperationStream_, "Unexpected copy operation stream");
        
        ServiceGroupReplicationEventSource::GetEvents().Warning_1_StatefulServiceGroupOperationStreams(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_
            );
        
        return S_FALSE;
    }
    else
    {
        ASSERT_IF(NULL == replicationOperationStreamCallback_, "Unexpected replication operation stream callback");
        ASSERT_IF(NULL == copyOperationStreamCallback_, "Unexpected copy operation stream callback");
        ASSERT_IF(NULL == outerReplicationOperationStream_, "Unexpected replication operation stream");
        ASSERT_IF(NULL == outerCopyOperationStream_, "Unexpected copy operation stream");
    }
    
    ServiceGroupReplicationEventSource::GetEvents().Info_3_StatefulServiceGroupOperationStreams(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this),
        replicaId_,
        currentReplicaRole_
        );
    
    HRESULT hr = S_OK;
    HRESULT hrUpdateOperationStreams = S_OK;
    BOOLEAN isClosing = (CLOSE == reason) || (ABORT == reason);
    BOOLEAN doNotFail = (isClosing || (FABRIC_REPLICA_ROLE_NONE == newReplicaRole));
    BOOLEAN oom = FALSE;
    //
    // Request that each service member update its operation streams.
    //
    PartitionId_StatefulServiceUnit_Iterator iterate;
    for (iterate = innerStatefulServiceUnits_.begin(); innerStatefulServiceUnits_.end() != iterate; iterate++)
    {
        hr = iterate->second->UpdateOperationStreams(isClosing, newReplicaRole);
        if (E_OUTOFMEMORY == hr)
        {
            //
            // Track E_OUTOFMEMORY separately from other HRESULTs. Close and ChangeRole on None can fail on OOM.
            //
            oom = TRUE;
        }
        if (FAILED(hr) && SUCCEEDED(hrUpdateOperationStreams))
        {
            hrUpdateOperationStreams = hr;
        }
    }
    if (FAILED(hrUpdateOperationStreams) && !doNotFail)
    {
        ServiceGroupReplicationEventSource::GetEvents().Info_4_StatefulServiceGroupOperationStreams(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            hrUpdateOperationStreams
            );
        
        return hrUpdateOperationStreams;
    }
    if ((TRUE == oom) && (ABORT != reason))
    {
        //
        // If we seem E_OUTOFMEMORY during close or change role to none, we return the error to indicate that the streams have not drained successfully. 
        //
        ASSERT_IFNOT(TRUE == doNotFail, "Unexpected error propagation. UpdateOperationStreams should already have returned.");
        ServiceGroupReplicationEventSource::GetEvents().Info_4_StatefulServiceGroupOperationStreams(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            E_OUTOFMEMORY
            );

        return E_OUTOFMEMORY;
    }

    if (isClosing || FABRIC_REPLICA_ROLE_PRIMARY == newReplicaRole || FABRIC_REPLICA_ROLE_NONE == newReplicaRole)
    {
        ServiceGroupReplicationEventSource::GetEvents().Info_5_StatefulServiceGroupOperationStreams(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_
            );

        BOOL copyStreamDrainingStarted = FALSE;
        BOOL replicationStreamDrainingStarted = FALSE;
        {
            Common::AcquireWriteLock lockReplicator(lock_);
            
            copyStreamDrainingStarted = copyStreamDrainingStarted_;
            replicationStreamDrainingStarted = replicationStreamDrainingStarted_;
            streamsEnding_ = TRUE;
        }

        //
        // If copy stream was started, wait for its completion.
        //
        if (copyStreamDrainingStarted)
        {
            ServiceGroupReplicationEventSource::GetEvents().Info_6_StatefulServiceGroupOperationStreams(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_
                );
            
            lastCopyOperationDispatched_.WaitOne();
            
            ServiceGroupReplicationEventSource::GetEvents().Info_7_StatefulServiceGroupOperationStreams(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_
                );
        }

        //
        // If replication stream was started, wait for its completion.
        //
        if (replicationStreamDrainingStarted)
        {
            ServiceGroupReplicationEventSource::GetEvents().Info_8_StatefulServiceGroupOperationStreams(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_
                );
            
            lastReplicationOperationDispatched_.WaitOne();
            
            ServiceGroupReplicationEventSource::GetEvents().Info_9_StatefulServiceGroupOperationStreams(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_
                );
        }

        //
        // Request that each service member clears its operation streams.
        //
        for (iterate = innerStatefulServiceUnits_.begin(); innerStatefulServiceUnits_.end() != iterate; iterate++)
        {
            hr = iterate->second->ClearOperationStreams();
            ASSERT_IF(FAILED(hr), "ClearOperationStreams");
        }

        Common::AcquireWriteLock lockReplicator(lock_);

        if (NULL != outerCopyOperationStream_)
        {
            copyOperationStreamCallback_->Release();
            copyOperationStreamCallback_ = NULL;
            copyStreamDrainingStarted_ = FALSE;
            
            outerCopyOperationStream_->Release();
            outerCopyOperationStream_ = NULL;
        }
        if (NULL != outerReplicationOperationStream_)
        {
            replicationOperationStreamCallback_->Release();
            replicationOperationStreamCallback_ = NULL;
            replicationStreamDrainingStarted_ = FALSE;
            
            outerReplicationOperationStream_->Release();
            outerReplicationOperationStream_ = NULL;
        }
    }
    return S_OK;
}

HRESULT CStatefulServiceGroup::GetReplicatorSettingsFromActivationContext(__in ServiceGroup::ReplicatorSettings & replicatorSettings)
{
    useReplicatorSettingsFromConfigPackage_ = FALSE;

    if (activationContext_ == NULL)
    {
        ServiceGroupStatefulEventSource::GetEvents().Info_0_StatefulServiceGroupActivation(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_
            );

        return S_FALSE;
    }

    try
    {
        // when trying to find replicator settings, we look for a configuration package with the name
        //            <service group type name>.ReplicatorSettings
        replicatorSettingsConfigPackageName_ = serviceTypeName_.append(L".").append(ReplicatorSettingsNames::ReplicatorSettings);

        IFabricConfigurationPackage* configPackage = NULL;

        HRESULT hr = activationContext_->GetConfigurationPackage(replicatorSettingsConfigPackageName_.c_str(), &configPackage);

        if (FAILED(hr))
        {
            ServiceGroupStatefulEventSource::GetEvents().Info_2_StatefulServiceGroupActivation(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_,
                replicatorSettingsConfigPackageName_
                );

            return S_FALSE;
        }

        // this indicates that we need to listen for config change events after the replicator has been created
        useReplicatorSettingsFromConfigPackage_ = TRUE;

        hr = replicatorSettings.ReadFromConfigurationPackage(configPackage, activationContext_);

        configPackage->Release();

        if (FAILED(hr))
        {
            ServiceGroupStatefulEventSource::GetEvents().Error_0_StatefulServiceGroupActivation(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_,
                hr,
                replicatorSettingsConfigPackageName_
                );
        }
        else
        {
            ServiceGroupStatefulEventSource::GetEvents().Info_1_StatefulServiceGroupActivation(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this),
                replicaId_,
                replicatorSettingsConfigPackageName_
                );
        }

        return hr;
    }
    catch (std::bad_alloc const &) { return E_OUTOFMEMORY; }
    catch (...) { return E_FAIL; }
}

HRESULT CStatefulServiceGroup::RegisterConfigurationChangeHandler()
{
    if (!useReplicatorSettingsFromConfigPackage_)
    {
        return S_FALSE;
    }

    HRESULT hr = activationContext_->RegisterConfigurationPackageChangeHandler(this, &configurationChangedCallbackHandle_);
    if (FAILED(hr))
    {
        ServiceGroupStatefulEventSource::GetEvents().Error_14_StatefulServiceGroupPackage(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            hr
            );
    }
    else
    {
        ServiceGroupStatefulEventSource::GetEvents().Info_5_StatefulServiceGroupPackage(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_
            );
    }

    return hr;
}

HRESULT CStatefulServiceGroup::UnregisterConfigurationChangeHandler()
{
    if (!useReplicatorSettingsFromConfigPackage_)
    {
        return S_FALSE;
    }

    HRESULT hr = activationContext_->UnregisterConfigurationPackageChangeHandler(configurationChangedCallbackHandle_);
    if (FAILED(hr))
    {
        ServiceGroupStatefulEventSource::GetEvents().Error_15_StatefulServiceGroupPackage(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_,
            hr
            );
    }
    else
    {
        ServiceGroupStatefulEventSource::GetEvents().Info_6_StatefulServiceGroupPackage(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            replicaId_
            );
    }

    return hr;
}

//
// Constructor/Destructor.
//
CStatefulCompositeUndoProgressOperation::CStatefulCompositeUndoProgressOperation( __in_opt CBaseAsyncOperation* parent, __in FABRIC_PARTITION_ID partitionId)
    : CCompositeAsyncOperationAsync(parent, partitionId)
{
    WriteNoise(TraceSubComponentStatefulCompositeUndoProgressOperation, "this={0} - ctor", this);
}

CStatefulCompositeUndoProgressOperation::~CStatefulCompositeUndoProgressOperation(void)
{
    WriteNoise(TraceSubComponentStatefulCompositeUndoProgressOperation, "this={0} - dtor", this);

    for (std::vector<CStatefulServiceUnit*>::iterator iterate = statefulServiceUnits_.begin(); statefulServiceUnits_.end() != iterate; iterate++)
    {
        (*iterate)->Release();
    }
}

//
// Overrides from CCompositeAsyncOperation.
//
STDMETHODIMP CStatefulCompositeUndoProgressOperation::Begin(void)
{
    HRESULT hr = E_FAIL;
    FABRIC_SEQUENCE_NUMBER innerSequenceNumber = _I64_MIN;
    FABRIC_SEQUENCE_NUMBER minSequenceNumber = _I64_MAX;
    ULONG atLeastTwoChanges = 0;

    for (std::vector<CStatefulServiceUnit*>::iterator iterate = statefulServiceUnits_.begin(); statefulServiceUnits_.end() != iterate; iterate++)
    {
        if (!(*iterate)->HasInnerAtomicGroupStateProvider())
        {
            continue;
        }
        hr = (*iterate)->GetLastCommittedSequenceNumber(&innerSequenceNumber);
        if (FAILED(hr))
        {
            ServiceGroupStatefulEventSource::GetEvents().Error_0_StatefulCompositeUndoProgressOperation(
                reinterpret_cast<uintptr_t>(this),
                partitionId_,
                (*iterate)->ServiceName,
                hr
                );

            errorCode_ = hr;
            return hr;
        }
        if (FABRIC_INVALID_SEQUENCE_NUMBER != innerSequenceNumber && 0 != innerSequenceNumber)
        {
            if (minSequenceNumber > innerSequenceNumber)
            {
                minSequenceNumber = innerSequenceNumber;
            }
            atLeastTwoChanges++;
        }
    }

    if (atLeastTwoChanges < 2)
    {
        ServiceGroupStatefulEventSource::GetEvents().Info_0_StatefulCompositeUndoProgressOperation(
            reinterpret_cast<uintptr_t>(this),
            partitionId_
            );

        COperationContext::set_IsCompleted(); 
        if (NULL != parentOperation_)
        {
           parentOperation_->set_IsCompleted();
        }

        return S_OK;
    }

    ServiceGroupStatefulEventSource::GetEvents().Info_1_StatefulCompositeUndoProgressOperation(
        reinterpret_cast<uintptr_t>(this),
        partitionId_,
        minSequenceNumber
        );

    for (std::vector<CStatefulServiceUnit*>::iterator iterate = statefulServiceUnits_.begin(); statefulServiceUnits_.end() != iterate; iterate++)
    {
        if (!(*iterate)->HasInnerAtomicGroupStateProvider())
        {
            continue;
        }

        CAtomicGroupUndoProgressOperation* sUndoProgressAsyncOperation = new (std::nothrow) CAtomicGroupUndoProgressOperation(
            this,
            (IFabricAtomicGroupStateProvider*)(*iterate),
            minSequenceNumber
            );
        if (NULL == sUndoProgressAsyncOperation)
        {
            hr = E_OUTOFMEMORY;
            break;
        }
        hr = sUndoProgressAsyncOperation->FinalConstruct();
        if (FAILED(hr))
        {
            sUndoProgressAsyncOperation->Release();
            break;
        }
        hr = Compose(sUndoProgressAsyncOperation);
        if (FAILED(hr))
        {
            sUndoProgressAsyncOperation->Release();
            break;
        }
    }
    if (FAILED(hr))
    {
        ServiceGroupStatefulEventSource::GetEvents().Error_1_StatefulCompositeUndoProgressOperation(
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            hr
            );

        errorCode_ = hr;
        return hr;
    }
    return CCompositeAsyncOperationAsync::Begin();
}

//
// Additional methods.
//
HRESULT CStatefulCompositeUndoProgressOperation::Register(__in CStatefulServiceUnit* statefulServiceUnit)
{
    HRESULT hr = S_OK;
    try { statefulServiceUnits_.push_back(statefulServiceUnit); }
    catch (std::bad_alloc const &) { hr = E_OUTOFMEMORY; }
    catch (...) { hr = E_FAIL; }
    if (FAILED(hr))
    {
        ServiceGroupStatefulEventSource::GetEvents().Error_2_StatefulCompositeUndoProgressOperation(
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            hr
            );

        return hr;
    }
    statefulServiceUnit->AddRef();
    return S_OK;
}

void ServiceGroup::AtomicGroupStatus::WriteToTextWriter(__in Common::TextWriter & w, Enum val)
{
    w.Write(ATOMIC_GROUP_STATUS_TEXT(val));
}
