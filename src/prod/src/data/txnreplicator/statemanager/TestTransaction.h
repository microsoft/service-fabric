// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace StateManagerTests
{
    class TestTransaction : public TxnReplicator::TransactionBase
    {
        K_FORCE_SHARED(TestTransaction)

    public:
        static SPtr Create(__in LONG64 transactionId,__in KAllocator& allocator);

    private:
        NOFAIL TestTransaction(LONG64 transactionId);
    };
}
