// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace RepairManager
    {
        class NodeRepairImpactDescription 
            : public RepairImpactDescriptionBase
        {
        public:
            NodeRepairImpactDescription();
            NodeRepairImpactDescription(NodeRepairImpactDescription const &);
            NodeRepairImpactDescription(NodeRepairImpactDescription &&);

            __declspec(property(get=get_NodeImpactList)) std::vector<NodeImpact> const & NodeImpactList;
            std::vector<NodeImpact> const & get_NodeImpactList() const { return nodeImpactList_; }

            bool operator == (RepairImpactDescriptionBase const & other) const override;

            Common::ErrorCode FromPublicApi(FABRIC_REPAIR_IMPACT_DESCRIPTION const &) override;
            void ToPublicApi(__in Common::ScopedHeap &, __out FABRIC_REPAIR_IMPACT_DESCRIPTION &) const override;

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const override;

            bool IsValid() const override;
            bool IsZeroImpact() const override;

            RepairImpactDescriptionBaseSPtr Clone() const override;

            FABRIC_FIELDS_01(nodeImpactList_);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY_CHAIN()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::NodeImpactList, nodeImpactList_)
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            std::vector<NodeImpact> nodeImpactList_;
        };

        DEFINE_REPAIR_IMPACT_ACTIVATOR(NodeRepairImpactDescription, FABRIC_REPAIR_IMPACT_KIND_NODE)
    }
}
