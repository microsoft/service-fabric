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
        ResourceDescription(
            Hosting2::CodePackageInstanceIdentifier const & codePackageIdentifier,
            std::wstring const & appName,
            bool isLinuxContainerIsolated = false);
        ResourceDescription(const ResourceDescription & other) = default;
        ~ResourceDescription() = default;

        __declspec(property(get = get_PartitionId)) Common::Guid const & PartitionId;
        Common::Guid const & get_PartitionId() const { return codePackageIdentifier_.ServicePackageInstanceId.ActivationContext.ActivationGuid; }

        __declspec(property(get = get_ServicePackageId)) ServiceModel::ServicePackageIdentifier const & ServicePackageId;
        ServiceModel::ServicePackageIdentifier const & get_ServicePackageId() const { return codePackageIdentifier_.ServicePackageInstanceId.ServicePackageId; }

        __declspec(property(get = get_IsExclusive)) bool IsExclusive;
        bool get_IsExclusive() const { return codePackageIdentifier_.ServicePackageInstanceId.ActivationContext.IsExclusive; }

        __declspec(property(get = get_IsLinuxContainerIsolated)) bool IsLinuxContainerIsolated;
        bool get_IsLinuxContainerIsolated() const { return isLinuxContainerIsolated_; }

        __declspec(property(get = get_AppHosts)) std::set<std::wstring> AppHosts;
        std::set<std::wstring> get_AppHosts() const { return appHosts_; }

        void UpdateAppHosts(std::wstring const& appHost, bool IsAdd);

        ResourceUsage resourceUsage_;
        Hosting2::CodePackageInstanceIdentifier codePackageIdentifier_;
        std::wstring appName_;

        // need to determin if we need to summarize all container load or not
        // if container is isolated => load that is sent is on service package level, so do not summarize all reports
        bool isLinuxContainerIsolated_;

        // used for partition usage => number of active code packages inside service package
        std::set<std::wstring> appHosts_;
    };
}
