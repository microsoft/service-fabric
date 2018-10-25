// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Common;
using namespace Data::Utilities;
using namespace TxnReplicator;
using namespace Data::StateManager;

NTSTATUS StateManagerTransactionContext::Create(
    __in LONG64 transactionId,
    __in StateManagerLockContext& stateManagerLockContext,
    __in OperationType::Enum operationType,
    __in KAllocator& allocator,
    __out SPtr& result) noexcept
{
    ASSERT_IF(
        transactionId == Constants::InvalidTransactionId,
        "StateManagerTransactionContext should not have InvalidTransactionId, TxnId: {0}, operationType: {1}",
        transactionId,
        static_cast<LONG64>(operationType));
  
    result = _new(SM_TRANSACTIONCONTEXT_TAG, allocator) StateManagerTransactionContext(transactionId, stateManagerLockContext, operationType);
    if (result == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(result->Status()))
    {
        // null result while fetching failure status with no extra AddRefs or Releases; should opt very well
        return (SPtr(Ktl::Move(result)))->Status();
    }

    return STATUS_SUCCESS;
}

void StateManagerTransactionContext::Unlock()
{
    THROW_ON_FAILURE(lockContextSPtr_->Unlock(*this));
}

StateManagerTransactionContext::StateManagerTransactionContext(
    __in LONG64 transactionId, 
    __in StateManagerLockContext & stateManagerLockContext, 
    __in OperationType::Enum operationType)
    : TxnReplicator::LockContext()
    , transactionId_(transactionId)
    , lockContextSPtr_(&stateManagerLockContext)
    , operationType_(operationType)
{
}

StateManagerTransactionContext::~StateManagerTransactionContext()
{
}
