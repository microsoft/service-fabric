// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class SecurityBuffers : public SecBufferDesc
    {
    public:
        SecurityBuffers();
        SecurityBuffers(bool freeSecurityBuffers);
        SecurityBuffers(PSecBufferDesc pBuffers);
        SecurityBuffers(PSecBufferDesc pBuffers, bool freeSecurityBuffers);

        virtual ~SecurityBuffers();

        void Reserve(size_t size);

        PSecBufferDesc GetBuffers() { return static_cast<PSecBufferDesc>(this); }

        void AddBuffer(int type, void * pData, size_t size);
        void AddBuffer(int type, std::vector<byte> & data);

        void AddEmptyBuffer();

        void AddDataBuffer(void * pData, size_t size);
        void AddDataBuffer(std::vector<byte> & data);
        void AddDataBuffer(WSABUF & data);

        void AddTokenBuffer(std::vector<byte> & token);
        void AddPaddingBuffer(std::vector<byte> & padding);

        void AddTokenBuffer(void* token, size_t size);
        void AddPaddingBuffer(void* padding, size_t size);

        void AddStreamHeaderBuffer(std::vector<byte> & header);
        void AddStreamTrailerBuffer(std::vector<byte> & trailer);

        void RequestBuffer(int type);
        void RequestToken();

        void ResizeBuffer(int type, std::vector<byte> & buffer);
        void ResizeTokenBuffer(std::vector<byte> & token);
        void ResizePaddingBuffer(std::vector<byte> & padding);
        void ResizeStreamHeaderBuffer(std::vector<byte> & header);
        void ResizeStreamTrailerBuffer(std::vector<byte> & trailer);

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

    private:
        std::vector<SecBuffer> buffers_;
        bool freeSecurityBuffers_;
    };
}
