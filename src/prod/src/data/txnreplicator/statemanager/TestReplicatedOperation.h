// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace StateManagerTests
{
    class TestReplicatedOperation : public TestOperation
    {
    public:
        TestReplicatedOperation();

        TestReplicatedOperation(
            __in LONG64 seqeunceNumber,
            __in_opt Data::Utilities::OperationData const * const metadataPtr,
            __in_opt Data::Utilities::OperationData const * const undoPtr,
            __in_opt Data::Utilities::OperationData const * const redoPtr,
            __in_opt TxnReplicator::OperationContext const * const operationContextPtr);

        ~TestReplicatedOperation();

    public:
        __declspec(property(get = get_SequenceNumber)) LONG64 SequenceNumber;
        LONG64 get_SequenceNumber() const;

    private:
        LONG64 sequenceNumber_ = LONG64_MIN;
    };
}
