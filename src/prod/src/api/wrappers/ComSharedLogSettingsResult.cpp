// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Api;
using namespace Common;
using namespace ServiceModel;
using namespace std;

ComSharedLogSettingsResult::ComSharedLogSettingsResult(ISharedLogSettingsResultPtr const & impl)
    : impl_(impl)
{
}

ComSharedLogSettingsResult::~ComSharedLogSettingsResult()
{
}

const KTLLOGGER_SHARED_LOG_SETTINGS * STDMETHODCALLTYPE ComSharedLogSettingsResult::get_Settings()
{
    auto publicSettings = heap_.AddItem<KTLLOGGER_SHARED_LOG_SETTINGS>();
    impl_->ToPublicApi(heap_, *publicSettings);

    return publicSettings.GetRawPointer();
}
