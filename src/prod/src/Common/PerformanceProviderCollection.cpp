// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "PerformanceProviderCollection.h"

using namespace Common;

RwLock PerformanceProviderCollection::lock_;
std::map<Guid, std::weak_ptr<PerformanceProvider>> PerformanceProviderCollection::providers_;

PerformanceProviderCollection::PerformanceProviderCollection(void)
{
}


PerformanceProviderCollection::~PerformanceProviderCollection(void)
{
}

std::shared_ptr<PerformanceProvider> PerformanceProviderCollection::GetOrCreateProvider(Guid const & providerId)
{
    AcquireExclusiveLock lock(lock_);

    auto it = providers_.find(providerId);

    if (end(providers_) != it)
    {
        auto cached = it->second.lock();
        if (nullptr != cached)
        {
            return cached;
        }
    }

    auto newProvider = std::make_shared<PerformanceProvider>(providerId);

    providers_[providerId] = newProvider;

    return newProvider;
}
