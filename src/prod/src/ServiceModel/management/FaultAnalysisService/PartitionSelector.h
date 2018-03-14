// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Management
{
    namespace FaultAnalysisService
    {
        class PartitionSelector : public Serialization::FabricSerializable
        {
        public:
            PartitionSelector();

            PartitionSelector(Common::NamingUri && serviceName, PartitionSelectorType::Enum partitionSelectorType, std::wstring const & partitionKey);
            PartitionSelector(PartitionSelector const & other);
            PartitionSelector & operator = (PartitionSelector const & other);
            bool operator == (PartitionSelector const & rhs) const;
            bool operator != (PartitionSelector const & rhs) const;

            PartitionSelector(PartitionSelector && other);
            PartitionSelector & operator = (PartitionSelector && other);

            __declspec(property(get = get_ServiceName)) Common::NamingUri const& ServiceName;
            Common::NamingUri const& get_ServiceName() const { return serviceName_; }

            __declspec(property(get = get_PartitionSelectorType)) PartitionSelectorType::Enum const& PartitionSelectorType;
            PartitionSelectorType::Enum const& get_PartitionSelectorType() const { return partitionSelectorType_; }

            __declspec(property(get = get_PartitionKey)) std::wstring const& PartitionKey;
            std::wstring const& get_PartitionKey() const { return partitionKey_; }

            Common::ErrorCode FromPublicApi(__in FABRIC_PARTITION_SELECTOR const & partitionSelector);
            void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_PARTITION_SELECTOR &) const;

            void WriteTo(__in Common::TextWriter&, Common::FormatOptions const &) const;

            FABRIC_FIELDS_03(serviceName_, partitionSelectorType_, partitionKey_);

        private:
            Common::NamingUri serviceName_;
            PartitionSelectorType::Enum partitionSelectorType_;
            std::wstring partitionKey_;
        };
    }
}
