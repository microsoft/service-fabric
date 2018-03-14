// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {    
        class ProvisionFabricBody : public ServiceModel::ClientServerMessageBody
        {
        public:
            ProvisionFabricBody() 
            {
            }

            ProvisionFabricBody(std::wstring const & codeFilepath, std::wstring const & clusterManifestFilepath) 
                : codeFilepath_(codeFilepath)
                , clusterManifestFilepath_(clusterManifestFilepath)
            {
            }

            __declspec(property(get=get_CodeFilepath)) std::wstring const & CodeFilepath;
            __declspec(property(get=get_ClusterManifestFilepath)) std::wstring const & ClusterManifestFilepath;

            std::wstring const & get_CodeFilepath() const { return codeFilepath_; }
            std::wstring const & get_ClusterManifestFilepath() const { return clusterManifestFilepath_; }

            FABRIC_FIELDS_02(codeFilepath_, clusterManifestFilepath_);

        private:
            std::wstring codeFilepath_;
            std::wstring clusterManifestFilepath_;
        };
    }
}
