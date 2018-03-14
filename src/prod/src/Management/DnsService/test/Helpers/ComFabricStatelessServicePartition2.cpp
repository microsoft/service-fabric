// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "ComFabricStatelessServicePartition2.h"

/*static*/
void ComFabricStatelessServicePartition2::Create(
    __out ComFabricStatelessServicePartition2::SPtr& spManager,
    __in KAllocator& allocator
)
{
    spManager = _new(TAG, allocator) ComFabricStatelessServicePartition2();
    KInvariant(spManager != nullptr);
}

ComFabricStatelessServicePartition2::ComFabricStatelessServicePartition2()
{
}

ComFabricStatelessServicePartition2::~ComFabricStatelessServicePartition2()
{
}

HRESULT ComFabricStatelessServicePartition2::ReportInstanceHealth(
    __in const FABRIC_HEALTH_INFORMATION *healthInfo
)
{
    UNREFERENCED_PARAMETER(healthInfo);
    return S_OK;
}
