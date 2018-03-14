// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class ServiceModelApplicationId : public TypeSafeString
        {
        public:
            ServiceModelApplicationId() : TypeSafeString() { }
            explicit ServiceModelApplicationId(std::wstring const & value) : TypeSafeString(value) { }
        };
    }
}

