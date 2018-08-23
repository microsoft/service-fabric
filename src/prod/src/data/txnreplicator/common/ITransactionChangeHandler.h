// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    interface ITransactionChangeHandler
    {
        K_SHARED_INTERFACE(ITransactionChangeHandler)

    public:
        virtual void OnTransactionCommitted(
            __in ITransactionalReplicator & source,
            __in ITransaction const & transaction) = 0;
    };
}
