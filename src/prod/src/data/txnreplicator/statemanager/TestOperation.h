// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace StateManagerTests    
{
    class TestOperation
    {
    public:
        enum Type
        {
            Null = 0,
            Random = 1,         
        };

    public:
        TestOperation();
        TestOperation(__in Type type, __in LONG64 stateProviderId, __in bool nullOperationContext, __in KAllocator & allocator);
        TestOperation(
            __in_opt Data::Utilities::OperationData const * const metadataPtr,
            __in_opt Data::Utilities::OperationData const * const undoPtr,
            __in_opt Data::Utilities::OperationData const * const redoPtr,
            __in_opt TxnReplicator::OperationContext const * const operationContextPtr);

        virtual ~TestOperation();

    public:
        bool operator==(TestOperation const & otherOperation) const;

    public:
        __declspec(property(get = get_MetadataOperation)) Data::Utilities::OperationData::CSPtr Metadata;
        Data::Utilities::OperationData::CSPtr get_MetadataOperation() const;

        __declspec(property(get = get_UndoOperation)) Data::Utilities::OperationData::CSPtr Undo;
        Data::Utilities::OperationData::CSPtr get_UndoOperation() const;

        __declspec(property(get = get_RedoOperation)) Data::Utilities::OperationData::CSPtr Redo;
        Data::Utilities::OperationData::CSPtr get_RedoOperation() const;

        __declspec(property(get = get_OperationContext)) TxnReplicator::OperationContext::CSPtr Context;
        TxnReplicator::OperationContext::CSPtr get_OperationContext() const;

    private:
        Data::Utilities::OperationData::CSPtr GenerateOperationData(__in KAllocator & allocator);
        TxnReplicator::OperationContext::CSPtr GenerateOperationContext(
            __in bool nullOperationContext,
            __in LONG64 stateProviderId, 
            __in KAllocator & allocator);

    private:
        Data::Utilities::OperationData::CSPtr metadataSPtr_ = nullptr;
        Data::Utilities::OperationData::CSPtr undoSPtr_ = nullptr;
        Data::Utilities::OperationData::CSPtr redoSPtr_ = nullptr;
        TxnReplicator::OperationContext::CSPtr operationContextSPtr_ = nullptr;

        Common::Random random_ = Common::Random();
        byte defaultBuffer_[4]{ 1,0,1,0 };
    };
}
