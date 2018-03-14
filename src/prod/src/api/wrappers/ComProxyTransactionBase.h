// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class ComProxyTransactionBase :
        public Common::ComponentRoot,
        public ITransactionBase
    {
        DENY_COPY(ComProxyTransactionBase);

    public:
        ComProxyTransactionBase(Common::ComPointer<IFabricTransactionBase> const & comImpl);
        virtual ~ComProxyTransactionBase();
            
        Common::Guid get_Id();

        FABRIC_TRANSACTION_ISOLATION_LEVEL get_IsolationLevel();

    private:
        Common::ComPointer<IFabricTransactionBase> const comImpl_;
    };
}
