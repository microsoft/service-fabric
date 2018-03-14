// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    struct GatewayNotificationReplyBuilder
    {
        DENY_COPY(GatewayNotificationReplyBuilder)

    public:
        GatewayNotificationReplyBuilder(
            GatewayEventSource const & trace,
            std::wstring const & traceId);

        ~GatewayNotificationReplyBuilder();

        __declspec(property(get=get_MessageSizeLimitEncountered)) bool MessageSizeLimitEncountered;
        bool get_MessageSizeLimitEncountered() const { return messageSizeLimitEncountered_; }

        __declspec(property(get=get_HasResults)) bool HasResults;
        bool get_HasResults() const { return (partitions_.size() + failures_.size()) > 0; }

        bool TryAdd(
            ServiceLocationNotificationRequestData const & request,
            ResolvedServicePartition && entry);

        bool TryAdd(
            ServiceLocationNotificationRequestData const & request,
            AddressDetectionFailure && failure);

        Transport::MessageUPtr CreateMessage(size_t firstNonProcessedRequestIndex);

    private:

        bool CanFitRequestReply(size_t estimateSize);

        GatewayEventSource const & trace_;
        std::wstring const & traceId_;

        std::vector<ResolvedServicePartition> partitions_;
        std::vector<AddressDetectionFailure> failures_;
        std::set<Reliability::ConsistencyUnitId> currentCuids_;

        // Keeps track of the size of AddressDetectionFailures and ResolvedServicePartitions
        // added to Reply. If next to add doesn't fit due to MaxMessageSize being hit,
        // the processing stops.
        bool messageSizeLimitEncountered_;
        size_t currentReplyEstimateSize_;

        // Keeps track of how many objects are added in reply, to be able to cap it 
        // at the configured maximum value
        int replyEntries_;
        size_t messageContentThreshold_;
    };
}
