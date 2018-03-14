// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class IDockerComposeAppTypeNameVersionGenerator
        {
        public:

            IDockerComposeAppTypeNameVersionGenerator() { }
            virtual ~IDockerComposeAppTypeNameVersionGenerator() { }

            virtual Common::ErrorCode GetTypeNameAndVersion(
                Store::StoreTransaction const &,
                Common::NamingUri const &,
                __out ServiceModelTypeName &,
                __out ServiceModelVersion &) = 0;

            virtual Common::ErrorCode GetNextVersion(
                Store::StoreTransaction const &,
                ComposeDeploymentContext const &,
                __out ServiceModelVersion &) = 0;

        };
    }
}
