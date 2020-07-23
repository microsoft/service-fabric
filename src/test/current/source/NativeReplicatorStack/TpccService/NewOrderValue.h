// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    class NewOrderValue
        : public KObject<NewOrderValue>
        , public KShared<NewOrderValue>
    {
        K_FORCE_SHARED(NewOrderValue)

    public:
        static NTSTATUS Create(
            __in KAllocator& allocator,
            __out SPtr& result);

    private:

    };
}
