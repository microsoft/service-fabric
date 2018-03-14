// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace TxnReplicator;

ComTransactionalReplicatorSettingsResult::ComTransactionalReplicatorSettingsResult(__in TransactionalReplicatorSettingsUPtr const & value)
    : ComUnknownBase()
    , heap_()
    , settings_()
{
    settings_ = heap_.AddItem<TRANSACTIONAL_REPLICATOR_SETTINGS>();
    value->ToPublicApi(*settings_);
}

TxnReplicator::ComTransactionalReplicatorSettingsResult::~ComTransactionalReplicatorSettingsResult()
{
}

const TRANSACTIONAL_REPLICATOR_SETTINGS * TxnReplicator::ComTransactionalReplicatorSettingsResult::get_TransactionalReplicatorSettings()
{
    return settings_.GetRawPointer();
}

HRESULT TxnReplicator::ComTransactionalReplicatorSettingsResult::ReturnTransactionalReplicatorSettingsResult(
    __in TransactionalReplicatorSettingsUPtr && settings,
    __out IFabricTransactionalReplicatorSettingsResult ** value)
{
    ComPointer<IFabricTransactionalReplicatorSettingsResult> replicatorSettingResult = make_com<ComTransactionalReplicatorSettingsResult, IFabricTransactionalReplicatorSettingsResult>(settings);
    *value = replicatorSettingResult.DetachNoRelease();
    return ComUtility::OnPublicApiReturn(S_OK);
}
