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
    ComGetNetworkApplicationListResult::ComGetNetworkApplicationListResult(
        vector<NetworkApplicationQueryResult> && networkApplicationList,
        PagingStatusUPtr && pagingStatus)
        : networkApplicationList_()
        , pagingStatus_()
        , heap_()
    {
        networkApplicationList_ = heap_.AddItem<FABRIC_NETWORK_APPLICATION_QUERY_RESULT_LIST>();
        ComConversionUtility::ToPublicList<
            NetworkApplicationQueryResult,
            FABRIC_NETWORK_APPLICATION_QUERY_RESULT_ITEM,
            FABRIC_NETWORK_APPLICATION_QUERY_RESULT_LIST>(
                heap_, 
                move(networkApplicationList),
                *networkApplicationList_);

        if (pagingStatus)
        {
            pagingStatus_ = heap_.AddItem<FABRIC_PAGING_STATUS>();
            pagingStatus->ToPublicApi(heap_, *pagingStatus_);
        }
    }

    const FABRIC_NETWORK_APPLICATION_QUERY_RESULT_LIST *STDMETHODCALLTYPE ComGetNetworkApplicationListResult::get_NetworkApplicationList()
    {
        return networkApplicationList_.GetRawPointer();
    }

    const FABRIC_PAGING_STATUS *STDMETHODCALLTYPE ComGetNetworkApplicationListResult::get_PagingStatus()
    {
        return pagingStatus_.GetRawPointer();
    }
}
