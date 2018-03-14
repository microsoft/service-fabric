// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace RepairManager
    {
        class NodeImpact
            : public Serialization::FabricSerializable
            , public Common::IFabricJsonSerializable
        {
            DEFAULT_COPY_CONSTRUCTOR(NodeImpact)
        public:
            NodeImpact();
            NodeImpact(NodeImpact &&);

            __declspec(property(get=get_NodeName)) std::wstring const & NodeName;
            __declspec(property(get=get_ImpactLevel)) NodeImpactLevel::Enum const ImpactLevel;

            std::wstring const & get_NodeName() const { return nodeName_; }
            NodeImpactLevel::Enum const get_ImpactLevel() const { return impactLevel_; }

            bool operator == (NodeImpact const & other) const;
            bool operator != (NodeImpact const & other) const;

            Common::ErrorCode FromPublicApi(FABRIC_REPAIR_NODE_IMPACT const &);
            void ToPublicApi(__in Common::ScopedHeap &, __out FABRIC_REPAIR_NODE_IMPACT &) const;

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

            bool NodeImpact::IsValid() const;

            FABRIC_FIELDS_02(nodeName_, impactLevel_);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::NodeName, nodeName_)
                SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::ImpactLevel, impactLevel_)
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            std::wstring nodeName_;
            NodeImpactLevel::Enum impactLevel_;
        };

    }
}
DEFINE_USER_ARRAY_UTILITY(Management::RepairManager::NodeImpact)

