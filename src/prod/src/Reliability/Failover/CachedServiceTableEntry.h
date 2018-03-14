// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    // ServiceTableEntry stores the service name as a string.
    // The entries in NotificationCacheIndex are indexed by parsed NamingUri.
    // This interface lets us reverse lookup from a partition to its parent
    // service without re-parsing the service name.
    //
    class ICachedServiceTableEntryOwner
    {
    public:
        virtual Common::NamingUri const & GetUri() const = 0;
    };

    typedef std::shared_ptr<ICachedServiceTableEntryOwner> OwnerSPtr;

    // ServiceTableEntry is the class contained in FM broadcasts. There is
    // one-to-one mapping between ServiceTableEntries and Partitions (CUIDs).
    //
    // To support notifications, we need to maintain some additional state for
    // each ServiceTableEntry. This class is used to keep each broadcasted entry
    // along with this additional state in the gateway cache.
    //
    class CachedServiceTableEntry
        : public Common::TextTraceComponent<Common::TraceTaskCodes::NamingGateway>        
    {
    private:
        CachedServiceTableEntry(
            CachedServiceTableEntry const &);

    public:
        CachedServiceTableEntry(
            Common::ActivityId const &, 
            ServiceTableEntrySPtr const &,
            OwnerSPtr const &);

        __declspec(property(get=get_Version)) int64 Version;
        int64 get_Version() const { return partition_->Version; }

        __declspec(property(get=get_LastPrimaryVersion)) int64 LastPrimaryVersion;
        int64 get_LastPrimaryVersion() const { return lastPrimaryChangeVersion_; }

        __declspec(property(get=get_Uri)) Common::NamingUri const & Uri;
        Common::NamingUri const & get_Uri() const { return owner_->GetUri(); }

        __declspec(property(get=get_ServiceName)) std::wstring const & ServiceName;
        std::wstring const & get_ServiceName() const { return partition_->ServiceName; }

        __declspec(property(get=get_ServiceTableEntry)) ServiceTableEntrySPtr const & Partition;
        ServiceTableEntrySPtr const & get_ServiceTableEntry() const { return partition_; }

        std::shared_ptr<CachedServiceTableEntry> Clone() const;

        void UpdateServiceTableEntry(
            Common::ActivityId const &,
            ServiceTableEntrySPtr const &);

        bool TryGetServiceTableEntry(
            Common::ActivityId const &,
            Common::VersionRangeCollectionSPtr const & clientVersions,
            bool primaryChangeOnly,
            __out ServiceTableEntrySPtr &);

    private:
        void UpdateLastPrimaryLocation(Common::ActivityId const &);

        ServiceTableEntrySPtr partition_;
        int64 lastPrimaryChangeVersion_;
        std::wstring lastPrimaryLocation_;

        OwnerSPtr owner_;
    };

    typedef std::shared_ptr<CachedServiceTableEntry> CachedServiceTableEntrySPtr;
}
