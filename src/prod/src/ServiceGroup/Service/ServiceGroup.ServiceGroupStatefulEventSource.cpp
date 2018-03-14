// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ServiceGroup;

// Define statics in anonymous namespace
namespace
{
    static std::unique_ptr<ServiceGroupStatefulEventSource> singletonServiceGroupStatefulEventSource;
    static INIT_ONCE initOnceServiceGroupStatefulEventSource;

    static BOOL CALLBACK InitServiceGroupStatefulEventSourceFunction(PINIT_ONCE, PVOID, PVOID *)
    {
        singletonServiceGroupStatefulEventSource = Common::make_unique<ServiceGroupStatefulEventSource>();
        return TRUE;
    }
}

ServiceGroupStatefulEventSource const & ServiceGroupStatefulEventSource::GetEvents()
{
    BOOL  bStatus = FALSE;             
    PVOID lpContext = NULL;            

    bStatus = ::InitOnceExecuteOnce(   
        &initOnceServiceGroupStatefulEventSource,
        InitServiceGroupStatefulEventSourceFunction,
        NULL,                             
        &lpContext);                      
                                          
    ASSERT_IF(!bStatus, "Failed to initialize ServiceGroupStatefulEventSource singleton");
    return *singletonServiceGroupStatefulEventSource;
}
