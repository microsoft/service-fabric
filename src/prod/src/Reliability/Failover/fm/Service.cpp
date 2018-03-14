// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;

StringLiteral const TraceReplicaSet("ReplicaSet");

FailoverManagerService::FailoverManagerService(
    Common::Guid const & partitionId,
    FABRIC_REPLICA_ID replicaId,
    Federation::NodeInstance const & nodeInstance,
    Common::ComponentRoot const & root)
    : KeyValueStoreReplica(
        partitionId,
        replicaId)
    , RootedObject(root)
    , nodeInstance_(nodeInstance)
{
    TraceInfo(
        TraceTaskCodes::FM,
        TraceReplicaSet,
        "FailoverManagerService::ctor: node instance = {0}",
        nodeInstance_);
}

FailoverManagerService::~FailoverManagerService()
{
    TraceInfo(
        TraceTaskCodes::FM,
        TraceReplicaSet,
        "FailoverManagerService::~dtor: trace ID = {0}",
        nodeInstance_);
}

ErrorCode FailoverManagerService::OnOpen(ComPointer<IFabricStatefulServicePartition> const & servicePartition)
{
    TraceInfo(
        TraceTaskCodes::FM,
        TraceReplicaSet,
        "OnOpen(): node instance = {0}",
        nodeInstance_);

    OpenReplicaCallback snapshot = OnOpenReplicaCallback;
    if (snapshot)
    {
        auto componentRoot = shared_from_this();
        auto failoverManagerServiceSPtr = static_pointer_cast<FailoverManagerService>(componentRoot);
        snapshot(failoverManagerServiceSPtr);
    }

    servicePartition_ = servicePartition;

    return ErrorCodeValue::Success;
}

ErrorCode FailoverManagerService::OnChangeRole(::FABRIC_REPLICA_ROLE newRole, __out wstring & serviceLocation)
{
    TraceInfo(
        TraceTaskCodes::FM,
        TraceReplicaSet,
        "OnChangeRole(): role = {0} at node instance = {1}",
        static_cast<int>(newRole),
        nodeInstance_);

    serviceLocation = nodeInstance_.ToString();

    ChangeRoleCallback snapshot = OnChangeRoleCallback;
    if (snapshot)
    {
        snapshot(newRole, servicePartition_);
    }

    return ErrorCodeValue::Success;
}

ErrorCode FailoverManagerService::OnClose()
{
    TraceInfo(
        TraceTaskCodes::FM,
        TraceReplicaSet,
        "OnClose() called at node instance = {0}",
        nodeInstance_);

    CloseReplicaCallback snapshot = OnCloseReplicaCallback;
    OnCloseReplicaCallback = nullptr;
    OnChangeRoleCallback = nullptr;
    OnOpenReplicaCallback = nullptr;

    if (snapshot)
    {
        snapshot();
    }

    return ErrorCodeValue::Success;
}

void FailoverManagerService::OnAbort()
{
    TraceInfo(
        TraceTaskCodes::FM,
        TraceReplicaSet,
        "OnAbort() called at node instance = {0}",
        nodeInstance_);

    OnClose();
}
