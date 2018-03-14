// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Reliability/Reliability.h"

namespace ResourceMonitor
{
    class ResourceDescription
    {
    public:
        ResourceDescription() = default;
        ResourceDescription(Hosting2::CodePackageInstanceIdentifier const & codePackageIdentifier, std::wstring const & appName);
        ResourceDescription(const ResourceDescription & other) = default;
        ~ResourceDescription() = default;

        __declspec(property(get = get_PartitionId)) Common::Guid const & PartitionId;
        Common::Guid const & get_PartitionId() const { return codePackageIdentifier_.ServicePackageInstanceId.ActivationContext.ActivationGuid; }

        __declspec(property(get = get_IsExclusive)) bool IsExclusive;
        bool get_IsExclusive() const { return codePackageIdentifier_.ServicePackageInstanceId.ActivationContext.IsExclusive; }

        ResourceUsage resourceUsage_;
        Hosting2::CodePackageInstanceIdentifier codePackageIdentifier_;
        std::wstring appName_;
    };
}
