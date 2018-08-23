// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "ComFabricServiceDescriptionResult.h"

LPCWSTR PartitionNames[] = { L"0", L"1", L"2" };

/*static*/
void ComFabricServiceDescriptionResult::Create(
    __out ComFabricServiceDescriptionResult::SPtr& spResult,
    __in KAllocator& allocator,
    __in FABRIC_PARTITION_SCHEME scheme
)
{
    spResult = _new(TAG, allocator) ComFabricServiceDescriptionResult(scheme);
    KInvariant(spResult != nullptr);
}

ComFabricServiceDescriptionResult::ComFabricServiceDescriptionResult(
    __in FABRIC_PARTITION_SCHEME scheme
)
{
    ZeroMemory(&_int64Desc, sizeof(_int64Desc));
    ZeroMemory(&_namedDesc, sizeof(_namedDesc));
    ZeroMemory(&_statelessDesc, sizeof(_statelessDesc));
    ZeroMemory(&_statefulDesc, sizeof(_statefulDesc));
    ZeroMemory(&_desc, sizeof(_desc));

    if (scheme == FABRIC_PARTITION_SCHEME_SINGLETON)
    {
        _statelessDesc.PartitionScheme = scheme;
        _statelessDesc.PartitionSchemeDescription = nullptr;
        _desc.Kind = FABRIC_SERVICE_DESCRIPTION_KIND_STATELESS;
        _desc.Value = static_cast<void*>(&_statelessDesc);
    }
    else
    {
        _statefulDesc.PartitionScheme = scheme;
        _desc.Kind = FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL;
        _desc.Value = static_cast<void*>(&_statefulDesc);

        if (scheme == FABRIC_PARTITION_SCHEME_NAMED)
        {
            _statefulDesc.PartitionSchemeDescription = static_cast<void*>(&_namedDesc);
            _namedDesc.PartitionCount = ARRAYSIZE(PartitionNames);
            _namedDesc.Names = PartitionNames;
        }
        else if (scheme == FABRIC_PARTITION_SCHEME_UNIFORM_INT64_RANGE)
        {
            _statefulDesc.PartitionSchemeDescription = static_cast<void*>(&_int64Desc);
            _int64Desc.PartitionCount = 12;
            _int64Desc.LowKey = -10000;
            _int64Desc.HighKey = 10000;
        }
    }
}

ComFabricServiceDescriptionResult::~ComFabricServiceDescriptionResult()
{
}

const FABRIC_SERVICE_DESCRIPTION* ComFabricServiceDescriptionResult::get_Description()
{
    return &_desc;
}

