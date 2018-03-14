// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class DeployedApplicationHealthId : public Serialization::FabricSerializable
        {
        public:
            DeployedApplicationHealthId();

            DeployedApplicationHealthId(
                std::wstring const & applicationName,
                NodeHealthId const & nodeId);

            DeployedApplicationHealthId(
                std::wstring && applicationName,
                NodeHealthId const & nodeId);

            DeployedApplicationHealthId(DeployedApplicationHealthId const & other);
            DeployedApplicationHealthId & operator = (DeployedApplicationHealthId const & other);

            DeployedApplicationHealthId(DeployedApplicationHealthId && other);
            DeployedApplicationHealthId & operator = (DeployedApplicationHealthId && other);
            
            ~DeployedApplicationHealthId();
                        
            __declspec(property(get=get_ApplicationName)) std::wstring const & ApplicationName;
            std::wstring const & get_ApplicationName() const { return applicationName_; }

            __declspec (property(get=get_NodeId)) NodeHealthId const & NodeId;
            NodeHealthId const & get_NodeId() const { return nodeId_; }
            
            bool operator == (DeployedApplicationHealthId const & other) const;
            bool operator != (DeployedApplicationHealthId const & other) const;
            bool operator < (DeployedApplicationHealthId const & other) const;

            void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
            std::wstring ToString() const;

            FABRIC_FIELDS_02(applicationName_, nodeId_);

            // Test only
            static bool TryParse(
                std::wstring const & entityIdStr,
                __inout std::wstring & applicationName,
                __inout NodeHealthId & nodeId);
            
        private:
            std::wstring applicationName_;
            NodeHealthId nodeId_;
        };
    }
}
