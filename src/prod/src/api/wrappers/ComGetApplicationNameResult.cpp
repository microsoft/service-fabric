// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Api;
using namespace Common;
using namespace ServiceModel;

using namespace std;

ComGetApplicationNameResult::ComGetApplicationNameResult(
    ApplicationNameQueryResult && applicationName)
    : heap_()
    , applicationName_()
{
    applicationName_ = heap_.AddItem<FABRIC_APPLICATION_NAME_QUERY_RESULT>();
    applicationName.ToPublicApi(heap_, *applicationName_);
}

ComGetApplicationNameResult::~ComGetApplicationNameResult()
{
}

const FABRIC_APPLICATION_NAME_QUERY_RESULT * STDMETHODCALLTYPE ComGetApplicationNameResult::get_ApplicationName()
{
    return applicationName_.GetRawPointer();
}
