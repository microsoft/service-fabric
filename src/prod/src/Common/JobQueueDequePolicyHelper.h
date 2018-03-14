// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    enum DequePolicy
    {
        Fifo,
        Lifo,
        FifoLifo
    };

    class JobQueueDequePolicyHelper
    {
        DENY_COPY(JobQueueDequePolicyHelper);

    public:
        explicit JobQueueDequePolicyHelper(DequePolicy dequePolicy);
        ~JobQueueDequePolicyHelper();

        void UpdateOnQueueFull();
        void UpdateOnQueueEmpty();

        bool ShouldTakeFistItem() const;

    private:
        DequePolicy dequePolicy_;
        bool isFifo_;
    };
}
