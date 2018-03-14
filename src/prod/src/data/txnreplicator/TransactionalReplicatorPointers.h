// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    class ITransactionalReplicatorFactory;

    struct TransactionalReplicatorFactoryConstructorParameters
    {
        Common::ComponentRoot const * Root;
    };

    typedef std::unique_ptr<ITransactionalReplicatorFactory> ITransactionalReplicatorFactoryUPtr;
    typedef ITransactionalReplicatorFactoryUPtr TransactionalReplicatorFactoryFactoryFunctionPtr(TransactionalReplicatorFactoryConstructorParameters &);

    TransactionalReplicatorFactoryFactoryFunctionPtr TransactionalReplicatorFactoryFactory;
}
