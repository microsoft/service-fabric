// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        // Unordered map for int64 keys.
        template <class T>
        using Uint64UnorderedMap = std::unordered_map < uint64, T > ;

        struct GuidHasher
        {
            size_t operator() (Common::Guid const & guid) const
            {
                return guid.GetHashCode();
            }

            bool operator() (Common::Guid const & left, Common::Guid const & right) const
            {
                return (left == right);
            }
        };

        // Unordered map for GUIDs.
        template <class T>
        using GuidUnorderedMap = std::unordered_map < Common::Guid, T, GuidHasher > ;
    }
}
