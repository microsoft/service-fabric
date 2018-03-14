// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class BiqueChunkIterator
    {
        DENY_COPY_ASSIGNMENT(BiqueChunkIterator);

    public:
        explicit BiqueChunkIterator(ByteBiqueRange const & first); 
        explicit BiqueChunkIterator(ByteBiqueRange const & first, ByteBiqueRange const & second); 
        static BiqueChunkIterator End(ByteBiqueRange const & first);
        static BiqueChunkIterator End(ByteBiqueRange const & first, ByteBiqueRange const & second);

        BiqueChunkIterator(BiqueChunkIterator const & source);
        BiqueChunkIterator & operator ++ ();
        Common::const_buffer operator * () const;
        Common::const_buffer * operator -> () const;
        bool operator == (BiqueChunkIterator const & rhs) const;
        bool operator != (BiqueChunkIterator const & rhs) const;

    private:
        void SetChunk();
        bool InFirstRange() const;

        ByteBiqueRange first_;
        ByteBiqueRange second_;
        ByteBiqueRange* currentRange_;
        ByteBiqueIterator cursor_;
        mutable Common::const_buffer chunk_;
    };

    inline bool BiqueChunkIterator::operator != (BiqueChunkIterator const & rhs) const
    {
        return !(*this == rhs);
    }

    inline Common::const_buffer BiqueChunkIterator::operator * () const
    {
        return chunk_;
    }

    inline Common::const_buffer * BiqueChunkIterator::operator -> () const
    {
        return &chunk_;
    }

    inline bool BiqueChunkIterator::operator == (BiqueChunkIterator const & rhs) const
    {
        return (this->InFirstRange() == rhs.InFirstRange()) && (cursor_ == rhs.cursor_);
    }

    inline bool BiqueChunkIterator::InFirstRange() const
    {
        return currentRange_ == &first_;
    }
}
