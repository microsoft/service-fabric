// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class ServiceNotification : public Serialization::FabricSerializable
    {
        DENY_COPY(ServiceNotification)

        DECLARE_SERIALIZATION_OVERHEAD_ESTIMATE()

    public:
        ServiceNotification()
            : notificationPageId_()
            , generation_()
            , versions_()
            , partitions_()
        {
        }

        ServiceNotification(ServiceNotification && other)
            : notificationPageId_(std::move(other.notificationPageId_))
            , generation_(std::move(other.generation_))
            , versions_(std::move(other.versions_))
            , partitions_(std::move(other.partitions_))
        {
        }

        ServiceNotification(
            ServiceNotificationPageId const & pageId,
            Reliability::GenerationNumber const & generation,
            std::shared_ptr<Common::VersionRangeCollection> && versions,
            std::vector<Reliability::ServiceTableEntryNotificationSPtr> && partitions)
            : notificationPageId_(pageId)
            , generation_(generation)
            , versions_(std::move(versions))
            , partitions_(std::move(partitions))
        {
        }

        ServiceNotificationPageId const & GetNotificationPageId() const { return notificationPageId_; }
        Reliability::GenerationNumber const & GetGenerationNumber() const { return generation_; }

        Common::VersionRangeCollectionSPtr const & GetVersions() const { return versions_; }
        Common::VersionRangeCollectionSPtr && TakeVersions() { return std::move(versions_); }

        std::vector<Reliability::ServiceTableEntryNotificationSPtr> const & GetPartitions() const { return partitions_; }
        std::vector<Reliability::ServiceTableEntryNotificationSPtr> && TakePartitions() { return std::move(partitions_); }

        FABRIC_FIELDS_04(notificationPageId_, generation_, versions_, partitions_);

    private:
        ServiceNotificationPageId notificationPageId_;
        Reliability::GenerationNumber generation_;
        Common::VersionRangeCollectionSPtr versions_;
        std::vector<Reliability::ServiceTableEntryNotificationSPtr> partitions_;
    };

    typedef std::unique_ptr<ServiceNotification> ServiceNotificationUPtr;
    typedef std::shared_ptr<ServiceNotification> ServiceNotificationSPtr;
}
