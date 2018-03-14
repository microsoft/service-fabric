// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Transport
{
    BiqueWriteStream::BiqueWriteStream(ByteBique & b) : q_(b), iter_(q_.begin()) 
    {
    }

    void BiqueWriteStream::WriteBytes(const void * buf, size_t count) 
    {
        const byte * beg = static_cast<const byte*>(buf);

        if (iter_ < q_.end())
        {
            ASSERT_IF(count > static_cast<size_t>(q_.end() - iter_), "Update can only be done at existing locations in bique.");

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
        else
        {
            ASSERT_IF(iter_ > q_.end(), "Append can only happen at the end, not beyond the end of bique.");

            // Append at the end
            q_.insert(q_.end(), beg, beg + count);
            iter_ = q_.end();
        } 
    }
}
