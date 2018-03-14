// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ServiceGroup;

// Define statics in anonymous namespace
namespace
{
    static std::unique_ptr<ServiceGroupCommonEventSource> singletonServiceGroupCommonEventSource;
    static INIT_ONCE initOnceServiceGroupCommonEventSource;

    static BOOL CALLBACK InitServiceGroupCommonEventSourceFunction(PINIT_ONCE, PVOID, PVOID *)
    {
        singletonServiceGroupCommonEventSource = Common::make_unique<ServiceGroupCommonEventSource>();
        return TRUE;
    }
}

ServiceGroupCommonEventSource const & ServiceGroupCommonEventSource::GetEvents()
{
    BOOL  bStatus = FALSE;
    PVOID lpContext = NULL;
    
    bStatus = ::InitOnceExecuteOnce(
        &initOnceServiceGroupCommonEventSource,
        InitServiceGroupCommonEventSourceFunction,
        NULL,
        &lpContext);

    ASSERT_IF(!bStatus, "Failed to initialize ServiceGroupCommonEventSource singleton");
    return *singletonServiceGroupCommonEventSource;
}
