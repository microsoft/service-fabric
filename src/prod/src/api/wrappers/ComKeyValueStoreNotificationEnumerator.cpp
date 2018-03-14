// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;

// ********************************************************************************************************************
// ComKeyValueStoreNotificationEnumerator::ComKeyValueStoreNotificationEnumerator Implementation
//

ComKeyValueStoreNotificationEnumerator::ComKeyValueStoreNotificationEnumerator(IStoreNotificationEnumeratorPtr const & impl)
    : IFabricKeyValueStoreNotificationEnumerator2(),
    ComUnknownBase(),
    impl_(impl)
{
}

ComKeyValueStoreNotificationEnumerator::~ComKeyValueStoreNotificationEnumerator()
{
}

HRESULT ComKeyValueStoreNotificationEnumerator::MoveNext(void)
{
    return impl_->MoveNext().ToHResult();
}
        
HRESULT ComKeyValueStoreNotificationEnumerator::TryMoveNext(BOOLEAN * success)
{
    bool hasNext = false;
    auto error = impl_->TryMoveNext(hasNext);

    if (error.IsSuccess())
    {
        *success = (hasNext ? TRUE : FALSE);
    }

    return error.ToHResult();
}
        
IFabricKeyValueStoreNotification * ComKeyValueStoreNotificationEnumerator::get_Current(void)
{
    auto resultWrapper = WrapperFactory::create_com_wrapper(impl_->GetCurrentItem());
    return resultWrapper.DetachNoRelease();
}

void ComKeyValueStoreNotificationEnumerator::Reset(void)
{
    impl_->Reset();
}
