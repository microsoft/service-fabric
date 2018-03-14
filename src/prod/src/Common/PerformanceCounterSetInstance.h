// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class PerformanceCounterSet;

    class PerformanceCounterSetInstance
    {
        DENY_COPY(PerformanceCounterSetInstance)

    public:
        PerformanceCounterSetInstance(std::shared_ptr<PerformanceCounterSet> const & PerformanceCounterSet, std::wstring const & instanceName, bool allocateCounterMemory);
        ~PerformanceCounterSetInstance(void);

        PerformanceCounterData & GetCounter(PerformanceCounterId id);

        HRESULT PerformanceCounterSetInstance::SetCounterRefValue(PerformanceCounterId id, void *addr);

    private:

        PPERF_COUNTERSET_INSTANCE counterSetInstance_;

        std::shared_ptr<PerformanceCounterSet> counterSet_;
        
        std::map<PerformanceCounterId, PerformanceCounterData *> counterIdToCounterData_;

        std::vector<PerformanceCounterData> counterData_;

        bool allocateCounterMemory_;
    };

    typedef std::shared_ptr<PerformanceCounterSetInstance> PerformanceCounterSetInstanceSPtr;
}
