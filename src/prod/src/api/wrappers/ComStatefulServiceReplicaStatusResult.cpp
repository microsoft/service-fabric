// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace Store;

ComStatefulServiceReplicaStatusResult::ComStatefulServiceReplicaStatusResult(
    IStatefulServiceReplicaStatusResultPtr const & impl)
    : impl_(impl)
{
}

ComStatefulServiceReplicaStatusResult::~ComStatefulServiceReplicaStatusResult()
{
}

const FABRIC_REPLICA_STATUS_QUERY_RESULT * ComStatefulServiceReplicaStatusResult::get_Result()
{
    auto result = heap_.AddItem<FABRIC_REPLICA_STATUS_QUERY_RESULT>();
    impl_->GetResult(heap_, *result);
    return result.GetRawPointer();
}
