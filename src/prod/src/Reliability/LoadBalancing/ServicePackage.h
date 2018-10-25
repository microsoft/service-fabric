// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "ServicePackageDescription.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class ServicePackage
        {
            DENY_COPY_ASSIGNMENT(ServicePackage);
            DENY_COPY_CONSTRUCTOR(ServicePackage);

        public:
            static Common::GlobalWString const FormatHeader;

            ServicePackage(ServicePackageDescription && description);

            ServicePackage(ServicePackage && other);

            __declspec(property(get = get_Description)) ServicePackageDescription const& Description;
            ServicePackageDescription const& get_Description() const { return description_; }

            __declspec(property(get = get_HasRGMetric)) bool HasRGMetrics;
            bool get_HasRGMetric() const { return (description_.RequiredResources.size() > 0); }

            __declspec(property(get = get_HasContainerImages)) bool HasContainerImages;
            bool get_HasContainerImages() const { return (description_.ContainersImages.size() > 0); }

            __declspec(property(get = get_Services)) std::unordered_set<uint64> & Services;
            std::unordered_set<uint64> & get_Services() { return services_; }

            bool UpdateDescription(ServicePackageDescription && description);

        private:
            ServicePackageDescription description_;
            std::unordered_set<uint64> services_;
        };
    }
}
