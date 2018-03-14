// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    interface IDataLossHandler
    {
        K_SHARED_INTERFACE(IDataLossHandler)

    public:
        virtual  ktl::Awaitable<bool> DataLossAsync(__in ktl::CancellationToken const & cancellationToken) = 0;
    };
}
