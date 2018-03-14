// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class ServiceModelServiceName : public TypeSafeString
        {
        public:
            ServiceModelServiceName() : TypeSafeString() { }
            explicit ServiceModelServiceName(std::wstring const & value) : TypeSafeString(value) { }
            explicit ServiceModelServiceName(std::wstring  && value) : TypeSafeString(move(value)) { }
        };
    }
}

DEFINE_USER_ARRAY_UTILITY(Management::ClusterManager::ServiceModelServiceName);
