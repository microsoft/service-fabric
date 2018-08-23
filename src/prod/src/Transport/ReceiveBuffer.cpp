// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include"stdafx.h"

using namespace Transport;
using namespace Common;
using namespace std;

static StringLiteral const TraceType("RecvBuf");

ReceiveBuffer::ReceiveBuffer(TcpConnection* connectionPtr)
: connectionPtr_(connectionPtr)
, receiveQueue_(connectionPtr->receiveChunkSize_)
, msgBuffers_(&receiveQueue_)
, decrypted_(connectionPtr->receiveChunkSize_)
{
}

ReceiveBuffer::Buffers const & ReceiveBuffer::GetBuffers(size_t count)
{
    if (count < currentFrameMissing_)
    {
        count = currentFrameMissing_;
    }

    receiveQueue_.reserve_back(count);
    buffers_.resize(0);

    size_t len = 0;

    Common::bique<byte>::iterator walker = receiveQueue_.end();

    while (walker < receiveQueue_.uninitialized_end())
    {
        buffers_.push_back(MutableBuffer(walker.fragment_begin(), walker.fragment_size()));
        len += walker.fragment_size();
        walker += walker.fragment_size();
    }

    ASSERT_IF(len == 0, "empty receive buffers");

    return buffers_;
}

void ReceiveBuffer::Commit(size_t count)
{
    receiveQueue_.no_fill_advance_back(count);
    receivedByteTotal_ += count;
}

