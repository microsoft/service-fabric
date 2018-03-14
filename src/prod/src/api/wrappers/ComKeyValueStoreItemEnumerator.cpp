// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;

// ********************************************************************************************************************
// ComKeyValueStoreItemEnumerator::ComKeyValueStoreItemEnumerator Implementation
//

ComKeyValueStoreItemEnumerator::ComKeyValueStoreItemEnumerator(IKeyValueStoreItemEnumeratorPtr const & impl)
    : IFabricKeyValueStoreItemEnumerator2(),
    ComUnknownBase(),
    impl_(impl)
{
}

ComKeyValueStoreItemEnumerator::~ComKeyValueStoreItemEnumerator()
{
}

HRESULT ComKeyValueStoreItemEnumerator::MoveNext(void)
{
    return impl_->MoveNext().ToHResult();
}
        
HRESULT ComKeyValueStoreItemEnumerator::TryMoveNext(BOOLEAN * success)
{
    bool hasNext = false;
    auto error = impl_->TryMoveNext(hasNext);

    if (error.IsSuccess())
    {
        *success = (hasNext ? TRUE : FALSE);
    }

    return error.ToHResult();
}
        
IFabricKeyValueStoreItemResult * ComKeyValueStoreItemEnumerator::get_Current(void)
{
    auto resultWrapper = WrapperFactory::create_com_wrapper(impl_->get_Current());
    return resultWrapper.DetachNoRelease();
}
