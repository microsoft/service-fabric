// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class ITransactionBase
    {
    public:
        virtual ~ITransactionBase() {};

        virtual Common::Guid get_Id() = 0;

        virtual FABRIC_TRANSACTION_ISOLATION_LEVEL get_IsolationLevel() = 0;
    };
}
