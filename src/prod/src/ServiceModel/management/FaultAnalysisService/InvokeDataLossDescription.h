// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class InvokeDataLossDescription 
            : public Serialization::FabricSerializable
        {
        public:

            InvokeDataLossDescription();

            InvokeDataLossDescription(
                Common::Guid, 
                std::shared_ptr<PartitionSelector> const&, 
                DataLossMode::Enum);

            InvokeDataLossDescription(InvokeDataLossDescription const &);
            InvokeDataLossDescription(InvokeDataLossDescription &&);

            InvokeDataLossDescription & operator = (InvokeDataLossDescription && other);

            __declspec(property(get=get_OperationId)) Common::Guid const & Id;
            __declspec(property(get = get_PartitionSelector)) std::shared_ptr<PartitionSelector> const & Selector;
            __declspec(property(get=get_DataLossMode)) DataLossMode::Enum const & Mode;

            Common::Guid const & get_OperationId() const { return operationId_; }
            std::shared_ptr<PartitionSelector> const & get_PartitionSelector() const { return partitionSelector_; }
            DataLossMode::Enum const & get_DataLossMode() const { return dataLossMode_; }

            bool operator == (InvokeDataLossDescription const & other) const;
            bool operator != (InvokeDataLossDescription const & other) const;

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            Common::ErrorCode FromPublicApi(FABRIC_START_PARTITION_DATA_LOSS_DESCRIPTION const &);
            void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_START_PARTITION_DATA_LOSS_DESCRIPTION &) const;

            FABRIC_FIELDS_03(
                operationId_, 
                partitionSelector_,
                dataLossMode_);

        private:

            Common::Guid operationId_;
            std::shared_ptr<PartitionSelector> partitionSelector_;
            DataLossMode::Enum dataLossMode_;
        };
    }
}
