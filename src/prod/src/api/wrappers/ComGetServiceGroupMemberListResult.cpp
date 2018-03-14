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
    ComGetServiceGroupMemberListResult::ComGetServiceGroupMemberListResult(
        vector<ServiceGroupMemberQueryResult> && ServiceGroupMemberList)
        : serviceGroupMemberList_(),
        heap_()
    {
        serviceGroupMemberList_ = heap_.AddItem<FABRIC_SERVICE_GROUP_MEMBER_QUERY_RESULT_LIST>();
        ComConversionUtility::ToPublicList<
            ServiceGroupMemberQueryResult,
            FABRIC_SERVICE_GROUP_MEMBER_QUERY_RESULT_ITEM,
            FABRIC_SERVICE_GROUP_MEMBER_QUERY_RESULT_LIST>(
                heap_, 
                move(ServiceGroupMemberList), 
                *serviceGroupMemberList_);
    }

    const FABRIC_SERVICE_GROUP_MEMBER_QUERY_RESULT_LIST *STDMETHODCALLTYPE ComGetServiceGroupMemberListResult::get_ServiceGroupMemberList(void)
    {
        return serviceGroupMemberList_.GetRawPointer();
    }
}
