// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data::StateManager;
using namespace TxnReplicator;

NTSTATUS NamedOperationContext::Create(
    __in OperationContext const* operationContext, 
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in KAllocator& allocator, 
    __out SPtr& result) noexcept
{
    result = _new(NAMED_OPERATION_CONTEXT_TAG, allocator) NamedOperationContext(operationContext, stateProviderId);
    if (result == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(result->Status()))
    {
        return (SPtr(Ktl::Move(result)))->Status();
    }

    return STATUS_SUCCESS;
}

OperationContext::CSPtr NamedOperationContext::get_OperationContext() const
{
    return operationContextCSPtr_;
}

FABRIC_STATE_PROVIDER_ID NamedOperationContext::get_StateProviderId() const
{
    return stateProviderId_;
}

NamedOperationContext::NamedOperationContext(
    __in OperationContext const* operationContext, 
    __in FABRIC_STATE_PROVIDER_ID stateProviderId)
    : TxnReplicator::OperationContext()
    , operationContextCSPtr_(operationContext)
    , stateProviderId_(stateProviderId)
{
}

NamedOperationContext::~NamedOperationContext()
{
}

