// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;

// ********************************************************************************************************************
// ComStoreItemEnumerator::ComStoreItemEnumerator Implementation
//

ComStoreItemEnumerator::ComStoreItemEnumerator(IStoreItemEnumeratorPtr const & impl)
    : IFabricKeyValueStoreItemEnumerator2(),
    ComUnknownBase(),
    impl_(impl)
{
}

ComStoreItemEnumerator::~ComStoreItemEnumerator()
{
}

HRESULT ComStoreItemEnumerator::MoveNext(void)
{
    return impl_->MoveNext().ToHResult();
}
        
HRESULT ComStoreItemEnumerator::TryMoveNext(BOOLEAN * success)
{
    bool hasNext = false;
    auto error = impl_->TryMoveNext(hasNext);

    if (error.IsSuccess())
    {
        *success = (hasNext ? TRUE : FALSE);
    }

    return error.ToHResult();
}
        
IFabricKeyValueStoreItemResult * ComStoreItemEnumerator::get_Current(void)
{
    IStoreItemPtr ptr = impl_->GetCurrentItem();

    auto resultWrapper = WrapperFactory::create_com_wrapper(ptr->ToKeyValueItem());
    return resultWrapper.DetachNoRelease();
}

