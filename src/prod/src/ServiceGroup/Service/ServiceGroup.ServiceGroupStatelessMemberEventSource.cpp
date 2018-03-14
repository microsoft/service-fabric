// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ServiceGroup;

// Define statics in anonymous namespace
namespace
{
    static std::unique_ptr<ServiceGroupStatelessMemberEventSource> singletonServiceGroupStatelessMemberEventSource;
    static INIT_ONCE initOnceServiceGroupStatelessMemberEventSource;

    static BOOL CALLBACK InitServiceGroupStatelessMemberEventSourceFunction(PINIT_ONCE, PVOID, PVOID *)
    {
        singletonServiceGroupStatelessMemberEventSource = Common::make_unique<ServiceGroupStatelessMemberEventSource>();
        return TRUE;
    }
}

ServiceGroupStatelessMemberEventSource const & ServiceGroupStatelessMemberEventSource::GetEvents()
{
    BOOL  bStatus = FALSE;
    PVOID lpContext = NULL;

    bStatus = ::InitOnceExecuteOnce(   
        &initOnceServiceGroupStatelessMemberEventSource,
        InitServiceGroupStatelessMemberEventSourceFunction,
        NULL,                             
        &lpContext);                      

    ASSERT_IF(!bStatus, "Failed to initialize ServiceGroupStatelessMemberEventSource singleton");
    return *singletonServiceGroupStatelessMemberEventSource;
}
