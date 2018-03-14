// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;

// ********************************************************************************************************************
// ComProxyNativeImageStoreProgressEventHandler Implementation
//
ComProxyNativeImageStoreProgressEventHandler::ComProxyNativeImageStoreProgressEventHandler(
    ComPointer<IFabricNativeImageStoreProgressEventHandler> const & comImpl)
    : ComponentRoot()
    , INativeImageStoreProgressEventHandler()
    , comImpl_(comImpl)
{
}

ComProxyNativeImageStoreProgressEventHandler::~ComProxyNativeImageStoreProgressEventHandler()
{
}

ErrorCode ComProxyNativeImageStoreProgressEventHandler::GetUpdateInterval(__out TimeSpan & result)
{
    DWORD milliseconds = 0;
    auto hr = comImpl_->GetUpdateInterval(&milliseconds);

    if (SUCCEEDED(hr)) 
    {
        result = TimeSpan::FromMilliseconds(milliseconds);
    }

    return ErrorCode::FromHResult(hr);
}

ErrorCode ComProxyNativeImageStoreProgressEventHandler::OnUpdateProgress(size_t completedItems, size_t totalItems, ServiceModel::ProgressUnitType::Enum itemType)
{
    return ErrorCode::FromHResult(comImpl_->OnUpdateProgress(completedItems, totalItems, ServiceModel::ProgressUnitType::ToPublicApi(itemType)));
}
