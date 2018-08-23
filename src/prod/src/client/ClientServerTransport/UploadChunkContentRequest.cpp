// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Management::FileStoreService;
using namespace Transport;

static const Common::StringLiteral TraceType("Client.UploadChunkContentRequest");

UploadChunkContentRequest::UploadChunkContentRequest()
    : uploadChunkContentDescription_()
    , chunkContent_()
{
}
UploadChunkContentRequest::UploadChunkContentRequest(
    Transport::MessageUPtr && chunkContentMessage,
    Management::FileStoreService::UploadChunkContentDescription && uploadChunkContentDescription)
    : uploadChunkContentDescription_(move(uploadChunkContentDescription))
    , chunkContentMessage_(move(chunkContentMessage))
    , chunkContent_()
{
}

Common::ErrorCode UploadChunkContentRequest::InitializeBuffer()
{
    std::vector<Common::const_buffer> buffers;
    if (chunkContentMessage_->GetBody(buffers))
    {
        uint64 chunkSize = uploadChunkContentDescription_.StartPosition != uploadChunkContentDescription_.EndPosition ?
            (uploadChunkContentDescription_.EndPosition - uploadChunkContentDescription_.StartPosition + 1) : 0;

        chunkContent_.reserve(chunkSize);
        for (auto i = 0; i < buffers.size(); ++i)
        {
            auto bufLen = buffers[i].len;
            chunkContent_.insert(chunkContent_.end(), &(buffers[i].buf[0]), &(buffers[i].buf[bufLen]));
        }
    }
    else
    {
        WriteError(
            TraceType,
            "Unable to get buffer from the message for sessionId:{0} startPosition:{1} endPosition:{2}",
            uploadChunkContentDescription_.SessionId,
            uploadChunkContentDescription_.StartPosition,
            uploadChunkContentDescription_.EndPosition);

        return ErrorCodeValue::NotFound;
    }

    return ErrorCodeValue::Success;
}

void UploadChunkContentRequest::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    uint64 chunkSize = uploadChunkContentDescription_.StartPosition != uploadChunkContentDescription_.EndPosition ?
        (uploadChunkContentDescription_.EndPosition - uploadChunkContentDescription_.StartPosition + 1) : 0;

    w.Write("UploadChunkContentRequest{ChunkContentSize={0},SessionId={1},StartPosition={2},EndPosition={3}}", chunkSize, uploadChunkContentDescription_.SessionId, uploadChunkContentDescription_.StartPosition, uploadChunkContentDescription_.EndPosition);
}
