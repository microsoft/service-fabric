// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class IStoreFactory
    {
    public:
        virtual Common::ErrorCode CreateLocalStore(
            StoreFactoryParameters const & factoryParameters,
            __out ILocalStoreSPtr & store) = 0;

        virtual ~IStoreFactory() {}
    };
}
