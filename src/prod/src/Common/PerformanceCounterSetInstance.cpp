// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;

PerformanceCounterSetInstance::PerformanceCounterSetInstance(PerformanceCounterSetSPtr const & PerformanceCounterSet, std::wstring const & instanceName, bool allocateMemory) :
    counterSet_(PerformanceCounterSet),
    counterSetInstance_(nullptr),
    allocateCounterMemory_(allocateMemory)
{
    // create a new instance of the counter set
    counterSetInstance_ = ::PerfCreateInstance(counterSet_->ProviderHandle, &counterSet_->CounterSetId.AsGUID(), instanceName.c_str(), 0);

    if (NULL == counterSetInstance_)
    {
        Common::Assert::TestAssert("PerfCreateInstance '{0}' failed with {1}", instanceName, ErrorCode::FromWin32Error());
    }

    // skip allocating backing storage for the managed counters. This will be done later when managed counter allocates individual counter.
    if (false == allocateCounterMemory_)
    {
        return;
    }

    // set up the backing store for all counter values in the counter set instance
    PerformanceCounterData init = {0};
    counterData_.insert(begin(counterData_), counterSet_->CounterTypes.size(), init);

    size_t count = 0;
    for (auto it = begin(counterSet_->CounterTypes); end(counterSet_->CounterTypes) != it; ++it)
    {
        PerformanceCounterData & data = counterData_.at(count);
        count++;

        counterIdToCounterData_.insert(std::make_pair(it->first, &data));

        if (NULL != counterSetInstance_)
        {
            // connect PerformanceCounterSet instance to backing store
            auto error = ::PerfSetCounterRefValue(counterSet_->ProviderHandle, counterSetInstance_, it->first, &data.RawValue);

            TESTASSERT_IF(ERROR_SUCCESS != error, "PerfSetCounterRefValue failed");
        }
    }

}

PerformanceCounterSetInstance::~PerformanceCounterSetInstance(void)
{
    if (nullptr != counterSetInstance_)
    {
        // destroy the instance
        auto error = ::PerfDeleteInstance(counterSet_->ProviderHandle, counterSetInstance_);
        ASSERT_IF(ERROR_SUCCESS != error, "PerfDeleteInstance failed");
        counterSetInstance_ = nullptr;
    }
}

PerformanceCounterData & PerformanceCounterSetInstance::GetCounter(PerformanceCounterId id)
{
    auto it = counterIdToCounterData_.find(id);
    
    ASSERT_IF(end(counterIdToCounterData_) == it, "Unknown counter id");

    // return a direct reference to the backing store
    return *(it->second);
}

HRESULT PerformanceCounterSetInstance::SetCounterRefValue(PerformanceCounterId id, void *addr)
{
    return HRESULT_FROM_WIN32(::PerfSetCounterRefValue(counterSet_->ProviderHandle, counterSetInstance_, id, addr));
}
