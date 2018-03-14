// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class TransportObject
    {
    public:
        TransportObject();
        virtual ~TransportObject() = 0;

        __declspec(property(get=GetStatus)) NTSTATUS Status;
        __declspec(property(get=GetIsValid)) bool IsValid;

        virtual NTSTATUS GetStatus() const;
        bool GetIsValid() const;

        void Fault(NTSTATUS status);

    private:
        virtual void OnFault(NTSTATUS /*status*/);

    private:
        NTSTATUS status_;
    };

    //
    // This stream supports read/write at random existing locations, but NO appending is allowed.
    //
    class BiqueRangeStream : public Common::ByteStream<BiqueRangeStream>, public TransportObject
    {
        DENY_COPY_ASSIGNMENT(BiqueRangeStream);

    public:
        explicit BiqueRangeStream(ByteBiqueIterator const& begin, ByteBiqueIterator const& end);

        // Stream classes need to implement the following two methods for serialization and deserialization respectively.
        void WriteBytes(const void * buf, size_t count);
        void ReadBytes(void * buf, size_t count);

        void SeekForward(size_t offset);
        bool Eos() const;
        void SetBookmark();
        void SeekToBookmark() const;
        bool HasBookmarkEqualTo(const BiqueRangeStream& rhs) const;
        void SeekToBegin() const;
        void SeekToEnd() const;
        const ByteBiqueIterator GetBookmark() const { return bookmark_; }

        void GetCurrentSegment(Common::const_buffer & buffer);

        size_t Size() const;

    private:
        const ByteBiqueIterator begin_;
        const ByteBiqueIterator end_;
        const size_t size_;
        mutable ByteBiqueIterator iter_;
        ByteBiqueIterator bookmark_;
    };

    inline size_t BiqueRangeStream::Size() const
    {
        return size_;
    }

    inline bool BiqueRangeStream::Eos() const
    {
        return iter_ == end_;
    }

    inline void BiqueRangeStream::SetBookmark()
    {
        bookmark_ = iter_;
    }

    inline void BiqueRangeStream::SeekToBookmark() const
    {
        iter_ = bookmark_;
    }

    inline void BiqueRangeStream::SeekToBegin() const
    {
        iter_ = begin_;
    }

    inline void BiqueRangeStream::SeekToEnd() const
    {
        iter_ = end_;
    }

    inline bool BiqueRangeStream::HasBookmarkEqualTo(const BiqueRangeStream& rhs) const
    {
        return bookmark_ == rhs.bookmark_;
    }
}
