// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel;
using namespace Api;
using namespace Naming;

ComServiceDescriptionResult::ComServiceDescriptionResult(PartitionedServiceDescriptor && nativeResult)
    : heap_(),
    description_()
{
    description_ = heap_.AddItem<FABRIC_SERVICE_DESCRIPTION>();
    nativeResult.ToPublicApi(heap_, *description_);
}

const FABRIC_SERVICE_DESCRIPTION * STDMETHODCALLTYPE ComServiceDescriptionResult::get_Description(void)
{
    return description_.GetRawPointer();
}

