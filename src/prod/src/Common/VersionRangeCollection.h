// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include <limits>

namespace Common
{
    class VersionRangeCollection 
        : public Serialization::FabricSerializable
        , public Common::ISizeEstimator
    {
    private:
        VersionRangeCollection(std::vector<VersionRange> &&, int64 initialVersion);

    public:
        VersionRangeCollection(int64 initialVersion = 1);
        VersionRangeCollection(int64 startVersion, int64 endVersion);
        VersionRangeCollection(VersionRangeCollection const& other);

        __declspec (property(get=get_VersionRanges)) std::vector<VersionRange> const& VersionRanges;
        std::vector<VersionRange> const& get_VersionRanges() const { return versionRanges_; }

        __declspec (property(get=get_IsEmpty)) bool IsEmpty;
        bool get_IsEmpty() const { return versionRanges_.empty(); }

        __declspec (property(get=get_EndVersion)) int64 EndVersion;
        int64 get_EndVersion() const;

        __declspec (property(get=get_StartVersion)) int64 StartVersion;
        int64 get_StartVersion() const;

        // Add a VersionRange to the VersionRangeCollection. Merge VersionRanges whenever possible.
        bool AddRange(int64 startVersion, int64 EndVersion);
        bool Add(VersionRange versionRange);

        // Merge with another VersionRangeCollection.
        bool Test_Add(VersionRangeCollection const& versionRangeCollection);

        // This function has been optimized to merge with another (sorted) collection in O(N).
        // Do not use Test_Add() except for testing since it is O(N^2).
        //
        void Merge(VersionRangeCollection const &);

        void Merge(VersionRange const &);

        void Split(int64 endVersion, __out VersionRangeCollection & remainder);

        // Remove a specific version
        void Remove(int64 version);

        // Remove a VersionRange from the collection of VersionRanges.
        void Remove(VersionRange const& versionRange);

        // Remove the VersionRanges from the VersionRangeCollection.
        void Test_Remove(VersionRangeCollection const& versionRangeCollection);

        // Functionally equivalent to Test_Remove(VersionRangeCollection const&) but
        // optimized to run in O(N) instead of O(N^2). Keep the original
        // implementation for testing purposes.
        //
        void Remove(VersionRangeCollection const& versionRangeCollection);

        void RemoveFirstRange();

        void Clear();

        void Swap(VersionRangeCollection &&);

        bool Contains(int64);
        bool Contains(int64, __out int64 & endVersion);

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;

        static std::string AddField(TraceEvent & traceEvent, std::string const & name);
        void FillEventData(TraceEventContext & context) const;

        FABRIC_FIELDS_01(versionRanges_);

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(versionRanges_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        void MergeBack(__inout std::vector<VersionRange> &, VersionRange const &);
        bool Contains(int64, __out std::vector<VersionRange>::iterator &);
        void Remove(
            VersionRange const & rangeToCheck,
            VersionRange const & rangeToRemove,
            __out std::unique_ptr<VersionRange> & rangeToKeep,
            __out std::unique_ptr<VersionRange> & rangeRemainder,
            __out bool & advanceRangeToCheck,
            __out bool & advanceRangeToRemove);

        std::vector<VersionRange> versionRanges_;
        int64 initialVersion_;
    };

    typedef std::shared_ptr<VersionRangeCollection> VersionRangeCollectionSPtr;
}
