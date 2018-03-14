// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace StateManagerTests;

TestOperation::TestOperation()
{
}

TestOperation::TestOperation(
    __in_opt Data::Utilities::OperationData const * const metadataPtr,
    __in_opt Data::Utilities::OperationData const * const undoPtr,
    __in_opt Data::Utilities::OperationData const * const redoPtr,
    __in_opt TxnReplicator::OperationContext const * const operationContextPtr)
    : metadataSPtr_(metadataPtr)
    , undoSPtr_(undoPtr)
    , redoSPtr_(redoPtr)
    , operationContextSPtr_(operationContextPtr)
{
}

TestOperation::TestOperation(
    __in Type type, 
    __in LONG64 stateProviderId,
    __in bool nullOperation, 
    __in KAllocator & allocator)
{
    if (type == Type::Null)
    {
        CODING_ERROR_ASSERT(nullOperation == true);

        metadataSPtr_ = nullptr;
        undoSPtr_ = nullptr;
        redoSPtr_ = nullptr;
        operationContextSPtr_ = nullptr;

        return;
    }

    if (type == Type::Random)
    {
        metadataSPtr_ = GenerateOperationData(allocator);
        undoSPtr_ = GenerateOperationData(allocator);
        redoSPtr_ = GenerateOperationData(allocator);
        operationContextSPtr_ = GenerateOperationContext(nullOperation, stateProviderId, allocator);
        return;
    }

    CODING_ERROR_ASSERT(false);
}

TestOperation::~TestOperation()
{
}

bool TestOperation::operator==(TestOperation const& otherOperation) const
{
    BOOL metadatasMatch = TestHelper::Equals(metadataSPtr_.RawPtr(), otherOperation.Metadata.RawPtr());

    BOOL redoDatasMatch = TestHelper::Equals(redoSPtr_.RawPtr(), otherOperation.Redo.RawPtr());

    // TODO: Add support for undo verification.
    // TestHelper::Equals(metadataSPtr_.RawPtr(), otherOperation.Undo.RawPtr());

    // TODO: Add support for operationContext verification
    // TestHelper::Equals(metadataSPtr_.RawPtr(), otherOperation.Context.RawPtr());

    return (metadatasMatch && redoDatasMatch) == TRUE;
}

Data::Utilities::OperationData::CSPtr TestOperation::get_MetadataOperation() const
{
    return metadataSPtr_;
}

Data::Utilities::OperationData::CSPtr TestOperation::get_UndoOperation() const
{
    return undoSPtr_;
}

Data::Utilities::OperationData::CSPtr TestOperation::get_RedoOperation() const
{
    return redoSPtr_;
}

TxnReplicator::OperationContext::CSPtr TestOperation::get_OperationContext() const
{
    return operationContextSPtr_;
}

Data::Utilities::OperationData::CSPtr TestOperation::GenerateOperationData(__in KAllocator & allocator)
{
    auto divineChoice = random_.Next() % 3;

    if (divineChoice == 0)
    {
        return nullptr;
    }

    if (divineChoice == 1)
    {
        auto emptyOperationData = Data::Utilities::OperationData::Create(allocator);
        return emptyOperationData.RawPtr();
    }

    if (divineChoice == 2)
    {
        int numberOfBuffers = random_.Next() % 16;

        auto operationData = Data::Utilities::OperationData::Create(allocator);

        for (int i = 0; i < numberOfBuffers; i++)
        {
            KBuffer::SPtr buffer;
            NTSTATUS status = KBuffer::Create(4, buffer, allocator);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            KMemCpySafe(buffer->GetBuffer(), 4, defaultBuffer_, 4);

            operationData->Append(*buffer);
        }

        return operationData.RawPtr();
    }

    CODING_ERROR_ASSERT(false);
    return nullptr;
}

TxnReplicator::OperationContext::CSPtr TestOperation::GenerateOperationContext(
    __in bool nullOperationContext,
    __in LONG64 stateProviderId, 
    __in KAllocator & allocator)
{
    if (nullOperationContext)
    {
        return nullptr;
    }

    return TestOperationContext::Create(stateProviderId, allocator).RawPtr();
}
