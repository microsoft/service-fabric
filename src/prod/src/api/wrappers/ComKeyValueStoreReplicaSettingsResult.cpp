// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Api;
using namespace Common;
using namespace ServiceModel;
using namespace std;

ComKeyValueStoreReplicaSettingsResult::ComKeyValueStoreReplicaSettingsResult(IKeyValueStoreReplicaSettingsResultPtr const & impl)
    : impl_(impl)
{
}

ComKeyValueStoreReplicaSettingsResult::~ComKeyValueStoreReplicaSettingsResult()
{
}

const FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS * STDMETHODCALLTYPE ComKeyValueStoreReplicaSettingsResult::get_Settings()
{
    auto publicSettings = heap_.AddItem<FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS>();
    impl_->ToPublicApi(heap_, *publicSettings);

    return publicSettings.GetRawPointer();
}
