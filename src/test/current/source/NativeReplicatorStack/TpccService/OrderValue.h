// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    class OrderValue
        : public KObject<OrderValue>
        , public KShared<OrderValue>
    {
        K_FORCE_SHARED(OrderValue)

    public:
        static NTSTATUS Create(
            __in KAllocator& allocator,
            __out SPtr& result);

        PROPERTY(int, carrier_, Carrier)
        PROPERTY(int, lineCount_, LineCount)
        PROPERTY(bool, local_, Local)

    private:
        // DateTime entry_;
        int carrier_;
        int lineCount_;
        bool local_;
    };
}
