// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "PerformanceCounterApi.h"
#include "ManagedPerformanceCounterSetWrapper.h"

using namespace ManagedPerformanceCounter;
using namespace Common;

//Should be called before creating instances.
HRESULT FabricPerfCounterCreateCounterSet(
    __RPC__in PFABRIC_COUNTER_SET_INITIALIZER counterSetInitializer, 
    __RPC__deref_out_opt HANDLE* handle)
{
    Common::Guid counterSetId(counterSetInitializer->CounterSetId);
    Common::PerformanceCounterSetDefinition counterSetDefinition(
                counterSetId,
                counterSetInitializer->CounterSetName,
                counterSetInitializer->CounterSetDescription,
                (Common::PerformanceCounterSetInstanceType::Enum) counterSetInitializer->CounterSetInstanceType);
    
    PFABRIC_COUNTER_INITIALIZER counterInitializer = counterSetInitializer->Counters;
    for (ULONG i = 0; i < counterSetInitializer->NumCountersInSet; i++, counterInitializer++)
    {        
        if (counterInitializer->BaseCounterId == UINT_MAX)
        {
            Common::PerformanceCounterDefinition counterDefinition(
                    counterInitializer->CounterId,
                    (Common::PerformanceCounterType::Enum) counterInitializer->CounterType,
                    counterInitializer->CounterName,
                    counterInitializer->CounterDescription);
            counterSetDefinition.AddCounterDefinition(std::move(counterDefinition));
        }
        else
        {
            Common::PerformanceCounterDefinition counterDefinition(
                    counterInitializer->CounterId,
                    counterInitializer->BaseCounterId,
                    (Common::PerformanceCounterType::Enum) counterInitializer->CounterType,
                    counterInitializer->CounterName,
                    counterInitializer->CounterDescription);
            counterSetDefinition.AddCounterDefinition(std::move(counterDefinition));
        }
    }
    
    Common::PerformanceProviderDefinition::Singleton()->AddCounterSetDefinition(std::move(counterSetDefinition));
    Common::PerformanceCounterSetDefinition const & definition = Common::PerformanceProviderDefinition::Singleton()->GetCounterSetDefinition(counterSetId);
    
    std::shared_ptr<Common::PerformanceCounterSet> counterSet = std::make_shared<Common::PerformanceCounterSet>(
                Common::PerformanceProviderDefinition::Singleton()->Identifier,
                definition.Identifier,
                definition.InstanceType);
    auto counters = definition.CounterDefinitions;
    for (auto it = begin(counters); end(counters) != it; ++it)
    {
        counterSet->AddCounter(it->second.Identifier, it->second.Type);
    }
    *handle = new ManagedPerformanceCounterSetWrapper(counterSet);
    return S_OK;
}

HRESULT FabricPerfCounterCreateCounterSetInstance(
    __RPC__in HANDLE hCounterSet, 
    __RPC__in PCWSTR instanceName,
    __RPC__deref_out_opt HANDLE* handle)
{
    HRESULT hresult = E_NOTIMPL;
    PerformanceCounterSetInstance *instance = nullptr;
    ManagedPerformanceCounterSetWrapper *counterSetPtr = (ManagedPerformanceCounterSetWrapper *)hCounterSet;
    std::shared_ptr<Common::PerformanceCounterSet> counterSet = counterSetPtr->PerfCounterSet;
    //Create CounterSet without allocating backing storage for performance counters.
    hresult = counterSet->CreateCounterSetInstance(instanceName, false, true, instance);
    *handle = instance;
    return hresult;
}

HRESULT FabricPerfCounterSetPerformanceCounterRefValue(
    __RPC__in HANDLE hCounterSetInstance,
    __RPC__in PerformanceCounterId id,
    __RPC__in void *counterAddress)
{
    Common::PerformanceCounterSetInstance *PerformanceCounterSetInstance = (Common::PerformanceCounterSetInstance *)hCounterSetInstance;
    return PerformanceCounterSetInstance->SetCounterRefValue(id, counterAddress);
}

HRESULT FabricPerfCounterDeleteCounterSetInstance(__RPC__in HANDLE hCounterSetInstance)
{
    Common::PerformanceCounterSetInstance *genericCounterSetInstance = (Common::PerformanceCounterSetInstance*)hCounterSetInstance;
    delete genericCounterSetInstance;
    return S_OK;
}

HRESULT FabricPerfCounterDeleteCounterSet(__RPC__in HANDLE hCounterSet)
{
    ManagedPerformanceCounterSetWrapper *counterSetPtr = (ManagedPerformanceCounterSetWrapper *)hCounterSet;
    delete counterSetPtr;
    return S_OK;
}
