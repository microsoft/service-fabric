// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Api;
using namespace Common;
using namespace ServiceModel;

using namespace std;

ComClientSettingsResult::ComClientSettingsResult(
    FabricClientSettings const & settings)
    : IFabricClientSettingsResult()
    , ComUnknownBase()
    , heap_()
    , settings_()
{
    settings_ = heap_.AddItem<FABRIC_CLIENT_SETTINGS>();
    settings.ToPublicApi(heap_, *settings_);
}

ComClientSettingsResult::~ComClientSettingsResult()
{
}

const FABRIC_CLIENT_SETTINGS * STDMETHODCALLTYPE ComClientSettingsResult::get_Settings()
{
    return settings_.GetRawPointer();
}
