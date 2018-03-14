// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;

Guid PerformanceProviderDefinition::identifier_(L"B8225977-FE30-42B6-A071-E8F591988F9C");

PerformanceProviderDefinition* PerformanceProviderDefinition::singletonInstance_ = new PerformanceProviderDefinition();

PerformanceProviderDefinition::PerformanceProviderDefinition(void)
{
}

PerformanceProviderDefinition::~PerformanceProviderDefinition(void)
{
}

void PerformanceProviderDefinition::AddCounterSetDefinition(PerformanceCounterSetDefinition & performanceCounterSetDefinition)
{
    AcquireExclusiveLock lock(counterSetDefinitionsLock_);

    auto it = counterSetDefinitions_.find(performanceCounterSetDefinition.Identifier);

    ASSERT_IF(end(counterSetDefinitions_) != it, "Counter set already exists");

    counterSetDefinitions_.insert(std::make_pair(performanceCounterSetDefinition.Identifier, performanceCounterSetDefinition));
}

void PerformanceProviderDefinition::AddCounterSetDefinition(PerformanceCounterSetDefinition && performanceCounterSetDefinition)
{
    AcquireExclusiveLock lock(counterSetDefinitionsLock_);

    auto it = counterSetDefinitions_.find(performanceCounterSetDefinition.Identifier);

    ASSERT_IF(end(counterSetDefinitions_) != it, "Counter set already exists");

    counterSetDefinitions_.insert(std::make_pair(performanceCounterSetDefinition.Identifier, std::move(performanceCounterSetDefinition)));
}
        
PerformanceCounterSetDefinition const & PerformanceProviderDefinition::GetCounterSetDefinition(Guid const & counterSetId)
{
    AcquireReadLock lock(counterSetDefinitionsLock_);

    auto it = counterSetDefinitions_.find(counterSetId);

    ASSERT_IF(end(counterSetDefinitions_) == it, "Counter set not defined");

    return it->second;
}
