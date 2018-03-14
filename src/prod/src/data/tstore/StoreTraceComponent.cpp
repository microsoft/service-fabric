// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::TStore;
using namespace Common;

NTSTATUS StoreTraceComponent::Create(
    __in KGuid partitionId,
    __in FABRIC_REPLICA_ID replicaId,
    __in LONG64 stateProviderId,
    __in KAllocator & allocator,
    __out SPtr & result)
{
    NTSTATUS status;

    SPtr output = _new(STORE_TRACE_COMPONENT_TAG, allocator) StoreTraceComponent(partitionId, replicaId, stateProviderId);

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

StoreTraceComponent::StoreTraceComponent(
    __in KGuid partitionId,
    __in FABRIC_REPLICA_ID replicaId,
    __in LONG64 stateProviderId)
{
    traceTagString_.FromLONGLONG(replicaId);
    traceTagString_.Concat(KStringView(L":"));
    traceTagString_.FromLONGLONG(stateProviderId, TRUE);
    traceTagString_.SetNullTerminator();

    traceTag_ = ToStringLiteral(traceTagString_);

    assertTagString_.FromGUID(partitionId);
    assertTagString_.Concat(KStringView(L":"));
    assertTagString_.FromLONGLONG(replicaId, TRUE);
    assertTagString_.Concat(KStringView(L":"));
    assertTagString_.FromLONGLONG(stateProviderId, TRUE);
    assertTagString_.SetNullTerminator();

    assertTag_ = ToStringLiteral(assertTagString_);

    partitionId_ = Common::Guid(partitionId);
}

StoreTraceComponent::~StoreTraceComponent()
{
}
