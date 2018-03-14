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
    ComGetProvisionedFabricCodeVersionListResult::ComGetProvisionedFabricCodeVersionListResult(
        vector<ProvisionedFabricCodeVersionQueryResultItem> && clusterCodeVersionList)
        : codeVersionList_(),
        heap_()
    {
        codeVersionList_ = heap_.AddItem<FABRIC_PROVISIONED_CODE_VERSION_QUERY_RESULT_LIST>();
        ComConversionUtility::ToPublicList<
            ProvisionedFabricCodeVersionQueryResultItem,
            FABRIC_PROVISIONED_CODE_VERSION_QUERY_RESULT_ITEM,
            FABRIC_PROVISIONED_CODE_VERSION_QUERY_RESULT_LIST>(
                heap_, 
                move(clusterCodeVersionList), 
                *codeVersionList_);
    }

    const FABRIC_PROVISIONED_CODE_VERSION_QUERY_RESULT_LIST *STDMETHODCALLTYPE ComGetProvisionedFabricCodeVersionListResult::get_ProvisionedCodeVersionList(void)
    {
        return codeVersionList_.GetRawPointer();
    }
}
