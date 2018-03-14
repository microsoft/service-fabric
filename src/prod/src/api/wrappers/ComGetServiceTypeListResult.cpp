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
    ComGetServiceTypeListResult::ComGetServiceTypeListResult(
        vector<ServiceTypeQueryResult> && serviceTypeList)
        : serviceTypeList_(),
        heap_()
    {
        serviceTypeList_ = heap_.AddItem<FABRIC_SERVICE_TYPE_QUERY_RESULT_LIST>();
        ComConversionUtility::ToPublicList<
            ServiceTypeQueryResult,
            FABRIC_SERVICE_TYPE_QUERY_RESULT_ITEM,
            FABRIC_SERVICE_TYPE_QUERY_RESULT_LIST>(
                heap_, 
                move(serviceTypeList), 
                *serviceTypeList_);
    }

    const FABRIC_SERVICE_TYPE_QUERY_RESULT_LIST *STDMETHODCALLTYPE ComGetServiceTypeListResult::get_ServiceTypeList(void)
    {
        return serviceTypeList_.GetRawPointer();
    }
}
