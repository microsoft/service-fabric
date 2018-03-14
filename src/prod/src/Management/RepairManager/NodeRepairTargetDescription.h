// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace RepairManager
    {
        class NodeRepairTargetDescription 
            : public RepairTargetDescriptionBase
        {
        public:
            NodeRepairTargetDescription();
            NodeRepairTargetDescription(NodeRepairTargetDescription const &);
            NodeRepairTargetDescription(NodeRepairTargetDescription &&);

            bool operator == (RepairTargetDescriptionBase const & other) const override;

            Common::ErrorCode FromPublicApi(FABRIC_REPAIR_TARGET_DESCRIPTION const &) override;
            void ToPublicApi(__in Common::ScopedHeap &, __out FABRIC_REPAIR_TARGET_DESCRIPTION &) const override;

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const override;

            bool IsValid() const override;

            RepairTargetDescriptionBaseSPtr Clone() const override;

            FABRIC_FIELDS_01(nodeList_);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY_CHAIN()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::NodeNames, nodeList_)
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            std::vector<std::wstring> nodeList_;
        };

        DEFINE_REPAIR_TARGET_ACTIVATOR(NodeRepairTargetDescription, FABRIC_REPAIR_TARGET_KIND_NODE)
    }
}
