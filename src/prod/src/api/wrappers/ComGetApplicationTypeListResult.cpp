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
    ComGetApplicationTypeListResult::ComGetApplicationTypeListResult(
        vector<ApplicationTypeQueryResult> && applicationTypeList)
        : applicationTypeList_(),
        heap_()
    {
        applicationTypeList_ = heap_.AddItem<FABRIC_APPLICATION_TYPE_QUERY_RESULT_LIST>();
        ComConversionUtility::ToPublicList<
            ApplicationTypeQueryResult,
            FABRIC_APPLICATION_TYPE_QUERY_RESULT_ITEM,
            FABRIC_APPLICATION_TYPE_QUERY_RESULT_LIST>(
                heap_, 
                move(applicationTypeList), 
                *applicationTypeList_);
    }

    const FABRIC_APPLICATION_TYPE_QUERY_RESULT_LIST *STDMETHODCALLTYPE ComGetApplicationTypeListResult::get_ApplicationTypeList(void)
    {
        return applicationTypeList_.GetRawPointer();
    }
}
