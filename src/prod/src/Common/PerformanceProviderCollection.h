// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include <map>

#include "PerformanceProvider.h"

namespace Common
{
    class PerformanceProviderCollection
    {
    public:
        static std::shared_ptr<PerformanceProvider> GetOrCreateProvider(Guid const & providerId);

    private:
        PerformanceProviderCollection(void);
        ~PerformanceProviderCollection(void);

        static std::map<Guid, std::weak_ptr<PerformanceProvider>> providers_;

        static RwLock lock_;
    };
}
