// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "ComServicePartitionResult.h"

/*static*/
void ComServicePartitionResult::Create(
    __out ComServicePartitionResult::SPtr& spResult,
    __in KAllocator& allocator,
    __in KString& result
)
{
    spResult = _new(TAG, allocator) ComServicePartitionResult(result);
    KInvariant(spResult != nullptr);
}

ComServicePartitionResult::ComServicePartitionResult(
    __in KString& result
) : _spResult(&result)
{
    _endpoint.Address = static_cast<LPCWSTR>(*_spResult);
    _partition.EndpointCount = 1;
    _partition.Endpoints = &_endpoint;
}

ComServicePartitionResult::~ComServicePartitionResult()
{
}

const FABRIC_RESOLVED_SERVICE_PARTITION *ComServicePartitionResult::get_Partition(void)
{
    return &_partition;
}

HRESULT ComServicePartitionResult::GetEndpoint(
    /* [retval][out] */ const FABRIC_RESOLVED_SERVICE_ENDPOINT **endpoint)
{
    *endpoint = &_endpoint;
    return S_OK;
}

HRESULT ComServicePartitionResult::CompareVersion(
    /* [in] */ IFabricResolvedServicePartitionResult *,
    /* [retval][out] */ LONG *)
{
    return E_NOTIMPL;
}
