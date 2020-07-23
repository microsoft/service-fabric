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
    ComGetNetworkNodeListResult::ComGetNetworkNodeListResult(
        vector<NetworkNodeQueryResult> && networkNodeList,
        PagingStatusUPtr && pagingStatus)
        : networkNodeList_()
        , pagingStatus_()
        , heap_()
    {
        networkNodeList_ = heap_.AddItem<FABRIC_NETWORK_NODE_QUERY_RESULT_LIST>();
        ComConversionUtility::ToPublicList<
            NetworkNodeQueryResult,
            FABRIC_NETWORK_NODE_QUERY_RESULT_ITEM,
            FABRIC_NETWORK_NODE_QUERY_RESULT_LIST>(
                heap_, 
                move(networkNodeList),
                *networkNodeList_);

        if (pagingStatus)
        {
            pagingStatus_ = heap_.AddItem<FABRIC_PAGING_STATUS>();
            pagingStatus->ToPublicApi(heap_, *pagingStatus_);
        }
    }

    const FABRIC_NETWORK_NODE_QUERY_RESULT_LIST *STDMETHODCALLTYPE ComGetNetworkNodeListResult::get_NetworkNodeList()
    {
        return networkNodeList_.GetRawPointer();
    }

    const FABRIC_PAGING_STATUS *STDMETHODCALLTYPE ComGetNetworkNodeListResult::get_PagingStatus()
    {
        return pagingStatus_.GetRawPointer();
    }
}
