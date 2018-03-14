// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Federation/TimeRange.h"

using namespace std;
using namespace Federation;

void TimeRange::Merge(TimeRange const & rhs)
{
    bool isValid = (rhs.end_ > rhs.begin_);

    if (end_ > begin_)
    {
        if (!isValid)
        {
            return;
        }

        if (rhs.begin_ < end_)
        {
            if (begin_ <= rhs.end_)
            {
                begin_ = min(begin_, rhs.begin_);
                end_ = max(end_, rhs.end_);
            }

            return;
        }
    }
    else if (!isValid && rhs.end_ < end_)
    {
        return;
    }

    begin_ = rhs.begin_;
    end_ = rhs.end_;
}
