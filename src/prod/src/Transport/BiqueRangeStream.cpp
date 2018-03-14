// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Transport
{
    TransportObject::TransportObject() : status_(STATUS_SUCCESS)
    {
    }

    TransportObject::~TransportObject()
    {
    }

    NTSTATUS TransportObject::GetStatus() const
    {
        return status_;
    }

    bool TransportObject::GetIsValid() const
    {
        return NT_SUCCESS(GetStatus());
    }

    void TransportObject::Fault(NTSTATUS status)
    {
        TESTASSERT_IFNOT(NT_SUCCESS(status));

        // failure status will not be overwritten
        if (NT_SUCCESS(status_))
        {
            status_ = status;
            OnFault(status);
        }
    }

    void TransportObject::OnFault(NTSTATUS /*status*/)
    {
    }

    BiqueRangeStream::BiqueRangeStream(ByteBiqueIterator const & begin, ByteBiqueIterator const & end) : begin_(begin), end_(end), size_(end_ - begin_), iter_(begin), bookmark_(begin)
    {
    }

    void BiqueRangeStream::WriteBytes(const void * buf, size_t count) 
    {
        if (!this->IsValid) return;

        const byte * beg = static_cast<const byte*>(buf);

        ASSERT_IF((iter_ >= end_) || (static_cast<size_t>(end_ - iter_) < count), "Write can only happen at existing locations, no appendding is allowed.");

        if (iter_ < end_)
        {
            // Update at existing locations
            while (count > 0)
            {
                // Write to the current bique fragment
                size_t available = std::min(count, iter_.fragment_size());
                KMemCpySafe(&(*iter_), available, beg, available);

                iter_ += available;
                beg += available;
                count -= available;
            }
        }
    }

    void BiqueRangeStream::ReadBytes(void * buf, size_t count) 
    {
        if (!this->IsValid) return;

        if (static_cast<size_t>(end_ - iter_) < count)
        {
            Fault(STATUS_UNSUCCESSFUL);
            trace.BiqueRangeStreamRangeCheckingFailure(TraceThis);
            Common::Assert::TestAssert();
            SeekToEnd();
            return;
        }

        byte * beg = static_cast<byte*>(buf);
        while (count > 0)
        {
            // Read from the current bique fragment
            size_t available = std::min(count, iter_.fragment_size());
            copy_n_checked(iter_, available, beg, available);

            // Move to the next bique fragment
            beg += available;
            iter_ += available;
            count -= available;
        }
    }

    void BiqueRangeStream::GetCurrentSegment(Common::const_buffer & buffer)
    {
        if (!this->IsValid) return;

        buffer.buf = reinterpret_cast<CHAR*>(iter_.fragment_begin());

        size_t remaining = end_ - iter_;
        if (iter_.fragment_size() <= remaining)
        {
            buffer.len = static_cast<ULONG>(iter_.fragment_size());
        }
        else
        {
            buffer.len = static_cast<ULONG>(remaining);
        }
    }

    void BiqueRangeStream::SeekForward(size_t offset)
    {
        if (!this->IsValid) return;

        size_t remaining = end_ - iter_;
        if (offset > remaining) // Need to work around a bique bug
        {
            Fault(STATUS_UNSUCCESSFUL);
            trace.BiqueRangeStreamRangeCheckingFailure(TraceThis);
            Common::Assert::TestAssert();
            SeekToEnd();
            return;
        }

        iter_ += offset;
    }
}
