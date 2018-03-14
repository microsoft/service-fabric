// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    template <typename TKey>
    interface IComparer
    {
        K_SHARED_INTERFACE(IComparer);

    public:

        virtual int Compare(__in const TKey& x, __in const TKey& y) const = 0;
    };
}
