// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "stdafx.h"

namespace ManagedPerformanceCounter
{
    typedef struct _FABRIC_COUNTER_INITIALIZER
    {
        UINT CounterId;
        UINT BaseCounterId;
        DWORD CounterType;
        PCWSTR CounterName;
        PCWSTR CounterDescription;
    } FABRIC_COUNTER_INITIALIZER, *PFABRIC_COUNTER_INITIALIZER;

    typedef struct _FABRIC_COUNTER_SET_INITIALIZER
    {
        PCWSTR CounterSetId;
        PCWSTR CounterSetName;
        PCWSTR CounterSetDescription;
        DWORD CounterSetInstanceType;
        ULONG NumCountersInSet;
        PFABRIC_COUNTER_INITIALIZER Counters;
    } FABRIC_COUNTER_SET_INITIALIZER, *PFABRIC_COUNTER_SET_INITIALIZER;
}
