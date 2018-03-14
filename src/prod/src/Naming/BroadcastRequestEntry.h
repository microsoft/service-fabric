// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    // Types kept in the list of waiters, associated with a service name
    struct BroadcastRequestEntry
    {
    public:
        BroadcastRequestEntry( 
            Common::NamingUri const & name,
            Reliability::ConsistencyUnitId cuid);

        __declspec(property(get=get_Name)) Common::NamingUri const & Name;
        Common::NamingUri const & get_Name() const { return entry_.first; }

        __declspec(property(get=get_Cuid)) Reliability::ConsistencyUnitId const & Cuid;
        Reliability::ConsistencyUnitId const & get_Cuid() const { return entry_.second; }

        bool CoversEntry(Common::NamingUri const & name, Reliability::ConsistencyUnitId const cuid) const;

        void WriteTo(__in Common::TextWriter & writer, Common::FormatOptions const &) const;

        void WriteToEtw(uint16 contextSequenceId) const;

    private:
        std::pair<Common::NamingUri, Reliability::ConsistencyUnitId> entry_;
    };
}
