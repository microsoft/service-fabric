// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data::Utilities;
using namespace TStoreTests;

TestTransactionContext::TestTransactionContext()
{
}

TestTransactionContext::~TestTransactionContext()
{
}

TestTransactionContext::TestTransactionContext(
    __in LONG64 seqeunceNumber,
    __in_opt Data::Utilities::OperationData const * const metadata,
    __in_opt Data::Utilities::OperationData const * const undo,
    __in_opt Data::Utilities::OperationData const * const redo,
    __in_opt TxnReplicator::OperationContext const * const operationContext,
    __in_opt LONG64 stateProviderId)
    :metaData_(metadata),
    sequenceNumber_(seqeunceNumber),
    undo_(undo),
    redo_(redo),
    operationContext_(operationContext),
    stateproviderId_(stateProviderId)
{

}

