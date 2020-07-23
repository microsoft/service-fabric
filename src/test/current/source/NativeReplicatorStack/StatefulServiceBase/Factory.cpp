// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace TXRStatefulServiceBase;

Factory::Factory(__in CreateReplicaCallback createReplicaCallback)
    : createReplicaCallBack_(createReplicaCallback)
{
}
        
Factory::~Factory()
{
}

shared_ptr<Factory> Factory::Create(__in CreateReplicaCallback createReplicaCallback)
{
    return shared_ptr<Factory>(new Factory(createReplicaCallback));
}

StatefulServiceBaseCPtr Factory::CreateReplica(
    FABRIC_PARTITION_ID partitionId,
    FABRIC_REPLICA_ID replicaId,
    ComponentRootSPtr const & root)
{
    StatefulServiceBaseCPtr service = createReplicaCallBack_(partitionId, replicaId, root);
    return service;
}
