// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Api;
using namespace Common;
using namespace ServiceModel;
using namespace std;

ComEseLocalStoreSettingsResult::ComEseLocalStoreSettingsResult(IEseLocalStoreSettingsResultPtr const & impl)
    : impl_(impl)
{
}

ComEseLocalStoreSettingsResult::~ComEseLocalStoreSettingsResult()
{
}

const FABRIC_ESE_LOCAL_STORE_SETTINGS * STDMETHODCALLTYPE ComEseLocalStoreSettingsResult::get_Settings()
{
    auto publicSettings = heap_.AddItem<FABRIC_ESE_LOCAL_STORE_SETTINGS>();
    impl_->ToPublicApi(heap_, *publicSettings);

    return publicSettings.GetRawPointer();
}
