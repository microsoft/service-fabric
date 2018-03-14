// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class DockerComposeAppTypeNameVersionGenerator : public IDockerComposeAppTypeNameVersionGenerator
        {
            DENY_COPY(DockerComposeAppTypeNameVersionGenerator);
        public:
            DockerComposeAppTypeNameVersionGenerator() { }
            virtual ~DockerComposeAppTypeNameVersionGenerator() { }

            virtual Common::ErrorCode GetTypeNameAndVersion(
                Store::StoreTransaction const &,
                Common::NamingUri const &,
                __out ServiceModelTypeName &,
                __out ServiceModelVersion &);

            virtual Common::ErrorCode GetNextVersion(
                Store::StoreTransaction const &,
                ComposeDeploymentContext const &,
                __out ServiceModelVersion &);

        private:
            Common::ErrorCode GetPreviousTargetVersion(Store::StoreTransaction const &, std::wstring const &deploymentName, __out ServiceModelVersion &);

        };
    }
}
