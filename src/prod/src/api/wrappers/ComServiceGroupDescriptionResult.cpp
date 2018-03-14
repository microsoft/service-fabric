// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Api
{
    using namespace Common;
    using namespace Naming;

    ComServiceGroupDescriptionResult::ComServiceGroupDescriptionResult(ServiceGroupDescriptor const & nativeResult)
        : heap_(),
        description_()
    {
        description_ = heap_.AddItem<FABRIC_SERVICE_GROUP_DESCRIPTION>();
        nativeResult.ToPublicApi(heap_, *description_);
    }

    const FABRIC_SERVICE_GROUP_DESCRIPTION * STDMETHODCALLTYPE ComServiceGroupDescriptionResult::get_Description(void)
    {
        return description_.GetRawPointer();
    }
}
