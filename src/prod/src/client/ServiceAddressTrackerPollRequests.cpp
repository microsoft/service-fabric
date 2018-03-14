// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Client
{
    using namespace std;
    using namespace Common;
    using namespace Transport;

    ServiceAddressTrackerPollRequests::ServiceAddressTrackerPollRequests(
        FabricActivityHeader const & activityHeader,
        std::wstring const & traceId,
        uint64 pollIndex,
        size_t messageContentThreshold) 
        : activityHeader_(activityHeader)
        , traceId_(traceId)
        , pollIndex_(pollIndex)
        , requests_()
        , estimatedRequestsSize_(0) 
        , messageContentThreshold_(messageContentThreshold)
    {
    }

    ServiceAddressTrackerPollRequests::ServiceAddressTrackerPollRequests(
        ServiceAddressTrackerPollRequests && other) 
        : activityHeader_(std::move(other.activityHeader_))
        , traceId_(std::move(other.traceId_))
        , pollIndex_(std::move(other.pollIndex_))
        , requests_(std::move(other.requests_))
        , estimatedRequestsSize_(std::move(other.estimatedRequestsSize_)) 
        , messageContentThreshold_(std::move(other.messageContentThreshold_))
    {
    }

    ServiceAddressTrackerPollRequests & ServiceAddressTrackerPollRequests::operator = (
        ServiceAddressTrackerPollRequests && other)
    {
        if (this != &other)
        {
            activityHeader_ = std::move(other.activityHeader_);
            traceId_ = std::move(other.traceId_);
            pollIndex_ = std::move(other.pollIndex_);
            requests_ = std::move(other.requests_);
            estimatedRequestsSize_ = std::move(other.estimatedRequestsSize_);
            messageContentThreshold_ = std::move(other.messageContentThreshold_);
        }

        return *this;
    }

    ServiceAddressTrackerPollRequests::~ServiceAddressTrackerPollRequests() 
    {
    }

    std::vector<ServiceLocationNotificationRequestData> ServiceAddressTrackerPollRequests::MoveRequests() 
    { 
        if (requests_.empty())
        {
            FabricClientImpl::WriteWarning(
                Constants::FabricClientSource,
                traceId_,
                "{0}: ServiceAddressTrackerPollRequests has 0 requests", 
                activityHeader_.ActivityId);
            Assert::TestAssert();
        }

        FabricClientImpl::Trace.TrackerStartPoll(
            traceId_,
            activityHeader_.ActivityId, 
            pollIndex_,
            static_cast<uint64>(requests_.size()),
            static_cast<uint64>(estimatedRequestsSize_),
            static_cast<uint64>(messageContentThreshold_));
        return std::move(requests_); 
    }

    bool ServiceAddressTrackerPollRequests::CanFit(
        ServiceLocationNotificationRequestData const & pendingRequest, 
        size_t pendingRequestSize) const
    {
        int maxConfigEntries = ClientConfig::GetConfig().MaxServiceChangePollBatchedRequests;
        if ((maxConfigEntries > 0 && static_cast<int>(requests_.size()) >= maxConfigEntries) || 
            pendingRequestSize + estimatedRequestsSize_ >= messageContentThreshold_)
        {
            FabricClientImpl::Trace.TrackerMsgSizeLimit(
                traceId_,
                activityHeader_.ActivityId, 
                pollIndex_,
                pendingRequest,
                static_cast<uint64>(estimatedRequestsSize_),
                static_cast<uint64>(requests_.size()));
            
            return false;
        }

        return true;
    }

    void ServiceAddressTrackerPollRequests::AddRequest(
        ServiceLocationNotificationRequestData && pendingRequest, 
        size_t pendingRequestSize)
    {
        estimatedRequestsSize_ += pendingRequestSize;
        requests_.push_back(std::move(pendingRequest));
    }
}
