// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    template <typename T>
    struct WPtrHash
    {
        size_t operator()(std::weak_ptr<T> const & wPtr) const
        {
            return std::hash<uint64>()((uint64)(wPtr._Get()));
        }
    };

    template <typename T>
    struct WPtrEqualTo
    {
        bool operator()(std::weak_ptr<T> const & left, std::weak_ptr<T> const & right) const
        {
            return left._Get() == right._Get();
        }
    };
}
