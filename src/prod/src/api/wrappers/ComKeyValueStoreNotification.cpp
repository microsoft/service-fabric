// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;

// ********************************************************************************************************************
// ComKeyValueStoreNotification::ComKeyValueStoreNotification Implementation
//

ComKeyValueStoreNotification::ComKeyValueStoreNotification(IStoreItemPtr const & impl)
    : IFabricKeyValueStoreNotification(),
    ComKeyValueStoreItemResult(impl->ToKeyValueItem()),
    impl_(impl)
{
}

ComKeyValueStoreNotification::~ComKeyValueStoreNotification()
{
}

const FABRIC_KEY_VALUE_STORE_ITEM * ComKeyValueStoreNotification::get_Item( void)
{
    return ComKeyValueStoreItemResult::get_Item();
}

BOOLEAN ComKeyValueStoreNotification::IsDelete()
{
    return impl_->IsDelete() ? TRUE : FALSE;
}
