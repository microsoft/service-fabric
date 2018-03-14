// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    // This class can be used to get unique, increasing value at each call
    // to GetNext
    class SequenceNumber
    {
    public:
        static int64 GetNext();

    private:
        static __volatile __int64 LastTimeStamp;
    };
}
