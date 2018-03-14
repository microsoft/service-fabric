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
    ComGetNodeListResult::ComGetNodeListResult(
        vector<NodeQueryResult> && nodeList,
        ServiceModel::PagingStatusUPtr && pagingStatus)
        : nodeList_()
        , pagingStatus_()
        , heap_()
    {
        nodeList_ = heap_.AddItem<FABRIC_NODE_QUERY_RESULT_LIST>();
        ComConversionUtility::ToPublicList<
            NodeQueryResult,
            FABRIC_NODE_QUERY_RESULT_ITEM,
            FABRIC_NODE_QUERY_RESULT_LIST>(
                heap_, 
                move(nodeList), 
                *nodeList_);

        if (pagingStatus)
        {
            pagingStatus_ = heap_.AddItem<FABRIC_PAGING_STATUS>();
            pagingStatus->ToPublicApi(heap_, *pagingStatus_);
        }
    }

    const FABRIC_NODE_QUERY_RESULT_LIST *STDMETHODCALLTYPE ComGetNodeListResult::get_NodeList(void)
    {
        return nodeList_.GetRawPointer();
    }

    const FABRIC_PAGING_STATUS *STDMETHODCALLTYPE ComGetNodeListResult::get_PagingStatus()
    {
        return pagingStatus_.GetRawPointer();
    }
}
