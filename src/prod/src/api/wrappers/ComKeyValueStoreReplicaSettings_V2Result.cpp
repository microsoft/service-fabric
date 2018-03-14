// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Api;
using namespace Common;
using namespace ServiceModel;
using namespace std;

ComKeyValueStoreReplicaSettings_V2Result::ComKeyValueStoreReplicaSettings_V2Result(IKeyValueStoreReplicaSettings_V2ResultPtr const & impl)
    : impl_(impl)
{
}

ComKeyValueStoreReplicaSettings_V2Result::~ComKeyValueStoreReplicaSettings_V2Result()
{
}

const FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS_V2 * STDMETHODCALLTYPE ComKeyValueStoreReplicaSettings_V2Result::get_Settings()
{
    auto publicSettings = heap_.AddItem<FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS_V2>();
    impl_->ToPublicApi(heap_, *publicSettings);

    return publicSettings.GetRawPointer();
}
