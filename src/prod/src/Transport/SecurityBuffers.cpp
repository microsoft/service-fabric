// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

static const size_t defaultReserveSize = 3;

namespace Transport
{
    SecurityBuffers::SecurityBuffers()
    {
        ulVersion = SECBUFFER_VERSION;
        cBuffers = 0;
        pBuffers = nullptr;
        freeSecurityBuffers_ = false;
        Reserve(defaultReserveSize);
    }

    SecurityBuffers::SecurityBuffers(bool freeSecurityBuffers)
    {
        ulVersion = SECBUFFER_VERSION;
        cBuffers = 0;
        pBuffers = nullptr;
        freeSecurityBuffers_ = freeSecurityBuffers;
        Reserve(defaultReserveSize);
    }

    SecurityBuffers::SecurityBuffers(PSecBufferDesc pDesc, bool freeSecurityBuffers)
    {
        ulVersion = SECBUFFER_VERSION;
        cBuffers = 0;
        pBuffers = nullptr;
        freeSecurityBuffers_ = freeSecurityBuffers;

        if (pDesc != nullptr) 
        {
            Reserve(pDesc->cBuffers);

            for (ULONG i = 0; i < pDesc->cBuffers; i++)
            {
                AddBuffer(pDesc->pBuffers[i].BufferType, pDesc->pBuffers[i].pvBuffer, pDesc->pBuffers[i].cbBuffer);
            }
        }
    }

    void SecurityBuffers::Reserve(size_t size)
    {
        buffers_.reserve(size);
    }

    SecurityBuffers::~SecurityBuffers()
    {
        if (freeSecurityBuffers_)
        {
            for (ULONG i = 0; i < cBuffers; i++)
            {
                ::FreeContextBuffer(pBuffers[i].pvBuffer);
            }
        }
    }

    void SecurityBuffers::AddBuffer(int type, void * pData, size_t size)
    {
        SecBuffer buffer;
        buffer.BufferType = type;
        buffer.pvBuffer = pData;
        buffer.cbBuffer = static_cast<ULONG>(size);
        buffers_.push_back(buffer);
        cBuffers = static_cast<ULONG>(buffers_.size());
        pBuffers = buffers_.data();
    }

    void SecurityBuffers::AddEmptyBuffer()
    {
        AddBuffer(SECBUFFER_EMPTY, NULL, NULL);
    }

    void SecurityBuffers::AddBuffer(int type, std::vector<byte> & data)
    {
        AddBuffer(type, data.data(), data.size());
    }

    void SecurityBuffers::AddDataBuffer(void * pData, size_t size)
    {
        AddBuffer(SECBUFFER_DATA, pData, size);
    }

    void SecurityBuffers::AddDataBuffer(std::vector<byte> & data)
    {
        AddBuffer(SECBUFFER_DATA, data);
    }

    void SecurityBuffers::AddDataBuffer(WSABUF & data)
    {
        AddBuffer(SECBUFFER_DATA, data.buf, data.len);
    }

    void SecurityBuffers::AddTokenBuffer(void* token, size_t size)
    {
        AddBuffer(SECBUFFER_TOKEN, token, size);
    }

    void SecurityBuffers::AddTokenBuffer(std::vector<byte> & token)
    {
        AddBuffer(SECBUFFER_TOKEN, token);
    }

    void SecurityBuffers::AddPaddingBuffer(void* padding, size_t size)
    {
        AddBuffer(SECBUFFER_PADDING, padding, size);
    }

    void SecurityBuffers::AddPaddingBuffer(std::vector<byte> & padding)
    {
        AddBuffer(SECBUFFER_PADDING, padding);
    }

    void SecurityBuffers::AddStreamHeaderBuffer(std::vector<byte> & header)
    {
        AddBuffer(SECBUFFER_STREAM_HEADER, header);
    }

    void SecurityBuffers::AddStreamTrailerBuffer(std::vector<byte> & trailer)
    {
        AddBuffer(SECBUFFER_STREAM_TRAILER, trailer);
    }

    void SecurityBuffers::RequestBuffer(int type)
    {
        AddBuffer(type, nullptr, 0);
    }

    void SecurityBuffers::RequestToken()
    {
        AddBuffer(SECBUFFER_TOKEN, nullptr, 0);
    }

    void SecurityBuffers::ResizeBuffer(int type, std::vector<byte> & buffer)
    {
        for (ULONG i = 0; i < cBuffers; i++)
        {
            if (pBuffers[i].BufferType == static_cast<ULONG>(type))
            {
                buffer.resize(pBuffers[i].cbBuffer);
            }
        }
    }

    void SecurityBuffers::ResizeTokenBuffer(std::vector<byte> & token)
    {
        ResizeBuffer(SECBUFFER_TOKEN, token);
    }

    void SecurityBuffers::ResizePaddingBuffer(std::vector<byte> & padding)
    {
        ResizeBuffer(SECBUFFER_PADDING, padding);
    }

    void SecurityBuffers::ResizeStreamHeaderBuffer(std::vector<byte> & header)
    {
        ResizeBuffer(SECBUFFER_STREAM_HEADER, header);
    }

    void SecurityBuffers::ResizeStreamTrailerBuffer(std::vector<byte> & trailer)
    {
        ResizeBuffer(SECBUFFER_STREAM_TRAILER, trailer);
    }

    void SecurityBuffers::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
    {
        w.WriteLine();
        for (auto iter = buffers_.cbegin(); iter != buffers_.cend(); ++ iter)
        {
            w.Write(" type = {0}, size = {1}, data =", iter->BufferType, iter->cbBuffer);
            for (ULONG i = 0; i < iter->cbBuffer; ++ i)
            {
                byte byteValue = ((byte*)(iter->pvBuffer))[i];
                w.Write(" {0:x}", byteValue);
            }
            w.WriteLine();
        }
    }
}
