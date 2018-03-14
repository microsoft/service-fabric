// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    // ServiceTableEntry is the class contained in FM broadcasts.
    //
    // CachedServiceTableEntry is the class stored in the gateway ServiceTableEntry (indexed) cache.
    //
    // To support notifications, we need to maintain some additional state about the results
    // of a matching operation - either filters against cache or broadcast updates against
    // filters. This class keeps the matching cache entry along with any such additional
    // state.
    //
    class MatchedServiceTableEntry
        : public Common::TextTraceComponent<Common::TraceTaskCodes::NamingGateway>        
    {
    private:

        MatchedServiceTableEntry(
            CachedServiceTableEntrySPtr const &,
            bool checkPrimaryChangeOnly,
            bool matchedPrimaryOnly);

    public:
        
        static std::shared_ptr<MatchedServiceTableEntry> CreateOnUpdateMatch(
            CachedServiceTableEntrySPtr const &,
            bool checkPrimaryChangeOnly);

        static std::shared_ptr<MatchedServiceTableEntry> CreateOnConnectionMatch(
            CachedServiceTableEntrySPtr const &,
            bool matchedPrimaryOnly);

        __declspec(property(get=get_Version)) int64 Version;
        int64 get_Version() const { return cachedEntry_->Version; }

        __declspec(property(get=get_LastPrimaryVersion)) int64 LastPrimaryVersion;
        int64 get_LastPrimaryVersion() const { return cachedEntry_->LastPrimaryVersion; }

        __declspec(property(get=get_CheckPrimaryChangeOnly)) bool CheckPrimaryChangeOnly;
        bool get_CheckPrimaryChangeOnly() const { return checkPrimaryChangeOnly_; }

        void UpdateCheckPrimaryChangeOnly(bool);

        void UpdateMatchedPrimaryOnly(bool);

        bool TryGetServiceTableEntry(
            Common::ActivityId const &,
            Common::VersionRangeCollectionSPtr const & clientVersions,
            __out ServiceTableEntryNotificationSPtr &);

    private:
        // This must be a deep copy of the underlying cache entry. Otherwise,
        // the cache entry may be changed before the notification is actually
        // sent.
        //
        CachedServiceTableEntrySPtr cachedEntry_;

        // Tracks whether a primary endpoint version check should occur while
        // building the actual notification to be sent. This is true if
        // the match occurred against only filters with primary-only specified.
        // This is always false for matches performed during reconnection.
        //
        bool checkPrimaryChangeOnly_;


        // Tracks whether this was a primary only match. This can be true even
        // if checkPrimaryChangeOnly_ is false, such as in the case where
        // this match was produced from client reconnection.
        //
        bool matchedPrimaryOnly_;
    };

    typedef std::shared_ptr<MatchedServiceTableEntry> MatchedServiceTableEntrySPtr;
    typedef std::pair<int64, MatchedServiceTableEntrySPtr> MatchedServiceTableEntryPair;
    typedef std::map<int64, MatchedServiceTableEntrySPtr> MatchedServiceTableEntryMap;
}
