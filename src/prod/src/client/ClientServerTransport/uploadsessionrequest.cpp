// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Management::FileStoreService;

UploadSessionRequest::UploadSessionRequest()
    : storeRelativePath_()
    , sessionId_()
{
}

UploadSessionRequest::UploadSessionRequest(
    std::wstring const & storeRelativePath,
    Common::Guid const & sessionId)
    : storeRelativePath_(storeRelativePath)
    , sessionId_(sessionId)
{
}

void UploadSessionRequest::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write("UploadSessionRequest{StoreRelativePath={0},SessionId={1}}", this->storeRelativePath_, this->sessionId_);
}
