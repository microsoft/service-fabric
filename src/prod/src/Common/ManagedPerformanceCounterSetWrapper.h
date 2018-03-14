// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    //This class holds the reference of shared_ptr<PerformanceCounterSet> object to prevent its premature memory release. Managed performance counter setup 
    //apis create the PerformanceCounterSet object and later uses it to create counter set instance. Since this object doesn't have any reference in the 
    //native code, it can get freed up and managed code might end up having dangling pointer. This class holds the reference to PerformanceCounterSet 
    //object to prevent such scenario.
    class ManagedPerformanceCounterSetWrapper
    {
        DENY_COPY(ManagedPerformanceCounterSetWrapper)

    public:
        ManagedPerformanceCounterSetWrapper(std::shared_ptr<PerformanceCounterSet> perfCounterSet);
        ~ManagedPerformanceCounterSetWrapper(void);

        __declspec(property(get = get_PerfCounterSet)) std::shared_ptr<PerformanceCounterSet> PerfCounterSet;
        inline std::shared_ptr<PerformanceCounterSet> get_PerfCounterSet()  { return perfCounterSet_; }

    private:
        std::shared_ptr<PerformanceCounterSet> perfCounterSet_;
    };
}
