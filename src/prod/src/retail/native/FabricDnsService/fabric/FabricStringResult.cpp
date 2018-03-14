// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "FabricStringResult.h"

/*static*/
void FabricStringResult::Create(
    __out FabricStringResult::SPtr& spResult,
    __in KAllocator& allocator,
    __in KStringView strAddress
    )
{
    spResult = _new(TAG, allocator) FabricStringResult(strAddress);
    KInvariant(spResult != nullptr);
}

FabricStringResult::FabricStringResult(
    __in KStringView strAddress
    ) : _strAddress(strAddress)
{
}

FabricStringResult::~FabricStringResult()
{
}

LPCWSTR FabricStringResult::get_String()
{
    return static_cast<LPCWSTR>(_strAddress);
}
