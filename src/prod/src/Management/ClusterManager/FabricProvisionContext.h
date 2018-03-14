// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class FabricProvisionContext : public RolloutContext
        {
            DENY_COPY(FabricProvisionContext)

        public:
            static const RolloutContextType::Enum ContextType;

            FabricProvisionContext();

            FabricProvisionContext(
                FabricProvisionContext &&);

            FabricProvisionContext & operator=(
                FabricProvisionContext &&);

            FabricProvisionContext(
                Common::ComponentRoot const &,
                ClientRequestSPtr const &,
                std::wstring const & codeFilepath,
                std::wstring const & clusterManifestFilepath,
                Common::FabricVersion const &);

            FabricProvisionContext(
                Common::ComponentRoot const &,
                Store::ReplicaActivityId const &,
                std::wstring const & codeFilepath,
                std::wstring const & clusterManifestFilepath,
                Common::FabricVersion const &);

            virtual ~FabricProvisionContext();

            __declspec(property(get=get_CodeFilepath)) std::wstring const & CodeFilepath;
            std::wstring const & get_CodeFilepath() const { return codeFilepath_; }

            __declspec(property(get=get_ClusterManifestFilepath)) std::wstring const & ClusterManifestFilepath;
            std::wstring const & get_ClusterManifestFilepath() const { return clusterManifestFilepath_; }

            __declspec(property(get=get_FabricVersion)) Common::FabricVersion const & Version;
            Common::FabricVersion const & get_FabricVersion() const { return version_; }

            __declspec(property(get=get_IsProvision)) bool IsProvision;
            bool get_IsProvision() const { return isProvision_; }

            __declspec(property(get=get_ProvisionedCodeVersions)) std::set<Common::FabricCodeVersion> const & ProvisionedCodeVersions;
            std::set<Common::FabricCodeVersion> const & get_ProvisionedCodeVersions() const { return provisionedCodeVersions_; }

            __declspec(property(get=get_ProvisionedConfigVersions)) std::set<Common::FabricConfigVersion> const & ProvisionedConfigVersions;
            std::set<Common::FabricConfigVersion> const & get_ProvisionedConfigVersions() const { return provisionedConfigVersions_; }

            void OnFailRolloutContext() override { } // no-op

            bool ContainsCodeVersion(Common::FabricVersion const &);
            bool ContainsConfigVersion(Common::FabricVersion const &);

            std::wstring FormatProvisionedCodeVersions() const;
            std::wstring FormatProvisionedConfigVersions() const;

            Common::ErrorCode StartProvisioning(
                Store::StoreTransaction const &,
                Common::ComponentRoot const &,
                ClientRequestSPtr const &,
                std::wstring const & codeFilepath, 
                std::wstring const & clusterManifestFilepath, 
                Common::FabricVersion const &);
            Common::ErrorCode StartUnprovisioning(
                Store::StoreTransaction const &,
                Common::ComponentRoot const &,
                ClientRequestSPtr const &,
                Common::FabricVersion const &);
            Common::ErrorCode CompleteProvisioning(Store::StoreTransaction const &);
            Common::ErrorCode CompleteUnprovisioning(Store::StoreTransaction const &);

            virtual std::wstring const & get_Type() const;
            virtual void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
                
            FABRIC_FIELDS_06(codeFilepath_, clusterManifestFilepath_, version_, isProvision_, provisionedCodeVersions_, provisionedConfigVersions_);
            
        protected:
            virtual std::wstring ConstructKey() const;
            
        private:
            std::wstring codeFilepath_;
            std::wstring clusterManifestFilepath_;
            Common::FabricVersion version_;
            bool isProvision_;

            std::set<Common::FabricCodeVersion> provisionedCodeVersions_;
            std::set<Common::FabricConfigVersion> provisionedConfigVersions_;
        };
    }
}
