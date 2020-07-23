// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace V1ReplPerf;

shared_ptr<Factory> Factory::Create()
{
    return shared_ptr<Factory>(new Factory());
}

ServiceSPtr Factory::CreateReplica(
    FABRIC_PARTITION_ID partitionId,
    FABRIC_REPLICA_ID replicaId,
    Common::ComponentRootSPtr const & root)
{
    auto service = make_shared<Service>(
        partitionId,
        replicaId,
        root);

    return service;
}
