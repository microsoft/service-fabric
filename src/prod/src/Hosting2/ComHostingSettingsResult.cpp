// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

ComHostingSettingsResult::ComHostingSettingsResult()
    : IFabricHostingSettingsResult()
    , ComUnknownBase()
    , heap_()
    , settings_()
{
    settings_ = heap_.AddItem<FABRIC_HOSTING_SETTINGS>();
    HostingConfig::GetConfig().ToPublicApi(heap_, *settings_);
}

ComHostingSettingsResult::~ComHostingSettingsResult()
{
}

const FABRIC_HOSTING_SETTINGS * STDMETHODCALLTYPE ComHostingSettingsResult::get_Result()
{
    return settings_.GetRawPointer();
}
