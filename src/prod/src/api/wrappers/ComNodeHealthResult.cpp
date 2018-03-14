// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Api;
using namespace Common;
using namespace ServiceModel;

using namespace std;

ComNodeHealthResult::ComNodeHealthResult(
    NodeHealth const & nodeHealth)
    : IFabricNodeHealthResult()
    , ComUnknownBase()
    , heap_()
    , nodeHealth_()
{
    nodeHealth_ = heap_.AddItem<FABRIC_NODE_HEALTH>();
    nodeHealth.ToPublicApi(heap_, *nodeHealth_);
}

ComNodeHealthResult::~ComNodeHealthResult()
{
}

const FABRIC_NODE_HEALTH * STDMETHODCALLTYPE ComNodeHealthResult::get_NodeHealth()
{
    return nodeHealth_.GetRawPointer();
}
