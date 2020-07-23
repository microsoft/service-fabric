// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventStore.Service.Cache
{
    using System;

    internal interface ITimeComparable
    {
        int CompareTimeRangeOrder(DateTimeOffset startTime, DateTimeOffset endTime);

        int CompareTimeOrder(DateTimeOffset time);
    }
}
