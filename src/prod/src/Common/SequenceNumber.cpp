// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Common/SequenceNumber.h"

namespace Common
{
    __volatile __int64 SequenceNumber::LastTimeStamp = 0;

    int64 SequenceNumber::GetNext()
    {
        int64 value = DateTime::Now().Ticks;
        int64 oldValue;

        do
        {
            oldValue = LastTimeStamp;
            if (value <= oldValue)
            {
                value = oldValue + 1;
            }
        } while (InterlockedCompareExchange64(&LastTimeStamp, value, oldValue) != oldValue);

        return value;
    }
}
