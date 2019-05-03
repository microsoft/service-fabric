// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class IApplicationDeploymentTypeNameVersionGenerator
        {
        public:

            IApplicationDeploymentTypeNameVersionGenerator() { }
            virtual ~IApplicationDeploymentTypeNameVersionGenerator() { }

            virtual Common::ErrorCode GetTypeNameAndVersion(
                Store::StoreTransaction const &,
                Common::NamingUri const &,
                __out ServiceModelTypeName &,
                __out ServiceModelVersion &) = 0;

            virtual Common::ErrorCode GetNextVersion(
                Store::StoreTransaction const &,
                std::wstring const & deploymentName,
                ServiceModelVersion const &,
                __out ServiceModelVersion &) = 0;

        };
    }
}
