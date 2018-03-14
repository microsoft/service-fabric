// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    //
    // Base class for Service Lookup Table. It contains the service lookup information for each FailoverUnit.
    //
    class ServiceLookupTable : public Common::RootedObject
    {
        DENY_COPY(ServiceLookupTable);

    public:
        bool TryGetEntry(ConsistencyUnitId const & cuid, __out ServiceTableEntry & entry) const;

    protected:
        ServiceLookupTable(Common::ComponentRoot const & root);

        __declspec(property(get=get_LockObject)) Common::RwLock& LockObject;
        Common::RwLock& get_LockObject() const { return lockObject_; }

        bool TryGetEntryCallerHoldsReadLock(ConsistencyUnitId const& cuid, _Out_ ServiceTableEntry & entry) const;

        __int64 GetVersionForEntryCallerHoldingLock(ConsistencyUnitId const & cuid) const;

        // If the newEntry does not already exists, inserts it to the table.
        // Otherwise, updates the existing entry.
        bool TryUpdateEntryCallerHoldingLock(ServiceTableEntrySPtr && newEntry);

        // Get updated entries whose lookup version does not appear in the specified VersionRangeCollection.
        int64 GetUpdatedEntriesCallerHoldingLock(
            size_t pageSizeLimit,
            std::vector<ServiceTableEntry> & entries,
            Common::VersionRangeCollection const& rangeToExclude) const;

        bool TryRemoveEntryCallerHoldingLock(ConsistencyUnitId const& consistencyUnitId, int64 versionToRemove = std::numeric_limits<int64>::max());

        void ClearCallerHoldingLock();

    private:

        // The mapping from ConsistencyUnitId to ServiceTableEntries
        std::unordered_map<ConsistencyUnitId, ServiceTableEntrySPtr, ConsistencyUnitId::Hasher> idEntries_;

        // The list of FailoverUnit versions, sorted by lookup version.
        std::map<int64, ConsistencyUnitId> vEntries_;

        mutable Common::RwLock lockObject_;
    };
}
