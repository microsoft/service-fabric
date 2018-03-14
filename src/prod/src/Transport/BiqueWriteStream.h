// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    //
    // This stream supports insert/append at the end and write at random existing locations.
    //
    class BiqueWriteStream : public Common::ByteStream<BiqueWriteStream>
    {
        DENY_COPY(BiqueWriteStream);

    public:
        explicit BiqueWriteStream(ByteBique & b);

        // Stream classes need to implement the following method for serialization.
        void WriteBytes(const void * buf, size_t count);

        void SeekToEnd();
        void SeekTo(ByteBiqueIterator cursor);
        ByteBiqueIterator GetCursor() const;
        size_t Size() const;

    private:
        ByteBique & q_;
        ByteBiqueIterator iter_;
    };

    inline void BiqueWriteStream::SeekTo(ByteBiqueIterator cursor)
    {
        iter_ = cursor;
    }

    inline void BiqueWriteStream::SeekToEnd()
    {
        iter_ = q_.end();
    }

    inline ByteBiqueIterator BiqueWriteStream::GetCursor() const
    {
        return iter_;
    }

    inline size_t BiqueWriteStream::Size() const
    {
        return q_.size();
    } 
}
