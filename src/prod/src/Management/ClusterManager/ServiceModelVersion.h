// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class ServiceModelVersion : public TypeSafeString
        {
        public:
            ServiceModelVersion() : TypeSafeString() { }
            explicit ServiceModelVersion(std::wstring const & value) : TypeSafeString(value) { }
        };
    }
}

