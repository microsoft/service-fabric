// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#include "stdafx.h"

using Common::ComUtility;

using namespace Common;
using namespace Api;
using namespace ServiceModel;
using namespace std;

using namespace Management::FaultAnalysisService;

ComGetTestCommandStatusResult::ComGetTestCommandStatusResult(
    vector<TestCommandListQueryResult> && result)
    : result_()
    , heap_()
{
    result_ = heap_.AddItem<TEST_COMMAND_QUERY_RESULT_LIST>();

    ComConversionUtility::ToPublicList<
        TestCommandListQueryResult,
        TEST_COMMAND_QUERY_RESULT_ITEM,
        TEST_COMMAND_QUERY_RESULT_LIST>(
            heap_, 
            move(result), 
            *result_);
}

const TEST_COMMAND_QUERY_RESULT_LIST *STDMETHODCALLTYPE ComGetTestCommandStatusResult::get_Result(void)
{
    return result_.GetRawPointer();
}


