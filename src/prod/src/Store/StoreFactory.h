// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{ 
    class StoreFactory : public IStoreFactory
    {
    public:
        Common::ErrorCode CreateLocalStore(
            StoreFactoryParameters const & factoryParameters,
            __out ILocalStoreSPtr & store);
    };
}
