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
    ComGetServiceGroupMemberTypeListResult::ComGetServiceGroupMemberTypeListResult(
        vector<ServiceGroupMemberTypeQueryResult> && ServiceGroupMemberTypeList)
        : ServiceGroupMemberTypeList_(),
        heap_()
    {
        ServiceGroupMemberTypeList_ = heap_.AddItem<FABRIC_SERVICE_GROUP_MEMBER_TYPE_QUERY_RESULT_LIST>();
        ComConversionUtility::ToPublicList<
            ServiceGroupMemberTypeQueryResult,
            FABRIC_SERVICE_GROUP_MEMBER_TYPE_QUERY_RESULT_ITEM,
            FABRIC_SERVICE_GROUP_MEMBER_TYPE_QUERY_RESULT_LIST>(
                heap_, 
                move(ServiceGroupMemberTypeList),
                *ServiceGroupMemberTypeList_);
    }

    const FABRIC_SERVICE_GROUP_MEMBER_TYPE_QUERY_RESULT_LIST *STDMETHODCALLTYPE ComGetServiceGroupMemberTypeListResult::get_ServiceGroupMemberTypeList(void)
    {
        return ServiceGroupMemberTypeList_.GetRawPointer();
    }
}
