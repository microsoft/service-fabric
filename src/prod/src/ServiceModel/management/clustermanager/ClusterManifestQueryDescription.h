// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class ClusterManifestQueryDescription : public ServiceModel::ClientServerMessageBody
        {
            DEFAULT_COPY_CONSTRUCTOR(ClusterManifestQueryDescription)
            DEFAULT_COPY_ASSIGNMENT(ClusterManifestQueryDescription)
        
        public:
            ClusterManifestQueryDescription();
            explicit ClusterManifestQueryDescription(Common::FabricConfigVersion &&);
            ClusterManifestQueryDescription(ClusterManifestQueryDescription &&);

            __declspec(property(get=get_ConfigVersion)) Common::FabricConfigVersion const & ConfigVersion;
            Common::FabricConfigVersion const & get_ConfigVersion() const { return version_; }

            Common::ErrorCode FromPublicApi(FABRIC_CLUSTER_MANIFEST_QUERY_DESCRIPTION const &);

            FABRIC_FIELDS_01(version_);

        private:
            Common::FabricConfigVersion version_;
        };
    }
}

