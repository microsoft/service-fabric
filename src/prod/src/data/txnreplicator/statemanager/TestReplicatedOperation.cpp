// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace StateManagerTests;

TestReplicatedOperation::TestReplicatedOperation()
    : TestOperation()
{
    
}

TestReplicatedOperation::TestReplicatedOperation(
    __in LONG64 seqeunceNumber,
    __in_opt Data::Utilities::OperationData const * const metadataPtr,
    __in_opt Data::Utilities::OperationData const * const undoPtr,
    __in_opt Data::Utilities::OperationData const * const redoPtr,
    __in_opt TxnReplicator::OperationContext const * const operationContextPtr)
    : TestOperation(metadataPtr, undoPtr, redoPtr, operationContextPtr)
    , sequenceNumber_(seqeunceNumber)
{
    
}

TestReplicatedOperation::~TestReplicatedOperation()
{
    
}

LONG64 TestReplicatedOperation::get_SequenceNumber() const
{
    return sequenceNumber_;
}
