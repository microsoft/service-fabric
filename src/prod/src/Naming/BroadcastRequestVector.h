// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    struct BroadcastRequestVector
    {
        DENY_COPY(BroadcastRequestVector)

    public:
        BroadcastRequestVector();

        BroadcastRequestVector(BroadcastRequestVector && other);

        __declspec(property(get=get_Entries)) std::vector<BroadcastRequestEntry> const & Entries;
        std::vector<BroadcastRequestEntry> const & get_Entries() const { return entries_; }

        void Clear() { entries_.clear(); }

        bool IsEmpty() const { return entries_.empty(); }

        bool TryAdd(
            Common::NamingUri const & name,
            Reliability::ConsistencyUnitId cuid);

        void WriteTo(__in Common::TextWriter & writer, Common::FormatOptions const &) const;

        std::wstring ToString() const;

        typedef std::vector<BroadcastRequestEntry>::iterator iterator;
        typedef std::vector<BroadcastRequestEntry>::const_iterator const_iterator;

        iterator begin() { return entries_.begin(); }
        iterator end() { return entries_.end(); }
        const_iterator begin() const { return entries_.begin(); }
        const_iterator end() const { return entries_.end(); }

    private:
        std::vector<BroadcastRequestEntry> entries_;
    };
}
