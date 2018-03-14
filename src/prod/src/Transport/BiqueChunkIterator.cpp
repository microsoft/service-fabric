// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Transport
{
    BiqueChunkIterator::BiqueChunkIterator(ByteBiqueRange const & first)
        : first_(first.Begin, first.End, false), 
        second_(EmptyByteBique.begin(), EmptyByteBique.end(), false), 
        currentRange_(&first_), 
        cursor_(first_.Begin)
    {
        if (cursor_ == first_.End)
        {
            currentRange_ = &second_;
            cursor_ = second_.Begin;
        }

        SetChunk();
    }

    BiqueChunkIterator::BiqueChunkIterator(ByteBiqueRange const & first, ByteBiqueRange const & second)
        : first_(first.Begin, first.End, false), second_(second.Begin, second.End, false), currentRange_(&first_), cursor_(first_.Begin)
    {
        CODING_ERROR_ASSERT(&first_ != &second_);

        if (cursor_ == first_.End)
        {
            currentRange_ = &second_;
            cursor_ = second_.Begin;
        }

        SetChunk();
    }

    BiqueChunkIterator BiqueChunkIterator::End(ByteBiqueRange const & first)
    {
        BiqueChunkIterator result = End(first, EmptyByteBiqueRange);
        return result;
    }

    BiqueChunkIterator BiqueChunkIterator::End(ByteBiqueRange const & first, ByteBiqueRange const & second)
    {
        BiqueChunkIterator end(first, second);

        end.currentRange_ = &(end.second_);
        end.cursor_ = end.second_.End;
        end.SetChunk();

        return end;
    }

    BiqueChunkIterator::BiqueChunkIterator(BiqueChunkIterator const & source) 
        : first_(source.first_.Begin, source.first_.End, false), 
        second_(source.second_.Begin, source.second_.End, false), 
        currentRange_(&first_), 
        cursor_(source.cursor_), 
        chunk_(source.chunk_)
    {
        if (!source.InFirstRange())
        {
            currentRange_ = &second_;
        }
    }

    BiqueChunkIterator & BiqueChunkIterator::operator ++ ()
    {
        cursor_ += chunk_.size(); // Move to the next chunk
        if (InFirstRange() && (cursor_ == first_.End))
        {
            // We are at the last chunk of the first range, so we move the second range
            currentRange_ = &second_;
            cursor_ = second_.Begin;
        }

        SetChunk();
        return *this;
    }

    void BiqueChunkIterator::SetChunk()
    {
        if (cursor_ < currentRange_->End)
        {
            chunk_.buf = reinterpret_cast<char*>(cursor_.fragment_begin());

            if ((cursor_ + cursor_.fragment_size()) <= currentRange_->End)
            {
                chunk_.len = static_cast<ULONG>(cursor_.fragment_size());
            }
            else
            {
                // This is the last chunk and it is not filled yet
                chunk_.len = static_cast<ULONG>(currentRange_->End - cursor_);
            }
        }
        else
        {
            chunk_.buf = nullptr;
            chunk_.len = 0;
        }
    }
}
