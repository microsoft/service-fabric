// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;

// ********************************************************************************************************************
// ComKeyValueStoreItemMetadataResult::ComKeyValueStoreItemMetadataResult Implementation
//

ComKeyValueStoreItemMetadataResult::ComKeyValueStoreItemMetadataResult(IKeyValueStoreItemMetadataResultPtr const & impl)
    : IFabricKeyValueStoreItemMetadataResult(),
    ComUnknownBase(),
    impl_(impl)
{
}

ComKeyValueStoreItemMetadataResult::~ComKeyValueStoreItemMetadataResult()
{
}

const FABRIC_KEY_VALUE_STORE_ITEM_METADATA * ComKeyValueStoreItemMetadataResult::get_Metadata(void)
{
    return impl_->get_Metadata();
}
