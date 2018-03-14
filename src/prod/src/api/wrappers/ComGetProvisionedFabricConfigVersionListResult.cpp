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
    ComGetProvisionedFabricConfigVersionListResult::ComGetProvisionedFabricConfigVersionListResult(
        vector<ProvisionedFabricConfigVersionQueryResultItem> && configVersionList)
        : configVersionList_(),
        heap_()
    {
        configVersionList_ = heap_.AddItem<FABRIC_PROVISIONED_CONFIG_VERSION_QUERY_RESULT_LIST>();
        ComConversionUtility::ToPublicList<
            ProvisionedFabricConfigVersionQueryResultItem,
            FABRIC_PROVISIONED_CONFIG_VERSION_QUERY_RESULT_ITEM,
            FABRIC_PROVISIONED_CONFIG_VERSION_QUERY_RESULT_LIST>(
                heap_, 
                move(configVersionList), 
                *configVersionList_);
    }

    const FABRIC_PROVISIONED_CONFIG_VERSION_QUERY_RESULT_LIST *STDMETHODCALLTYPE ComGetProvisionedFabricConfigVersionListResult::get_ProvisionedConfigVersionList(void)
    {
        return configVersionList_.GetRawPointer();
    }
}
