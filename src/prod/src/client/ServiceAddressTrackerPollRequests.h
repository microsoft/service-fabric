// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class ServiceAddressTrackerPollRequests
    {
        DENY_COPY(ServiceAddressTrackerPollRequests);

    public:
        ServiceAddressTrackerPollRequests(
            Transport::FabricActivityHeader const & activityHeader,
            std::wstring const & traceId,
            uint64 pollIndex,
            size_t messageContentThreshold);
        ~ServiceAddressTrackerPollRequests();

        ServiceAddressTrackerPollRequests(ServiceAddressTrackerPollRequests && other);
        ServiceAddressTrackerPollRequests & operator = (ServiceAddressTrackerPollRequests && other);
        
        __declspec(property(get=get_Count)) size_t Count;
        size_t get_Count() const { return requests_.size(); }

        __declspec(property(get=get_EstimatedRequestsSize)) size_t EstimatedRequestsSize;
        size_t get_EstimatedRequestsSize() const { return estimatedRequestsSize_; }

        bool CanFit(
            Naming::ServiceLocationNotificationRequestData const & pendingRequest, 
            size_t pendingRequestSize) const;

        void AddRequest(
            Naming::ServiceLocationNotificationRequestData && pendingRequest, 
            size_t pendingRequestSize);

        std::vector<Naming::ServiceLocationNotificationRequestData> MoveRequests();

    private:
        Transport::FabricActivityHeader activityHeader_;
        
        std::wstring traceId_;
        
        uint64 pollIndex_;

        std::vector<Naming::ServiceLocationNotificationRequestData> requests_;
        size_t estimatedRequestsSize_;
        size_t messageContentThreshold_;
    };
}
