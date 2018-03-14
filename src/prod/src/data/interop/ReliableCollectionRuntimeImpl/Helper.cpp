// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace TxnReplicator;
using namespace Data::TStore;
using namespace Data::Interop;

StoreTransactionReadIsolationLevel::Enum IsolationHelper::GetIsolationLevel(Transaction& txn, OperationType operationType)
{
    ASSERT_IF(operationType == OperationType::Invalid, "unexpected operation type");

    // All multi-entity read operations are SNAPSHOT.
    if (operationType == OperationType::MultiEntity)
    {
        return StoreTransactionReadIsolationLevel::Snapshot;
    }

    return txn.IsPrimaryTransaction ? StoreTransactionReadIsolationLevel::ReadRepeatable : StoreTransactionReadIsolationLevel::Snapshot;
}

extern "C" void Buffer_Release(BufferHandle handle)
{
    KBuffer::SPtr kBuffer;
    kBuffer.Attach((KBuffer*)handle);
}
