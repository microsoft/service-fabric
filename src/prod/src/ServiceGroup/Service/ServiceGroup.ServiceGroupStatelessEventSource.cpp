// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ServiceGroup;

// Define statics in anonymous namespace
namespace
{
    static std::unique_ptr<ServiceGroupStatelessEventSource> singletonServiceGroupStatelessEventSource;
    static INIT_ONCE initOnceServiceGroupStatelessEventSource;

    static BOOL CALLBACK InitServiceGroupStatelessEventSourceFunction(PINIT_ONCE, PVOID, PVOID *)
    {
        singletonServiceGroupStatelessEventSource = Common::make_unique<ServiceGroupStatelessEventSource>();
        return TRUE;
    }
}

ServiceGroupStatelessEventSource const & ServiceGroupStatelessEventSource::GetEvents()
{
    BOOL  bStatus = FALSE;
    PVOID lpContext = NULL;

    bStatus = ::InitOnceExecuteOnce(   
        &initOnceServiceGroupStatelessEventSource,
        InitServiceGroupStatelessEventSourceFunction,
        NULL,
        &lpContext);

    ASSERT_IF(!bStatus, "Failed to initialize ServiceGroupStatelessEventSource singleton");
    return *singletonServiceGroupStatelessEventSource;
}
