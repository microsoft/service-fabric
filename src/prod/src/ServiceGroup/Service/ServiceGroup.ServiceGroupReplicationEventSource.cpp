// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ServiceGroup;

// Define statics in anonymous namespace
namespace
{
    static std::unique_ptr<ServiceGroupReplicationEventSource> singletonServiceGroupReplicationEventSource;
    static INIT_ONCE initOnceServiceGroupReplicationEventSource;

    static BOOL CALLBACK InitServiceGroupReplicationEventSourceFunction(PINIT_ONCE, PVOID, PVOID *)
    {
        singletonServiceGroupReplicationEventSource = Common::make_unique<ServiceGroupReplicationEventSource>();
        return TRUE;
    }
}

ServiceGroupReplicationEventSource const & ServiceGroupReplicationEventSource::GetEvents()
{
    BOOL  bStatus = FALSE;             
    PVOID lpContext = NULL;            

    bStatus = ::InitOnceExecuteOnce(   
        &initOnceServiceGroupReplicationEventSource,
        InitServiceGroupReplicationEventSourceFunction,
        NULL,                             
        &lpContext);                      

    ASSERT_IF(!bStatus, "Failed to initialize ServiceGroupReplicationEventSource singleton");
    return *singletonServiceGroupReplicationEventSource;
}
