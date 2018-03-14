// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class ServiceModelPackageName : public TypeSafeString
        {
        public:
            ServiceModelPackageName() : TypeSafeString() { }
            explicit ServiceModelPackageName(std::wstring const & value) : TypeSafeString(value) { }
        };
    }
}
