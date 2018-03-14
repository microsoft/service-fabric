// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class PerformanceProvider
    {
    public:
        PerformanceProvider(Guid const & providerId);
        ~PerformanceProvider(void);

        inline HANDLE Handle() { return providerHandle_; }

    private:

        Guid providerId_;
        HANDLE providerHandle_;
    };

    typedef std::shared_ptr<PerformanceProvider> PerformanceProviderSPtr;
}

