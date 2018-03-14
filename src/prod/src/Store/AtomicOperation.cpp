// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

using namespace Store;

// ***************
// AtomicOperation

AtomicOperation::AtomicOperation() 
    : activityId_()
    , replicationOperations_()
    , lastQuorumAcked_(0)
{ 
}

AtomicOperation::AtomicOperation(
    Guid const & activityId,
    vector<ReplicationOperation> && replicationOperations,
    ::FABRIC_SEQUENCE_NUMBER lastQuorumAck)
    : activityId_(activityId)
    , replicationOperations_(std::move(replicationOperations))
    , lastQuorumAcked_(lastQuorumAck)
{
}

ErrorCode AtomicOperation::TakeReplicationOperations(__out std::vector<ReplicationOperation> & operations)
{
    operations = move(replicationOperations_);
    return ErrorCodeValue::Success;
}
