// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::LogRecordLib;
using namespace Data::LoggingReplicator;
using namespace TxnReplicator;
using namespace Data::Utilities;

RecoveredOrCopiedCheckpointState::RecoveredOrCopiedCheckpointState()
    : KObject()
    , KShared()
    , sequenceNumber_(Constants::InvalidLsn)
{
}

RecoveredOrCopiedCheckpointState::~RecoveredOrCopiedCheckpointState()
{
}
 
RecoveredOrCopiedCheckpointState::SPtr RecoveredOrCopiedCheckpointState::Create(__in KAllocator & allocator)
{
    RecoveredOrCopiedCheckpointState * pointer = _new(LOGGINGREPLICATOR_TAG, allocator)RecoveredOrCopiedCheckpointState();

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return RecoveredOrCopiedCheckpointState::SPtr(pointer);
}

void RecoveredOrCopiedCheckpointState::Update(LONG64 lsn)
{
    sequenceNumber_ = lsn;
}
