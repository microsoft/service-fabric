// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Api;
using namespace Common;
using namespace ServiceModel;

using namespace std;

ComPartitionHealthResult::ComPartitionHealthResult(
    PartitionHealth const & partitionHealth)
    : IFabricPartitionHealthResult()
    , ComUnknownBase()
    , heap_()
    , partitionHealth_()
{
    partitionHealth_ = heap_.AddItem<FABRIC_PARTITION_HEALTH>();
    partitionHealth.ToPublicApi(heap_, *partitionHealth_);
}

ComPartitionHealthResult::~ComPartitionHealthResult()
{
}

const FABRIC_PARTITION_HEALTH * STDMETHODCALLTYPE ComPartitionHealthResult::get_PartitionHealth()
{
    return partitionHealth_.GetRawPointer();
}
