// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace std;
    using namespace Common;
    using namespace Transport;
    using namespace Reliability;
            
    GatewayNotificationReplyBuilder::GatewayNotificationReplyBuilder(
        GatewayEventSource const & trace,
        std::wstring const & traceId)
        : trace_(trace)
        , traceId_(traceId)
        , partitions_()
        , failures_()
        , currentCuids_() 
        , messageSizeLimitEncountered_(false)
        , currentReplyEstimateSize_(ServiceLocationNotificationReplyBody::SerializationOverheadEstimate) // make room for empty reply content
        , replyEntries_(0)      
        , messageContentThreshold_(NamingUtility::GetMessageContentThreshold())
    {
    }

    GatewayNotificationReplyBuilder::~GatewayNotificationReplyBuilder()
    {
    }

    bool GatewayNotificationReplyBuilder::TryAdd(
        ServiceLocationNotificationRequestData const & request,
        ResolvedServicePartition && entry)
    {
        auto estimateSize = entry.EstimateSize();
        auto error = NamingUtility::CheckEstimatedSize(estimateSize, messageContentThreshold_);
        if (!error.IsSuccess())
        {
            AddressDetectionFailure failure(
                request.Name, 
                request.Partition, 
                error.ReadValue(), 
                entry.StoreVersion);
            return TryAdd(request, move(failure));
        }

        // If the entry is already in the vector or rsps, do not add it again
        if (currentCuids_.find(entry.Locations.ConsistencyUnitId) == currentCuids_.end())
        {
            // Check whether the new RSP fits the message.
            if (CanFitRequestReply(estimateSize))
            {
                trace_.NotificationAddRsp(
                    traceId_, 
                    entry, 
                    static_cast<uint64>(currentReplyEstimateSize_));
                partitions_.push_back(move(entry));
                currentCuids_.insert(entry.Locations.ConsistencyUnitId);
                return true;
            }
            else
            {
                trace_.NotificationMsgSizeLimitRsp(
                    traceId_,  
                    entry, 
                    static_cast<uint64>(currentReplyEstimateSize_),
                    replyEntries_);
                return false;
            }
        }

        return false;
    }

    bool GatewayNotificationReplyBuilder::TryAdd(
        ServiceLocationNotificationRequestData const & request,
        AddressDetectionFailure && failure)
    {
        // Only add the error if it's newer than the previous one in the request
        if (!AddressDetectionFailure::IsObsolete(
            request.PreviousError, 
            request.PreviousErrorVersion, 
            failure.Error, 
            failure.StoreVersion))
        {
            trace_.NotificationFailureNotNewer(
                traceId_,
                failure,
                request.PreviousErrorVersion);
             return false;
        }

        auto estimateSize = failure.EstimateSize();
        if (!NamingUtility::CheckEstimatedSize(estimateSize, messageContentThreshold_).IsSuccess())
        {
            // Ignore this entry, do not send it back to the fabric client
            return false;
        }

        if (CanFitRequestReply(estimateSize))
        {
            trace_.NotificationAddFailure(
                traceId_,
                request.Name.ToString(),
                failure,
                ErrorCode(request.PreviousError).ErrorCodeValueToString(),
                request.PreviousErrorVersion);
            failures_.push_back(move(failure));
            return true;
        }
        else
        {
            trace_.NotificationMsgSizeLimitFailure(
                traceId_, 
                failure, 
                static_cast<uint64>(currentReplyEstimateSize_),
                replyEntries_);
            return false;
        }
    }

    Transport::MessageUPtr GatewayNotificationReplyBuilder::CreateMessage(size_t firstNonProcessedRequestIndex) 
    {
        trace_.NotificationSendReply(
            traceId_,
            static_cast<uint64>(partitions_.size()),
            static_cast<uint64>(failures_.size()),
            static_cast<uint64>(currentReplyEstimateSize_),
            static_cast<uint64>(firstNonProcessedRequestIndex));
        return NamingMessage::GetServiceLocationChangeListenerReply(ServiceLocationNotificationReplyBody(
            std::move(partitions_),
            std::move(failures_),
            firstNonProcessedRequestIndex));
    }

    bool GatewayNotificationReplyBuilder::CanFitRequestReply(size_t estimateSize)
    {
        int maxConfigEntries = NamingConfig::GetConfig().MaxNotificationReplyEntryCount;
        if ((maxConfigEntries > 0 && replyEntries_ >= maxConfigEntries) ||
            estimateSize + currentReplyEstimateSize_ >= messageContentThreshold_)
        {
            messageSizeLimitEncountered_ = true;
            return false;
        }

        ++replyEntries_;
        currentReplyEstimateSize_ += estimateSize;
        return true;
    }
}
