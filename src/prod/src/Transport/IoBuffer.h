// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport 
{
#ifdef PLATFORM_UNIX
    struct ConstBuffer : public iovec 
    {
        ConstBuffer() { iov_base = nullptr; iov_len = 0; }
        ConstBuffer(void const * data, size_t size) { iov_base = const_cast<void*>(data); iov_len = size; }
        ConstBuffer(void const * beg, void const * end)
        {
            iov_base = const_cast<void*>(beg);
            iov_len = reinterpret_cast<byte const*>(end) - reinterpret_cast<byte const*>(beg);
            coding_error_assert(iov_len >= 0);
        }

        ConstBuffer & operator += (size_t increment)
        {
            coding_error_assert(increment <= iov_len);
            iov_base = (byte*)iov_base + increment;
            iov_len -= increment;
            return *this;
        }

        byte const * cbegin() const { return reinterpret_cast<byte const*>(iov_base); }
        byte const * cend() const { return cbegin() + iov_len; }
        size_t size() const { return iov_len; }
    };

    struct MutableBuffer : public ConstBuffer
    {
        MutableBuffer() {}
        MutableBuffer(void * data, size_t size) : ConstBuffer(data, size) {}
        MutableBuffer(void * beg, void * end) : ConstBuffer(beg, end) {}

        MutableBuffer & operator += (size_t increment)
        {
            ConstBuffer::operator +=(increment);
            return *this;
        }

        byte * begin() const { return reinterpret_cast<byte*>(iov_base); }
        byte * end() const { return begin() + iov_len; }
    };

#else
    typedef Common::const_buffer ConstBuffer;
    typedef Common::mutable_buffer MutableBuffer;
#endif

    inline ConstBuffer operator + (ConstBuffer buf, size_t increment) { return buf += increment; }
    inline MutableBuffer operator + (MutableBuffer buf, size_t increment) { return buf += increment; }
}
