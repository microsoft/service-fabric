// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Management::ImageStore;

ImageStoreListDescription::ImageStoreListDescription()
    : remoteLocation_()
    , continuationToken_()
    , isRecursive_(false)
{
}

ImageStoreListDescription::ImageStoreListDescription(
    wstring const & remoteLocation,
    wstring const & continuationToken,
    bool isRecursive)
    : remoteLocation_(remoteLocation)
    , continuationToken_(continuationToken)
    , isRecursive_(isRecursive)
{
}

Common::ErrorCode ImageStoreListDescription::FromPublicApi(__in FABRIC_IMAGE_STORE_LIST_DESCRIPTION const & listDes)
{
    HRESULT hr = StringUtility::LpcwstrToWstring(listDes.RemoteLocation, true, remoteLocation_);
    if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

    hr = StringUtility::LpcwstrToWstring(listDes.ContinuationToken, true, continuationToken_);
    if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

    isRecursive_ = (listDes.IsRecursive == TRUE) ? true : false;
    return ErrorCodeValue::Success;
}

void ImageStoreListDescription::ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_IMAGE_STORE_LIST_DESCRIPTION & listDes) const
{
    listDes.RemoteLocation = heap.AddString(remoteLocation_);
    listDes.ContinuationToken = heap.AddString(continuationToken_);
    listDes.IsRecursive = isRecursive_ ? TRUE : FALSE;
}
