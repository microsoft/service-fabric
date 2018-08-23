// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management::FileStoreService;

UploadChunkContentDescription::UploadChunkContentDescription()
    : sessionId_()
    , startPosition_()
    , endPosition_()
{
}

UploadChunkContentDescription::UploadChunkContentDescription(
    Common::Guid const & sessionId,
    uint64 startPosition,
    uint64 endPosition)
    : sessionId_(sessionId)
    , startPosition_(startPosition)
    , endPosition_(endPosition)
{
}

void UploadChunkContentDescription::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    uint64 chunkSize = 0;
    if (startPosition_ != endPosition_)
    {
        chunkSize = endPosition_ - startPosition_ + 1;
    }

    w.Write("UploadChunkContentDescription{ChunkContentSize={0},SessionId={1},StartPosition={2},EndPosition={3}}", chunkSize, sessionId_, startPosition_, endPosition_);
}
