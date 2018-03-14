// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

PerformanceCounterSet::PerformanceCounterSet(Guid const & providerId, Guid const & counterSetId, PerformanceCounterSetInstanceType::Enum instanceType) :
    instanceType_(instanceType),
    counterSetId_(counterSetId),
    providerId_(providerId),
    isInitialized_(false),
    nextCounterInstanceId_(0)
{
    provider_ = PerformanceProviderCollection::GetOrCreateProvider(providerId_);
}

PerformanceCounterSet::~PerformanceCounterSet(void)
{
}

void PerformanceCounterSet::AddCounter(PerformanceCounterId counterId, PerformanceCounterType::Enum counterType)
{
    ASSERT_IF(end(counterTypes_) != counterTypes_.find(counterId), "Counter already exisits.");

    counterTypes_[counterId] = counterType;
}

HRESULT PerformanceCounterSet::CreateCounterSetInstance(std::wstring const & instanceName, bool allocateCounterMemory, bool avoidAssert, PerformanceCounterSetInstance* &counterSetInstance)
{
    HRESULT hresult;
    counterSetInstance = nullptr;
    if ((hresult = EnsureInitialized(avoidAssert)) == S_OK)
    {
        counterSetInstance = new PerformanceCounterSetInstance(shared_from_this(), instanceName, allocateCounterMemory);
    }
    return hresult;
}

std::shared_ptr<PerformanceCounterSetInstance> PerformanceCounterSet::CreateCounterSetInstance(std::wstring const & instanceName)
{
    PerformanceCounterSetInstance *perfcounterSetInstance; 
    CreateCounterSetInstance(instanceName, true, false, perfcounterSetInstance);
    std::shared_ptr<PerformanceCounterSetInstance> sp(perfcounterSetInstance);
    return sp;
}

HRESULT PerformanceCounterSet::EnsureInitialized(bool avoidAssert)
{
    if (!isInitialized_)
    {
        AcquireExclusiveLock lock(initializationLock_);

        if (!isInitialized_)
        {
            std::vector<BYTE> counterSetTemplate;

            counterSetTemplate.insert(
                begin(counterSetTemplate),
                sizeof(::PERF_COUNTERSET_INFO) +                        // counter set header
                counterTypes_.size() * sizeof(::PERF_COUNTERSET_INFO),  // counter infos for each counter
                0);

            ::PPERF_COUNTERSET_INFO counterSetInfo = reinterpret_cast<::PPERF_COUNTERSET_INFO>(counterSetTemplate.data());

            counterSetInfo->CounterSetGuid = counterSetId_.AsGUID();
            counterSetInfo->ProviderGuid = providerId_.AsGUID();
            counterSetInfo->NumCounters = static_cast<ULONG>(counterTypes_.size());
            counterSetInfo->InstanceType = instanceType_;

            ::PPERF_COUNTER_INFO counterInfo = reinterpret_cast<::PPERF_COUNTER_INFO>(&counterSetTemplate[sizeof(::PERF_COUNTERSET_INFO)]);

            ULONG offset = 0;

            for (auto it = begin(counterTypes_); end(counterTypes_) != it; ++it)
            {
                counterInfo->CounterId = it->first;
                counterInfo->Type = it->second;
                counterInfo->Attrib = PERF_ATTRIB_BY_REFERENCE;
                counterInfo->Size = sizeof(PerformanceCounterValue);
                counterInfo->DetailLevel = PERF_DETAIL_NOVICE;
                counterInfo->Scale = 0; // scale is exponential, thus 0 is a multiplier of 1, todo: make this customizable
                counterInfo->Offset = offset; 

                offset += counterInfo->Size;

                counterInfo++;
            }

            auto error = ::PerfSetCounterSetInfo(ProviderHandle, counterSetInfo, static_cast<ULONG>(counterSetTemplate.size()));

            if (avoidAssert)
            {
                if (error != ERROR_SUCCESS)
                {
                    return HRESULT_FROM_WIN32(error);
                }
            }
            else
            {
                ASSERT_IF(ERROR_SUCCESS != error, "PerfSetCounterSetInfo failed");
            }
            isInitialized_ = true;
        }
    }
    return S_OK;
}
