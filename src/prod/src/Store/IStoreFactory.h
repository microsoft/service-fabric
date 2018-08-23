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
            StoreFactoryParameters const &,
            __out ILocalStoreSPtr &) = 0;

        virtual Common::ErrorCode CreateLocalStore(
            StoreFactoryParameters const &,
            __out ILocalStoreSPtr &,
            __out ServiceModel::HealthReport &) = 0;

        virtual ~IStoreFactory() {}
    };
}
