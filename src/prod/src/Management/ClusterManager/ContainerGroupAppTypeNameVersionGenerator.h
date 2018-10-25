//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class ContainerGroupAppTypeNameVersionGenerator : public IApplicationDeploymentTypeNameVersionGenerator
        {
            DENY_COPY(ContainerGroupAppTypeNameVersionGenerator);

        public:
            ContainerGroupAppTypeNameVersionGenerator() { }
            virtual ~ContainerGroupAppTypeNameVersionGenerator() { }

            Common::ErrorCode GetTypeNameAndVersion(
                Store::StoreTransaction const &,
                Common::NamingUri const &,
                __out ServiceModelTypeName &,
                __out ServiceModelVersion &) override;

            Common::ErrorCode GetNextVersion(
                Store::StoreTransaction const &,
                std::wstring const &,
                ServiceModelVersion const &,
                __out ServiceModelVersion &) override;
        };
    }
}
