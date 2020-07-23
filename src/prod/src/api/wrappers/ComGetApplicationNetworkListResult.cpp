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
    ComGetApplicationNetworkListResult::ComGetApplicationNetworkListResult(
        vector<ApplicationNetworkQueryResult> && applicationNetworkList,
        PagingStatusUPtr && pagingStatus)
        : applicationNetworkList_()
        , pagingStatus_()
        , heap_()
    {
        applicationNetworkList_ = heap_.AddItem<FABRIC_APPLICATION_NETWORK_QUERY_RESULT_LIST>();
        ComConversionUtility::ToPublicList<
            ApplicationNetworkQueryResult,
            FABRIC_APPLICATION_NETWORK_QUERY_RESULT_ITEM,
            FABRIC_APPLICATION_NETWORK_QUERY_RESULT_LIST>(
                heap_, 
                move(applicationNetworkList),
                *applicationNetworkList_);

        if (pagingStatus)
        {
            pagingStatus_ = heap_.AddItem<FABRIC_PAGING_STATUS>();
            pagingStatus->ToPublicApi(heap_, *pagingStatus_);
        }
    }

    const FABRIC_APPLICATION_NETWORK_QUERY_RESULT_LIST *STDMETHODCALLTYPE ComGetApplicationNetworkListResult::get_ApplicationNetworkList()
    {
        return applicationNetworkList_.GetRawPointer();
    }

    const FABRIC_PAGING_STATUS *STDMETHODCALLTYPE ComGetApplicationNetworkListResult::get_PagingStatus()
    {
        return pagingStatus_.GetRawPointer();
    }
}
