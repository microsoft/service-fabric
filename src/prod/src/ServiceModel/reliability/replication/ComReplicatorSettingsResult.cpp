// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;

using namespace Common;
using namespace Reliability::ReplicationComponent;
    
ComReplicatorSettingsResult::ComReplicatorSettingsResult(ReplicatorSettingsUPtr const & value)
    : ComUnknownBase()
    , heap_()
    , settings_()
{
    settings_ = heap_.AddItem<FABRIC_REPLICATOR_SETTINGS>();
    value->ToPublicApi(heap_, *settings_);
}

ComReplicatorSettingsResult::~ComReplicatorSettingsResult()
{
}

const FABRIC_REPLICATOR_SETTINGS * STDMETHODCALLTYPE ComReplicatorSettingsResult::get_ReplicatorSettings(void)
{
    return settings_.GetRawPointer();
}


HRESULT STDMETHODCALLTYPE ComReplicatorSettingsResult::ReturnReplicatorSettingsResult(
    ReplicatorSettingsUPtr && settings,
    IFabricReplicatorSettingsResult ** value)
{
    ComPointer<IFabricReplicatorSettingsResult> replicatorSettingResult = make_com<ComReplicatorSettingsResult, IFabricReplicatorSettingsResult>(settings);
    *value = replicatorSettingResult.DetachNoRelease();
    return ComUtility::OnPublicApiReturn(S_OK);
}
