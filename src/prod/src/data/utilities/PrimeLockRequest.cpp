// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data::Utilities;

#define PRIMELOCKREQUEST_TAG 'trLP'

PrimeLockRequest::PrimeLockRequest(
    __in LockManager& lockManager,
    __in LockMode::Enum lockMode)
    : lockManagerSPtr_(&lockManager),
    lockMode_(lockMode)
{    
}

PrimeLockRequest::~PrimeLockRequest()
{
}

NTSTATUS
PrimeLockRequest::Create(
    __in LockManager& lockManager,
    __in LockMode::Enum lockMode,
    __in KAllocator& allocator,
    __out PrimeLockRequest::SPtr& result)
{
    NTSTATUS status;
    auto output = _new(PRIMELOCKREQUEST_TAG, allocator) PrimeLockRequest(lockManager, lockMode);

    if (!output)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = output->Status();
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    result = Ktl::Move(output);
    return STATUS_SUCCESS;
}
