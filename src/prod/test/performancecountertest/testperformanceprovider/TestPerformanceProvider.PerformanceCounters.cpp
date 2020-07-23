// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include "TestPerformanceProvider.PerformanceCounters.h"

namespace TestPerformanceProvider
{
    INITIALIZE_TEST_COUNTER_SET(TestCounterSet1)
    INITIALIZE_TEST_COUNTER_SET(TestCounterSet2)
}

using namespace TestPerformanceProvider;

Guid TestPerformanceProviderDefinition::identifier_(L"9E9EE26B-6103-408E-B1DD-0DB9DE3B4430");

TestPerformanceProviderDefinition* TestPerformanceProviderDefinition::singletonInstance_ = new TestPerformanceProviderDefinition();

TestPerformanceProviderDefinition::TestPerformanceProviderDefinition(void)
{
}

TestPerformanceProviderDefinition::~TestPerformanceProviderDefinition(void)
{
}

void TestPerformanceProviderDefinition::AddCounterSetDefinition(PerformanceCounterSetDefinition & performanceCounterSetDefinition)
{
    AcquireExclusiveLock lock(counterSetDefinitionsLock_);

    auto it = counterSetDefinitions_.find(performanceCounterSetDefinition.Identifier);

    ASSERT_IF(end(counterSetDefinitions_) != it, "Counter set already exists");

    counterSetDefinitions_.insert(std::make_pair(performanceCounterSetDefinition.Identifier, performanceCounterSetDefinition));
}

void TestPerformanceProviderDefinition::AddCounterSetDefinition(PerformanceCounterSetDefinition && performanceCounterSetDefinition)
{
    AcquireExclusiveLock lock(counterSetDefinitionsLock_);

    auto it = counterSetDefinitions_.find(performanceCounterSetDefinition.Identifier);

    ASSERT_IF(end(counterSetDefinitions_) != it, "Counter set already exists");

    counterSetDefinitions_.insert(std::make_pair(performanceCounterSetDefinition.Identifier, std::move(performanceCounterSetDefinition)));
}
        
PerformanceCounterSetDefinition const & TestPerformanceProviderDefinition::GetCounterSetDefinition(Guid const & counterSetId)
{
    // todo: make read lock
    AcquireExclusiveLock lock(counterSetDefinitionsLock_);

    auto it = counterSetDefinitions_.find(counterSetId);

    ASSERT_IF(end(counterSetDefinitions_) == it, "Counter set not defined");

    return it->second;
}
