// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IStoreItemEnumerator : public IStoreItemMetadataEnumerator
    {
    public:
        virtual ~IStoreItemEnumerator() {};

        virtual IStoreItemPtr GetCurrentItem() = 0;
    };
}
