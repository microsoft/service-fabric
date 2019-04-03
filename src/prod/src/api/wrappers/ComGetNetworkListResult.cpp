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
    ComGetNetworkListResult::ComGetNetworkListResult(
        vector<NetworkInformation> && networkList,
        PagingStatusUPtr && pagingStatus)
        : networkList_()
        , pagingStatus_()
        , heap_()
    {
        networkList_ = heap_.AddItem<FABRIC_NETWORK_QUERY_RESULT_LIST>();
        ComConversionUtility::ToPublicList<
            NetworkInformation,
            FABRIC_NETWORK_INFORMATION,
            FABRIC_NETWORK_QUERY_RESULT_LIST>(
                heap_, 
                move(networkList),
                *networkList_);

        if (pagingStatus)
        {
            pagingStatus_ = heap_.AddItem<FABRIC_PAGING_STATUS>();
            pagingStatus->ToPublicApi(heap_, *pagingStatus_);
        }
    }

    const FABRIC_NETWORK_QUERY_RESULT_LIST *STDMETHODCALLTYPE ComGetNetworkListResult::get_NetworkList()
    {
        return networkList_.GetRawPointer();
    }

    const FABRIC_PAGING_STATUS *STDMETHODCALLTYPE ComGetNetworkListResult::get_PagingStatus()
    {
        return pagingStatus_.GetRawPointer();
    }
}
