// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ServiceGroup;

// Define statics in anonymous namespace
namespace
{
    static std::unique_ptr<ServiceGroupStatefulMemberEventSource> singletonServiceGroupStatefulMemberEventSource;
    static INIT_ONCE initOnceServiceGroupStatefulMemberEventSource;

    static BOOL CALLBACK InitServiceGroupStatefulMemberEventSourceFunction(PINIT_ONCE, PVOID, PVOID *)
    {
        singletonServiceGroupStatefulMemberEventSource = Common::make_unique<ServiceGroupStatefulMemberEventSource>();
        return TRUE;
    }
}

ServiceGroupStatefulMemberEventSource const & ServiceGroupStatefulMemberEventSource::GetEvents()
{
    BOOL  bStatus = FALSE;             
    PVOID lpContext = NULL;            

    bStatus = ::InitOnceExecuteOnce(   
        &initOnceServiceGroupStatefulMemberEventSource,
        InitServiceGroupStatefulMemberEventSourceFunction,
        NULL,                             
        &lpContext);                      

    ASSERT_IF(!bStatus, "Failed to initialize ServiceGroupStatefulMemberEventSource singleton");
    return *singletonServiceGroupStatefulMemberEventSource;
}
