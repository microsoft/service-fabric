// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include <winsock2.h>

namespace Common
{
    struct const_buffer : public WSABUF
    {
        const_buffer() { len = 0; buf = 0; }
        const_buffer(void const * data, size_t size) { len = (ULONG)size; buf =  (char*)const_cast<void*>(data); }
        const_buffer(void const * beg, void const * end) 
        { 
            ptrdiff_t diff = reinterpret_cast<byte const*>(end) - reinterpret_cast<byte const*>(beg);
            coding_error_assert(diff >= 0);
            len = (ULONG)diff; 
            buf = (char*)const_cast<void*>(beg); 
        }

        const_buffer & operator += (size_t increment)
        {
            coding_error_assert(increment <= len);
            buf += increment;
            len -= (ULONG)increment;
            return *this;
        }

        byte const * cbegin() const { return reinterpret_cast<byte const*>(buf); }
        byte const * cend() const { return cbegin() + len; }
        size_t size() const { return len; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const & option) const;
    };

    struct mutable_buffer : public const_buffer
    {
        mutable_buffer() {}
        mutable_buffer(void * data, size_t size) : const_buffer(data, size) {}
        mutable_buffer(void * beg, void * end) : const_buffer(beg, end) {}

        mutable_buffer & operator += (size_t increment)
        {
            const_buffer::operator +=(increment);
            return *this;
        }

        byte * begin() const { return reinterpret_cast<byte*>(buf); }
        byte * end() const { return begin() + len; }
    };

    inline const_buffer operator + (const_buffer buf, size_t increment) { return buf += increment; }
    inline mutable_buffer operator + (mutable_buffer buf, size_t increment) { return buf += increment; }

    template <class T> T buffer_cast(mutable_buffer const & b)
    {
        return static_cast<T>( (void*)b.buf );
    }

    template <class T> T buffer_cast(const_buffer const & b)
    {
        return static_cast<T>( (void const*)b.buf );
    }

    inline size_t buffer_size(const_buffer const & b) { return b.size(); }

    // TODO: cleanup reinterpret_cast mess

    struct ConstBufferSequence_t
    {
        ConstBufferSequence_t (byte const * buf, DWORD len)
            : bufCount_(1)
        {
            wsaBuf_.buf = reinterpret_cast<char*>( const_cast<byte*>(buf) );
            wsaBuf_.len = len;
        }
        ConstBufferSequence_t (std::vector<const_buffer> const & buf)
            : bufCount_(static_cast<DWORD>(buf.size()) )
        {
            if (bufCount_ == 1)
            {
                wsaBuf_ = buf[0];
            }
            else
            {
                bufPtr_ = const_cast<const_buffer*>(buf.data());
            }
        }
        ConstBufferSequence_t (std::vector<WSABUF> const & buf)
            : bufCount_(static_cast<DWORD>(buf.size()) )
        {
            if (bufCount_ == 1)
            {
                wsaBuf_ = buf[0];
            }
            else
            {
                bufPtr_ = const_cast<WSABUF*>(buf.data());
            }
        }
        DWORD GetBufCount() const { return bufCount_; }
        WSABUF* GetWSABUFs() const { return bufCount_ == 1? &wsaBuf_ : bufPtr_; }
    private:
        DWORD bufCount_;
        union {
            mutable WSABUF *bufPtr_;
            mutable WSABUF wsaBuf_;
        } ;
    };

    struct BufferSequence_t : public ConstBufferSequence_t
    {
        BufferSequence_t (byte* buf, DWORD len)
            : ConstBufferSequence_t(buf, len)
        {}

        BufferSequence_t (std::vector<WSABUF> & buf)
            : ConstBufferSequence_t(buf)
        {}

        BufferSequence_t (std::vector<mutable_buffer> const & buf)
            : ConstBufferSequence_t(reinterpret_cast<std::vector<WSABUF> const&>(buf)) 
        {}
    };

    inline ConstBufferSequence_t BufferSequence(byte const * buf, DWORD len) { return ConstBufferSequence_t(buf, len); }
    inline BufferSequence_t BufferSequence(byte * buf, DWORD len) { return BufferSequence_t(buf, len); }

}
