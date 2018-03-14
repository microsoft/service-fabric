// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data;
using namespace StateManager;

NTSTATUS OperationDataEnumerator::Create(
    __in KAllocator& allocator,
    __out SPtr & result)
{
    result = _new(OPERATION_DATA_ENUMERATOR, allocator) OperationDataEnumerator();

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

Utilities::OperationData::CSPtr OperationDataEnumerator::Current()
{
    KArray<KBuffer::CSPtr> buffers(GetThisAllocator());

    NTSTATUS status = buffers.Append(bufferCollection_[currentIndex_]);
    KInvariant(NT_SUCCESS(status));

    return Utilities::OperationData::Create(buffers, GetThisAllocator());
}

bool OperationDataEnumerator::MoveNext()
{
    // Make sure read phase starts and any add operation should asset
    if (isReadCalled_ == false)
    {
        isReadCalled_ = true;
    }

    if (static_cast<ULONG>(currentIndex_ + 1) < bufferCollection_.Count())
    {
        currentIndex_++;
        return true;
    }

    return false;
}

NTSTATUS OperationDataEnumerator::Add(__in KBuffer const & buffer)
{
    ASSERT_IFNOT(
        isReadCalled_ == false,
        "OperationDataEnumerator: Add: Reader should not start at this point. IsReadCalled: {0}",
        isReadCalled_);
    return bufferCollection_.Append(&buffer);
}

OperationDataEnumerator::OperationDataEnumerator()
    : bufferCollection_(GetThisAllocator())
{
}

OperationDataEnumerator::~OperationDataEnumerator()
{
}
