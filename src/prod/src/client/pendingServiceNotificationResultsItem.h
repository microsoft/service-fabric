// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    // Used to exchange the list of incoming notifications with the list of notifications
    // after being processed through the cache (to build RSP from STE). This notification
    // container may be queued if the client is still synchronizing deletes with
    // the gateway.
    //
    class PendingServiceNotificationResultsItem
    {
    public:
       
        PendingServiceNotificationResultsItem(
            Naming::ServiceNotificationSPtr && notification,
            std::vector<ServiceNotificationResultSPtr> && notificationResults)
            : notificationPageId_(notification->GetNotificationPageId())
            , generation_(notification->GetGenerationNumber())
            , versions_(notification->TakeVersions())
            , notificationResults_(std::move(notificationResults))
        {
        }

        Naming::ServiceNotificationPageId const & GetNotificationPageId() const { return notificationPageId_; }
        Reliability::GenerationNumber const & GetGenerationNumber() const { return generation_; }

        Common::VersionRangeCollectionSPtr && TakeVersions() { return std::move(versions_); }
        std::vector<ServiceNotificationResultSPtr> && TakeNotificationResults() { return std::move(notificationResults_); }

    private:
        Naming::ServiceNotificationPageId notificationPageId_;
        Reliability::GenerationNumber generation_;
        Common::VersionRangeCollectionSPtr versions_;
        std::vector<ServiceNotificationResultSPtr> notificationResults_;
    };

    typedef std::shared_ptr<PendingServiceNotificationResultsItem> PendingServiceNotificationResultsItemSPtr;
}
