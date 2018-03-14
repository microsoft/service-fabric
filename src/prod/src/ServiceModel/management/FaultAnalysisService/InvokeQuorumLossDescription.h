// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class InvokeQuorumLossDescription 
            : public Serialization::FabricSerializable
        {
        public:

            InvokeQuorumLossDescription();
            InvokeQuorumLossDescription(
                Common::Guid, 
                std::shared_ptr<PartitionSelector> const&, 
                QuorumLossMode::Enum,
                Common::TimeSpan &);
            InvokeQuorumLossDescription(InvokeQuorumLossDescription const &);
            InvokeQuorumLossDescription(InvokeQuorumLossDescription &&);
            InvokeQuorumLossDescription & operator = (InvokeQuorumLossDescription && other);

            __declspec(property(get=get_OperationId)) Common::Guid const & Id;
            __declspec(property(get = get_PartitionSelector)) std::shared_ptr<PartitionSelector> const & Selector;
            __declspec(property(get=get_QuorumLossMode)) QuorumLossMode::Enum const & Mode;
            __declspec(property(get = get_QuorumLossDuration)) Common::TimeSpan QuorumLossDuration;

            Common::Guid const & get_OperationId() const { return operationId_; }
            std::shared_ptr<PartitionSelector> const & get_PartitionSelector() const { return partitionSelector_; }
            QuorumLossMode::Enum const & get_QuorumLossMode() const { return quorumLossMode_; }
            Common::TimeSpan get_QuorumLossDuration() const { return quorumLossDuration_; }

            bool operator == (InvokeQuorumLossDescription const & other) const;
            bool operator != (InvokeQuorumLossDescription const & other) const;

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            Common::ErrorCode FromPublicApi(FABRIC_START_PARTITION_QUORUM_LOSS_DESCRIPTION const &);
            void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_START_PARTITION_QUORUM_LOSS_DESCRIPTION &) const;

            FABRIC_FIELDS_04(
                operationId_, 
                partitionSelector_,
                quorumLossMode_,
                quorumLossDuration_);

        private:

            Common::Guid operationId_;
            std::shared_ptr<PartitionSelector> partitionSelector_;
            QuorumLossMode::Enum quorumLossMode_;
            Common::TimeSpan quorumLossDuration_;
        };
    }
}
