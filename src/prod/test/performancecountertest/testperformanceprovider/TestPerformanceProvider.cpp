// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "StdAfx.h"

#include "TestPerformanceProvider.PerformanceCounters.h"

using namespace TestPerformanceProvider;

int __cdecl wmain(int argc, __in_ecount( argc ) wchar_t ** argv)
{
    if (argc != 7)
    {
        return -1;
    }

    std::wstring counterSet(argv[1]);
    std::wstring instanceId(argv[2]);
    
    std::wstring value1(argv[3]);
    std::wstring value2(argv[4]);
    std::wstring value3(argv[5]);
    std::wstring value4(argv[6]);

    if (counterSet == L"1")
    {
        auto instance = TestCounterSet1::CreateInstance(instanceId);
    
        instance->Counter1.Value = Common::Int64_Parse(value1);
        instance->Counter2.Value = Common::Int64_Parse(value2);
        instance->Counter3.Value = Common::Int64_Parse(value3);
        instance->Counter4.Value = Common::Int64_Parse(value4);

        getchar();
    }
    else if (counterSet == L"2")
    {
        auto instance = TestCounterSet2::CreateInstance(instanceId);
    
        instance->Counter1.Value = Common::Int64_Parse(value1);
        instance->Counter2.Value = Common::Int64_Parse(value2);
        instance->Counter3.Value = Common::Int64_Parse(value3);
        instance->Counter4.Value = Common::Int64_Parse(value4);

        getchar();
    }

    return 0;
}

