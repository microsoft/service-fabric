// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class RestartPartitionDescription
            : public Serialization::FabricSerializable
        {
        public:

            RestartPartitionDescription();
            RestartPartitionDescription(
                Common::Guid, 
                std::shared_ptr<PartitionSelector> const&, 
                RestartPartitionMode::Enum);
            RestartPartitionDescription(RestartPartitionDescription const &);
            RestartPartitionDescription(RestartPartitionDescription &&);
            RestartPartitionDescription & operator = (RestartPartitionDescription && other);

            __declspec(property(get = get_OperationId)) Common::Guid const & Id;
            __declspec(property(get = get_PartitionSelector)) std::shared_ptr<PartitionSelector> const & Selector;
            __declspec(property(get = get_RestartPartitionMode)) RestartPartitionMode::Enum const & Mode;

            Common::Guid const & get_OperationId() const { return operationId_; }
            std::shared_ptr<PartitionSelector> const & get_PartitionSelector() const { return partitionSelector_; }
            RestartPartitionMode::Enum const & get_RestartPartitionMode() const { return restartPartitionMode_; }

            bool operator == (RestartPartitionDescription const & other) const;
            bool operator != (RestartPartitionDescription const & other) const;

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            Common::ErrorCode FromPublicApi(FABRIC_START_PARTITION_RESTART_DESCRIPTION const &);
            void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_START_PARTITION_RESTART_DESCRIPTION &) const;

            FABRIC_FIELDS_03(
                operationId_,
                partitionSelector_,
                restartPartitionMode_);

        private:

            Common::Guid operationId_;
            std::shared_ptr<PartitionSelector> partitionSelector_;
            RestartPartitionMode::Enum restartPartitionMode_;
        };
    }
}
