// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Management::FileStoreService;

UploadChunkRequest::UploadChunkRequest()
: stagingRelativePath_()
, sessionId_()
, startPosition_()
, endPosition_()
{
}

UploadChunkRequest::UploadChunkRequest(
    std::wstring const & stagingRelativePath,
    Common::Guid const & sessionId,
    uint64 startPosition,
    uint64 endPosition)
    : stagingRelativePath_(stagingRelativePath)
    , sessionId_(sessionId)
    , startPosition_(startPosition)
    , endPosition_(endPosition)
{
}

void UploadChunkRequest::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write("UploadChunkRequest{StagingRelativePath={0},SessionId={1},StartPosition={2},EndPosition={3}}", stagingRelativePath_, sessionId_, startPosition_, endPosition_);
}
