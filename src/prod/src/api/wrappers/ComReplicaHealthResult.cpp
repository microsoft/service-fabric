// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Api;
using namespace Common;
using namespace ServiceModel;

using namespace std;

ComReplicaHealthResult::ComReplicaHealthResult(
    ReplicaHealth const & replicaHealth)
    : IFabricReplicaHealthResult()
    , ComUnknownBase()
    , heap_()
    , replicaHealth_()
{
    replicaHealth_ = heap_.AddItem<FABRIC_REPLICA_HEALTH>();
    replicaHealth.ToPublicApi(heap_, *replicaHealth_);
}

ComReplicaHealthResult::~ComReplicaHealthResult()
{
}

const FABRIC_REPLICA_HEALTH * STDMETHODCALLTYPE ComReplicaHealthResult::get_ReplicaHealth()
{
    return replicaHealth_.GetRawPointer();
}
