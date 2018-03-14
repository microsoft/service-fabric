// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class ServiceLocationNotificationReplyBody : public Serialization::FabricSerializable
    {
        DECLARE_SERIALIZATION_OVERHEAD_ESTIMATE()

    public:
        ServiceLocationNotificationReplyBody();

        ServiceLocationNotificationReplyBody(
            std::vector<ResolvedServicePartition> && partitions,
            std::vector<AddressDetectionFailure> && failures,
            uint64 firstNonProcessedRequestIndex);

        __declspec(property(get=get_Partitions)) std::vector<ResolvedServicePartition> const & Partitions;
        std::vector<ResolvedServicePartition> const & get_Partitions() const { return partitions_; }

        __declspec(property(get=get_Failures)) std::vector<AddressDetectionFailure> const & Failures;
        std::vector<AddressDetectionFailure> const & get_Failures() const { return failures_; }

        __declspec(property(get=get_FirstNonProcessedRequestIndex)) uint64 FirstNonProcessedRequestIndex;
        uint64 get_FirstNonProcessedRequestIndex() const { return firstNonProcessedRequestIndex_; }

        std::vector<ResolvedServicePartition> && MovePartitions() { return std::move(partitions_); }
        std::vector<AddressDetectionFailure> && MoveFailures() { return std::move(failures_); }

        FABRIC_FIELDS_03(partitions_, failures_, firstNonProcessedRequestIndex_);
        
    private:
        std::vector<ResolvedServicePartition> partitions_;
        std::vector<AddressDetectionFailure> failures_;
        // If there are too many requests, mark the first one that didn't fit in the reply.
        // The naming client will ask for re-processing on this request
        uint64 firstNonProcessedRequestIndex_;
    };
}
