// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    typedef std::vector<BYTE> ByteBuffer;
    typedef std::unique_ptr<std::vector<BYTE>> ByteBufferUPtr;

    class ByteBuffer2 // avoid std::vector overhead of initializing every new element when resizing
    {
        DENY_COPY(ByteBuffer2);

    public:
        ByteBuffer2()
        {
        }

        ByteBuffer2(size_t size)
            : data_(size? new unsigned char[size] : nullptr)
            , capacity_(size)
            , size_(size)
            , appendCursor_(data_)
        {
        }

        ByteBuffer2(ByteBuffer2 && other)
        {
            *this = std::move(other);
        }

        ~ByteBuffer2()
        {
            reset();
        }

        ByteBuffer2 & operator = (ByteBuffer2 && other)
        {
            std::swap(data_, other.data_);
            std::swap(capacity_, other.capacity_);
            std::swap(size_, other.size_);
            std::swap(appendCursor_, other.appendCursor_);

            other.reset();
            return *this;
        }

        size_t size() const noexcept { return size_; }
        bool empty() const noexcept { return size_ == 0; }

        unsigned char* data() const noexcept { return data_; }
        unsigned char* end() const noexcept { return data_ + size_; }

        void resize(size_t newSize)
        {
            if (newSize <= capacity_)
            {
                size_ = newSize;
                return;
            }

            auto newData = new unsigned char[newSize];
            memcpy(newData, data_, size_);
            std::swap(data_, newData);
            delete newData;

            capacity_ = newSize;
            size_ = newSize;
            appendCursor_ = data_;
        }

        void reset()
        {
            if (!data_) return;

            delete data_;
            data_ = nullptr;
            capacity_ = 0;
            size_ = 0;
        }

        void append(void const* buf, uint size)
        {
            ASSERT(data_);
            ASSERT(size_ > 0);
            ASSERT(data_ <= appendCursor_);
            ASSERT(appendCursor_ < (data_ + size_));

            if (!size) return;

            int64 remaining = data_ + size_ - appendCursor_;
            Invariant((int64)size <= remaining);
            memcpy(appendCursor_, buf, size);
            appendCursor_ += size;
        }

        unsigned char* AppendCursor() const noexcept { return appendCursor_; }

        unsigned char* AdvanceAppendCursor(uint offset) noexcept
        {
            Invariant((appendCursor_ + offset) <= (data_ + size_));
            appendCursor_ += offset;
            return appendCursor_;
        }

        void ResetAppendCursor() noexcept
        {
            Invariant(data_);
            appendCursor_ = data_;
        }

        void SetSizeAfterAppend() noexcept
        {
            size_ = appendCursor_ - data_;
            Invariant(size_ <= capacity_);
        }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const & option) const;

    private:
        unsigned char* data_ = nullptr;
        size_t capacity_ = 0;
        size_t size_ = 0;
        unsigned char* appendCursor_ = nullptr;
    };

    bool operator == (ByteBuffer const & b1, ByteBuffer const & b2);
    bool operator < (ByteBuffer const & b1, ByteBuffer const & b2);

    ByteBuffer StringToByteBuffer(std::wstring const& str);
    std::wstring ByteBufferToString(ByteBuffer const & buffer);

    bool operator == (ByteBuffer2 const & b1, ByteBuffer2 const & b2);
    bool operator < (ByteBuffer2 const & b1, ByteBuffer2 const & b2);

    ByteBuffer2 StringToByteBuffer2(std::wstring const& str);

#ifdef PLATFORM_UNIX

    struct BioDeleter
    {
        void operator()(BIO* bio)
        {
            if (bio)
            {
                BIO_free(bio);
            }
        }
    };

    typedef std::unique_ptr<BIO, BioDeleter> BioUPtr;

    BioUPtr CreateBioMemFromBufferAndSize(BYTE const* buffer, size_t size);

    ByteBuffer BioMemToByteBuffer(BIO* bio);
    BioUPtr ByteBufferToBioMem(ByteBuffer const & buffer);

    ByteBuffer2 BioMemToByteBuffer2(BIO* bio);
    BioUPtr ByteBuffer2ToBioMem(ByteBuffer2 const & buffer);
#endif
}
