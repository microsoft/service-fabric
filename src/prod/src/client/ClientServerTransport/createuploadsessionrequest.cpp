// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Management::FileStoreService;

CreateUploadSessionRequest::CreateUploadSessionRequest()
: storeRelativePath_()
, sessionId_()
, fileSize_()
{
}

CreateUploadSessionRequest::CreateUploadSessionRequest(
    std::wstring const & storeRelativePath,
    Common::Guid const & sessionId,
    uint64 fileSize)
    : storeRelativePath_(storeRelativePath)
    , sessionId_(sessionId)
    , fileSize_(fileSize)
{
}

void CreateUploadSessionRequest::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write("CreateUploadSessionRequest{StoreRelativePath={0},SessionId={1},FileSize={2}}", storeRelativePath_, sessionId_, fileSize_);
}
