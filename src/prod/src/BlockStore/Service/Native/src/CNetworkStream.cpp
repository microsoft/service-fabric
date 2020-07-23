// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------
// Network Stream class implementation.

// TODO: Review this.
// Silence warnings for stdenv

#define _CRT_SECURE_NO_WARNINGS

#include "stdafx.h"
#include "../inc/CNetworkStream.h"
#include <stdlib.h>
#include <stdio.h>

#ifdef PLATFORM_UNIX
#include <sys/syscall.h>
#include <unistd.h>
#endif

using namespace std;
using namespace Common;
using boost::asio::deadline_timer;

auto CNetworkStream::Recv(size_t max_length, unsigned long long tracking)
{
    struct async_read_some_operation : public BoostAsyncAwaitable<size_t>
    {
        async_read_some_operation(CNetworkStream *self, size_t max_length, unsigned long long tracking)
            :_stream(self), _max_length(max_length), _tracking(tracking)
        {
        }

        async_read_some_operation(async_read_some_operation &&other)
        {
            UNREFERENCED_PARAMETER(other);
        }

        virtual bool on_start()
        {
            TRACE_NOISE("ID:" << _tracking << " async_read(" << _max_length << ")");

            // Read the entire buffer in a single go.
            boost::asio::async_read(*(_stream->socket_), boost::asio::buffer(_stream->dataEnd_, _max_length),
                [this](boost::system::error_code ec, std::size_t bytesTransferred)
            {
                if (ec)
                {
                    TRACE_NOISE("ID:" << _tracking << " async_read error: " << ec);
                    this->set_error(ec);
                }
                else
                {
                    TRACE_NOISE("ID:" << _tracking << " async_read success: " << bytesTransferred << " bytes received");

                    // advance _cur pointer
                    _stream->AfterRecv(bytesTransferred);

                    this->set_result(bytesTransferred);
                }
                this->post_resume();
            });
            return false;
        }

        CNetworkStream *_stream;
        size_t  _max_length;    // max network request size
        unsigned long long _tracking;    // max network request size
    };

    return async_read_some_operation(this, max_length, tracking);
}

auto CNetworkStream::Send(const BlockStoreByte *bytes, size_t length, unsigned long long tracking)
{
    struct AsyncWriteOperation : public BoostAsyncAwaitable<size_t>
    {
        AsyncWriteOperation(CNetworkStream *self, const BlockStoreByte *bytes, size_t length, unsigned long long tracking)
            :stream_(self), bytes_(bytes), length_(length), tracking_(tracking)
        {}

        virtual bool on_start()
        {
            // Write the entire buffer in a single go.
            boost::asio::async_write(*(stream_->Socket), boost::asio::buffer(bytes_, length_), 
                [this](boost::system::error_code ec, std::size_t bytesTransferred)
            {
                if (ec){
                    TRACE_ERROR("[async_send] Async read failed TrackingID:" << tracking_ << " error:" << ec);
                    this->set_error(ec);
                }
                else{
                    this->set_result(bytesTransferred);
                }
                this->post_resume();
            });

            return false;
        }
        CNetworkStream *stream_;
        const BlockStoreByte *bytes_;
        size_t length_;
        unsigned long long tracking_;
    };

    return AsyncWriteOperation(this, bytes, length, tracking);
}

// Resets the IO buffers to be used by the stream for reading or writing.
// Must be set to the applicable buffer that needs to be used for I/O.
void CNetworkStream::ResetIOBuffers(BlockStoreByte* pBuffer, size_t length)
{
    buf_ = pBuffer;
    if (buf_ != nullptr)
    {
        bufEnd_ = buf_ + length;
        cur_ = buf_;
        dataEnd_ = cur_;
    }
    else
    {
        bufEnd_ = nullptr;
        cur_ = nullptr;
        dataEnd_ = nullptr;
    }
}

void CNetworkStream::AfterRecv(size_t new_data)
{
    _ASSERT(buf_ != nullptr);

    // We should have enough buffer since prepare_recv is called
    _ASSERT(dataEnd_ + new_data <= bufEnd_);

    // we've got enough buffer for the new data we just got
    dataEnd_ += new_data;
}

// Async socket read
task<const BlockStoreByte *> CNetworkStream::Read(BlockStoreByte* pBuffer, size_t read_size, unsigned long long tracking)
{
    _ASSERT(pBuffer != nullptr);

    ResetIOBuffers(pBuffer, read_size);
    
    size_t originalReadLength = read_size;
    while (read_size > 0) 
    {
        size_t bytesRead = co_await Recv(read_size, tracking);
        read_size -= bytesRead;
    }

    cur_ += originalReadLength;

    co_return pBuffer;
}

// Async socket write
task<size_t> CNetworkStream::Write(const BlockStoreByte *bytes, size_t length, unsigned long long tracking)
{
    const BlockStoreByte *pWriteFrom = bytes;
    size_t LengthToWrite = length;

    while (LengthToWrite > 0)
    {
        size_t bytesWritten = co_await Send(pWriteFrom, LengthToWrite, tracking);
        LengthToWrite -= bytesWritten;
        pWriteFrom += bytesWritten;
    }

    co_return length;
}

// what is the available remaining data size in the buffer
size_t CNetworkStream::GetRemainingDataSize()
{
    return dataEnd_ - cur_;
}

// Closes the socket associated with the stream
void CNetworkStream::CloseSocket()
{
    socket_->close();
    socket_ = nullptr;
}
