// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IEseLocalStoreSettingsResult
    {
    public:
        virtual ~IEseLocalStoreSettingsResult() {};

        virtual void ToPublicApi(__in Common::ScopedHeap &, __out FABRIC_ESE_LOCAL_STORE_SETTINGS &) const = 0;
    };
}
