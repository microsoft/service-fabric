// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

namespace Api
{
    ComGetSystemServiceListResult::ComGetSystemServiceListResult(
        vector<ServiceQueryResult> && systemServiceList)
        : systemServiceList_(),
        heap_()
    {
        systemServiceList_ = heap_.AddItem<FABRIC_SERVICE_QUERY_RESULT_LIST>();
        ComConversionUtility::ToPublicList<
            ServiceQueryResult,
            FABRIC_SERVICE_QUERY_RESULT_ITEM,
            FABRIC_SERVICE_QUERY_RESULT_LIST>(
                heap_, 
                move(systemServiceList), 
                *systemServiceList_);
    }

    const FABRIC_SERVICE_QUERY_RESULT_LIST *STDMETHODCALLTYPE ComGetSystemServiceListResult::get_SystemServiceList(void)
    {
        return systemServiceList_.GetRawPointer();
    }
}
