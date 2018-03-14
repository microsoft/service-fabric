// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    // This class represents a key range for a partition (a lowkey and a highkey)
    // It is not serializable as it is intended to be lightweight and not 
    // incur the overhead of including all the serialization related members and a vtable
    class KeyRange
    {
    public:
        KeyRange(__int64 lowKeyInclusive, __int64 highKeyInclusive) :
            highKeyInclusive_(highKeyInclusive),
            lowKeyInclusive_(lowKeyInclusive)
        {
            ASSERT_IF(highKeyInclusive_ < lowKeyInclusive_, "Invalid key range {0} {1}", lowKeyInclusive, highKeyInclusive);
        }

        __declspec(property(get = get_LowKeyInclusive)) __int64 LowKey;
        inline __int64 get_LowKeyInclusive() const { return lowKeyInclusive_; }

        __declspec(property(get = get_HighKeyInclusive)) __int64 HighKey;
        inline __int64 get_HighKeyInclusive() const { return highKeyInclusive_; }

        bool ContainsKey(__int64 key) const
        {
            return key >= lowKeyInclusive_ && key <= highKeyInclusive_;
        }

    private:
        __int64 lowKeyInclusive_;
        __int64 highKeyInclusive_;
    };
}


