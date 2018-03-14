// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;

// ********************************************************************************************************************
// ComStoreItemMetadataEnumerator::ComStoreItemMetadataEnumerator Implementation
//

ComStoreItemMetadataEnumerator::ComStoreItemMetadataEnumerator(IStoreItemMetadataEnumeratorPtr const & impl)
    : IFabricKeyValueStoreItemMetadataEnumerator2(),
    ComUnknownBase(),
    impl_(impl)
{
}

ComStoreItemMetadataEnumerator::~ComStoreItemMetadataEnumerator()
{
}

HRESULT ComStoreItemMetadataEnumerator::MoveNext(void)
{
    return impl_->MoveNext().ToHResult();
}
        
HRESULT ComStoreItemMetadataEnumerator::TryMoveNext(BOOLEAN * success)
{
    bool hasNext = false;
    auto error = impl_->TryMoveNext(hasNext);

    if (error.IsSuccess())
    {
        *success = (hasNext ? TRUE : FALSE);
    }

    return error.ToHResult();
}
        
IFabricKeyValueStoreItemMetadataResult * ComStoreItemMetadataEnumerator::get_Current(void)
{
    IStoreItemMetadataPtr ptr = impl_->GetCurrentItemMetadata();

    auto resultWrapper = WrapperFactory::create_com_wrapper(ptr->ToKeyValueItemMetadata());
    return resultWrapper.DetachNoRelease();
}

