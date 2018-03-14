// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;

// ********************************************************************************************************************
// ComKeyValueStoreItemMetadataEnumerator::ComKeyValueStoreItemMetadataEnumerator Implementation
//

ComKeyValueStoreItemMetadataEnumerator::ComKeyValueStoreItemMetadataEnumerator(IKeyValueStoreItemMetadataEnumeratorPtr const & impl)
    : IFabricKeyValueStoreItemMetadataEnumerator2(),
    ComUnknownBase(),
    impl_(impl)
{
}

ComKeyValueStoreItemMetadataEnumerator::~ComKeyValueStoreItemMetadataEnumerator()
{
}

HRESULT ComKeyValueStoreItemMetadataEnumerator::MoveNext(void)
{
    return impl_->MoveNext().ToHResult();
}
        
HRESULT ComKeyValueStoreItemMetadataEnumerator::TryMoveNext(BOOLEAN * success)
{
    bool hasNext = false;
    auto error = impl_->TryMoveNext(hasNext);

    if (error.IsSuccess())
    {
        *success = (hasNext ? TRUE : FALSE);
    }

    return error.ToHResult();
}
        
IFabricKeyValueStoreItemMetadataResult * ComKeyValueStoreItemMetadataEnumerator::get_Current(void)
{
    auto resultWrapper = WrapperFactory::create_com_wrapper(impl_->get_Current());
    return resultWrapper.DetachNoRelease();
}
