// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace std;
    using namespace Common;

    BroadcastRequestVector::BroadcastRequestVector() 
        : entries_() 
    {
    }

    BroadcastRequestVector::BroadcastRequestVector(BroadcastRequestVector && other)
        : entries_(std::move(other.entries_))
    {
    }

    bool BroadcastRequestVector::TryAdd(
        NamingUri const & name,
        Reliability::ConsistencyUnitId cuid)
    {
        for (auto it = entries_.begin(); it != entries_.end(); ++it)
        {
            if (it->CoversEntry(name, cuid))
            {
                // The name and cuid are already registered, nothing to do
                return false;
            }
        }

        entries_.push_back(BroadcastRequestEntry(name, cuid));
        return true;
    }

    void BroadcastRequestVector::WriteTo(__in Common::TextWriter & writer, Common::FormatOptions const &) const
    {
        for (auto const & entry : entries_)
        {
            writer << entry;
        }
    }

    std::wstring BroadcastRequestVector::ToString() const
    {
        std::wstring product;
        Common::StringWriter writer(product);
        WriteTo(writer, Common::FormatOptions(0, false, ""));
        return product;
    }
}
