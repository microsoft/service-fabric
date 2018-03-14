// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class DeployedServicePackageHealthId : public Serialization::FabricSerializable
        {
        public:
            DeployedServicePackageHealthId();

            DeployedServicePackageHealthId(
                std::wstring const & applicationName,
                std::wstring const & serviceManifestName,
                std::wstring const & servicePackageActivationId,
                NodeHealthId const & nodeId);

            DeployedServicePackageHealthId(
                std::wstring && applicationName,
                std::wstring && serviceManifestName,
                std::wstring && servicePackageActivationId,
                NodeHealthId const & nodeId);

            DeployedServicePackageHealthId(DeployedServicePackageHealthId const & other);
            DeployedServicePackageHealthId & operator = (DeployedServicePackageHealthId const & other);

            DeployedServicePackageHealthId(DeployedServicePackageHealthId && other);
            DeployedServicePackageHealthId & operator = (DeployedServicePackageHealthId && other);
            
            ~DeployedServicePackageHealthId();
                        
            __declspec(property(get=get_ApplicationName)) std::wstring const & ApplicationName;
            std::wstring const & get_ApplicationName() const { return applicationName_; }

            __declspec (property(get=get_ServiceManifestName)) std::wstring const & ServiceManifestName;
            std::wstring const & get_ServiceManifestName() const { return serviceManifestName_; }

            __declspec(property(get = get_ServicePackageActivationId)) std::wstring const & ServicePackageActivationId;
            std::wstring const & get_ServicePackageActivationId() const { return servicePackageActivationId_; }

            __declspec (property(get=get_NodeId)) NodeHealthId const & NodeId;
            NodeHealthId const & get_NodeId() const { return nodeId_; }
            
            bool operator == (DeployedServicePackageHealthId const & other) const;
            bool operator != (DeployedServicePackageHealthId const & other) const;
            bool operator < (DeployedServicePackageHealthId const & other) const;

            void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
            std::wstring ToString() const;

            FABRIC_FIELDS_04(applicationName_, serviceManifestName_, nodeId_, servicePackageActivationId_);

            // Test only
            static bool TryParse(
                std::wstring const & entityIdStr,
                __inout std::wstring & applicationName,
                __inout std::wstring & serviceManifestName,
                __inout std::wstring & servicePackageActivationId,
                __inout NodeHealthId & nodeId);
            
        private:
            std::wstring applicationName_;
            std::wstring serviceManifestName_;
            std::wstring servicePackageActivationId_;
            NodeHealthId nodeId_;
        };
    }
}
