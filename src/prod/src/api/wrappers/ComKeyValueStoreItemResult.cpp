// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;

// ********************************************************************************************************************
// ComKeyValueStoreItemResult::ComKeyValueStoreItemResult Implementation
//

ComKeyValueStoreItemResult::ComKeyValueStoreItemResult(IKeyValueStoreItemResultPtr const & impl)
    : IFabricKeyValueStoreItemResult(),
    ComUnknownBase(),
    impl_(impl)
{
}

ComKeyValueStoreItemResult::~ComKeyValueStoreItemResult()
{
}

const FABRIC_KEY_VALUE_STORE_ITEM * ComKeyValueStoreItemResult::get_Item( void)
{
    return impl_->get_Item();
}
