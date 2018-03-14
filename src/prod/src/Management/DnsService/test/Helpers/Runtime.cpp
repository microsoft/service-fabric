// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Runtime.h"

/*static*/
Runtime* Runtime::Create()
{
    Runtime* pRuntime = (Runtime*)ExAllocatePoolWithTag(
        GetProcessHeap(),
        POOL_TYPE::NonPagedPool,
        sizeof(Runtime),
        TAG);


    pRuntime->Runtime::Runtime(GUID_NULL);

    return pRuntime;
}

Runtime::Runtime(
    __in const GUID& id
    ) : KtlSystemBase(id)
{
    SetDefaultSystemThreadPoolUsage(FALSE);

    SetStrictAllocationChecks(TRUE);

    KtlSystemBase::Activate(KtlSystem::InfiniteTime);
}

Runtime::~Runtime()
{
}

void Runtime::Delete()
{
    const ULONG timeoutInMs = 5000;
    ULONG timeout = GetStrictAllocationChecks() ? timeoutInMs : KtlSystem::InfiniteTime;
    KtlSystemBase::Deactivate(timeout);
    this->Runtime::~Runtime();
    ExFreePool(GetProcessHeap(), this);
}
