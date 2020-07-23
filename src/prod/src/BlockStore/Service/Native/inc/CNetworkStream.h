// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------
#pragma once

#include "common.h"
#include "boost_async_awaitable.h"
#include "trace.h"

struct AsyncWriteOperation;
class CBlockStoreRequest;

// It wraps over supplied buffer and size
class CNetworkStream
{
public:
    CNetworkStream(tcp::socket *socket, CBlockStoreRequest* requestForStream)
        :socket_(socket), blockStoreRequest_(requestForStream)
    {
        ResetIOBuffers(nullptr, 0);
    }

    __declspec(property(get = get_Socket)) tcp::socket* Socket;
    tcp::socket* get_Socket()
    {
        return socket_;
    }

    task<const BlockStoreByte *> Read(BlockStoreByte *bytes, size_t length, unsigned long long);
    task<size_t> Write(const BlockStoreByte *bytes, size_t length, unsigned long long);

    void CloseSocket();

private:
    void ResetIOBuffers(BlockStoreByte* pBuffer, size_t length);
    auto Recv(size_t maxLength, unsigned long long);
    auto Send(const BlockStoreByte *bytes, size_t length, unsigned long long );
    void AfterRecv(size_t newData);
    size_t GetRemainingDataSize();

    BlockStoreByte *buf_;
    BlockStoreByte *dataEnd_;
    BlockStoreByte *bufEnd_;
    BlockStoreByte *cur_;

    tcp::socket *socket_;
    CBlockStoreRequest *blockStoreRequest_;
};
