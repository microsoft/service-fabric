// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Management::ImageStore;

ImageStoreCopyDescription::ImageStoreCopyDescription()
    : remoteSource_()
    , remoteDestination_()
    , skipFiles_()
    , copyFlag_(CopyFlag::Overwrite)
    , checkMarkFile_(false)
{
}

ImageStoreCopyDescription::ImageStoreCopyDescription(
    std::wstring const & remoteSource,
    std::wstring const & remoteDestination,
    Common::StringCollection const & skipFiles,
    Management::ImageStore::CopyFlag::Enum copyFlag,
    bool checkMarkFile)
    : remoteSource_(remoteSource)
    , remoteDestination_(remoteDestination)
    , skipFiles_(skipFiles)
    , copyFlag_(copyFlag)
    , checkMarkFile_(checkMarkFile)
{
}

Common::ErrorCode ImageStoreCopyDescription::FromPublicApi(__in FABRIC_IMAGE_STORE_COPY_DESCRIPTION const & copyDes)
{
    HRESULT hr = StringUtility::LpcwstrToWstring(copyDes.RemoteSource, false, remoteSource_);
    if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

    hr = StringUtility::LpcwstrToWstring(copyDes.RemoteDestination, false, remoteDestination_);
    if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

    copyFlag_ = CopyFlag::FromPublicApi(copyDes.CopyFlag);
    checkMarkFile_ = (copyDes.CheckMarkFile == TRUE) ? true : false;
    if (copyDes.SkipFiles != NULL && copyDes.SkipFiles->Count > 0)
    {
        Common::StringCollection skipFilesCollection;
        StringList::FromPublicApi(*copyDes.SkipFiles, skipFilesCollection);
    }

    return ErrorCodeValue::Success;
}

void ImageStoreCopyDescription::ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_IMAGE_STORE_COPY_DESCRIPTION & copyDes) const
{
    copyDes.RemoteSource = heap.AddString(remoteSource_);
    copyDes.RemoteDestination = heap.AddString(remoteDestination_);

    auto publicSkipFiles = heap.AddItem<FABRIC_STRING_LIST>();
    StringList::ToPublicAPI(heap, skipFiles_, publicSkipFiles);
    copyDes.SkipFiles = publicSkipFiles.GetRawPointer();

    copyDes.CheckMarkFile = checkMarkFile_ ? TRUE : FALSE;
    copyDes.CopyFlag = CopyFlag::ToPublicApi(copyFlag_);
}
