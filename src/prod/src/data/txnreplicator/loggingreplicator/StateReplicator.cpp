// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::LoggingReplicator;
using namespace TxnReplicator;
using namespace Data::Utilities;

StateReplicator::StateReplicator(__in IStateReplicator & fabricReplicator)
    : IStateReplicator()
    , KObject()
    , KShared()
    , fabricReplicator_(&fabricReplicator)
{
}

StateReplicator::~StateReplicator()
{
}

StateReplicator::SPtr StateReplicator::Create(
    __in IStateReplicator & fabricReplicator,
    __in KAllocator & allocator)
{
    StateReplicator * pointer = _new(V1REPLICATOR_TAG, allocator) StateReplicator(fabricReplicator);

    if (pointer == nullptr)
    {
        throw Exception(STATUS_INSUFFICIENT_RESOURCES);
    }

    return StateReplicator::SPtr(pointer);
}

Awaitable<LONG64> StateReplicator::ReplicateAsync(
    __in OperationData const & replicationData,
    __out LONG64 & logicalSequenceNumber)
{
    UNREFERENCED_PARAMETER(replicationData);
    UNREFERENCED_PARAMETER(logicalSequenceNumber);
    co_await suspend_never{};
    co_return 1;
}

IOperationStream::SPtr StateReplicator::GetCopyStream()
{
    return{};
}

IOperationStream::SPtr StateReplicator::GetReplicationStream()
{
    return{};
}
